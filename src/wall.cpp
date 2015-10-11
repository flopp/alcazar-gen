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

#include <algorithm>
#include "wall.h"

Wall::Wall(const Coordinates& coordinates, Orientation orientation) :
    m_coordinates(coordinates),
    m_orientation(orientation)
{}


bool Wall::isBetween(const Coordinates& c1, const Coordinates& c2) const
{
    if (c1.x() == c2.x() && m_orientation == Orientation::H)
    {
        if (m_coordinates.x() != c1.x()) return false;
        if (m_coordinates.y() <= std::min(c1.y(), c2.y())) return false;
        if (m_coordinates.y() > std::max(c1.y(), c2.y())) return false;
        
        return true;
    }
    
    if (c1.y() == c2.y() && m_orientation == Orientation::V)
    {
        if (m_coordinates.y() != c1.y()) return false;
        if (m_coordinates.x() <= std::min(c1.x(), c2.x())) return false;
        if (m_coordinates.x() > std::max(c1.x(), c2.x())) return false;
        
        return true;
    }
    
    return false;
}
