/*******************************************************************************
* alcazar-gen
*
* Copyright (c) 2015 Florian Pigorsch
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*******************************************************************************/

#include "board.h"

Board::Board(int w, int h) :
    m_width(w),
    m_height(h)
{}


void Board::encode(Minisat::Solver& s, std::map<std::pair<int, int>, Minisat::Lit>& field_pathpos2lit, std::map<Wall, Minisat::Lit>& wall2lit) const
{
    const int pathLength = m_width * m_height;
    
    for (int field = 0; field < pathLength; ++field)
    {
        for (int pathpos = 0; pathpos < pathLength; ++pathpos)
        {
            field_pathpos2lit[{field, pathpos}] = Minisat::mkLit(s.newVar());
        }
    }
    for (auto wall: getPossibleWalls())
    {
        wall2lit[wall] = Minisat::mkLit(s.newVar());
    }
    
    // every field must appear on the path
    for (int field = 0; field < pathLength; ++field)
    {
        Minisat::vec<Minisat::Lit> clause;
        for (int pos = 0; pos < pathLength; ++pos)
        {
            // ith node:
            // vi0 + ... + vin
            const auto lit = field_pathpos2lit[{field, pos}];
            clause.push(lit);
        }
        s.addClause(clause);
    }
    
    // every field must not appear twice on the path
    for (int field = 0; field < pathLength; ++field)
    {
        for (int pos1 = 0; pos1 < pathLength; ++pos1)
        {
            for (int pos2 = pos1+1; pos2 < pathLength; ++pos2)
            {
                const auto lit1 = field_pathpos2lit[{field, pos1}];
                const auto lit2 = field_pathpos2lit[{field, pos2}];
                s.addClause(~lit1, ~lit2);
            }
        }
    }
    
    // some field must be the path's ith step
    for (int pos = 0; pos < pathLength; ++pos)
    {
        Minisat::vec<Minisat::Lit> clause;
        for (int field = 0; field < pathLength; ++field)
        {
            const auto lit = field_pathpos2lit[{field, pos}];
            clause.push(lit);
        }
        s.addClause(clause);
    }
    
    // two fields must not be the path's ith step at the same time
    for (int pos = 0; pos < pathLength; ++pos)
    {
        for (int field1 = 0; field1 < pathLength; ++field1)
        {
            for (int field2 = field1+1; field2 < pathLength; ++field2)
            {
                const auto lit1 = field_pathpos2lit[{field1, pos}];
                const auto lit2 = field_pathpos2lit[{field2, pos}];
                s.addClause(~lit1, ~lit2);
            }
        }
    }
    
    // consecutive path positions only between adjacent fields
    // vip -> (vnorth(i)p+1 + veast(i)p+1 + vsouth(i)p+1 + vwest(i)p+1)
    // <=>  !vip + vnorth(i)p+1 + veast(i)p+1 + vsouth(i)p+1 + vwest(i)p+1
    
    for (int x = 0; x < m_width; ++x)
    {
        for (int y = 0; y < m_height; ++y)
        {
            const int field = index(x, y);
            for (int p = 0; p < pathLength - 1; ++p)
            {
                Minisat::vec<Minisat::Lit> clause;
                clause.push(~field_pathpos2lit[{field, p}]);
                
                if (x > 0)            clause.push(field_pathpos2lit[{index(x-1, y), p+1}]);
                if (x + 1 < m_width)  clause.push(field_pathpos2lit[{index(x+1, y), p+1}]);
                if (y > 0)            clause.push(field_pathpos2lit[{index(x, y-1), p+1}]);
                if (y + 1 < m_height) clause.push(field_pathpos2lit[{index(x, y+1), p+1}]);
                
                s.addClause(clause);
            }
        }
    }
    
    // no consecutive path positions between fields separated by wall
    // wall(f1, f2) -> (!f1@p + !f2@p+1) <=> (!wall(f1, f2) + !f1@p + !f2@p+1)
    for (int x = 0; x < m_width; ++x)
    {
        for (int y = 0; y < m_height; ++y)
        {
            const int field = index(x, y);
            for (int p = 0; p < pathLength - 1; ++p)
            {
                const auto lit1 = field_pathpos2lit[{field, p}];
                
                // left wall
                if (x > 0)
                {
                    const Wall w({x, y}, Orientation::Vertical);
                    const auto litw = wall2lit[w];
                    const auto lit2 = field_pathpos2lit[{index(x-1, y), p+1}];
                    s.addClause(~litw, ~lit1, ~lit2);
                }
            
                // right wall
                if (x + 1 < m_width)
                {
                    const Wall w({x+1, y}, Orientation::Vertical);
                    const auto litw = wall2lit[w];
                    const auto lit2 = field_pathpos2lit[{index(x+1, y), p+1}];
                    s.addClause(~litw, ~lit1, ~lit2);
                }
            
                // top wall
                if (y > 0)
                {
                    const Wall w({x, y}, Orientation::Horizontal);
                    const auto litw = wall2lit[w];
                    const auto lit2 = field_pathpos2lit[{index(x, y-1), p+1}];
                    s.addClause(~litw, ~lit1, ~lit2);
                }
                
                // bottom wall
                if (y + 1 < m_height)
                {
                    const Wall w({x, y+1}, Orientation::Horizontal);
                    const auto litw = wall2lit[w];
                    const auto lit2 = field_pathpos2lit[{index(x, y+1), p+1}];
                    s.addClause(~litw, ~lit1, ~lit2);
                }
            }
        }
    }

    // path must start/end at edge
    std::vector<int> edgeFields;
    for (int x = 0; x < m_width; ++x)
    {
        edgeFields.push_back(index(x, 0));
        edgeFields.push_back(index(x, m_height-1));
    }
    for (int y = 1; y < m_height-1; ++y)
    {
        edgeFields.push_back(index(0, y));
        edgeFields.push_back(index(m_width-1, y));
    }
    
    Minisat::vec<Minisat::Lit> entryClause;
    Minisat::vec<Minisat::Lit> exitClause;
    for (auto field: edgeFields)
    {
        entryClause.push(field_pathpos2lit[{field, 0}]);
        exitClause.push(field_pathpos2lit[{field, pathLength-1}]);
    }
    s.addClause(entryClause);
    s.addClause(exitClause);
    
    // avoid symmetry -> enforce: entry < exit
    for (auto field1: edgeFields)
    {
        for (auto field2: edgeFields)
        {
            if (field2 < field1)
            {
                const auto lit1 = field_pathpos2lit[{field1, 0}];
                const auto lit2 = field_pathpos2lit[{field2, pathLength-1}]; 
                s.addClause(~lit1, ~lit2);
            }
        }
    }
    
    // walls can block entry/exit fields
    // top/bottom edge
    for (int x = 1; x < m_width-2; ++x)
    {
        {
            const Wall w({x, 0}, Orientation::Horizontal);
            const auto litw = wall2lit[w];
            const auto lit1 = field_pathpos2lit[{index(x, 0), 0}];
            const auto lit2 = field_pathpos2lit[{index(x, 0), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }
        
        {
            const Wall w({x, m_height}, Orientation::Horizontal);
            const auto litw = wall2lit[w];
            const auto lit1 = field_pathpos2lit[{index(x, m_height-1), 0}];
            const auto lit2 = field_pathpos2lit[{index(x, m_height-1), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }
    }
    // left/right edge
    for (int y = 1; y < m_height-2; ++y)
    {
        {
            const Wall w({0, y}, Orientation::Vertical);
            const auto litw = wall2lit[w];
            const auto lit1 = field_pathpos2lit[{index(0, y), 0}];
            const auto lit2 = field_pathpos2lit[{index(0, y), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }
        
        {
            const Wall w({m_width, y}, Orientation::Vertical);
            const auto litw = wall2lit[w];
            const auto lit1 = field_pathpos2lit[{index(m_width-1, y), 0}];
            const auto lit2 = field_pathpos2lit[{index(m_width-1, y), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }
    }
    // top left corner
    {
        const Wall w1({0, 0}, Orientation::Vertical);
        const Wall w2({0, 0}, Orientation::Horizontal);
        const auto litw1 = wall2lit[w1];
        const auto litw2 = wall2lit[w2];
        const auto lit1 = field_pathpos2lit[{index(0, 0), 0}];
        const auto lit2 = field_pathpos2lit[{index(0, 0), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
    // top right corner
    {
        const Wall w1({m_width, 0}, Orientation::Vertical);
        const Wall w2({m_width-1, 0}, Orientation::Horizontal);
        const auto litw1 = wall2lit[w1];
        const auto litw2 = wall2lit[w2];
        const auto lit1 = field_pathpos2lit[{index(m_width-1, 0), 0}];
        const auto lit2 = field_pathpos2lit[{index(m_width-1, 0), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
    // bottom left corner
    {
        const Wall w1({0, m_height-1}, Orientation::Vertical);
        const Wall w2({0, m_height}, Orientation::Horizontal);
        const auto litw1 = wall2lit[w1];
        const auto litw2 = wall2lit[w2];
        const auto lit1 = field_pathpos2lit[{index(0, m_height-1), 0}];
        const auto lit2 = field_pathpos2lit[{index(0, m_height-1), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
    // bottom right corner
    {
        const Wall w1({m_width, m_height-1}, Orientation::Vertical);
        const Wall w2({m_width-1, m_height}, Orientation::Horizontal);
        const auto litw1 = wall2lit[w1];
        const auto litw2 = wall2lit[w2];
        const auto lit1 = field_pathpos2lit[{index(m_width-1, m_height-1), 0}];
        const auto lit2 = field_pathpos2lit[{index(m_width-1, m_height-1), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
}


std::tuple<bool, bool, Path> Board::solve() const
{
    Minisat::Solver s;
    std::map<std::pair<int, int>, Minisat::Lit> field_pathpos2lit;
    std::map<Wall, Minisat::Lit> wall2lit;
    
    encode(s, field_pathpos2lit, wall2lit);
    
    const int pathLength = m_width * m_height;
    
    std::map<Minisat::Var, std::pair<int, int>> var2field_pathpos;
    for (auto it : field_pathpos2lit)
    {
        var2field_pathpos[Minisat::var(it.second)] = it.first;
    }
    
    // assumptions: current walls
    Minisat::vec<Minisat::Lit> wallAssumptions;
    for (auto wall = wall2lit.begin(); wall != wall2lit.end(); ++wall)
    {
        if (hasWall(wall->first))
        {
            wallAssumptions.push(wall->second);
        }
        else
        {
            wallAssumptions.push(~wall->second);
        }
    }
    
    bool satisfiable = s.solve(wallAssumptions);
    if (satisfiable)
    {
        // path found
        Path path(pathLength);
        Minisat::vec<Minisat::Lit> pathClause;
        for (int field = 0; field < pathLength; ++field)
        {
            for (int pos = 0; pos < pathLength; ++pos)
            {
                const auto lit = field_pathpos2lit[{field, pos}];
                const Minisat::lbool value = s.modelValue(lit);
                
                if (value == Minisat::l_True)
                {
                    path.set(pos, coord(field));
                    pathClause.push(~lit);
                }
            }
        }
        
        s.addClause(pathClause);
        satisfiable = s.solve(wallAssumptions);
        
        if (satisfiable)
        {
            return std::make_tuple(true, false, path);
        }
        else
        {
            return std::make_tuple(true, true, path);
        }
    }
    else
    {
        // no path found
        return std::make_tuple(false, false, Path());
    }
    
    return std::make_tuple(false, false, Path());
}


std::vector<Wall> Board::getOpenWalls() const
{
    std::vector<Wall> result;
    
    for (auto w: getPossibleWalls())
    {
        if (!hasWall(w))
        {
            result.push_back(w);
        }
    }
    
    return result;
}


std::vector<Wall> Board::getPossibleWalls() const
{
    std::vector<Wall> walls;
    
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x <= m_width; ++x)
        {
            walls.push_back(Wall({x, y}, Orientation::Vertical));
        }
    }
    for (int y = 0; y <= m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            walls.push_back(Wall({x, y}, Orientation::Horizontal));
        }
    }    
    
    return walls;
}


std::string int2string(unsigned int value, unsigned int width)
{
    std::string s;
    if (value == 0)
    {
        s = "0";
    }
    while (value != 0)
    {
        const char c = '0' + (value % 10);
        s = c + s;
        value /= 10;
    }
    bool front = true;
    while (s.size() < width)
    {
        if (front) s = " " + s;
        else       s = s + " ";
        front = !front;
    }
    return s;
}

void Board::print(std::ostream& os, const Path& path) const
{
    os << "Board " << width() << "x" << height() << ":\n";
    
    const int dx = 4;
    const int dy = 2;
    std::vector<std::string> grid(dy * m_height + 1, std::string(dx * m_width + 1, ' '));
    
    for (int y = 0; y < m_height; ++y)
    {
        const int gy = dy * y;
        for (int x = 0; x < m_width; ++x)
        {
            const int gx = dx * x;
            
            grid[gy+0][gx+0]   = '+';
            grid[gy+0][gx+dx]  = '+';
            grid[gy+dy][gx+0]  = '+';
            grid[gy+dy][gx+dx] = '+';
        }
    }
    
    for (auto wall: m_walls)
    {
        const int gx = dx * wall.m_coordinates.x();
        const int gy = dy * wall.m_coordinates.y();
        
        switch (wall.m_orientation)
        {
            case Orientation::Horizontal:
                for (int i = 1; i < dx; ++i) grid[gy][gx+i] = '-';
                break;
            case Orientation::Vertical:
                for (int i = 1; i < dy; ++i) grid[gy+i][gx] = '|';
                break;
        }
    }
    
    for (unsigned int p = 0; p != path.size(); ++p)
    {
        const auto s = int2string(p, dx - 1);
        const int gx = dx * path.at(p).x();
        const int gy = dy * path.at(p).y();
        for (unsigned int i = 0; i != s.size(); ++i)
        {
            grid[gy + dy/2][gx + 1 + i] = s[i];
        }
    }
    
    for (auto line: grid)
    {
        os << line << "\n";
    }
}


std::ostream& operator<<(std::ostream& os, const Board& board)
{
    board.print(os, Path());
    return os;
}
