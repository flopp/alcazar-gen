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

#include <random>
#include "generator.h"

Board generate(int w, int h)
{
    const int pathLength = w * h;
    Board b(w, h);
    
    std::cout << "Creating initial path..." << std::flush;
    Minisat::Solver s;
    std::map<std::pair<int, int>, Minisat::Lit> field_pathpos2lit;
    std::map<Wall, Minisat::Lit> wall2lit;
    b.encode(s, field_pathpos2lit, wall2lit);
    
    std::vector<int> edgeFields;
    for (int x = 0; x < w; ++x)
    {
        edgeFields.push_back(b.index(x, 0));
        edgeFields.push_back(b.index(x, h-1));
    }
    for (int y = 1; y+1 < h; ++y)
    {
        edgeFields.push_back(b.index(0, y));
        edgeFields.push_back(b.index(w-1, y));
    }
    
    std::mt19937 rng;
    rng.seed(std::random_device()());
    
    std::uniform_int_distribution<std::mt19937::result_type> edgeFieldDist(0, edgeFields.size() - 1);
    
    for (;;)
    {
        Minisat::vec<Minisat::Lit> initialAssumptions;
       
        // fix entry and exit
        for (;;)
        {
            const int entryField = edgeFields[edgeFieldDist(rng)];
            const int exitField = edgeFields[edgeFieldDist(rng)];
            if (entryField < exitField)
            {
                initialAssumptions.push(field_pathpos2lit[{entryField, 0}]);
                initialAssumptions.push(field_pathpos2lit[{exitField, pathLength-1}]);
                break;
            }
        }
       
        // no walls
        for (auto wall: b.getPossibleWalls())
        {
            initialAssumptions.push(~wall2lit[wall]);
        }

        if (s.solve(initialAssumptions)) break;
    }
    
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
                path.set(pos, b.coord(field));
                pathClause.push(~lit);
            }
        }
    }
    s.addClause(pathClause);
    std::cout << " done" << std::endl;
    
    std::cout << "removing walls... " << std::flush;
        
    std::vector<Wall> candidateWalls = path.getNonblockingWalls(b.getPossibleWalls());
        
    // fixed variables of open walls
    std::set<Wall> openWalls;
    for (auto w: b.getOpenWalls())
    {
        openWalls.insert(w);
    }
    for (auto w: candidateWalls)
    {
        openWalls.erase(w);
    }
    for (auto w: openWalls)
    {
        s.addClause(~wall2lit[w]);
    }
    
    std::vector<Wall> essentialWalls;
    while (!candidateWalls.empty())
    {
        std::cout << "\rRemoving walls... " << candidateWalls.size() << "        " << std::flush;
        Minisat::vec<Minisat::Lit> assumptions;
        
        std::uniform_int_distribution<std::mt19937::result_type> wallDist(0, candidateWalls.size() - 1);
        const int wallIndex = wallDist(rng);
        const Wall wall = candidateWalls[wallIndex];
        if (static_cast<unsigned int>(wallIndex + 1) != candidateWalls.size())
        {
            std::swap(candidateWalls[wallIndex], candidateWalls.back());
        }
        candidateWalls.pop_back();
        
        const auto lit = wall2lit[wall];
        assumptions.push(~lit);
        for (auto w: candidateWalls)
        {
            assumptions.push(wall2lit[w]);
        }
        
        if (s.solve(assumptions))
        {
            // wall is needed to keep path unique -> fix variable=1
            s.addClause(lit);
            essentialWalls.push_back(wall);
        }
        else
        {
            // wall can be removed -> fix variable=0
            s.addClause(~lit);
        }
    }
    std::cout << "\rRemoving walls... done        " << std::endl;
    
    // add non-blocking walls
    for (auto wall: essentialWalls)
    {
        b.addWall(wall);
    }   
        
    return b;
}
