#pragma once
#include <string>
#include "../IBoardStorage.h"

class BoardStorage : public IBoardStorage {
public:
    explicit BoardStorage(std::string connectionString);

    void migrate() const;

    void                 saveBoard(const Board& board) override;
    [[nodiscard]] std::vector<Board>   loadAll() const override;
    [[nodiscard]] std::optional<Board> loadBoard(const std::string& boardId) const override;
    void                 deleteBoard(const std::string& boardId) override;

private:
    std::string connectionString_;
};
