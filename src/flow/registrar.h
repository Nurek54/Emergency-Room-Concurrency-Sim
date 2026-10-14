#ifndef SOR_REGISTRAR_H
#define SOR_REGISTRAR_H

#include "flow/triage.h"
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

class Registrar {
    std::queue<int>           q_;
    std::mutex                mtx_;
    std::condition_variable   cv_;
    std::atomic<bool>&        running_;
    Triage&                   triage_;
    std::vector<std::thread>  workers_;

public:
    Registrar(Triage& t, std::atomic<bool>& run, int threadCount = 2);
    void newPatient(int id);
    void join();

private:
    void loop();
};

#endif // SOR_REGISTRAR_H
