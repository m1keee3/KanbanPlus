#include <iostream>
#include "core/BoardService.h"
#include "src/storage/postgres/BoardStorage.h"

void RunTest(const BoardService& service) {
    Board board = service.createBoard("My Board");
    std::cout << "Created board: " << board.name << " [" << board.id << "]\n";

    auto col  = std::get<Column>(service.addColumn(board.id, "To Do"));
    auto col2 = std::get<Column>(service.addColumn(board.id, "In Progress"));

    auto card = std::get<Card>(service.addCard(board.id, col.id, "Implement storage", "Use PostgreSQL"));
    std::ignore = service.moveCard(board.id, card.id, col2.id);

    std::optional<Board> snapshot = service.getBoard(board.id);
    std::cout << "\n" << board.name << ":\n";
    for (const Column& column : snapshot->columns) {
        std::cout << "  [" << column.name << "]\n";
        for (const Card& c : column.cards)
            std::cout << "    - " << c.title << "\n";
    }
}

int main() {
    const char* connStrEnv = std::getenv("DATABASE_URL");
    if (!connStrEnv) {
        std::cerr << "Error: DATABASE_URL environment variable is not set\n";
        return 1;
    }

    BoardStorage storage{connStrEnv};
    storage.migrate();

    const BoardService service{storage};

    RunTest(service);

    return 0;
}
