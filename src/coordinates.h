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

#ifndef COORDINATES_H
#define COORDINATES_H

class Coordinates
{
    public:
        Coordinates() : m_x(0), m_y(0) {}
        Coordinates(int x, int y) : m_x(x), m_y(y) {}
        
        const int& x() const { return m_x; }
        const int& y() const { return m_y; }
        
        Coordinates offset(int dx, int dy) const { return Coordinates(x() + dx, y() + dy); }
    
    private:
        int m_x;
        int m_y;
};

inline
bool operator==(const Coordinates& left, const Coordinates& right)
{
    return left.x() == right.x() && left.y() == right.y();
}


inline
bool operator<(const Coordinates& left, const Coordinates& right)
{
    if (left.x() != right.x())
    {
        return left.x() < right.x();
    }
    return left.y() < right.y();
}

#endif
