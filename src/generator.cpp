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

template<typename T> T randomChoice(const std::vector<T>& v, std::mt19937& rng)
{
    assert(!v.empty());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, v.size() - 1);
    return v[dist(rng)];
}

template<typename T> T popRandom(std::vector<T>& v, std::mt19937& rng)
{
    assert(!v.empty());
    
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, v.size() - 1);
    const int index = dist(rng);
    const T t = v[index];
    if (v.size() == 1)
    {
        v.clear();
    }
    else
    {
        if (static_cast<unsigned int>(index + 1) != v.size())
        {
            std::swap(v[index], v.back());
        }
        v.pop_back();
    }
    
    return t;
}


Board generate(int w, int h, unsigned int seed)
{
    std::mt19937 rng;
    if (seed == 0)
    {
        seed = std::random_device()();
    }
    std::cout << "Using seed " << seed << std::endl;
    rng.seed(seed);
    
    const int pathLength = w * h;
    Board b(w, h);
    
    SatSolver s;
    std::map<std::pair<int, int>, Minisat::Lit> field_pathpos2lit;
    std::map<Wall, Minisat::Lit> wall2lit;
    b.encode(s, field_pathpos2lit, wall2lit);
    
    std::cout << "SAT encoding has " << s.nVars() << " variables and " << s.nClauses() << " clauses" << std::endl;

    std::cout << "Creating initial path..." << std::flush;
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
        
    // find initial path in empty board with random fixed entry/exit
    for (;;)
    {
        Minisat::vec<Minisat::Lit> initialAssumptions;
       
        // fix entry and exit
        int field1 = -1;
        int field2 = -1;
        while (field1 == field2)
        {
            field1 = randomChoice(edgeFields, rng);
            field2 = randomChoice(edgeFields, rng);
        }
        initialAssumptions.push(field_pathpos2lit[{std::min(field1, field2), 0}]);
        initialAssumptions.push(field_pathpos2lit[{std::max(field1, field2), pathLength-1}]);
       
        // no walls
        for (auto wall: b.getPossibleWalls())
        {
            initialAssumptions.push(~wall2lit[wall]);
        }

        if (s.solve(initialAssumptions)) break;
    }
    
    // extract initial path
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
    // initial path is forbidden
    s.addClause(pathClause);
    std::cout << " done" << std::endl;
    
        
    std::vector<Wall> nonblockingWalls = path.getNonblockingWalls(b.getPossibleWalls());
        
    // fixed variables of open walls
    std::set<Wall> openWalls;
    for (auto w: b.getOpenWalls())
    {
        openWalls.insert(w);
    }
    for (auto w: nonblockingWalls)
    {
        openWalls.erase(w);
    }
    for (auto w: openWalls)
    {
        s.addClause(~wall2lit[w]);
    }
    
    // iteratively add non-blocking walls until the initial path is unique (after adding *all* non-blocking walls, the initial path is guaranteed to be unique)
    std::cout << "Adding walls... " << std::flush;
    std::vector<Wall> candidateWalls;
    for (;;)
    {
        Minisat::vec<Minisat::Lit> assumptions;

        const Wall wall = popRandom(nonblockingWalls, rng);        
        const auto lit = wall2lit[wall];
        assumptions.push(lit);
        for (auto w: candidateWalls)
        {
            assumptions.push(wall2lit[w]);
        }
        for (auto w: nonblockingWalls)
        {
            assumptions.push(~wall2lit[w]);
        }
        
        candidateWalls.push_back(wall);
        std::cout << "\rAdding walls... " << candidateWalls.size() << ", remaining " << nonblockingWalls.size() << "        " << std::flush;
        if (!s.solve(assumptions))
        {
            // initial path became unique
            
            // refine candidateWalls by last conflict clause
            std::set<Wall> oldCandidates;
            for (auto w: candidateWalls) oldCandidates.insert(w);
            
            candidateWalls.clear();
            for (int i = 0; i != s.conflict.toVec().size(); ++i)
            {
                auto clit = s.conflict.toVec()[i];
                if (Minisat::sign(clit))
                {
                    for (auto w = oldCandidates.begin(); w != oldCandidates.end(); /**/)
                    {
                        if (wall2lit[*w] == ~clit)
                        {
                            candidateWalls.push_back(*w);
                            w = oldCandidates.erase(w);
                            break;
                        }
                        else
                        {
                            ++w;
                        }
                    }
                }
            }
            
            for (auto w: oldCandidates)
            {
                nonblockingWalls.push_back(w);
            }
            break;
        }
    }
    for (auto w: nonblockingWalls)
    {
        s.addClause(~wall2lit[w]);
    }
    std::cout << "\rAdding walls... done, walls=" << candidateWalls.size() << "                            " << std::endl;
    
    std::cout << "Removing walls... " << std::flush;
    std::vector<Wall> essentialWalls;
    while (!candidateWalls.empty())
    {
        std::cout << "\rRemoving walls... " << candidateWalls.size() << "        " << std::flush;
        Minisat::vec<Minisat::Lit> assumptions;
        
        const Wall wall = popRandom(candidateWalls, rng);        
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
            // determining new candidates from final conflict clause
            std::vector<Wall> newCandidateWalls;
            for (int i = 0; i != s.conflict.toVec().size(); ++i)
            {
                auto clit = s.conflict.toVec()[i];
                if (Minisat::sign(clit))
                {
                    for (auto w : candidateWalls)
                    {
                        if (wall2lit[w] == ~clit)
                        {
                            newCandidateWalls.push_back(w);
                            break;
                        }
                    }
                }
            }
            candidateWalls = newCandidateWalls;

            // wall can be removed -> fix variable=0
            s.addClause(~lit);
        }
    }
    std::cout << "\rRemoving walls... done, walls=" << essentialWalls.size() << "        " << std::endl;
    
    // add non-blocking walls
    for (auto wall: essentialWalls)
    {
        b.addWall(wall);
    }   
    
    // cosmetic fix: make sure the corners have at least one wall
    // top left
    if (!b.hasWall(Wall({0,0}, Orientation::Vertical)) && !b.hasWall(Wall({0,0}, Orientation::Horizontal)))
    {
        b.addWall(randomChoice<Wall>({Wall({0,0}, Orientation::Vertical), Wall({0,0}, Orientation::Horizontal)}, rng));
    }
    // top right
    if (!b.hasWall(Wall({w,0}, Orientation::Vertical)) && !b.hasWall(Wall({w-1,0}, Orientation::Horizontal)))
    {
        b.addWall(randomChoice<Wall>({Wall({w,0}, Orientation::Vertical), Wall({w-1,0}, Orientation::Horizontal)}, rng));
    }
    // bottom left
    if (!b.hasWall(Wall({0,h-1}, Orientation::Vertical)) && !b.hasWall(Wall({0,h}, Orientation::Horizontal)))
    {
        b.addWall(randomChoice<Wall>({Wall({0,h-1}, Orientation::Vertical), Wall({0,h}, Orientation::Horizontal)}, rng));
    }
    // top right
    if (!b.hasWall(Wall({w,h-1}, Orientation::Vertical)) && !b.hasWall(Wall({w-1,h}, Orientation::Horizontal)))
    {
        b.addWall(randomChoice<Wall>({Wall({w,h-1}, Orientation::Vertical), Wall({w-1,h}, Orientation::Horizontal)}, rng));
    }
    return b;
}
