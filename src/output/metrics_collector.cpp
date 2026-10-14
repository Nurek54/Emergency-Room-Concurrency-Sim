#include "output/metrics_collector.h"
#include <chrono>
#include "core/sleep_util.h"

MetricsCollector::MetricsCollector(std::atomic<bool>& run,
                                   const Config& cfg,
                                   Department& s, Department& o,
                                   Department& c,
                                   ResourceManager& rm)
        : running_(run), cfg_(cfg),
          surg_(s), ortho_(o), cardio_(c), rm_(rm)
{
    out_.open("out/metrics.csv");
    out_ << "ts;queue_s;queue_o;queue_c;util_ct;util_or\n";
    th_ = std::thread(&MetricsCollector::loop, this);
}

void MetricsCollector::join()
{
    if (th_.joinable()) th_.join();
}

void MetricsCollector::loop()
{
    using namespace std::chrono;
    while (running_)
    {
        sleepWhileRunning(running_, seconds(10));
        if (!running_) break;

        auto ts = duration_cast<seconds>(
                steady_clock::now().time_since_epoch()).count();

        // Kolejki w oddziałach
        int qS  = surg_.queueLen();
        int qO  = ortho_.queueLen();
        int qC  = cardio_.queueLen();

        // Obliczamy wykorzystanie CT i OR „na piechotę”:
        int totCt = rm_.total("CT");
        int inUseCt = rm_.inUse("CT");
        double utilCt = totCt == 0 ? 0.0 : (double(inUseCt) / double(totCt));

        int totOr = rm_.total("OR");
        int inUseOr = rm_.inUse("OR");
        double utilOr = totOr == 0 ? 0.0 : (double(inUseOr) / double(totOr));

        out_ << ts   << ';'
             << qS   << ';'
             << qO   << ';'
             << qC   << ';'
             << utilCt << ';'
             << utilOr << '\n';

        out_.flush();
    }
}
