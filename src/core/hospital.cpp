#include "core/hospital.h"
#include "output/patient_logger.h"
#include <chrono>
#include <iostream>
#include <csignal>
#include <limits>
#include <algorithm>

Hospital* Hospital::gInstance_ = nullptr;

Hospital::Hospital(const Config& cfg)
        : cfg_(cfg),
          resMgr_(cfg_),
          surg_ (DepartmentID::SURGERY   , cfg_.chirurg  , running_),
          ortho_(DepartmentID::ORTHOPEDIC, cfg_.ortopeda , running_),
          cardio_(DepartmentID::CARDIO   , cfg_.kardiolog, running_),
          triage_(surg_, ortho_, cardio_, running_, cfg_),
          arrival_(triage_, cfg_, running_),
          deadlock_(resMgr_, cfg_, running_),
          metrics_(running_, cfg_, surg_, ortho_, cardio_, resMgr_),
          monitor_(arrival_, triage_, surg_, ortho_, cardio_, resMgr_, running_)
{
    gInstance_ = this;
    std::signal(SIGINT, &Hospital::onSignal);

    gPatientLogger.start();
}

void Hospital::run()
{
    std::cout << "\n=== SYMULACJA START ===  (Ctrl+C albo zamknij okno, aby zatrzymac)\n";

    {
        HospitalView view([this] { return snapshot(); }, [this] { arrival_.requestMassCasualty(); });

        while (running_) {
            if (view.isOpen()) {
                if (!view.frame()) running_ = false;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            if (cfg_.maxPatients > 0) {
                int totalHandled = surg_.getServed()
                                   + ortho_.getServed()
                                   + cardio_.getServed();

                if (totalHandled >= cfg_.maxPatients) {
                    /*std::cout << "[Hospital] Wszyscy pacjenci (" << totalHandled
                              << "/" << cfg_.maxPatients << ") zostali obsluzeni.\n";*/
                    running_ = false;
                }
            }
        }
    }

    std::cout << "\nZatrzymywanie...\n";

    // Watki czekajace na condition_variable trzeba obudzic, inaczej join() czekalby w nieskonczonosc
    triage_.wakeAll();
    surg_.wakeAll();
    ortho_.wakeAll();
    cardio_.wakeAll();

    arrival_.join();
    triage_.join();
    surg_.join();
    ortho_.join();
    cardio_.join();
    deadlock_.join();
    metrics_.join();
    monitor_.join();
    resMgr_.waitForRestocks(3000);

    gPatientLogger.stop();
    gPatientLogger.summary(std::cout);

    std::cout << "Koniec symulacji.\n";

    std::cout << "\nWcisnij ENTER, aby zakonczyc...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

HospitalSnapshot Hospital::snapshot()
{
    HospitalSnapshot s;
    s.elapsedSec   = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
    s.lambdaPerSec = cfg_.lambdaPerSec;
    s.arrived      = arrival_.arrived();
    s.triageQueue  = static_cast<int>(triage_.regLen());
    s.triageBusy   = triage_.busy.load();
    s.nurses       = triage_.nurseCount();
    s.triaged      = triage_.done();

    Department* depts[3] = {&surg_, &ortho_, &cardio_};
    for (int i = 0; i < 3; ++i) {
        s.dept[i].queue   = depts[i]->getQueueCounts();
        s.dept[i].busy    = depts[i]->busy.load();
        s.dept[i].doctors = depts[i]->getTotalDoctors();
        s.dept[i].served  = depts[i]->getServed();
    }

    for (const char* key : {"CT", "XRAY", "USG", "OR", "ANEST", "ICU", "DEFIB", "ECHO", "VENT",
                            "DIAL", "ENDO", "LAB", "BLOOD", "NEURO", "TRAUMA_KIT", "ORTHO_SET", "EKG"}) {
        ResourceSnapshot r;
        r.key       = key;
        r.total     = resMgr_.total(key);
        r.renewable = resMgr_.isRenewable(key);
        r.cooling   = r.renewable ? resMgr_.pendingRestockCount(key) : 0;
        r.inUse     = std::max(0, r.total - resMgr_.available(key) - r.cooling);
        s.resources.push_back(r);
    }

    s.massEnabled    = cfg_.massEnabled;
    s.massFired      = arrival_.massFired();
    s.massAtSec      = cfg_.massAtSec;
    s.massSize       = static_cast<int>(cfg_.massRatePerSec);
    s.deadlockAlarms = deadlock_.alarms();
    s.stats          = gPatientLogger.stats();
    return s;
}

void Hospital::onSignal(int)
{
    if (gInstance_)
        gInstance_->stop();
}
