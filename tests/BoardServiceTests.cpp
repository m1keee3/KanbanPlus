#include <gtest/gtest.h>
#include "storage/IBoardStorage.h"
#include "core/BoardService.h"
#include "storage/inmemory/BoardStorage.h"


class BoardServiceTest : public ::testing::Test {
protected:
    BoardStorage storage;
    BoardService service{storage};
};

// ─── Board tests ─────────────────────────────────────────────────────────────

TEST_F(BoardServiceTest, CreateBoard) {
    Board board = service.createBoard("My Board");

    EXPECT_FALSE(board.id.empty());
    EXPECT_EQ(board.name, "My Board");
    EXPECT_TRUE(board.columns.empty());
}

TEST_F(BoardServiceTest, GetBoard_ExistingId_ReturnsBoard) {
    Board created = service.createBoard("Test");

    std::optional<Board> found = service.getBoard(created.id);

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id, created.id);
    EXPECT_EQ(found->name, "Test");
}

TEST_F(BoardServiceTest, GetBoard_UnknownId_ReturnsNullopt) {
    std::optional<Board> found = service.getBoard("nonexistent");

    EXPECT_FALSE(found.has_value());
}

// ─── Column tests ─────────────────────────────────────────────────────────────

TEST_F(BoardServiceTest, AddColumn_ValidBoard_ReturnsColumn) {
    Board board = service.createBoard("Board");

    Result<Column> result = service.addColumn(board.id, "To Do");

    ASSERT_TRUE(std::holds_alternative<Column>(result));
    Column col = std::get<Column>(result);
    EXPECT_EQ(col.name, "To Do");
    EXPECT_FALSE(col.id.empty());
}

TEST_F(BoardServiceTest, AddColumn_UnknownBoard_ReturnsError) {
    Result<Column> result = service.addColumn("bad-id", "To Do");

    EXPECT_TRUE(std::holds_alternative<BoardError>(result));
    EXPECT_EQ(std::get<BoardError>(result), BoardError::BoardNotFound);
}

TEST_F(BoardServiceTest, RenameColumn) {
    Board board = service.createBoard("Board");
    Column col = std::get<Column>(service.addColumn(board.id, "Old Name"));

    Result<Column> result = service.renameColumn(board.id, col.id, "New Name");

    ASSERT_TRUE(std::holds_alternative<Column>(result));
    EXPECT_EQ(std::get<Column>(result).name, "New Name");
}

TEST_F(BoardServiceTest, RemoveColumn) {
    Board board = service.createBoard("Board");
    Column col = std::get<Column>(service.addColumn(board.id, "To Do"));

    Result<std::monostate> result = service.removeColumn(board.id, col.id);

    ASSERT_TRUE(std::holds_alternative<std::monostate>(result));
    std::optional<Board> updated = service.getBoard(board.id);
    EXPECT_TRUE(updated->columns.empty());
}

TEST_F(BoardServiceTest, RemoveColumn_UnknownColumn_ReturnsError) {
    Board board = service.createBoard("Board");

    Result<std::monostate> result = service.removeColumn(board.id, "bad-col");

    EXPECT_EQ(std::get<BoardError>(result), BoardError::ColumnNotFound);
}

// ─── Card tests ───────────────────────────────────────────────────────────────

TEST_F(BoardServiceTest, AddCard) {
    Board board = service.createBoard("Board");
    Column col = std::get<Column>(service.addColumn(board.id, "To Do"));

    Result<Card> result = service.addCard(board.id, col.id, "Task", "Details");

    ASSERT_TRUE(std::holds_alternative<Card>(result));
    Card card = std::get<Card>(result);
    EXPECT_EQ(card.title, "Task");
    EXPECT_EQ(card.description, "Details");
}

TEST_F(BoardServiceTest, AddCard_UnknownColumn_ReturnsError) {
    Board board = service.createBoard("Board");

    Result<Card> result = service.addCard(board.id, "bad-col", "Task");

    EXPECT_EQ(std::get<BoardError>(result), BoardError::ColumnNotFound);
}

TEST_F(BoardServiceTest, UpdateCard) {
    Board board = service.createBoard("Board");
    Column col = std::get<Column>(service.addColumn(board.id, "To Do"));
    Card card = std::get<Card>(service.addCard(board.id, col.id, "Old", ""));

    Result<Card> result = service.updateCard(board.id, col.id, card.id, "New", "Desc");

    ASSERT_TRUE(std::holds_alternative<Card>(result));
    Card updated = std::get<Card>(result);
    EXPECT_EQ(updated.title, "New");
    EXPECT_EQ(updated.description, "Desc");
}

TEST_F(BoardServiceTest, RemoveCard) {
    Board board = service.createBoard("Board");
    Column col = std::get<Column>(service.addColumn(board.id, "To Do"));
    Card card = std::get<Card>(service.addCard(board.id, col.id, "Task"));

    Result<std::monostate> result = service.removeCard(board.id, col.id, card.id);

    ASSERT_TRUE(std::holds_alternative<std::monostate>(result));
    std::optional<Board> updated = service.getBoard(board.id);
    EXPECT_TRUE(updated->columns.front().cards.empty());
}

TEST_F(BoardServiceTest, MoveCard_BetweenColumns) {
    Board board = service.createBoard("Board");
    Column col1 = std::get<Column>(service.addColumn(board.id, "To Do"));
    Column col2 = std::get<Column>(service.addColumn(board.id, "Done"));
    Card card = std::get<Card>(service.addCard(board.id, col1.id, "Task"));

    Result<std::monostate> result = service.moveCard(board.id, card.id, col2.id);

    ASSERT_TRUE(std::holds_alternative<std::monostate>(result));
    std::optional<Board> updated = service.getBoard(board.id);
    EXPECT_TRUE(updated->columns[0].cards.empty());
    EXPECT_EQ(updated->columns[1].cards.size(), 1u);
    EXPECT_EQ(updated->columns[1].cards[0].id, card.id);
}

TEST_F(BoardServiceTest, MoveCard_SameColumn_NoChange) {
    Board board = service.createBoard("Board");
    Column col = std::get<Column>(service.addColumn(board.id, "To Do"));
    Card card = std::get<Card>(service.addCard(board.id, col.id, "Task"));

    Result<std::monostate> result = service.moveCard(board.id, card.id, col.id);

    ASSERT_TRUE(std::holds_alternative<std::monostate>(result));
    std::optional<Board> updated = service.getBoard(board.id);
    EXPECT_EQ(updated->columns[0].cards.size(), 1u);
}

TEST_F(BoardServiceTest, MoveCard_UnknownCard_ReturnsError) {
    Board board = service.createBoard("Board");
    Column col = std::get<Column>(service.addColumn(board.id, "To Do"));

    Result<std::monostate> result = service.moveCard(board.id, "bad-card", col.id);

    EXPECT_EQ(std::get<BoardError>(result), BoardError::CardNotFound);
}
