#include "sync/deadlock_detector.h"
#include <chrono>
#include <iostream>
#include <thread>
#include "core/sleep_util.h"

DeadlockDetector::DeadlockDetector(ResourceManager& rm, const Config& cfg,
                                   std::atomic<bool>& run)
        : rm_(rm), cfg_(cfg), running_(run)
{
    th_ = std::thread(&DeadlockDetector::loop, this);
}

void DeadlockDetector::join() {
    if (th_.joinable()) th_.join();
}

void DeadlockDetector::loop() {
    using namespace std::chrono;
    while (running_) {
        sleepWhileRunning(running_, milliseconds(cfg_.backoffMinMs));
        if (detect()) {
            ++alarms_;
            //std::cout << "[DetektorDeadlock] Wykryto mozliwy deadlock\n";
            resolve();
            sleepWhileRunning(running_, milliseconds(cfg_.backoffMaxMs));
        }
    }
}

bool DeadlockDetector::detect() {
    bool allStuck = true;

    for (const auto& key : rm_.getAllKeys()) {
        int nowUsed = rm_.inUse(key);
        if (lastInUse_[key] != nowUsed) {
            lastInUse_[key] = nowUsed;
            sameCounter_[key] = 0;
        } else {
            sameCounter_[key]++;
        }
        if (sameCounter_[key] < 3) // trzy cykle bez zmiany – to nie deadlock
            allStuck = false;
    }
    return allStuck;
}

void DeadlockDetector::resolve() {
    //std::cout << "[DetektorDeadlock] Proba rozwiazania zakleszczenia...\n";
    // Próbujemy zwolnić któryś z kluczowych zasobów:
    for (const std::string& key : {
            "OR", "ANEST", "ICU", "XRAY", "CT", "EKG", "VENT", "DIAL"
    })
    {
        int used = rm_.inUse(key);
        if (used > 0) {
            //std::cout << "  > Sygnal do zwolnienia zasobu: " << key << "\n";
            return;
        }
    }
    //std::cout << "  > Brak zasobow do zwolnienia\n";
}
