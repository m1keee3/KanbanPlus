#pragma once
#include <string>
#include <optional>
#include <mutex>
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

    Board                createBoard(const std::string& name) const;
    std::optional<Board> getBoard(const std::string& boardId) const;
    std::vector<Board>   listBoards() const;

    Result<Column>          addColumn(const std::string& boardId, const std::string& name) const;
    Result<Column>          renameColumn(const std::string& boardId, const std::string& columnId,
                                         const std::string& newName) const;
    Result<std::monostate>  removeColumn(const std::string& boardId, const std::string& columnId) const;

    Result<Card>            addCard(const std::string& boardId, const std::string& columnId,
                                    const std::string& title, const std::string& description = "") const;
    Result<Card>            updateCard(const std::string& boardId, const std::string& columnId,
                                       const std::string& cardId, const std::string& title,
                                       const std::string& description) const;
    Result<std::monostate>  removeCard(const std::string& boardId, const std::string& columnId,
                                       const std::string& cardId) const;
    Result<std::monostate>  moveCard(const std::string& boardId, const std::string& cardId,
                                     const std::string& toColumnId) const;

    using ChangeCallback = std::function<void(const Board&)>;
    void setOnChange(ChangeCallback cb) { onChange_ = std::move(cb); }

private:
    IBoardStorage&  storage_;
    mutable std::mutex mutex_;
    ChangeCallback onChange_;

    static std::string generateId();

    static Column* findColumn(Board& board, const std::string& columnId);
    static Card*   findCard(Column& column, const std::string& cardId);

    void notifyChange(const Board& board) const;
};
