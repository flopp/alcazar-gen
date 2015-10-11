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
#include <set>
#include <vector>


int c2f(const Coordinates& c, int width)
{
    return c.x() + c.y() * width;
}


Coordinates f2c(int f, int width)
{
    return {f%width, f/width};
}


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


std::vector<int> getEdgeFields(int width, int height)
{
    std::vector<int> edgeFields;
    for (int x = 0; x < width; ++x)
    {
        edgeFields.push_back(c2f({x, 0}, width));
        edgeFields.push_back(c2f({x, height-1}, width));
    }
    for (int y = 1; y < height-1; ++y)
    {
        edgeFields.push_back(c2f({0, y}, width));
        edgeFields.push_back(c2f({width-1, y}, width));
    }
    return edgeFields;
}



typedef Minisat::vec<Minisat::Lit> Clause;


void buildFormula(int width, int height, SatSolver& s, std::map<std::pair<int, int>, Minisat::Lit>& fp2lit, std::map<Wall, Minisat::Lit>& w2lit)
{
    const int pathLength = width * height;

    for (int field = 0; field < pathLength; ++field)
    {
        for (int pathpos = 0; pathpos < pathLength; ++pathpos)
        {
            fp2lit[{field, pathpos}] = Minisat::mkLit(s.newVar());
        }
    }

    for (auto wall: allWalls(width, height))
    {
        w2lit[wall] = Minisat::mkLit(s.newVar());
    }

    /*
    // every field must have at least two open walls
    for (int field = 0; field < pathLength; ++field)
    {
        const auto c = f2c(field, width);
        const auto walln = w2lit[Wall(c, Orientation::H)];
        const auto walle = w2lit[Wall(c.offset(1, 0), Orientation::V)];
        const auto walls = w2lit[Wall(c.offset(0, 1), Orientation::H)];
        const auto wallw = w2lit[Wall(c, Orientation::V)];

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
        const auto wall1 = w2lit[Wall({0, 0}, Orientation::Horizontal)];
        const auto wall2 = w2lit[Wall({0, 0}, Orientation::Vertical)];
        s.addClause(wall1, wall2);
    }
    // top right
    {
        const auto wall1 = w2lit[Wall({width-1, 0}, Orientation::Horizontal)];
        const auto wall2 = w2lit[Wall({width, 0}, Orientation::Vertical)];
        s.addClause(wall1, wall2);
    }
    // bottom left
    {
        const auto wall1 = w2lit[Wall({0, height}, Orientation::Horizontal)];
        const auto wall2 = w2lit[Wall({0, height-1}, Orientation::Vertical)];
        s.addClause(wall1, wall2);
    }
    // bottom right
    {
        const auto wall1 = w2lit[Wall({width-1, height}, Orientation::Horizontal)];
        const auto wall2 = w2lit[Wall({width, height-1}, Orientation::Vertical)];
        s.addClause(wall1, wall2);
    }
    */

    // every field must appear on the path
    // (f@0 + f@1 + ... + f@P-1) for all f
    for (int field = 0; field < pathLength; ++field)
    {
        Clause clause;
        for (int pos = 0; pos < pathLength; ++pos)
        {
            const auto lit = fp2lit[{field, pos}];
            clause.push(lit);
        }
        s.addClause(clause);
    }

    // every field must not appear twice on the path
    // f@i -> ~f@j for all f for all i!=j
    for (int field = 0; field < pathLength; ++field)
    {
        for (int pos1 = 0; pos1 < pathLength; ++pos1)
        {
            for (int pos2 = pos1+1; pos2 < pathLength; ++pos2)
            {
                const auto lit1 = fp2lit[{field, pos1}];
                const auto lit2 = fp2lit[{field, pos2}];
                s.addClause(~lit1, ~lit2);
            }
        }
    }

    // some field must be the path's ith step
    // (0@p + 1@p + ... + F-1@p) for all p
    for (int pos = 0; pos < pathLength; ++pos)
    {
        Clause clause;
        for (int field = 0; field < pathLength; ++field)
        {
            const auto lit = fp2lit[{field, pos}];
            clause.push(lit);
        }
        s.addClause(clause);
    }

    // two fields must not be the path's ith step at the same time
    // i@p -> ~j@p for all p for all i!=j
    for (int pos = 0; pos < pathLength; ++pos)
    {
        for (int field1 = 0; field1 < pathLength; ++field1)
        {
            for (int field2 = field1+1; field2 < pathLength; ++field2)
            {
                const auto lit1 = fp2lit[{field1, pos}];
                const auto lit2 = fp2lit[{field2, pos}];
                s.addClause(~lit1, ~lit2);
            }
        }
    }

    // consecutive path positions only between neighbours
    std::set<Coordinates> allCoordinates;
    for (int x = 0; x < width; ++x)
    {
        for (int y = 0; y < height; ++y)
        {
            allCoordinates.insert({x, y});
        }
    }
    for (auto c: allCoordinates)
    {
        std::vector<Coordinates> neighbours;
        std::set<Coordinates> nonNeighbours = allCoordinates;
        nonNeighbours.erase(c);
        if (c.x() > 0)          { neighbours.push_back(c.offset(-1,0)); nonNeighbours.erase(c.offset(-1,0)); }
        if (c.x() + 1 < width)  { neighbours.push_back(c.offset(+1,0)); nonNeighbours.erase(c.offset(+1,0)); }
        if (c.y() > 0)          { neighbours.push_back(c.offset(0,-1)); nonNeighbours.erase(c.offset(0,-1)); }
        if (c.y() + 1 < height) { neighbours.push_back(c.offset(0,+1)); nonNeighbours.erase(c.offset(0,+1)); }

        const int field = c2f(c, width);
        for (int p = 0; p+1 < pathLength; ++p)
        {
            // f@p -> fn@p+1 v fe@p+1 v fs@p+1 v fw@p+1
            Clause clause;
            clause.push(~fp2lit[{field, p}]);
            for (auto n: neighbours) { clause.push(fp2lit[{c2f(n, width), p+1}]); }
            s.addClause(clause);

            // f@p+1 -> fn@p v fe@p v fs@p v fw@p
            Clause clause2;
            clause2.push(~fp2lit[{field, p+1}]);
            for (auto n: neighbours) { clause2.push(fp2lit[{c2f(n, width), p}]); }
            s.addClause(clause2);

            // f@p -> ~g@p for all non-neighbours g of f
            for (auto n: nonNeighbours)
            {
                s.addClause(~fp2lit[{field, p}], ~fp2lit[{c2f(n, width), p+1}]);
            }
        }
    }

    // no consecutive path positions between fields separated by wall
    // wall(f1, f2) -> (!f1@p + !f2@p+1) <=> (!wall(f1, f2) + !f1@p + !f2@p+1)
    for (auto c: allCoordinates)
    {
        const int field = c2f(c, width);
        for (int p = 0; p+1 < pathLength; ++p)
        {
            const auto lit1 = fp2lit[{field, p}];

            // left wall
            if (c.x() > 0)
            {
                const Wall w(c, Orientation::V);
                const auto litw = w2lit[w];
                const auto lit2 = fp2lit[{c2f(c.offset(-1,0), width), p+1}];
                s.addClause(~litw, ~lit1, ~lit2);
            }

            // right wall
            if (c.x() + 1 < width)
            {
                const Wall w(c.offset(+1,0), Orientation::V);
                const auto litw = w2lit[w];
                const auto lit2 = fp2lit[{c2f(c.offset(+1,0), width), p+1}];
                s.addClause(~litw, ~lit1, ~lit2);
            }

            // top wall
            if (c.y() > 0)
            {
                const Wall w(c, Orientation::H);
                const auto litw = w2lit[w];
                const auto lit2 = fp2lit[{c2f(c.offset(0,-1), width), p+1}];
                s.addClause(~litw, ~lit1, ~lit2);
            }

            // bottom wall
            if (c.y() + 1 < height)
            {
                const Wall w(c.offset(0,+1), Orientation::H);
                const auto litw = w2lit[w];
                const auto lit2 = fp2lit[{c2f(c.offset(0,+1), width), p+1}];
                s.addClause(~litw, ~lit1, ~lit2);
            }
        }
    }

    // path must start/end at edge
    const std::vector<int> edgeFields = getEdgeFields(width, height);

    Clause entryClause;
    Clause exitClause;
    for (auto field: edgeFields)
    {
        entryClause.push(fp2lit[{field, 0}]);
        exitClause.push(fp2lit[{field, pathLength-1}]);
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
                const auto lit1 = fp2lit[{field1, 0}];
                const auto lit2 = fp2lit[{field2, pathLength-1}];
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
            const auto litw = w2lit[w];
            const auto lit1 = fp2lit[{c2f({x, 0}, width), 0}];
            const auto lit2 = fp2lit[{c2f({x, 0}, width), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }

        {
            const Wall w({x, height}, Orientation::H);
            const auto litw = w2lit[w];
            const auto lit1 = fp2lit[{c2f({x, height-1}, width), 0}];
            const auto lit2 = fp2lit[{c2f({x, height-1}, width), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }
    }
    // left/right edge
    for (int y = 1; y < height-2; ++y)
    {
        {
            const Wall w({0, y}, Orientation::V);
            const auto litw = w2lit[w];
            const auto lit1 = fp2lit[{c2f({0, y}, width), 0}];
            const auto lit2 = fp2lit[{c2f({0, y}, width), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }

        {
            const Wall w({width, y}, Orientation::V);
            const auto litw = w2lit[w];
            const auto lit1 = fp2lit[{c2f({width-1, y}, width), 0}];
            const auto lit2 = fp2lit[{c2f({width-1, y}, width), pathLength-1}];
            s.addClause(~litw, ~lit1);
            s.addClause(~litw, ~lit2);
        }
    }
    // top left corner
    {
        const Wall w1({0, 0}, Orientation::V);
        const Wall w2({0, 0}, Orientation::H);
        const auto litw1 = w2lit[w1];
        const auto litw2 = w2lit[w2];
        const auto lit1 = fp2lit[{c2f({0, 0}, width), 0}];
        const auto lit2 = fp2lit[{c2f({0, 0}, width), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
    // top right corner
    {
        const Wall w1({width, 0}, Orientation::V);
        const Wall w2({width-1, 0}, Orientation::H);
        const auto litw1 = w2lit[w1];
        const auto litw2 = w2lit[w2];
        const auto lit1 = fp2lit[{c2f({width-1, 0}, width), 0}];
        const auto lit2 = fp2lit[{c2f({width-1, 0}, width), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
    // bottom left corner
    {
        const Wall w1({0, height-1}, Orientation::V);
        const Wall w2({0, height}, Orientation::H);
        const auto litw1 = w2lit[w1];
        const auto litw2 = w2lit[w2];
        const auto lit1 = fp2lit[{c2f({0, height-1}, width), 0}];
        const auto lit2 = fp2lit[{c2f({0, height-1}, width), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
    // bottom right corner
    {
        const Wall w1({width, height-1}, Orientation::V);
        const Wall w2({width-1, height}, Orientation::H);
        const auto litw1 = w2lit[w1];
        const auto litw2 = w2lit[w2];
        const auto lit1 = fp2lit[{c2f({width-1, height-1}, width), 0}];
        const auto lit2 = fp2lit[{c2f({width-1, height-1}, width), pathLength-1}];
        s.addClause(~litw1, ~litw2, ~lit1);
        s.addClause(~litw1, ~litw2, ~lit2);
    }
}
