#pragma once
#include <vector>
#include <optional>
#include "../models/Board.h"

class IBoardStorage {
public:
    virtual ~IBoardStorage() = default;

    virtual void                 saveBoard(const Board& board) = 0;
    [[nodiscard]] virtual std::vector<Board>   loadAll() const = 0;
    [[nodiscard]] virtual std::optional<Board> loadBoard(const std::string& boardId) const = 0;
    virtual void                 deleteBoard(const std::string& boardId) = 0;
};
