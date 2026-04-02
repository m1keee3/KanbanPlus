#pragma once
#include <chrono>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "session/SessionManager.h"

class BoardService;

class Server {
public:
    Server(boost::asio::io_context& ioc,
           BoardService& service,
           unsigned short port,
           std::chrono::seconds idleTimeout = std::chrono::seconds{0});

    void start();
    void run(int threadCount);

    unsigned short port() const;

private:
    void doAccept();

    boost::asio::io_context&       ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    BoardService&                  service_;
    SessionManager                 sessions_;
    std::chrono::seconds           idleTimeout_;
};
