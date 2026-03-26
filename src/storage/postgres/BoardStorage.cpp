#include "BoardStorage.h"
#include <pqxx/pqxx>
#include <stdexcept>
#include <utility>

BoardStorage::BoardStorage(std::string connectionString)
    : connectionString_(std::move(connectionString)) {}

void BoardStorage::migrate() const {
    pqxx::connection conn{connectionString_};
    pqxx::work tx{conn};

    tx.exec(R"sql(
        CREATE TABLE IF NOT EXISTS boards (
            id   TEXT PRIMARY KEY,
            name TEXT NOT NULL
        )
    )sql");

    tx.exec(R"sql(
        CREATE TABLE IF NOT EXISTS columns (
            id       TEXT PRIMARY KEY,
            board_id TEXT NOT NULL REFERENCES boards(id) ON DELETE CASCADE,
            name     TEXT NOT NULL,
            position INTEGER NOT NULL DEFAULT 0
        )
    )sql");

    tx.exec(R"sql(
        CREATE TABLE IF NOT EXISTS cards (
            id          TEXT PRIMARY KEY,
            column_id   TEXT NOT NULL REFERENCES columns(id) ON DELETE CASCADE,
            title       TEXT NOT NULL,
            description TEXT NOT NULL DEFAULT '',
            position    INTEGER NOT NULL DEFAULT 0
        )
    )sql");

    tx.commit();
}

void BoardStorage::saveBoard(const Board& board) {
    pqxx::connection conn{connectionString_};
    pqxx::work tx{conn};

    tx.exec(
        "INSERT INTO boards(id, name) VALUES($1, $2) "
        "ON CONFLICT(id) DO UPDATE SET name = EXCLUDED.name",
        pqxx::params{board.id, board.name});

    tx.exec("DELETE FROM columns WHERE board_id = $1",
        pqxx::params{board.id});

    for (int ci = 0; ci < static_cast<int>(board.columns.size()); ++ci) {
        const Column& col = board.columns[ci];
        tx.exec(
            "INSERT INTO columns(id, board_id, name, position) VALUES($1, $2, $3, $4)",
            pqxx::params{col.id, board.id, col.name, ci});

        for (int ki = 0; ki < static_cast<int>(col.cards.size()); ++ki) {
            const Card& card = col.cards[ki];
            tx.exec(
                "INSERT INTO cards(id, column_id, title, description, position) "
                "VALUES($1, $2, $3, $4, $5)",
                pqxx::params{card.id, col.id, card.title, card.description, ki});
        }
    }

    tx.commit();
}

std::vector<Board> BoardStorage::loadAll() const {
    pqxx::connection conn{connectionString_};
    pqxx::work tx{conn};

    std::vector<Board> boards;

    for (const auto& row : tx.exec("SELECT id, name FROM boards ORDER BY id")) {
        Board b;
        b.id   = row["id"].c_str();
        b.name = row["name"].c_str();

        for (const auto& crow : tx.exec(
                "SELECT id, name FROM columns WHERE board_id = $1 ORDER BY position",
                pqxx::params{b.id})) {
            Column col;
            col.id   = crow["id"].c_str();
            col.name = crow["name"].c_str();

            for (const auto& krow : tx.exec(
                    "SELECT id, title, description FROM cards "
                    "WHERE column_id = $1 ORDER BY position",
                    pqxx::params{col.id})) {
                Card card;
                card.id          = krow["id"].c_str();
                card.title       = krow["title"].c_str();
                card.description = krow["description"].c_str();
                col.cards.push_back(std::move(card));
            }

            b.columns.push_back(std::move(col));
        }

        boards.push_back(std::move(b));
    }

    tx.commit();
    return boards;
}

std::optional<Board> BoardStorage::loadBoard(const std::string& boardId) const {
    pqxx::connection conn{connectionString_};
    pqxx::work tx{conn};

    pqxx::result res = tx.exec("SELECT id, name FROM boards WHERE id = $1",
                                pqxx::params{boardId});
    if (res.empty()) return std::nullopt;

    Board b;
    b.id   = res[0]["id"].c_str();
    b.name = res[0]["name"].c_str();

    for (const auto& crow : tx.exec(
            "SELECT id, name FROM columns WHERE board_id = $1 ORDER BY position",
            pqxx::params{b.id})) {
        Column col;
        col.id   = crow["id"].c_str();
        col.name = crow["name"].c_str();

        for (const auto& krow : tx.exec(
                "SELECT id, title, description FROM cards "
                "WHERE column_id = $1 ORDER BY position",
                pqxx::params{col.id})) {
            Card card;
            card.id          = krow["id"].c_str();
            card.title       = krow["title"].c_str();
            card.description = krow["description"].c_str();
            col.cards.push_back(std::move(card));
        }

        b.columns.push_back(std::move(col));
    }

    tx.commit();
    return b;
}

void BoardStorage::deleteBoard(const std::string& boardId) {
    pqxx::connection conn{connectionString_};
    pqxx::work tx{conn};
    tx.exec("DELETE FROM boards WHERE id = $1", pqxx::params{boardId});
    tx.commit();
}
