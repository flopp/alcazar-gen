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
#include "templateBoard.h"

template<typename T> const T& randomChoice(const std::vector<T>& v, std::mt19937& rng)
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


Board generate(const TemplateBoard& templateBoard, unsigned int seed)
{
    if (templateBoard.width() < 2 || templateBoard.height() < 2)
    {
        std::cout << "Error: the template board must be at least 2x2" << std::endl;
        return Board();
    }

    const std::vector<Coordinates> edgeFields = templateBoard.getNonBlockedEdgeFields();
    if (edgeFields.size() < 2)
    {
        std::cout << "Error: the board template needs at least 2 open edge fields" << std::endl;
        return Board();
    }

    std::mt19937 rng;
    if (seed == 0)
    {
        seed = std::random_device()();
    }
    std::cout << "Info: using seed " << seed << std::endl;
    rng.seed(seed);
    
    const int w = templateBoard.width();
    const int h = templateBoard.height();
    const int pathLength = w * h;

    Board b(w, h);
    for (auto wall: templateBoard.getFixedClosedWalls())
    {
        b.addWall(wall);
    }
    
    SatSolver s;
    std::map<std::pair<int, int>, Minisat::Lit> field_pathpos2lit;
    std::map<Wall, Minisat::Lit> wall2lit;
    b.encode(s, field_pathpos2lit, wall2lit);
    
    std::cout << "Info: SAT encoding has " << s.nVars() << " variables and " << s.nClauses() << " clauses" << std::endl;

    std::cout << "Info: creating initial path" << std::flush;
    for (auto wall: templateBoard.getFixedClosedWalls())
    {
        s.addClause(wall2lit[wall]);
    }
    for (auto wall: templateBoard.getFixedOpenWalls())
    {
        s.addClause(~wall2lit[wall]);
    }

    // find initial path in empty board with random fixed entry/exit
    for (int count = 0; /**/; ++count)
    {
        Minisat::vec<Minisat::Lit> initialAssumptions;
       
        // fix entry and exit
        int field1 = -1;
        int field2 = -1;
        while (field1 == field2)
        {
            const Coordinates& c1 = randomChoice(edgeFields, rng);
            const Coordinates& c2 = randomChoice(edgeFields, rng);
            field1 = b.index(c1);
            field2 = b.index(c2);
        }
        initialAssumptions.push(field_pathpos2lit[{std::min(field1, field2), 0}]);
        initialAssumptions.push(field_pathpos2lit[{std::max(field1, field2), pathLength-1}]);

        for (auto wall: templateBoard.getPossibleWalls())
        {
            initialAssumptions.push(~wall2lit[wall]);
        }

        if (s.solve(initialAssumptions)) break;

        if (count > 100)
        {
            std::cout << "\nError: cannot find initial path within 100 tries. Check template!" << std::endl;
            return Board();
        }
    }
    
    // extract initialPath
    Path initialPath(pathLength);
    Minisat::vec<Minisat::Lit> pathClause;
    for (int field = 0; field < pathLength; ++field)
    {
        for (int pos = 0; pos < pathLength; ++pos)
        {
            const auto lit = field_pathpos2lit[{field, pos}];
            const Minisat::lbool value = s.modelValue(lit);
                
            if (value == Minisat::l_True)
            {
                initialPath.set(pos, b.coord(field));
                pathClause.push(~lit);
            }
        }
    }
    std::cout << "\rInfo: initial path created                     " << std::endl;

    // initialPath is forbidden
    s.addClause(pathClause);

    b.print(std::cout, initialPath);
        
    std::set<Wall> fixedClosedWalls = templateBoard.getFixedClosedWalls();
    std::set<Wall> fixedOpenWalls = templateBoard.getFixedOpenWalls();

    std::vector<Wall> possibleWalls;
    {
        std::vector<Wall> nonblockingWalls;
        for (auto w: templateBoard.getPossibleWalls())
        {
            nonblockingWalls.push_back(w);
        }
        nonblockingWalls = initialPath.getNonblockingWalls(nonblockingWalls);

        for (auto w: nonblockingWalls)
        {
            if (fixedClosedWalls.find(w) == fixedClosedWalls.end())
            {
                possibleWalls.push_back(w);
            }
        }

        for (auto w: initialPath.getBlockingWalls(templateBoard.getAllWalls()))
        {
            if (fixedOpenWalls.find(w) == fixedOpenWalls.end())
            {
                fixedOpenWalls.insert(w);
                s.addClause(~wall2lit[w]);
            }
        }
    }
    
    // iteratively add non-blocking walls until the initial path is unique (after adding *all* non-blocking walls, the initial path is guaranteed to be unique)
    std::cout << "\rInfo: adding walls...                     " << std::flush;
    std::vector<Wall> candidateClosedWalls;
    while (!possibleWalls.empty())
    {
        Minisat::vec<Minisat::Lit> assumptions;

        const Wall wall = popRandom(possibleWalls, rng);
        const auto lit = wall2lit[wall];
        assumptions.push(lit);
        for (auto w: candidateClosedWalls)
        {
            assumptions.push(wall2lit[w]);
        }
        for (auto w: possibleWalls)
        {
            assumptions.push(~wall2lit[w]);
        }
        
        candidateClosedWalls.push_back(wall);
        std::cout << "\rInfo: adding wall #" << candidateClosedWalls.size() << ", remaining " << possibleWalls.size() << "                     " << std::flush;
        if (!s.solve(assumptions))
        {
            // initial path became unique
            
            // refine candidateWalls by last conflict clause
            std::set<Wall> oldCandidates;
            for (auto w: candidateClosedWalls) oldCandidates.insert(w);
            
            candidateClosedWalls.clear();
            for (int i = 0; i != s.conflict.toVec().size(); ++i)
            {
                auto clit = s.conflict.toVec()[i];
                if (Minisat::sign(clit))
                {
                    for (auto w = oldCandidates.begin(); w != oldCandidates.end(); /**/)
                    {
                        if (wall2lit[*w] == ~clit)
                        {
                            candidateClosedWalls.push_back(*w);
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
                fixedOpenWalls.insert(w);
                s.addClause(~wall2lit[w]);
            }
            break;
        }
    }
    std::cout << "\rInfo: added walls => walls=" << candidateClosedWalls.size() << "                            " << std::endl;
    
    std::cout << "\rInfo: removing non-essential walls...                     " << std::flush;
    while (!candidateClosedWalls.empty())
    {
        std::cout << "\rInfo: removing walls... " << candidateClosedWalls.size() << "                     " << std::flush;
        Minisat::vec<Minisat::Lit> assumptions;
        
        const Wall wall = popRandom(candidateClosedWalls, rng);
        const auto lit = wall2lit[wall];
        assumptions.push(~lit);
        for (auto w: candidateClosedWalls)
        {
            assumptions.push(wall2lit[w]);
        }
        
        if (s.solve(assumptions))
        {
            // wall is needed to keep path unique -> fix variable=1
            s.addClause(lit);
            fixedClosedWalls.insert(wall);
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
                    for (auto w : candidateClosedWalls)
                    {
                        if (wall2lit[w] == ~clit)
                        {
                            newCandidateWalls.push_back(w);
                            break;
                        }
                    }
                }
            }
            candidateClosedWalls = newCandidateWalls;

            // wall can be removed -> fix variable=0
            fixedOpenWalls.insert(wall);
            s.addClause(~lit);
        }
    }
    std::cout << "\rInfo: removed non-essential walls => walls=" << fixedClosedWalls.size() << "                     " << std::endl;
    // add non-blocking walls
    for (auto wall: fixedClosedWalls)
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
