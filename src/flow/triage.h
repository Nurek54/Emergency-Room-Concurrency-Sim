#ifndef SOR_TRIAGE_H
#define SOR_TRIAGE_H

#include "flow/department.h"
#include "core/patient.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <chrono>
#include "core/config.h"

class Triage {
public:
    Triage(Department& surg, Department& ortho, Department& cardio,
           std::atomic<bool>& running, const Config& cfg);

    void newPatient(int id);
    std::size_t regLen();
    int nurseCount() const { return 4; }

    void join();
    void wakeAll();

    // Statystyki pracy
    void updateActivity();
    long getIdleTimeMs() const;
    long getFullLoadTimeMs() const;

    std::atomic<int> busy{0};
    int done() const { return processed_.load(); }

private:
    void nurseLoop();

    std::queue<int> regQueue_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;

    Department& surg_;
    Department& ortho_;
    Department& cardio_;
    std::atomic<bool>& running_;

    double pRed_, pYellow_, pGreen_, pBlue_;

    std::vector<std::thread> nurses_;

    std::atomic<int> processed_{0};

    long idleTimeMs_ = 0;
    long fullLoadTimeMs_ = 0;
    std::chrono::steady_clock::time_point lastCheck_ = std::chrono::steady_clock::now();
};

#endif // SOR_TRIAGE_H
