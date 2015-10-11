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
#include "formula.h"
#include "minisat/core/Solver.h"
#include "minisat/simp/SimpSolver.h"

Board::Board(int w, int h) :
    m_width(w),
    m_height(h)
{}


std::tuple<bool, bool, Path> Board::solve() const
{
    SatSolver s;
    std::map<std::pair<int, int>, Minisat::Lit> fp2lit;
    std::map<Wall, Minisat::Lit> w2lit;
    buildFormula(m_width, m_height, s, fp2lit, w2lit);
    
    const int pathLength = m_width * m_height;
    
    // assumptions: current walls
    Minisat::vec<Minisat::Lit> wallAssumptions;
    for (auto wall = w2lit.begin(); wall != w2lit.end(); ++wall)
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
                const auto lit = fp2lit[{field, pos}];
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
            case Orientation::H:
                for (int i = 1; i < dx; ++i) grid[gy][gx+i] = '-';
                break;
            case Orientation::V:
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
