#pragma once
#include <boost/json.hpp>
#include "../../models/Board.h"

namespace Serializer {

inline boost::json::object serialize(const Card& card) {
    return {{"id", card.id}, {"title", card.title}, {"description", card.description}};
}

inline boost::json::object serialize(const Column& col) {
    boost::json::array cards;
    for (const Card& c : col.cards)
        cards.push_back(serialize(c));
    return {{"id", col.id}, {"name", col.name}, {"cards", std::move(cards)}};
}

inline boost::json::object serialize(const Board& board) {
    boost::json::array columns;
    for (const Column& c : board.columns)
        columns.push_back(serialize(c));
    return {{"id", board.id}, {"name", board.name}, {"columns", std::move(columns)}};
}

inline std::string okResponse(boost::json::value data) {
    return boost::json::serialize(
        boost::json::object{{"status", "ok"}, {"data", std::move(data)}});
}

inline std::string errorResponse(std::string_view message) {
    return boost::json::serialize(
        boost::json::object{{"status", "error"}, {"message", message}});
}

inline std::string boardEvent(const Board& board) {
    return boost::json::serialize(
        boost::json::object{{"event", "board_updated"}, {"board", serialize(board)}});
}

}
