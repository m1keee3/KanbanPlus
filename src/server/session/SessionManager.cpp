#include "SessionManager.h"
#include "Session.h"
#include <vector>

void SessionManager::join(const std::string& boardId, std::shared_ptr<Session> session) {
    std::lock_guard lock{mutex_};
    rooms_[boardId].insert(std::move(session));
}

void SessionManager::leave(const std::string& boardId, const std::shared_ptr<Session>& session) {
    std::lock_guard lock{mutex_};
    const auto it = rooms_.find(boardId);
    if (it == rooms_.end()) return;
    it->second.erase(session);
    if (it->second.empty())
        rooms_.erase(it);
}

void SessionManager::broadcast(const std::string& boardId, const std::string& message) {
    std::vector<std::shared_ptr<Session>> targets;
    {
        std::lock_guard lock{mutex_};
        const auto it = rooms_.find(boardId);
        if (it == rooms_.end()) return;
        targets.assign(it->second.begin(), it->second.end());
    }
    for (const std::shared_ptr<Session>& s : targets)
        s->send(message);
}
