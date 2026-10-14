#ifndef SOR_DEADLOCK_DETECTOR_H
#define SOR_DEADLOCK_DETECTOR_H

#include <atomic>
#include <thread>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include "sync/resource_manager.h"
#include "core/config.h"

class DeadlockDetector {
public:
    DeadlockDetector(ResourceManager& rm, const Config& cfg, std::atomic<bool>& run);
    void join();
    int alarms() const { return alarms_.load(); }

private:
    ResourceManager& rm_;
    const Config& cfg_;
    std::atomic<bool>& running_;
    std::thread th_;
    std::atomic<int> alarms_{0};

    // Dwa słowniki: ostatnia obserwowana liczba użytych sztuk oraz licznik niezmienności.
    std::unordered_map<std::string, int> lastInUse_;
    std::unordered_map<std::string, int> sameCounter_;

    void loop();
    bool detect();
    void resolve();
};

#endif // SOR_DEADLOCK_DETECTOR_H
