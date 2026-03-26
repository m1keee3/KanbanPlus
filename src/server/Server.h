#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "session/SessionManager.h"

class BoardService;

class Server {
public:
    Server(boost::asio::io_context& ioc,
           BoardService& service,
           unsigned short port);

    void run(int threadCount);

private:
    void doAccept();

    boost::asio::io_context&       ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    BoardService&                  service_;
    SessionManager                 sessions_;
};
