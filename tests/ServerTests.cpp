#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/json.hpp>

#include "server/Server.h"
#include "core/BoardService.h"
#include "storage/inmemory/BoardStorage.h"

namespace asio      = boost::asio;
namespace beast     = boost::beast;
namespace websocket = beast::websocket;
namespace json      = boost::json;


struct WsClient {
    asio::io_context ioc;
    websocket::stream<asio::ip::tcp::socket> ws{ioc};

    void connect(const unsigned short port) {
        asio::ip::tcp::resolver resolver{ioc};
        const auto ep = resolver.resolve("127.0.0.1", std::to_string(port));
        asio::connect(ws.next_layer(), ep);
        ws.handshake("127.0.0.1", "/");
    }

    void write(const std::string& msg) {
        ws.write(asio::buffer(msg));
    }

    std::string send(const std::string& msg) {
        write(msg);
        return read();
    }

    std::string read() {
        beast::flat_buffer buf;
        ws.read(buf);
        return beast::buffers_to_string(buf.data());
    }

    beast::error_code readWithError() {
        beast::flat_buffer buf;
        beast::error_code ec;
        ws.read(buf, ec);
        return ec;
    }

    void close() {
        beast::error_code ec;
        ws.close(websocket::close_code::normal, ec);
    }
};

class ServerTest : public ::testing::Test {
protected:
    asio::io_context        ioc_;
    BoardStorage            storage_;
    BoardService            service_{storage_};
    std::unique_ptr<Server> server_;
    std::vector<std::thread> threads_;

    void startServer(std::chrono::seconds idleTimeout = std::chrono::seconds{2}) {
        server_ = std::make_unique<Server>(ioc_, service_, 8080, idleTimeout);
        server_->start();
        for (int i = 0; i < 4; ++i)
            threads_.emplace_back([this] { ioc_.run(); });
    }

    unsigned short port() const { return server_->port(); }

    void TearDown() override {
        ioc_.stop();
        for (auto& t : threads_) t.join();
    }
};

static json::object parseObj(const std::string& s) {
    return json::parse(s).as_object();
}

static std::string boardId(const std::string& createBoardResponse) {
    return std::string(parseObj(createBoardResponse).at("data").as_object().at("id").as_string());
}
TEST_F(ServerTest, ConnectAndDisconnect) {
    startServer();
    WsClient client;
    client.connect(port());
    client.close();
}

TEST_F(ServerTest, MultipleClientsConnectConcurrently) {
    startServer();
    WsClient c1, c2, c3;
    c1.connect(port());
    c2.connect(port());
    c3.connect(port());
    c1.close();
    c2.close();
    c3.close();
}

TEST_F(ServerTest, InvalidJson_ReturnsError) {
    startServer();
    WsClient client;
    client.connect(port());

    const json::object resp = parseObj(client.send("trash {{{{"));

    EXPECT_EQ(resp.at("status").as_string(), "error");
    EXPECT_EQ(resp.at("message").as_string(), "invalid_json");
    client.close();
}

TEST_F(ServerTest, UnknownAction_ReturnsError) {
    startServer();
    WsClient client;
    client.connect(port());

    const json::object resp = parseObj(client.send(R"({"action":"no_such_action"})"));

    EXPECT_EQ(resp.at("status").as_string(), "error");
    EXPECT_EQ(resp.at("message").as_string(), "unknown_action");
    client.close();
}

TEST_F(ServerTest, MissingActionField_ReturnsUnknownAction) {
    startServer();
    WsClient client;
    client.connect(port());

    const json::object resp = parseObj(client.send(R"({})"));

    EXPECT_EQ(resp.at("status").as_string(), "error");
    EXPECT_EQ(resp.at("message").as_string(), "unknown_action");
    client.close();
}

TEST_F(ServerTest, MultipleInvalidRequests_SessionRemainsAlive) {
    startServer();
    WsClient client;
    client.connect(port());

    for (int i = 0; i < 5; ++i) {
        const json::object resp = parseObj(client.send("bad" + std::to_string(i)));
        EXPECT_EQ(resp.at("status").as_string(), "error");
    }
    client.close();
}

TEST_F(ServerTest, CreateBoard_ReturnsBoard) {
    startServer();
    WsClient client;
    client.connect(port());

    const json::object resp = parseObj(client.send(R"({"action":"create_board","name":"Sprint 1"})"));

    ASSERT_EQ(resp.at("status").as_string(), "ok");
    const json::object data = resp.at("data").as_object();
    EXPECT_EQ(data.at("name").as_string(), "Sprint 1");
    EXPECT_FALSE(data.at("id").as_string().empty());
    EXPECT_TRUE(data.at("columns").as_array().empty());
    client.close();
}

TEST_F(ServerTest, JoinBoard_NotFound_ReturnsError) {
    startServer();
    WsClient client;
    client.connect(port());

    const json::object resp = parseObj(
        client.send(R"({"action":"join_board","boardId":"nonexistent-id"})"));

    EXPECT_EQ(resp.at("status").as_string(), "error");
    EXPECT_EQ(resp.at("message").as_string(), "board_not_found");
    client.close();
}

TEST_F(ServerTest, JoinBoard_Success_ReturnsBoardState) {
    startServer();
    WsClient client;
    client.connect(port());

    const std::string createResp = client.send(R"({"action":"create_board","name":"Board"})");
    const std::string id = boardId(createResp);

    const json::object joinResp = parseObj(client.send(
        json::serialize(json::object{{"action", "join_board"}, {"boardId", id}})));

    ASSERT_EQ(joinResp.at("status").as_string(), "ok");
    EXPECT_EQ(joinResp.at("data").as_object().at("id").as_string(), id);
    client.close();
}

TEST_F(ServerTest, AddColumn_UnknownBoard_ReturnsError) {
    startServer();
    WsClient client;
    client.connect(port());

    const json::object resp = parseObj(client.send(
        R"({"action":"add_column","boardId":"bad-id","name":"To Do"})"));

    EXPECT_EQ(resp.at("status").as_string(), "error");
    EXPECT_EQ(resp.at("message").as_string(), "board_not_found");
    client.close();
}

TEST_F(ServerTest, AddColumn_Success) {
    startServer();
    WsClient client;
    client.connect(port());

    const std::string id = boardId(client.send(R"({"action":"create_board","name":"Board"})"));

    const json::object resp = parseObj(client.send(json::serialize(json::object{
        {"action", "add_column"}, {"boardId", id}, {"name", "To Do"}})));

    ASSERT_EQ(resp.at("status").as_string(), "ok");
    EXPECT_EQ(resp.at("data").as_object().at("name").as_string(), "To Do");
    client.close();
}

TEST_F(ServerTest, Broadcast_AllJoinedClientsReceiveBoardEvent) {
    startServer();
    WsClient client1, client2;
    client1.connect(port());
    client2.connect(port());

    const std::string id = boardId(client1.send(R"({"action":"create_board","name":"Board"})"));
    const std::string joinMsg = json::serialize(
        json::object{{"action", "join_board"}, {"boardId", id}});
    client1.send(joinMsg);
    client2.send(joinMsg);

    client1.write(json::serialize(json::object{
        {"action", "add_column"}, {"boardId", id}, {"name", "To Do"}}));

    const std::string msg1 = client1.read();
    const std::string msg2 = client1.read();

    const std::string c2msg = client2.read();

    const bool client1HasEvent =
        parseObj(msg1).contains("event") || parseObj(msg2).contains("event");
    EXPECT_TRUE(client1HasEvent);

    const json::object c2obj = parseObj(c2msg);
    ASSERT_EQ(c2obj.at("event").as_string(), "board_updated");
    EXPECT_EQ(c2obj.at("board").as_object().at("id").as_string(), id);

    client1.close();
    client2.close();
}

TEST_F(ServerTest, Broadcast_ClientThatLeftBoardDoesNotReceiveEvent) {
    startServer();
    WsClient client1, client2;
    client1.connect(port());
    client2.connect(port());

    const std::string id1 = boardId(client1.send(R"({"action":"create_board","name":"Board1"})"));
    const std::string id2 = boardId(client1.send(R"({"action":"create_board","name":"Board2"})"));

    client2.send(json::serialize(json::object{{"action", "join_board"}, {"boardId", id1}}));
    client2.send(json::serialize(json::object{{"action", "join_board"}, {"boardId", id2}}));

    client1.send(json::serialize(json::object{
        {"action", "add_column"}, {"boardId", id1}, {"name", "Col"}}));

    const json::object probe = parseObj(client2.send(R"({"action":"no_such_action"})"));
    EXPECT_EQ(probe.at("status").as_string(), "error");
    EXPECT_EQ(probe.at("message").as_string(), "unknown_action");

    client1.close();
    client2.close();
}

TEST_F(ServerTest, IdleClient_DisconnectedByTimeout) {
    startServer(std::chrono::seconds{2});

    WsClient client;
    client.connect(port());

    const beast::error_code ec = client.readWithError();
    EXPECT_TRUE(ec.failed());
}
