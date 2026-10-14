#include "flow/registrar.h"
#include <chrono>
#include <random>

Registrar::Registrar(Triage& t, std::atomic<bool>& run, int threadCount)
        : triage_(t), running_(run)
{
    for (int i = 0; i < threadCount; ++i)
        workers_.emplace_back(&Registrar::loop, this);
}

void Registrar::newPatient(int id) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        q_.push(id);
    }
    cv_.notify_one();
}

void Registrar::loop() {
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(50,200);
    while (running_) {
        int id = -1;
        {
            std::unique_lock<std::mutex> lk(mtx_);
            cv_.wait(lk, [&]{ return !q_.empty() || !running_; });
            if (!running_ && q_.empty()) break;
            id = q_.front(); q_.pop();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
        triage_.newPatient(id);
    }
}

void Registrar::join() {
    for (auto& t : workers_) if (t.joinable()) t.join();
}
