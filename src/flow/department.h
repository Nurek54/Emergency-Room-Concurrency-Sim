#ifndef SOR_DEPARTMENT_H
#define SOR_DEPARTMENT_H

#include "flow/doctor.h"
#include "sync/safe_queue.h"
#include <vector>
#include <array>
#include <atomic>
#include <chrono>

class Department
{
public:
    Department(DepartmentID id, int doctorCount, std::atomic<bool>& run);
    void join();

    void applyAging();          // starzenie kolejki
    void updateActivity();      // wywoływane co 1 s – liczy idle / full-load

    void addPatient(const Patient& p) { queue_.push(p); }
    void wakeAll()                    { queue_.wakeAll(); }
    size_t queueLen() const           { return queue_.size(); }

    std::array<int,4> getQueueCounts() const { return queue_.priorityCounts(); }
    int  getFreeDoctors()  const { return totalDoctors_ - busy.load(); }
    int  getTotalDoctors() const { return totalDoctors_; }
    int  getServed()       const { return served.load(); }

    long getIdleTimeMs() const;
    long getFullLoadTimeMs() const;

    std::atomic<int> busy{0}, served{0};

private:
    DepartmentID id_;
    SafeQueue    queue_;
    std::vector<std::unique_ptr<Doctor>> docs_;
    int  totalDoctors_;

    std::chrono::steady_clock::time_point lastCheck_ = std::chrono::steady_clock::now();
    long idleTimeMs_      = 0;
    long fullLoadTimeMs_  = 0;
};

#endif /* SOR_DEPARTMENT_H */
