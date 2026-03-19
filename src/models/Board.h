#pragma once
#include <string>
#include <vector>
#include "Column.h"

struct Board {
    std::string id;
    std::string name;
    std::vector<Column> columns;

    bool operator==(const Board& other) const { return id == other.id; }
};
