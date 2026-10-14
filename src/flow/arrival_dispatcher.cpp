#include "flow/arrival_dispatcher.h"
#include <random>
#include <chrono>
#include <iostream>
#include "core/sleep_util.h"

ArrivalDispatcher::ArrivalDispatcher(Triage& tr, const Config& cfg,
                                     std::atomic<bool>& run)
        : triage_(tr), cfg_(cfg), running_(run),
          start_(std::chrono::steady_clock::now())
{
    th_ = std::thread(&ArrivalDispatcher::loop, this);
}

void ArrivalDispatcher::join() {
    if (th_.joinable()) th_.join();
}

const std::map<long, int>& ArrivalDispatcher::getArrivalTimeline() const {
    return arrivalTimeline_;
}

void ArrivalDispatcher::loop()
{
    std::mt19937 gen(std::random_device{}());

    double lambda = (cfg_.lambdaPerSec <= 0.0) ? 0.001 : cfg_.lambdaPerSec;
    std::exponential_distribution<double> exp(lambda);

    int nextId = 1;

    while (running_)
    {
        if (cfg_.maxPatients >= 0 && totalArrived_ >= cfg_.maxPatients) {
            /*std::cout << "[ArrivalDispatcher] Osiagnieto limit "
                      << cfg_.maxPatients << " pacjentow. Wstrzymano dalszy naplyw.\n";*/
            break;
        }

        sleepWhileRunning(running_, std::chrono::duration<double>(exp(gen)));
        if (!running_) break;

        triage_.newPatient(nextId++);
        ++totalArrived_;

        long ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_).count();
        arrivalTimeline_[ts / 1000]++;

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_).count();

        const bool scheduled = cfg_.massEnabled && !massFired_ && elapsed >= cfg_.massAtSec;
        if (scheduled || massRequested_.exchange(false))
        {
            massFired_ = true;
            int perSec = static_cast<int>(cfg_.massRatePerSec);
            /*std::cout << "[ArrivalDispatcher] zdarzenie masowe! +" << perSec
                      << " pacjentow/s\n";*/

            for (int i = 0; i < perSec; ++i) {
                if (cfg_.maxPatients >= 0 && totalArrived_ >= cfg_.maxPatients) break;
                triage_.newPatient(nextId++);
                ++totalArrived_;

                long tsMass = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start_).count();
                arrivalTimeline_[tsMass / 1000]++;
            }
        }
    }
}
