#include "MessageHandler.h"
#include "../session/Session.h"
#include "../session/SessionManager.h"
#include "../serialization/Serializer.h"

MessageHandler::MessageHandler(BoardService& service, SessionManager& sessions)
    : service_(service), sessions_(sessions) {}

std::string MessageHandler::handle(const boost::json::object& req, Session& session) const {
    const std::string_view action = field(req, "action");

    if (action == "create_board")  return handleCreateBoard(req);
    if (action == "join_board")    return handleJoinBoard(req, session);
    if (action == "add_column")    return handleAddColumn(req);
    if (action == "rename_column") return handleRenameColumn(req);
    if (action == "remove_column") return handleRemoveColumn(req);
    if (action == "add_card")      return handleAddCard(req);
    if (action == "update_card")   return handleUpdateCard(req);
    if (action == "remove_card")   return handleRemoveCard(req);
    if (action == "move_card")     return handleMoveCard(req);

    return Serializer::errorResponse("unknown_action");
}

std::string MessageHandler::handleCreateBoard(const boost::json::object& req) const {
    const Board board = service_.createBoard(std::string(field(req, "name")));
    return Serializer::okResponse(Serializer::serialize(board));
}

std::string MessageHandler::handleJoinBoard(const boost::json::object& req, Session& session) const {
    const std::string boardId{field(req, "boardId")};
    const std::optional<Board> board = service_.getBoard(boardId);
    if (!board) return Serializer::errorResponse("board_not_found");

    session.joinBoard(boardId);
    return Serializer::okResponse(Serializer::serialize(*board));
}

std::string MessageHandler::handleAddColumn(const boost::json::object& req) const {
    const Result<Column> result = service_.addColumn(
        std::string(field(req, "boardId")),
        std::string(field(req, "name")));
    if (std::holds_alternative<BoardError>(result))
        return Serializer::errorResponse(boardErrorMessage(std::get<BoardError>(result)));
    return Serializer::okResponse(Serializer::serialize(std::get<Column>(result)));
}

std::string MessageHandler::handleRenameColumn(const boost::json::object& req) const {
    const Result<Column> result = service_.renameColumn(
        std::string(field(req, "boardId")),
        std::string(field(req, "columnId")),
        std::string(field(req, "name")));
    if (std::holds_alternative<BoardError>(result))
        return Serializer::errorResponse(boardErrorMessage(std::get<BoardError>(result)));
    return Serializer::okResponse(Serializer::serialize(std::get<Column>(result)));
}

std::string MessageHandler::handleRemoveColumn(const boost::json::object& req) const {
    const Result<std::monostate> result = service_.removeColumn(
        std::string(field(req, "boardId")),
        std::string(field(req, "columnId")));
    if (std::holds_alternative<BoardError>(result))
        return Serializer::errorResponse(boardErrorMessage(std::get<BoardError>(result)));
    return Serializer::okResponse(nullptr);
}

std::string MessageHandler::handleAddCard(const boost::json::object& req) const {
    std::string description;
    if (req.contains("description"))
        description = std::string(field(req, "description"));

    const Result<Card> result = service_.addCard(
        std::string(field(req, "boardId")),
        std::string(field(req, "columnId")),
        std::string(field(req, "title")),
        description);
    if (std::holds_alternative<BoardError>(result))
        return Serializer::errorResponse(boardErrorMessage(std::get<BoardError>(result)));
    return Serializer::okResponse(Serializer::serialize(std::get<Card>(result)));
}

std::string MessageHandler::handleUpdateCard(const boost::json::object& req) const {
    const Result<Card> result = service_.updateCard(
        std::string(field(req, "boardId")),
        std::string(field(req, "columnId")),
        std::string(field(req, "cardId")),
        std::string(field(req, "title")),
        std::string(field(req, "description")));
    if (std::holds_alternative<BoardError>(result))
        return Serializer::errorResponse(boardErrorMessage(std::get<BoardError>(result)));
    return Serializer::okResponse(Serializer::serialize(std::get<Card>(result)));
}

std::string MessageHandler::handleRemoveCard(const boost::json::object& req) const {
    const Result<std::monostate> result = service_.removeCard(
        std::string(field(req, "boardId")),
        std::string(field(req, "columnId")),
        std::string(field(req, "cardId")));
    if (std::holds_alternative<BoardError>(result))
        return Serializer::errorResponse(boardErrorMessage(std::get<BoardError>(result)));
    return Serializer::okResponse(nullptr);
}

std::string MessageHandler::handleMoveCard(const boost::json::object& req) const {
    const Result<std::monostate> result = service_.moveCard(
        std::string(field(req, "boardId")),
        std::string(field(req, "cardId")),
        std::string(field(req, "toColumnId")));
    if (std::holds_alternative<BoardError>(result))
        return Serializer::errorResponse(boardErrorMessage(std::get<BoardError>(result)));
    return Serializer::okResponse(nullptr);
}

std::string_view MessageHandler::field(const boost::json::object& obj, std::string_view key) {
    const auto it = obj.find(key);
    if (it == obj.end() || !it->value().is_string()) return "";
    return it->value().as_string();
}

std::string MessageHandler::boardErrorMessage(const BoardError err) {
    switch (err) {
        case BoardError::BoardNotFound:  return "board_not_found";
        case BoardError::ColumnNotFound: return "column_not_found";
        case BoardError::CardNotFound:   return "card_not_found";
        default:                         return "unknown_error";
    }
}
