#pragma once
#include <string>
#include <boost/json.hpp>
#include "../../core/BoardService.h"

class SessionManager;
class Session;

class MessageHandler {
public:
    MessageHandler(BoardService& service, SessionManager& sessions);

    std::string handle(const boost::json::object& req, Session& session) const;

private:
    BoardService&   service_;
    SessionManager& sessions_;

    std::string handleCreateBoard(const boost::json::object& req) const;
    std::string handleJoinBoard(const boost::json::object& req, Session& session) const;
    std::string handleAddColumn(const boost::json::object& req) const;
    std::string handleRenameColumn(const boost::json::object& req) const;
    std::string handleRemoveColumn(const boost::json::object& req) const;
    std::string handleAddCard(const boost::json::object& req) const;
    std::string handleUpdateCard(const boost::json::object& req) const;
    std::string handleRemoveCard(const boost::json::object& req) const;
    std::string handleMoveCard(const boost::json::object& req) const;

    static std::string_view field(const boost::json::object& obj, std::string_view key);
    static std::string boardErrorMessage(BoardError err);
};
