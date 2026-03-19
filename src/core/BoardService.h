#pragma once
#include <string>
#include <optional>
#include <functional>
#include <variant>
#include "../models/Board.h"
#include "../storage/IBoardStorage.h"

enum class BoardError {
    BoardNotFound,
    ColumnNotFound,
    CardNotFound,
};

template<typename T>
using Result = std::variant<T, BoardError>;

class BoardService {
public:
    explicit BoardService(IBoardStorage& storage);

    [[nodiscard]] Board                createBoard(const std::string& name) const;
    [[nodiscard]] std::optional<Board> getBoard(const std::string& boardId) const;
    [[nodiscard]] std::vector<Board>   listBoards() const;

    [[nodiscard]] Result<Column>          addColumn(const std::string& boardId, const std::string& name) const;
    [[nodiscard]] Result<Column>          renameColumn(const std::string& boardId, const std::string& columnId,
                                         const std::string& newName) const;
    [[nodiscard]] Result<std::monostate>  removeColumn(const std::string& boardId, const std::string& columnId) const;

    [[nodiscard]] Result<Card>            addCard(const std::string& boardId, const std::string& columnId,
                                    const std::string& title, const std::string& description = "") const;
    [[nodiscard]] Result<Card>            updateCard(const std::string& boardId, const std::string& columnId,
                                       const std::string& cardId, const std::string& title,
                                       const std::string& description) const;
    [[nodiscard]] Result<std::monostate>  removeCard(const std::string& boardId, const std::string& columnId,
                                       const std::string& cardId) const;
    [[nodiscard]] Result<std::monostate>  moveCard(const std::string& boardId, const std::string& cardId,
                                     const std::string& toColumnId) const;

    using ChangeCallback = std::function<void(const Board&)>;
    void setOnChange(ChangeCallback cb) { onChange_ = std::move(cb); }

private:
    IBoardStorage& storage_;
    ChangeCallback onChange_;

    static std::string generateId();

    static Column* findColumn(Board& board, const std::string& columnId);
    static Card*   findCard(Column& column, const std::string& cardId);

    void notifyChange(const Board& board) const;
};
