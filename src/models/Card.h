#pragma once
#include <string>

struct Card {
    std::string id;
    std::string title;
    std::string description;

    bool operator==(const Card& other) const { return id == other.id; }
};
