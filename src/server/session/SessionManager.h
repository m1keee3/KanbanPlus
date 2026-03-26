#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>

class Session;

class SessionManager {
public:
    void join(const std::string& boardId, std::shared_ptr<Session> session);
    void leave(const std::string& boardId, const std::shared_ptr<Session>& session);
    void broadcast(const std::string& boardId, const std::string& message);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::unordered_set<std::shared_ptr<Session>>> rooms_;
};
