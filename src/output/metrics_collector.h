#ifndef SOR_METRICS_COLLECTOR_H
#define SOR_METRICS_COLLECTOR_H

#include <thread>
#include <atomic>
#include <fstream>
#include "core/config.h"
#include "flow/department.h"
#include "sync/resource_manager.h"

class MetricsCollector {
    std::atomic<bool>& running_;
    const Config&      cfg_;
    Department&        surg_;
    Department&        ortho_;
    Department&        cardio_;
    ResourceManager&   rm_;

    std::thread        th_;
    std::ofstream      out_;

public:
    MetricsCollector(std::atomic<bool>& run,
                     const Config& cfg,
                     Department& s,
                     Department& o,
                     Department& c,
                     ResourceManager& rm);

    void join();

private:
    void loop();
};

#endif /* SOR_METRICS_COLLECTOR_H */
