#ifndef SOR_HOSPITAL_H
#define SOR_HOSPITAL_H

#include <atomic>
#include <thread>
#include <csignal>
#include <chrono>
#include "core/config.h"
#include "sync/resource_manager.h"
#include "flow/arrival_dispatcher.h"
#include "flow/triage.h"
#include "flow/department.h"
#include "sync/deadlock_detector.h"
#include "output/metrics_collector.h"
#include "output/monitor.h"
#include "view/hospital_view.h"

class Hospital
{
    std::atomic<bool> running_{true};
    std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();
    Config            cfg_;

    /* moduły */
    ResourceManager   resMgr_;
    Department        surg_, ortho_, cardio_;
    Triage            triage_;
    ArrivalDispatcher arrival_;
    DeadlockDetector  deadlock_;
    MetricsCollector  metrics_;
    DashboardMonitor  monitor_;

    static Hospital*  gInstance_;

public:
    explicit Hospital(const Config& cfg);
    void run();
    void stop() { running_ = false; }

private:
    static void onSignal(int);
    HospitalSnapshot snapshot();
};

#endif /* SOR_HOSPITAL_H */
