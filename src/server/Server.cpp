#include "Server.h"
#include "session/Session.h"
#include "serialization/Serializer.h"
#include "../core/BoardService.h"
#include <boost/asio/strand.hpp>
#include <iostream>
#include <thread>
#include <vector>

namespace asio = boost::asio;

Server::Server(asio::io_context& ioc, BoardService& service, unsigned short port)
    : ioc_(ioc)
    , acceptor_(asio::make_strand(ioc), {asio::ip::tcp::v4(), port})
    , service_(service)
{
    service_.setOnChange([this](const Board& board) {
        sessions_.broadcast(board.id, Serializer::boardEvent(board));
    });
}

void Server::run(const int threadCount) {
    doAccept();

    std::vector<std::thread> threads;
    threads.reserve(threadCount - 1);
    for (int i = 0; i < threadCount - 1; ++i)
        threads.emplace_back([this] { ioc_.run(); });

    std::cout << "KanbanPlus listening on port "
              << acceptor_.local_endpoint().port()
              << " (" << threadCount << " threads)\n";

    ioc_.run();

    for (std::thread& t : threads)
        t.join();
}

void Server::doAccept() {
    acceptor_.async_accept(
        asio::make_strand(ioc_),
        [this](const boost::beast::error_code &ec, asio::ip::tcp::socket socket) {
            if (!ec)
                std::make_shared<Session>(std::move(socket), service_, sessions_)->run();
            else
                std::cerr << "accept: " << ec.message() << "\n";
            doAccept();
        });
}
