#include "flow/department.h"
#include <chrono>

Department::Department(DepartmentID id, int doctorCount,
                       std::atomic<bool>& run)
        : id_(id), totalDoctors_(doctorCount)
{
    for (int i = 1; i <= doctorCount; ++i)
        docs_.push_back(std::make_unique<Doctor>(
                i, id_, queue_, busy, served, run));
    lastCheck_ = std::chrono::steady_clock::now();
}

void Department::join()
{
    for (auto& d : docs_) d->join();
}

void Department::applyAging()
{
    auto now = std::chrono::steady_clock::now();

    static const std::chrono::milliseconds agingThreshold[] = {
            std::chrono::hours(999),         // RED — brak starzenia
            std::chrono::milliseconds(8000), // YELLOW → RED (8 s)
            std::chrono::milliseconds(12000),// GREEN → YELLOW (12 s)
            std::chrono::milliseconds(18000) // BLUE → GREEN (18 s)
    };

    queue_.applyAging(agingThreshold, now);
    updateActivity();
}

void Department::updateActivity()
{
    auto now = std::chrono::steady_clock::now();
    long elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCheck_).count();
    lastCheck_ = now;

    int busyNow = busy.load();
    if (busyNow == 0)
        idleTimeMs_ += elapsedMs;
    if (busyNow == totalDoctors_)
        fullLoadTimeMs_ += elapsedMs;
}

long Department::getIdleTimeMs() const {
    return idleTimeMs_;
}

long Department::getFullLoadTimeMs() const {
    return fullLoadTimeMs_;
}
