#include "sync/safe_queue.h"

void SafeQueue::push(const Patient& p) {
    std::lock_guard<std::mutex> lk(mtx);
    queues[static_cast<int>(p.priority)].push_back(p);
    cv.notify_one();
}

bool SafeQueue::pop(Patient& out, std::atomic<bool>& running) {
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [&] {
        for (auto& q : queues) if (!q.empty()) return true;
        return !running;
    });

    if (!running) {
        for (auto& q : queues)
            if (!q.empty()) goto found;
        return false;
    }

    found:
    for (int prio = 0; prio < 4; ++prio) {
        if (!queues[prio].empty()) {
            out = queues[prio].front();
            queues[prio].pop_front();
            return true;
        }
    }
    return false; // nie powinno się zdarzyć
}

void SafeQueue::wakeAll() {
    std::lock_guard<std::mutex> lk(mtx);
    cv.notify_all();
}

size_t SafeQueue::size() const {
    std::lock_guard<std::mutex> lk(mtx);
    size_t total = 0;
    for (const auto& q : queues)
        total += q.size();
    return total;
}

std::array<int, 4> SafeQueue::priorityCounts() const {
    std::lock_guard<std::mutex> lk(mtx);
    return {
            int(queues[0].size()),
            int(queues[1].size()),
            int(queues[2].size()),
            int(queues[3].size())
    };
}

bool SafeQueue::peekFront(int prio, Patient& out) const {
    std::lock_guard<std::mutex> lk(mtx);
    if (prio < 0 || prio > 3) return false;
    if (queues[prio].empty()) return false;
    out = queues[prio].front();
    return true;
}

void SafeQueue::promoteFront(int fromPrio) {
    std::lock_guard<std::mutex> lk(mtx);
    if (fromPrio < 1 || fromPrio > 3) return;
    if (queues[fromPrio].empty()) return;
    Patient p = queues[fromPrio].front();
    queues[fromPrio].pop_front();
    p.priority = static_cast<Priority>(fromPrio - 1);
    queues[fromPrio - 1].push_back(p);
}

void SafeQueue::applyAging(const std::chrono::milliseconds threshold[4],
                           const std::chrono::steady_clock::time_point& now)
{
    std::lock_guard<std::mutex> lk(mtx);

    for (int prio = 3; prio >= 1; --prio) {
        while (!queues[prio].empty()) {
            const Patient& p = queues[prio].front();
            if (now - p.queued >= threshold[prio]) {
                Patient prom = p;
                queues[prio].pop_front();
                prom.priority = static_cast<Priority>(prio - 1);
                queues[prio - 1].push_back(prom);
            } else {
                break;
            }
        }
    }
}
