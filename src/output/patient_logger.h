#ifndef SOR_PATIENT_LOGGER_H
#define SOR_PATIENT_LOGGER_H

#include "core/patient.h"
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <thread>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <array>

struct PatientStats {
    long long count = 0;
    double avgWaitMs = 0, avgTreatMs = 0;
    std::array<long long, 3> deptCount{};
    std::array<double, 3> deptAvgWaitMs{};
    std::array<long long, 4> prioCount{};
    std::array<double, 4> prioAvgWaitMs{};
};

struct LogEntry {
    Patient p;
    std::vector<std::string> equip;
    long waitMs;
    long treatMs;
};

class PatientLogger {
public:
    PatientLogger();
    ~PatientLogger();

    void start();                   // uruchom wątek
    void stop();                    // zakończ

    void log(const Patient& p,
             const std::vector<std::string>& equip,
             long waitMs,
             long treatMs);

    void summary(std::ostream& os) const;
    PatientStats stats() const;

private:
    void run();

    std::ofstream out_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<LogEntry> q_;
    std::thread th_;
    std::atomic<bool> running_{false};

    long long totalWait_  = 0;
    long long totalTreat_ = 0;
    long long count_      = 0;
    std::unordered_map<std::string,long long> equipCnt_;
    std::array<long long, 3> deptCount_{}, deptWait_{};
    std::array<long long, 4> prioCount_{}, prioWait_{};
};

extern PatientLogger gPatientLogger;

#endif // SOR_PATIENT_LOGGER_H
