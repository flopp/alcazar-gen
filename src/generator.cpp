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

#include "formula.h"
#include "generator.h"
#include "minisat/core/Solver.h"
#include "minisat/simp/SimpSolver.h"


Generator::Generator(const TemplateBoard& templateBoard, unsigned int seed) :
  m_template(templateBoard)
{
    if (seed == 0)
    {
        seed = std::random_device()();
    }
    std::cout << "Info: using seed " << seed << std::endl;
    m_rng.seed(seed);
}


Board Generator::get()
{
    if (w() < 2 || h() < 2)
    {
        std::cout << "Error: the template board must be at least 2x2" << std::endl;
        return Board();
    }

    const std::vector<Coordinates> edgeFields = m_template.getNonBlockedEdgeFields();
    if (edgeFields.size() < 2)
    {
        std::cout << "Error: the board template needs at least 2 open edge fields" << std::endl;
        return Board();
    }
    
    const int pathLength = w() * h();
    
    SatSolver s;
    m_fp2lit.clear();
    m_w2lit.clear();
    buildFormula(w(), h(), s, m_fp2lit, m_w2lit);
    
    std::cout << "Info: SAT encoding has " << s.nVars() << " variables and " << s.nClauses() << " clauses" << std::endl;

    std::cout << "Info: creating initial path" << std::flush;
    for (auto wall: m_template.getFixedClosedWalls())
    {
        s.addClause(w2lit(wall));
    }
    for (auto wall: m_template.getFixedOpenWalls())
    {
        s.addClause(~w2lit(wall));
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
            const Coordinates& c1 = choice(edgeFields);
            const Coordinates& c2 = choice(edgeFields);
            field1 = c2f(c1);
            field2 = c2f(c2);
        }
        initialAssumptions.push(fp2lit(std::min(field1, field2), 0));
        initialAssumptions.push(fp2lit(std::max(field1, field2), pathLength-1));


        for (auto wall: m_template.getPossibleWalls())
        {
            initialAssumptions.push(~w2lit(wall));
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
            const auto lit = fp2lit(field, pos);
            const Minisat::lbool value = s.modelValue(lit);
                
            if (value == Minisat::l_True)
            {
                initialPath.set(pos, f2c(field));
                pathClause.push(~lit);
            }
        }
    }
    std::cout << "\rInfo: initial path created                     " << std::endl;

    // initialPath is forbidden
    s.addClause(pathClause);

    //b.print(std::cout, initialPath);
        
    std::set<Wall> fixedClosedWalls = m_template.getFixedClosedWalls();
    std::set<Wall> fixedOpenWalls = m_template.getFixedOpenWalls();

    std::vector<Wall> possibleWalls;
    {
        std::vector<Wall> nonblockingWalls;
        for (auto w: m_template.getPossibleWalls())
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

        for (auto w: initialPath.getBlockingWalls(m_template.getAllWalls()))
        {
            if (fixedOpenWalls.find(w) == fixedOpenWalls.end())
            {
                fixedOpenWalls.insert(w);
                s.addClause(~w2lit(w));
            }
        }
    }
    
    // iteratively add non-blocking walls until the initial path is unique (after adding *all* non-blocking walls, the initial path is guaranteed to be unique)
    std::cout << "\rInfo: adding walls...                     " << std::flush;
    std::vector<Wall> candidateClosedWalls;
    while (!possibleWalls.empty())
    {
        Minisat::vec<Minisat::Lit> assumptions;

        const Wall wall = takeChoice(possibleWalls);
        const auto lit = w2lit(wall);
        assumptions.push(lit);
        for (auto w: candidateClosedWalls)
        {
            assumptions.push(w2lit(w));
        }
        for (auto w: possibleWalls)
        {
            assumptions.push(~w2lit(w));
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
                        if (w2lit(*w) == ~clit)
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
                s.addClause(~w2lit(w));
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
        
        const Wall wall = takeChoice(candidateClosedWalls);
        const auto lit = w2lit(wall);
        assumptions.push(~lit);
        for (auto w: candidateClosedWalls)
        {
            assumptions.push(w2lit(w));
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
                        if (w2lit(w) == ~clit)
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

    // create final board
    Board b(w(), h());
    for (auto wall: fixedClosedWalls)
    {
        b.addWall(wall);
    }   
    
    // cosmetic fix: make sure the corners have at least one wall
    // top left
    if (!b.hasWall(Wall({0,0}, Orientation::V)) && !b.hasWall(Wall({0,0}, Orientation::H)))
    {
        std::vector<Wall> walls{Wall({0,0}, Orientation::V), Wall({0,0}, Orientation::H)};
        b.addWall(takeChoice(walls));
    }
    // top right
    if (!b.hasWall(Wall({w(),0}, Orientation::V)) && !b.hasWall(Wall({w()-1,0}, Orientation::H)))
    {
        std::vector<Wall> walls{Wall({w(),0}, Orientation::V), Wall({w()-1,0}, Orientation::H)};
        b.addWall(takeChoice(walls));
    }
    // bottom left
    if (!b.hasWall(Wall({0,h()-1}, Orientation::V)) && !b.hasWall(Wall({0,h()}, Orientation::H)))
    {
        std::vector<Wall> walls{Wall({0,h()-1}, Orientation::V), Wall({0,h()}, Orientation::H)};
        b.addWall(takeChoice(walls));
    }
    // top right
    if (!b.hasWall(Wall({w(),h()-1}, Orientation::V)) && !b.hasWall(Wall({w()-1,h()}, Orientation::H)))
    {
        std::vector<Wall> walls{Wall({w(),h()-1}, Orientation::V), Wall({w()-1,h()}, Orientation::H)};
        b.addWall(takeChoice(walls));
    }

    return b;
}
