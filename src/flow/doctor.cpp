#include "flow/doctor.h"
#include "sync/resource_manager.h"
#include "output/patient_logger.h"
#include <random>
#include <chrono>
#include <vector>
#include <array>
#include <algorithm>

extern ResourceManager* gResMgr;

Doctor::Doctor(int id,
               DepartmentID dept,
               SafeQueue& queue,
               std::atomic<int>& busyCounter,
               std::atomic<int>& servedCounter,
               std::atomic<bool>& run)
        : id_(id),
          dept_(dept),
          queue_(queue),
          running_(run),
          busy_(busyCounter),
          served_(servedCounter)
{
    thread_ = std::thread(&Doctor::work, this);
}

void Doctor::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Doctor::work() {
    std::mt19937 gen{std::random_device{}()};
    // Kontakt z pacjentem: 200–500 ms
    std::uniform_int_distribution<int> contactTimeDist(200, 500);
    std::uniform_real_distribution<double> probDist(0.0, 1.0);

    while (running_) {
        Patient p;
        if (!queue_.pop(p, running_)) {
            break;
        }

        auto startTime = std::chrono::steady_clock::now();
        long waitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                startTime - p.queued)
                .count();

        std::vector<std::string> equipUsed;
        auto use = [&](const char* key) {
            equipUsed.push_back(key);
            gResMgr->acquire(key);
        };
        auto free = [&](const char* key) {
            gResMgr->release(key);
        };

        ++busy_;
        std::this_thread::sleep_for(std::chrono::milliseconds(contactTimeDist(gen)));

        // --- Leczenie według oddziału ---
        if (dept_ == DepartmentID::SURGERY) {
            if (probDist(gen) < 0.25) {
                // 25% szans na operację chirurgiczną
                use("ANEST");
                use("OR");
                // operacja: 600–1000 ms
                std::uniform_int_distribution<int> opTimeDist(600, 1000);
                int opTime = opTimeDist(gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(opTime));
                free("OR");
                free("ANEST");

                if (probDist(gen) < 0.50) {
                    // 50% szans na OIOM po operacji
                    use("ICU");
                    // pobyt na OIOM: 400–800 ms
                    std::uniform_int_distribution<int> icuTimeDist(400, 800);
                    int icuTime = icuTimeDist(gen);
                    std::this_thread::sleep_for(std::chrono::milliseconds(icuTime));
                    free("ICU");
                }
            } else {
                // 75% szans na skan CT
                use("CT");
                // skan CT: 300–600 ms
                std::uniform_int_distribution<int> scanTimeDist(300, 600);
                int scanTime = scanTimeDist(gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(scanTime));
                free("CT");
            }
        }
        else if (dept_ == DepartmentID::ORTHOPEDIC) {
            if (probDist(gen) < 0.30) {
                // 30% szans na operację ortopedyczną
                use("OR");
                // operacja ortopedyczna: 500–900 ms
                std::uniform_int_distribution<int> orthoOpTime(500, 900);
                int orthoTime = orthoOpTime(gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(orthoTime));
                free("OR");
            } else {
                // 70% szans na standardowe procedury
                use("XRAY");
                std::uniform_int_distribution<int> xrayTime(150, 300);
                int xrayDur = xrayTime(gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(xrayDur));
                free("XRAY");

                use("TRAUMA_KIT");
                use("ORTHO_SET");
                // procedury ortopedyczne: 200–400 ms
                std::uniform_int_distribution<int> orthoKitTime(200, 400);
                int kitTime = orthoKitTime(gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(kitTime));
                free("ORTHO_SET");
                free("TRAUMA_KIT");
            }
        }
        else { // DepartmentID::CARDIO
            // podstawowe EKG: 100–200 ms
            use("EKG");
            std::uniform_int_distribution<int> ekgTime(100, 200);
            int ekgDur = ekgTime(gen);
            std::this_thread::sleep_for(std::chrono::milliseconds(ekgDur));
            free("EKG");

            if (probDist(gen) < 0.50) {
                // 50% szans na echo: 200–400 ms
                use("ECHO");
                std::uniform_int_distribution<int> echoTime(200, 400);
                int echoDur = echoTime(gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(echoDur));
                free("ECHO");
            }
            if (probDist(gen) < 0.30) {
                // 30% szans na defibrylację: 80–150 ms
                use("DEFIB");
                std::uniform_int_distribution<int> defibTime(80, 150);
                int defibDur = defibTime(gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(defibDur));
                free("DEFIB");
            }
        }

        struct ToolSpec {
            const char* key;
            double      p;
            int         minMs;
            int         maxMs;
        };
        static const std::array<ToolSpec, 7> tools = {{
                                                              { "VENT",   0.25, 200, 400 },   // 25% szansy, 200–400 ms
                                                              { "DIAL",   0.10, 400, 800 },   // 10% szansy, 400–800 ms
                                                              { "LAB",    0.15, 150, 300 },   // 15% szansy, 150–300 ms
                                                              { "BLOOD",  0.10, 100, 250 },   // 10% szansy, 100–250 ms
                                                              { "ENDO",   0.15, 300, 600 },   // 15% szansy, 300–600 ms
                                                              { "NEURO",  0.12, 200, 400 },   // 12% szansy, 200–400 ms
                                                              { "USG",    0.18, 250, 500 }    // 18% szansy, 250–500 ms
                                                      }};

        for (const auto& tool : tools) {
            if (probDist(gen) < tool.p) {
                use(tool.key);
                std::uniform_int_distribution<int> dur(tool.minMs, tool.maxMs);
                int toolDur = dur(gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(toolDur));
                free(tool.key);
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        long treatMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                endTime - startTime)
                .count();

        gPatientLogger.log(p, equipUsed, waitMs, treatMs);

        ++served_;
        --busy_;
    }
}
