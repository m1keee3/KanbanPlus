#include "BoardStorage.h"
#include <ranges>
#include <unordered_map>
#include <vector>
#include "models/Board.h"
#include "storage/IBoardStorage.h"

void BoardStorage::saveBoard(const Board& board) {
    boards_[board.id] = board;
}

std::vector<Board> BoardStorage::loadAll() const {
    std::vector<Board> result;
    result.reserve(boards_.size());
    for (const auto &val: boards_ | std::views::values)
        result.push_back(val);
    return result;
}

std::optional<Board> BoardStorage::loadBoard(const std::string& boardId) const {
    const auto it = boards_.find(boardId);
    if (it == boards_.end()) return std::nullopt;
    return it->second;
}

void BoardStorage::deleteBoard(const std::string& boardId) {
    boards_.erase(boardId);
}
