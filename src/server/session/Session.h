#pragma once
#include <memory>
#include <string>
#include <deque>
#include <mutex>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

class BoardService;
class SessionManager;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket,
            BoardService& service,
            SessionManager& sessions);
    ~Session();

    void run();

    void send(std::string message);

    void joinBoard(const std::string& boardId);

private:
    void onAccept(const boost::beast::error_code &ec);
    void doRead();
    void onRead(const boost::beast::error_code &ec, std::size_t bytes);
    void doWrite();
    void onWrite(const boost::beast::error_code &ec, std::size_t bytes);
    void handleMessage(std::string_view text);
    void leaveBoard();

    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
    boost::beast::flat_buffer readBuffer_;

    BoardService&   service_;
    SessionManager& sessions_;
    std::string     boardId_;

    std::mutex              writeMutex_;
    std::deque<std::string> writeQueue_;
};
