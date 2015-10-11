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

#include "coordinates.h"
#include "formula.h"
#include "minisat/core/Solver.h"
#include "minisat/simp/SimpSolver.h"
#include "wall.h"
#include <vector>

std::vector<Wall> allWalls(int width, int height)
{
    std::vector<Wall> walls;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x <= width; ++x)
        {
            walls.push_back(Wall({x, y}, Orientation::V));
        }
    }
    for (int y = 0; y <= height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            walls.push_back(Wall({x, y}, Orientation::H));
        }
    }

    return walls;
}


int index(const Coordinates& c, int width)
{
    return c.x() + c.y() * width;
}


void buildFormula(int width, int height, SatSolver& s, std::map<std::pair<int, int>, Minisat::Lit>& field_pathpos2lit, std::map<Wall, Minisat::Lit>& wall2lit)
{
    const int pathLength = width * height;

    for (int field = 0; field < pathLength; ++field)
    {
        for (int pathpos = 0; pathpos < pathLength; ++pathpos)
        {
            field_pathpos2lit[{field, pathpos}] = Minisat::mkLit(s.newVar());
        }
    }

    for (auto wall: allWalls(width, height))
    {
        wall2lit[wall] = Minisat::mkLit(s.newVar());
    }

    /*
    // every field must have at least two open walls
    for (int field = 0; field < pathLength; ++field)
    {
        const auto c = coord(field);
        const auto walln = wall2lit[Wall(c, Orientation::Horizontal)];
        const auto walle = wall2lit[Wall(c.offset(1, 0), Orientation::Vertical)];
        const auto walls = wall2lit[Wall(c.offset(0, 1), Orientation::Horizontal)];
        const auto wallw = wall2lit[Wall(c, Orientation::Vertical)];

        s.addClause(~walln, ~walle, ~walls);
        s.addClause(~walln, ~walle, ~wallw);
        s.addClause(~walln, ~walls, ~wallw);
        s.addClause(~walle, ~walls, ~wallw);
    }
    */

    /*
    // corners must have at least one wall
    // top left
    {
        const auto wall1 = wall2lit[Wall({0, 0}, Orientation::Horizontal)];
        const auto wall2 = wall2lit[Wall({0, 0}, Orientation::Vertical)];
        s.addClause(wall1, wall2);
    }
    // top right
    {
        const auto wall1 = wall2lit[Wall({width-1, 0}, Orientation::Horizontal)];
        const auto wall2 = wall2lit[Wall({width, 0}, Orientation::Vertical)];
        s.addClause(wall1, wall2);
    }
    // bottom left
    {
        const auto wall1 = wall2lit[Wall({0, height}, Orientation::Horizontal)];
        const auto wall2 = wall2lit[Wall({0, height-1}, Orientation::Vertical)];
        s.addClause(wall1, wall2);
    }
    // bottom right
    {
        const auto wall1 = wall2lit[Wall({width-1, height}, Orientation::Horizontal)];
        const auto wall2 = wall2lit[Wall({width, height-1}, Orientation::Vertical)];
        s.addClause(wall1, wall2);
    }
    */


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

    for (int x = 0; x < width; ++x)
    {
        for (int y = 0; y < height; ++y)
        {
            const int field = index({x, y}, width);
            for (int p = 0; p < pathLength - 1; ++p)
            {
                Minisat::vec<Minisat::Lit> clause;
                clause.push(~field_pathpos2lit[{field, p}]);

                if (x > 0)          clause.push(field_pathpos2lit[{index({x-1, y}, width), p+1}]);
                if (x + 1 < width)  clause.push(field_pathpos2lit[{index({x+1, y}, width), p+1}]);
                if (y > 0)          clause.push(field_pathpos2lit[{index({x, y-1}, width), p+1}]);
                if (y + 1 < height) clause.push(field_pathpos2lit[{index({x, y+1}, width), p+1}]);

                s.addClause(clause);

                Minisat::vec<Minisat::Lit> clause2;
                clause2.push(~field_pathpos2lit[{field, p+1}]);

                if (x > 0)          clause2.push(field_pathpos2lit[{index({x-1, y}, width), p}]);
                if (x + 1 < width)  clause2.push(field_pathpos2lit[{index({x+1, y}, width), p}]);
                if (y > 0)          clause2.push(field_pathpos2lit[{index({x, y-1}, width), p}]);
                if (y + 1 < height) clause2.push(field_pathpos2lit[{index({x, y+1}, width), p}]);

                s.addClause(clause2);
            }
        }
    }

    // no consecutive path positions between fields separated by wall
    // wall(f1, f2) -> (!f1@p + !f2@p+1) <=> (!wall(f1, f2) + !f1@p + !f2@p+1)
    for (int x = 0; x < width; ++x)
    {
        for (int y = 0; y < height; ++y)
        {
            const int field = index({x, y}, width);
            for (int p = 0; p < pathLength - 1; ++p)
            {
                const auto lit1 = field_pathpos2lit[{field, p}];

                // left wall
                if (x > 0)
                {
                    const Wall w({x, y}, Orientation::V);
                    const auto litw = wall2lit[w];
                    const auto lit2 = field_pathpos2lit[{index({x-1, y}, width), p+1}];
                    s.addClause(~litw, ~lit1, ~lit2);
                }

                // right wall
                if (x + 1 < width)
                {
                    const Wall w({x+1, y}, Orientation::V);
                    const auto litw = wall2lit[w];
                    const auto lit2 = field_pathpos2lit[{index({x+1, y}, width), p+1}];
                    s.addClause(~litw, ~lit1, ~lit2);
                }

                // top wall
                if (y > 0)
                {
                    const Wall w({x, y}, Orientation::H);
                    const auto litw = wall2lit[w];
                    const auto lit2 = field_pathpos2lit[{index({x, y-1}, width), p+1}];
                    s.addClause(~litw, ~lit1, ~lit2);
                }

                // bottom wall
                if (y + 1 < height)
                {
                    const Wall w({x, y+1}, Orientation::H);
                    const auto litw = wall2lit[w];
                    const auto lit2 = field_pathpos2lit[{index({x, y+1}, width), p+1}];
                    s.addClause(~litw, ~lit1, ~lit2);
                }
            }
        }
    }

    // path must start/end at edge
    std::vector<int> edgeFields;
    for (int x = 0; x < width; ++x)
    {
        edgeFields.push_back(index({x, 0}, width));
        edgeFields.push_back(index({x, height-1}, width));
    }
    for (int y = 1; y < height-1; ++y)
    {
        edgeFields.push_back(index({0, y}, width));
        edgeFields.push_back(index({width-1, y}, width));
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
    for (int x = 1; x < width-2; ++x)
    {
        {
            const Wall w({x, 0}, Orientation::H);
            const auto litw = wall2lit[w];
            const auto lit1 = field_pathpos2lit[{index({x, 0}, width), 0}];
            const auto lit2 = field_pathpos2lit[{index({x, 0}, width), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }

        {
            const Wall w({x, height}, Orientation::H);
            const auto litw = wall2lit[w];
            const auto lit1 = field_pathpos2lit[{index({x, height-1}, width), 0}];
            const auto lit2 = field_pathpos2lit[{index({x, height-1}, width), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }
    }
    // left/right edge
    for (int y = 1; y < height-2; ++y)
    {
        {
            const Wall w({0, y}, Orientation::V);
            const auto litw = wall2lit[w];
            const auto lit1 = field_pathpos2lit[{index({0, y}, width), 0}];
            const auto lit2 = field_pathpos2lit[{index({0, y}, width), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }

        {
            const Wall w({width, y}, Orientation::V);
            const auto litw = wall2lit[w];
            const auto lit1 = field_pathpos2lit[{index({width-1, y}, width), 0}];
            const auto lit2 = field_pathpos2lit[{index({width-1, y}, width), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }
    }
    // top left corner
    {
        const Wall w1({0, 0}, Orientation::V);
        const Wall w2({0, 0}, Orientation::H);
        const auto litw1 = wall2lit[w1];
        const auto litw2 = wall2lit[w2];
        const auto lit1 = field_pathpos2lit[{index({0, 0}, width), 0}];
        const auto lit2 = field_pathpos2lit[{index({0, 0}, width), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
    // top right corner
    {
        const Wall w1({width, 0}, Orientation::V);
        const Wall w2({width-1, 0}, Orientation::H);
        const auto litw1 = wall2lit[w1];
        const auto litw2 = wall2lit[w2];
        const auto lit1 = field_pathpos2lit[{index({width-1, 0}, width), 0}];
        const auto lit2 = field_pathpos2lit[{index({width-1, 0}, width), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
    // bottom left corner
    {
        const Wall w1({0, height-1}, Orientation::V);
        const Wall w2({0, height}, Orientation::H);
        const auto litw1 = wall2lit[w1];
        const auto litw2 = wall2lit[w2];
        const auto lit1 = field_pathpos2lit[{index({0, height-1}, width), 0}];
        const auto lit2 = field_pathpos2lit[{index({0, height-1}, width), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
    // bottom right corner
    {
        const Wall w1({width, height-1}, Orientation::V);
        const Wall w2({width-1, height}, Orientation::H);
        const auto litw1 = wall2lit[w1];
        const auto litw2 = wall2lit[w2];
        const auto lit1 = field_pathpos2lit[{index({width-1, height-1}, width), 0}];
        const auto lit2 = field_pathpos2lit[{index({width-1, height-1}, width), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
}
