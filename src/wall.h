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

#include <iostream>
#include "coordinates.h"

enum class Orientation
{
    H,
    V
};

class Wall
{
    public:  
        Wall(const Coordinates& coordinates, Orientation orientation);
        
        bool isBetween(const Coordinates& c1, const Coordinates& c2) const;
        
        Coordinates m_coordinates;
        Orientation m_orientation = Orientation::H;
};


inline bool operator<(const Wall& left, const Wall& right)
{
    if (left.m_orientation != right.m_orientation)
    {
        return left.m_orientation < right.m_orientation;
    }
    return left.m_coordinates < right.m_coordinates;
}


inline std::ostream& operator<<(std::ostream& os, const Wall& w)
{
    os << w.m_coordinates << " " << (w.m_orientation == Orientation::H ? "H" : "V");
    return os;
}
