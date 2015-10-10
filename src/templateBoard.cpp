/************************************************
 *                                              *
 *    Copyright (c)                             *
 *    ATRiCS GmbH                               *
 *    Am Flughafen 7                            *
 *    79108 Freiburg                            *
 *    Germany                                   *
 *                                              *
 *    Author:   Florian Pigorsch                *
 *                                              *
 *    Created:  10.10.2015                     *
 *                                              *
 ************************************************/

#include "templateBoard.h"
#include <string>
#include <vector>


TemplateBoard::TemplateBoard(int w, int h) :
    m_width(w),
    m_height(h)
{
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x <= m_width; ++x)
        {
            m_possibleWalls.insert(Wall({x, y}, Orientation::Vertical));
        }
    }
    for (int y = 0; y <= m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            m_possibleWalls.insert(Wall({x, y}, Orientation::Horizontal));
        }
    }
}


enum class WallType
{
    FixedClosed,
    FixedOpen,
    Possible
};


std::vector<WallType> filter(const std::string& s)
{
    std::vector<WallType> res;
    for (auto c: s)
    {
        if (c == '|' || c == '-')
        {
            res.push_back(WallType::FixedClosed);
        }
        else if (c == '/')
        {
            res.push_back(WallType::FixedOpen);
        }
        else if (c == '?')
        {
            res.push_back(WallType::Possible);
        }
    }
    return res;
}


bool TemplateBoard::parse(std::istream& is)
{
    m_width = 0;
    m_height = 0;
    m_allWalls.clear();
    m_possibleWalls.clear();
    m_fixedClosedWalls.clear();
    m_fixedOpenWalls.clear();

    std::vector<std::vector<WallType>> lines;
    std::string s;
    unsigned int w = 0;
    while (std::getline(is, s))
    {
        // -,|  =>  fixed closed wall
        // /    =>  fixed open wall
        // ?    =>  possible wall

        if (s.empty())
        {
            continue;
        }
        if (s.size() < 5)
        {
            return false;
        }
        if ((s.size() & 1) == 0)
        {
            return false;
        }

        auto walls = filter(s);
        if (walls.size() < 2)
        {
            return false;
        }

        if (lines.empty())
        {
            w = walls.size();
        }
        else if ((lines.size() & 1) == 0)
        {
            if (walls.size() != w)
            {
                return false;
            }
        }
        else
        {
            if (walls.size() != w + 1)
            {
                return false;
            }
        }

        lines.push_back(walls);
    }

    if (lines.empty() || lines.size() < 5 || (lines.size() & 1) == 0)
    {
        return false;
    }

    m_width = w;
    m_height = static_cast<int>(lines.size() - 1)/2;

    for (unsigned int row = 0; row < lines.size(); ++row)
    {
        const auto& line = lines[row];
        if ((row & 1) == 0)
        {
            // horizontal
            const unsigned int y = row / 2;
            for (unsigned int x = 0; x < line.size(); ++x)
            {
                const Wall wall({static_cast<int>(x), static_cast<int>(y)}, Orientation::Horizontal);
                m_allWalls.insert(wall);
                switch (line[x])
                {
                    case WallType::FixedClosed:
                        m_fixedClosedWalls.insert(wall);
                        break;
                    case WallType::FixedOpen:
                        m_fixedOpenWalls.insert(wall);
                        break;
                    case WallType::Possible:
                        m_possibleWalls.insert(wall);
                        break;
                }
            }
        }
        else
        {
            // vertical
            const unsigned int y = (row - 1) / 2;
            for (unsigned int x = 0; x < line.size(); ++x)
            {
                const Wall wall({static_cast<int>(x), static_cast<int>(y)}, Orientation::Vertical);
                m_allWalls.insert(wall);
                switch (line[x])
                {
                    case WallType::FixedClosed:
                        m_fixedClosedWalls.insert(wall);
                        break;
                    case WallType::FixedOpen:
                        m_fixedOpenWalls.insert(wall);
                        break;
                    case WallType::Possible:
                        m_possibleWalls.insert(wall);
                        break;
                }
            }
        }
    }

    return true;
}


std::vector<Coordinates> TemplateBoard::getNonBlockedEdgeFields() const
{
    if (empty())
    {
        return {};
    }

    std::vector<Coordinates> edgeFields;

    for (int x = 1; x + 1 < m_width; ++x)
    {
        // top
        if (!isClosed(Wall({x, 0}, Orientation::Horizontal))) edgeFields.push_back({x, 0});
        // bottom
        if (!isClosed(Wall({x, m_height}, Orientation::Horizontal))) edgeFields.push_back({x, m_height - 1});
    }
    for (int y = 1; y + 1 < m_height; ++y)
    {
        // left
        if (!isClosed(Wall({0, y}, Orientation::Vertical))) edgeFields.push_back({0, y});
        // right
        if (!isClosed(Wall({m_width, y}, Orientation::Vertical))) edgeFields.push_back({m_width - 1, y});
    }

    // top left
    if (!isClosed(Wall({0, 0}, Orientation::Horizontal)) || !isClosed(Wall({0, 0}, Orientation::Vertical))) edgeFields.push_back({0, 0});

    // top right
    if (!isClosed(Wall({m_width - 1, 0}, Orientation::Horizontal)) || !isClosed(Wall({m_width, 0}, Orientation::Vertical))) edgeFields.push_back({m_width - 1, 0});

    // bottom left
    if (!isClosed(Wall({0, m_height}, Orientation::Horizontal)) || !isClosed(Wall({0, m_height - 1}, Orientation::Vertical))) edgeFields.push_back({0, m_height - 1});

    // bottom right
    if (!isClosed(Wall({m_width - 1, m_height}, Orientation::Horizontal)) || !isClosed(Wall({m_width, m_height - 1}, Orientation::Vertical))) edgeFields.push_back({m_width - 1, m_height - 1});

    return edgeFields;
}


bool TemplateBoard::isClosed(const Wall& w) const
{
    return m_fixedClosedWalls.find(w) != m_fixedClosedWalls.end();
}


std::ostream& operator<<(std::ostream& os, const TemplateBoard& b)
{
    os << "Template Board " << b.width() << "x" << b.height() << ":\n";

    const int dx = 4;
    const int dy = 2;
    std::vector<std::string> grid(dy * b.height() + 1, std::string(dx * b.width() + 1, ' '));

    for (int y = 0; y < b.height(); ++y)
    {
        const int gy = dy * y;
        for (int x = 0; x < b.width(); ++x)
        {
            const int gx = dx * x;

            grid[gy+0][gx+0]   = '+';
            grid[gy+0][gx+dx]  = '+';
            grid[gy+dy][gx+0]  = '+';
            grid[gy+dy][gx+dx] = '+';
        }
    }

    for (auto wall: b.getFixedClosedWalls())
    {
        const int gx = dx * wall.m_coordinates.x();
        const int gy = dy * wall.m_coordinates.y();

        switch (wall.m_orientation)
        {
            case Orientation::Horizontal:
                for (int i = 1; i < dx; ++i) grid[gy][gx+i] = '-';
                break;
            case Orientation::Vertical:
                for (int i = 1; i < dy; ++i) grid[gy+i][gx] = '|';
                break;
        }
    }

    for (auto wall: b.getFixedOpenWalls())
    {
        const int gx = dx * wall.m_coordinates.x();
        const int gy = dy * wall.m_coordinates.y();

        switch (wall.m_orientation)
        {
            case Orientation::Horizontal:
                for (int i = 1; i < dx; ++i) grid[gy][gx+i] = ' ';
                break;
            case Orientation::Vertical:
                for (int i = 1; i < dy; ++i) grid[gy+i][gx] = ' ';
                break;
        }
    }
    for (auto wall: b.getPossibleWalls())
    {
        const int gx = dx * wall.m_coordinates.x();
        const int gy = dy * wall.m_coordinates.y();

        switch (wall.m_orientation)
        {
            case Orientation::Horizontal:
                for (int i = 1; i < dx; ++i) grid[gy][gx+i] = '.';
                break;
            case Orientation::Vertical:
                for (int i = 1; i < dy; ++i) grid[gy+i][gx] = ':';
                break;
        }
    }

    for (auto line: grid)
    {
        os << line << "\n";
    }

    return os;
}
