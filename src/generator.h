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

#pragma once

#include "board.h"
#include "templateBoard.h"
#include <cassert>
#include <random>
#include <vector>
#include <map>
#include <utility>
#include "minisat/core/SolverTypes.h"

class Generator
{
    public:
      Generator(const TemplateBoard& templateBoard, unsigned int seed);

      Board get();

    private:
      int w() const { return m_template.width(); }
      int h() const { return m_template.height(); }

      int c2f(const Coordinates& c) const { return c.x() + w() * c.y(); }
      Coordinates f2c(int f) const { return {f%w(), f/w()}; }

      Minisat::Lit fp2lit(int f, int p) const { auto it = m_fp2lit.find({f, p}); return (it != m_fp2lit.end()) ? it->second : Minisat::Lit(); }
      Minisat::Lit w2lit(const Wall& wall) const { auto it = m_w2lit.find(wall); return (it != m_w2lit.end()) ? it->second : Minisat::Lit(); }

      template<typename T> const T& choice(const std::vector<T>& v);
      template<typename T> T takeChoice(std::vector<T>& v);

    private:
      std::mt19937 m_rng;
      TemplateBoard m_template;
      std::map<std::pair<int, int>, Minisat::Lit> m_fp2lit;
      std::map<Wall, Minisat::Lit> m_w2lit;
};


template<typename T> inline const T& Generator::choice(const std::vector<T>& v)
{
    assert(!v.empty());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, v.size() - 1);
    return v[dist(m_rng)];
}


template<typename T> inline T Generator::takeChoice(std::vector<T>& v)
{
    assert(!v.empty());

    std::uniform_int_distribution<std::mt19937::result_type> dist(0, v.size() - 1);
    const int index = dist(m_rng);
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
