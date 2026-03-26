#pragma once
#include <unordered_map>

#include "../IBoardStorage.h"

class BoardStorage : public IBoardStorage {
public:

    void                 saveBoard(const Board& board) override;
    [[nodiscard]] std::vector<Board>   loadAll() const override;
    [[nodiscard]] std::optional<Board> loadBoard(const std::string& boardId) const override;
    void                 deleteBoard(const std::string& boardId) override;

private:
    std::unordered_map<std::string, Board> boards_;
};