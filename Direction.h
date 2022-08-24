//
// Created by Marek Drozdik on 06/05/2022.
//
#pragma once

#include <cstddef>

constexpr const size_t DIRECTIONS = 4;

enum class Direction : size_t {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3,
};
