#pragma once
#include <string>
#include <vector>
#include "Card.h"

struct Column {
    std::string id;
    std::string name;
    std::vector<Card> cards;

    bool operator==(const Column& other) const { return id == other.id; }
};
