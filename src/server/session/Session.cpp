#include "Session.h"
#include "SessionManager.h"
#include "../handlers/MessageHandler.h"
#include "../serialization/Serializer.h"
#include <iostream>

namespace beast     = boost::beast;
namespace websocket = beast::websocket;

Session::Session(boost::asio::ip::tcp::socket socket,
                 BoardService& service,
                 SessionManager& sessions)
    : ws_(std::move(socket))
    , service_(service)
    , sessions_(sessions)
{}

Session::~Session() {
    leaveBoard();
}

void Session::run() {
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
        res.set(beast::http::field::server, "KanbanPlus");
    }));
    ws_.async_accept(beast::bind_front_handler(&Session::onAccept, shared_from_this()));
}

void Session::onAccept(const beast::error_code &ec) {
    if (ec) {
        std::cerr << "accept: " << ec.message() << "\n";
        return;
    }
    doRead();
}

void Session::doRead() {
    ws_.async_read(readBuffer_,
                   beast::bind_front_handler(&Session::onRead, shared_from_this()));
}

void Session::onRead(const beast::error_code &ec, std::size_t) {
    if (ec == websocket::error::closed) return;
    if (ec) {
        std::cerr << "read: " << ec.message() << "\n";
        return;
    }

    const std::string text = beast::buffers_to_string(readBuffer_.data());
    readBuffer_.consume(readBuffer_.size());

    handleMessage(text);
    doRead();
}

void Session::handleMessage(const std::string_view text) {
    boost::system::error_code ec;
    boost::json::value val = boost::json::parse(text, ec);
    if (ec || !val.is_object()) {
        send(Serializer::errorResponse("invalid_json"));
        return;
    }
    MessageHandler handler{service_, sessions_};
    send(handler.handle(val.as_object(), *this));
}

void Session::joinBoard(const std::string& boardId) {
    leaveBoard();
    boardId_ = boardId;
    sessions_.join(boardId_, shared_from_this());
}

void Session::leaveBoard() {
    if (!boardId_.empty()) {
        sessions_.leave(boardId_, shared_from_this());
        boardId_.clear();
    }
}

void Session::send(std::string message) {
    bool startWrite;
    {
        std::lock_guard lock{writeMutex_};
        writeQueue_.push_back(std::move(message));
        startWrite = writeQueue_.size() == 1;
    }
    if (startWrite)
        boost::asio::post(ws_.get_executor(),
                          [self = shared_from_this()] { self->doWrite(); });
}

void Session::doWrite() {
    std::string* front;
    {
        std::lock_guard lock{writeMutex_};
        if (writeQueue_.empty()) return;
        front = &writeQueue_.front();
    }
    ws_.async_write(boost::asio::buffer(*front),
                    beast::bind_front_handler(&Session::onWrite, shared_from_this()));
}

void Session::onWrite(const beast::error_code &ec, std::size_t) {
    if (ec) {
        std::cerr << "write: " << ec.message() << "\n";
        return;
    }
    bool more;
    {
        std::lock_guard lock{writeMutex_};
        writeQueue_.pop_front();
        more = !writeQueue_.empty();
    }
    if (more) doWrite();
}
