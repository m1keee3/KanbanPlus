#include "BoardService.h"
#include "../storage/IBoardStorage.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

BoardService::BoardService(IBoardStorage& storage) : storage_(storage) {}

std::string BoardService::generateId() {
    static boost::uuids::random_generator gen;
    return boost::uuids::to_string(gen());
}

Column* BoardService::findColumn(Board& board, const std::string& columnId) {
    const auto it = std::ranges::find_if(board.columns,
                                         [&](const Column& c) { return c.id == columnId; });
    return it != board.columns.end() ? &(*it) : nullptr;
}

Card* BoardService::findCard(Column& column, const std::string& cardId) {
    const auto it = std::ranges::find_if(column.cards,
                                   [&](const Card& c) { return c.id == cardId; });
    return it != column.cards.end() ? &(*it) : nullptr;
}

void BoardService::notifyChange(const Board& board) const {
    if (onChange_) onChange_(board);
}

Board BoardService::createBoard(const std::string& name) const {
    Board board{generateId(), name, {}};
    {
        std::lock_guard lock{mutex_};
        storage_.saveBoard(board);
    }
    return board;
}

std::optional<Board> BoardService::getBoard(const std::string& boardId) const {
    std::lock_guard lock{mutex_};
    return storage_.loadBoard(boardId);
}

std::vector<Board> BoardService::listBoards() const {
    std::lock_guard lock{mutex_};
    return storage_.loadAll();
}

Result<Column> BoardService::addColumn(const std::string& boardId, const std::string& name) const {
    std::lock_guard lock{mutex_};
    std::optional<Board> board = storage_.loadBoard(boardId);
    if (!board) return BoardError::BoardNotFound;

    Column col{generateId(), name, {}};
    board->columns.push_back(col);
    storage_.saveBoard(*board);
    notifyChange(*board);
    return col;
}

Result<Column> BoardService::renameColumn(const std::string& boardId,
                                          const std::string& columnId,
                                          const std::string& newName) const {
    std::lock_guard lock{mutex_};
    std::optional<Board> board = storage_.loadBoard(boardId);
    if (!board) return BoardError::BoardNotFound;

    Column* col = findColumn(*board, columnId);
    if (!col) return BoardError::ColumnNotFound;

    col->name = newName;
    Column updated = *col;
    storage_.saveBoard(*board);
    notifyChange(*board);
    return updated;
}

Result<std::monostate> BoardService::removeColumn(const std::string& boardId,
                                                   const std::string& columnId) const {
    std::lock_guard lock{mutex_};
    std::optional<Board> board = storage_.loadBoard(boardId);
    if (!board) return BoardError::BoardNotFound;

    const auto it = std::ranges::find_if(board->columns,
                                   [&](const Column& c) { return c.id == columnId; });
    if (it == board->columns.end()) return BoardError::ColumnNotFound;

    board->columns.erase(it);
    storage_.saveBoard(*board);
    notifyChange(*board);
    return std::monostate{};
}

Result<Card> BoardService::addCard(const std::string& boardId, const std::string& columnId,
                                   const std::string& title, const std::string& description) const {
    std::lock_guard lock{mutex_};
    std::optional<Board> board = storage_.loadBoard(boardId);
    if (!board) return BoardError::BoardNotFound;

    Column* col = findColumn(*board, columnId);
    if (!col) return BoardError::ColumnNotFound;

    Card card{generateId(), title, description};
    col->cards.push_back(card);
    storage_.saveBoard(*board);
    notifyChange(*board);
    return card;
}

Result<Card> BoardService::updateCard(const std::string& boardId, const std::string& columnId,
                                      const std::string& cardId, const std::string& title,
                                      const std::string& description) const {
    std::lock_guard lock{mutex_};
    std::optional<Board> board = storage_.loadBoard(boardId);
    if (!board) return BoardError::BoardNotFound;

    Column* col = findColumn(*board, columnId);
    if (!col) return BoardError::ColumnNotFound;

    Card* card = findCard(*col, cardId);
    if (!card) return BoardError::CardNotFound;

    card->title = title;
    card->description = description;
    Card updated = *card;
    storage_.saveBoard(*board);
    notifyChange(*board);
    return updated;
}

Result<std::monostate> BoardService::removeCard(const std::string& boardId,
                                                 const std::string& columnId,
                                                 const std::string& cardId) const {
    std::lock_guard lock{mutex_};
    std::optional<Board> board = storage_.loadBoard(boardId);
    if (!board) return BoardError::BoardNotFound;

    Column* col = findColumn(*board, columnId);
    if (!col) return BoardError::ColumnNotFound;

    const auto it = std::ranges::find_if(col->cards,
                                   [&](const Card& c) { return c.id == cardId; });
    if (it == col->cards.end()) return BoardError::CardNotFound;

    col->cards.erase(it);
    storage_.saveBoard(*board);
    notifyChange(*board);
    return std::monostate{};
}

Result<std::monostate> BoardService::moveCard(const std::string& boardId,
                                               const std::string& cardId,
                                               const std::string& toColumnId) const {
    std::lock_guard lock{mutex_};
    std::optional<Board> board = storage_.loadBoard(boardId);
    if (!board) return BoardError::BoardNotFound;

    Card* card = nullptr;
    Column* srcCol = nullptr;
    for (Column& col : board->columns) {
        card = findCard(col, cardId);
        if (card) { srcCol = &col; break; }
    }
    if (!card || !srcCol) return BoardError::CardNotFound;

    Column* dstCol = findColumn(*board, toColumnId);
    if (!dstCol) return BoardError::ColumnNotFound;

    if (srcCol->id != dstCol->id) {
        Card moved = *card;
        srcCol->cards.erase(std::ranges::find_if(srcCol->cards,
                                                 [&](const Card& c) { return c.id == cardId; }));
        dstCol->cards.push_back(std::move(moved));
        storage_.saveBoard(*board);
        notifyChange(*board);
    }

    return std::monostate{};
}
