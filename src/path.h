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

#include <set>
#include <vector>
#include "coordinates.h"
#include "wall.h"

class Path
{
    public:
        Path() = default;
        explicit Path(int length) : m_coordinates(length, Coordinates(-1, -1)) {}
        
        unsigned int size() const { return m_coordinates.size(); }
        bool isEmpty() const { return size() == 0; }
        
        const Coordinates& at(int index) const { return m_coordinates.at(index); }
        void set(int index, const Coordinates& c) { m_coordinates.at(index) = c; }
        
        bool isBlockedBy(const Wall& wall) const;
        std::vector<Wall> getNonblockingWalls(const std::vector<Wall>& walls) const;
        std::vector<Wall> getBlockingWalls(const std::set<Wall>& walls) const;
        
    private:
        std::vector<Coordinates> m_coordinates;
};
