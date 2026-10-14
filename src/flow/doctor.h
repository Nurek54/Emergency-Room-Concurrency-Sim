#ifndef SOR_DOCTOR_H
#define SOR_DOCTOR_H

#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include "core/patient.h"
#include "sync/safe_queue.h"
#include "core/types.h"

class Doctor {
public:
    Doctor(int id,
           DepartmentID dept,
           SafeQueue& queue,
           std::atomic<int>& busyCounter,
           std::atomic<int>& servedCounter,
           std::atomic<bool>& run);

    ~Doctor() = default;
    void join();

private:
    void work();

    int id_;
    DepartmentID dept_;
    SafeQueue& queue_;
    std::atomic<bool>& running_;
    std::atomic<int>& busy_;
    std::atomic<int>& served_;
    std::thread thread_;
};

#endif // SOR_DOCTOR_H
