//
// Created by Marek Drozdik on 06/05/2022.
//

#include "Coords.h"

size_t Coords::coordsWeight() const {
    return abs(x) + abs(y);
}

Coords & Coords::operator=(const Coords &rhs) {
    if (this == &rhs)
        return *this;
    x = rhs.x;
    y = rhs.y;
    return *this;
}

Coords operator+(const Coords &coord, Direction direction) {
    switch (direction) {
        case Direction::UP:
            return {coord.x, coord.y + 1};
        case Direction::DOWN:
            return {coord.x, coord.y - 1};
        case Direction::LEFT:
            return {coord.x - 1, coord.y};
        case Direction::RIGHT:
            return {coord.x + 1, coord.y};
        default:
            throw std::logic_error("Unknown direction");
    }
}


bool operator==(const Coords &first, const Coords &second) {
    return ((first.x == second.x) && (first.y == second.y));
}
