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

#pragma once

#include "coordinates.h"
#include "wall.h"
#include <iostream>
#include <set>
#include <vector>

class TemplateBoard
{
    public:
        TemplateBoard() = default;
        TemplateBoard(int w, int h);

        bool empty() const { return m_width == 0 || m_height == 0; }
        int width() const { return m_width; }
        int height() const { return m_height; }

        const std::set<Wall>& getAllWalls() const { return m_allWalls; }
        const std::set<Wall>& getPossibleWalls() const { return m_possibleWalls; }
        const std::set<Wall>& getFixedClosedWalls() const { return m_fixedClosedWalls; }
        const std::set<Wall>& getFixedOpenWalls() const { return m_fixedOpenWalls; }
        std::vector<Coordinates> getNonBlockedEdgeFields() const;

        bool parse(std::istream& is);

    private:
        bool isClosed(const Wall& w) const;

        int m_width = 0;
        int m_height = 0;
        std::set<Wall> m_allWalls;
        std::set<Wall> m_fixedClosedWalls;
        std::set<Wall> m_fixedOpenWalls;
        std::set<Wall> m_possibleWalls;
};

std::ostream& operator<<(std::ostream& os, const TemplateBoard& b);
