#ifndef SOR_SAFE_QUEUE_H
#define SOR_SAFE_QUEUE_H

#include "core/patient.h"
#include <deque>
#include <array>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

class SafeQueue
{
public:
    void push(const Patient& p);
    bool pop(Patient& out, std::atomic<bool>& running);
    size_t size() const;
    std::array<int, 4> priorityCounts() const;
    bool peekFront(int prio, Patient& out) const;
    void promoteFront(int fromPrio);
    void wakeAll();

    void applyAging(const std::chrono::milliseconds threshold[4],
                    const std::chrono::steady_clock::time_point& now);

private:
    std::array<std::deque<Patient>, 4> queues;
    mutable std::mutex mtx;
    std::condition_variable cv;
};

#endif // SOR_SAFE_QUEUE_H
