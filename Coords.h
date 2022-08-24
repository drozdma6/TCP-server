//
// Created by Marek Drozdik on 06/05/2022.
//
#pragma once

#include <stdexcept>
#include "Direction.h"


struct Coords {
    int x;
    int y;

    size_t coordsWeight() const;

    Coords &operator=(const Coords &rhs);

    /**
     * @param[in] coord 
     * @param[in] direction 
     * @return coords of next field in given direction
     */
    friend Coords operator+(const Coords &coord, Direction direction);


    /**
     * Decides if two coords are equal.
     * @param[in] first
     * @param[in] second
     * @return true if both x and y are equal, else false.
     */
    friend bool operator==(const Coords &first, const Coords &second);
};
