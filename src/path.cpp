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

#include "path.h"

bool Path::isBlockedBy(const Wall& wall) const
{
    if (isEmpty()) return false;
    
    int maxX = -1;
    int maxY = -1;
    for (auto c: m_coordinates)
    {
        maxX = std::max(maxX, c.x());
        maxY = std::max(maxY, c.y());
    }
    
    switch (wall.m_orientation)
    {
        case Orientation::Horizontal:
            // top border
            if (wall.m_coordinates.y() == 0)
            {
                if (m_coordinates[0] == wall.m_coordinates || m_coordinates.back() == wall.m_coordinates)
                {
                    return true;
                }
            }
            // bottom border
            else if (wall.m_coordinates.y() == maxY + 1)
            {
                if (m_coordinates[0].y() == maxY && m_coordinates[0].x() == wall.m_coordinates.x())
                {
                    return true;
                }
                else if (m_coordinates.back().y() == maxY && m_coordinates.back().x() == wall.m_coordinates.x())
                {
                    return true;
                }
            }
            break;
            
        case Orientation::Vertical:
            // left border
            if (wall.m_coordinates.x() == 0)
            {
                if (m_coordinates[0] == wall.m_coordinates || m_coordinates.back() == wall.m_coordinates)
                {
                    return true;
                }
            }
            // right border
            else if (wall.m_coordinates.x() == maxX + 1)
            {
                if (m_coordinates[0].x() == maxX && m_coordinates[0].y() == wall.m_coordinates.y())
                {
                    return true;
                }
                else if (m_coordinates.back().x() == maxX && m_coordinates.back().y() == wall.m_coordinates.y())
                {
                    return true;
                }
            }
            break;
    }
    
    for (unsigned int i = 0; i + 1 < m_coordinates.size(); ++i)
    {
        if (wall.isBetween(m_coordinates[i], m_coordinates[i+1]))
        {
            return true;
        }
    }
    
    return false;
}


std::vector<Wall> Path::getNonblockingWalls(const std::vector<Wall>& walls) const
{
    std::vector<Wall> result;
    for (auto wall: walls)
    {
        if (!isBlockedBy(wall))
        {
            result.push_back(wall);
        }
    }
    return result;
}


std::vector<Wall> Path::getBlockingWalls(const std::set<Wall>& walls) const
{
    std::vector<Wall> result;
    for (auto wall: walls)
    {
        if (isBlockedBy(wall))
        {
            result.push_back(wall);
        }
    }
    return result;
}
