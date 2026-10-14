#include "output/monitor.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "output/patient_logger.h"
#include "core/sleep_util.h"
#include "sync/resource_manager.h"

#define CLR_RESET    "\033[0m"
#define CLR_BOLD     "\033[1m"
#define CLR_YELLOW   "\033[1;33m"
#define CLR_GREEN    "\033[1;32m"
#define CLR_RED      "\033[1;31m"
#define CLR_WHITE    "\033[1;37m"
#define CLR_CYAN     "\033[1;36m"
#define CLR_MAGENTA  "\033[1;35m"
#define CLR_GRAY     "\033[1;30m"

DashboardMonitor::DashboardMonitor(ArrivalDispatcher& a,
                                   Triage& t,
                                   Department& s,
                                   Department& o,
                                   Department& c,
                                   ResourceManager& rm,
                                   std::atomic<bool>& run)
        : arrival_(a)
        , triage_(t)
        , surg_(s)
        , ortho_(o)
        , cardio_(c)
        , rm_(rm)
        , running_(run)
        , start_(std::chrono::steady_clock::now())
{
    th_ = std::thread(&DashboardMonitor::loop, this);
}

void DashboardMonitor::join() {
    if (th_.joinable()) th_.join();
}

void DashboardMonitor::loop() {
    using namespace std::chrono_literals;

    while (running_) {
        surg_.applyAging();  ortho_.applyAging();  cardio_.applyAging();

        triage_.updateActivity();
        surg_.updateActivity();
        ortho_.updateActivity();
        cardio_.updateActivity();

        sleepWhileRunning(running_, 1s);

        auto now      = std::chrono::steady_clock::now();
        auto elapsed  = std::chrono::duration_cast<std::chrono::seconds>(now - start_);
        std::string simTime = hhmmss(elapsed);

        int active = triage_.regLen() + triage_.busy
                     + surg_.queueLen() + ortho_.queueLen() + cardio_.queueLen()
                     + surg_.busy + ortho_.busy + cardio_.busy;

#ifdef _WIN32
        system("cls");
#else
        std::cout << "\033[2J\033[H" << std::flush;
#endif
        std::cout << CLR_BOLD << CLR_YELLOW
                  << "==================== S O R    D A S H B O A R D ====================\n"
                  << CLR_CYAN << "Czas symulacji: " << CLR_WHITE << simTime
                  << CLR_CYAN << "   Aktywni pacjenci: " << CLR_WHITE << std::setw(6) << active << "\n\n";

        /* wejście + triage */
        std::cout << CLR_BOLD << CLR_GREEN
                  << "[Wejscie] " << CLR_CYAN << "przyjeto: " << CLR_WHITE << arrival_.arrived()
                  << CLR_CYAN << "   w kolejce do triage: " << CLR_WHITE << triage_.regLen() << '\n'
                  << "[Triage]  pielegniarki zajete: "
                  << ((triage_.busy >= triage_.nurseCount()) ? CLR_RED : CLR_GREEN)
                  << triage_.busy << CLR_WHITE << "/" << triage_.nurseCount()
                  << CLR_CYAN << "   obsluzono: " << CLR_WHITE << triage_.done() << "\n\n";

        /* oddziały */
        std::cout << CLR_BOLD << CLR_MAGENTA
                  << "Oddzial      |  R |  Y |  G |  B | Wolni/Lek | Obsluzeni |\n"
                  << "-------------|----|----|----|----|------------|------------|\n";
        auto deptLine = [&](Department& d, const char* name){
            auto cnt  = d.getQueueCounts();
            int freeD = d.getFreeDoctors();
            int totD  = d.getTotalDoctors();

            std::cout << CLR_CYAN << std::left << std::setw(13) << name << CLR_WHITE << "|"
                      << std::right
                      << std::setw(3) << cnt[0] << " |"
                      << std::setw(3) << cnt[1] << " |"
                      << std::setw(3) << cnt[2] << " |"
                      << std::setw(3) << cnt[3] << " | "
                      << ((freeD == 0) ? CLR_RED : CLR_GREEN)
                      << std::setw(3) << freeD << "/" << std::setw(3) << totD
                      << CLR_WHITE << "   | "
                      << CLR_YELLOW << std::setw(10) << d.getServed() << CLR_WHITE << " |\n";
        };
        deptLine(surg_,  "Chirurgia");
        deptLine(ortho_, "Ortopedia");
        deptLine(cardio_,"Kardiologia");
        std::cout << '\n';

        /* zasoby */
        std::map<std::string,int> delays;
        std::vector<std::string>  renewable;
        {
            std::map<std::string,long> t; std::map<std::string,int> c; std::map<std::string,long> a;
            rm_.getStats(t,c,a,delays,renewable);
        }
        std::cout << CLR_BOLD << CLR_MAGENTA
                  << "Zasob               | Zajete/Wszystkie | Odnowione | W trakcie | % uzycia |\n"
                  << "--------------------|------------------|-----------|-----------|-----------|\n";

        auto resLine = [&](const char* label,const char* key){
            bool isRen   = std::find(renewable.begin(),renewable.end(),key)!=renewable.end();
            int totalCap = rm_.total(key);
            int pending  = isRen ? rm_.pendingRestockCount(key) : 0;
            int availRaw = rm_.available(key);

            int effTotal = std::max(0,totalCap - pending);
            int used     = std::max(0,(totalCap - availRaw) - pending);
            int util     = effTotal ? int(double(used)/effTotal * 100 + 0.5) : 0;

            std::string outRest = isRen ? std::to_string(rm_.restockedCount(key)) : "--";
            std::string outPend = isRen ? std::to_string(pending)                : "--";

            std::cout << CLR_CYAN << std::left << std::setw(20) << label << CLR_WHITE << "| "
                      << ((used==effTotal&&effTotal>0)?CLR_RED:(used==0?CLR_GRAY:CLR_GREEN))
                      << std::right << std::setw(4) << used << "/" << std::setw(3) << effTotal
                      << "    | "
                      << std::setw(4) << outRest << "    | "
                      << std::setw(4) << outPend << "    | "
                      << ((util>=90)?CLR_RED:(util>=50)?CLR_YELLOW:CLR_GREEN)
                      << std::setw(3) << util << " %" << CLR_WHITE
                      << "         |\n";
        };

        resLine("CT-scanner",   "CT");       resLine("RTG",          "XRAY");
        resLine("USG",          "USG");      resLine("Sale oper.",   "OR");
        resLine("Anestezjol.",  "ANEST");    resLine("Lozka OIOM",   "ICU");
        resLine("Defibrylatory","DEFIB");    resLine("Echo serca",   "ECHO");
        resLine("Respirator",   "VENT");     resLine("Dializator",   "DIAL");
        resLine("Endoskop",     "ENDO");     resLine("Laboratorium", "LAB");
        resLine("Zest. krwi",   "BLOOD");    resLine("Neuro konsult.","NEURO");
        resLine("Zest. trauma", "TRAUMA_KIT"); resLine("Ortho-set",  "ORTHO_SET");
        resLine("EKG",          "EKG");

        std::cout << CLR_YELLOW
                  << "====================================================================\n"
                  << CLR_RESET;
    }

    auto totalRun = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_);

    std::cout << CLR_BOLD << CLR_YELLOW
              << "\n=========== SYMULACJA ZAKONCZONA ===========\n"
              << CLR_CYAN << "Czas trwania: " << CLR_WHITE << hhmmss(totalRun) << "\n\n";

    /* pacjenci */
    std::cout << CLR_BOLD << CLR_CYAN << ">>> Statystyki pacjentow:\n" << CLR_WHITE
              << "    Obsluzonych w triage: " << triage_.done()      << '\n'
              << "    Chirurgia:   "           << surg_.getServed()  << '\n'
              << "    Ortopedia:   "           << ortho_.getServed() << '\n'
              << "    Kardiologia: "           << cardio_.getServed()<< "\n\n";
    gPatientLogger.summary(std::cout);

    /* przestoje + full load */
    std::cout << CLR_BOLD << CLR_CYAN << ">>> Przestoje oddzialow (ms):\n" << CLR_WHITE
              << "  Triage:      " << triage_.getIdleTimeMs()    << '\n'
              << "  Chirurgia:   " << surg_.getIdleTimeMs()      << '\n'
              << "  Ortopedia:   " << ortho_.getIdleTimeMs()     << '\n'
              << "  Kardiologia: " << cardio_.getIdleTimeMs()    << "\n\n";

    std::cout << CLR_BOLD << CLR_CYAN << ">>> 100 % obciazenia (ms):\n" << CLR_WHITE
              << "  Triage:      " << triage_.getFullLoadTimeMs() << '\n'
              << "  Chirurgia:   " << surg_.getFullLoadTimeMs()   << '\n'
              << "  Ortopedia:   " << ortho_.getFullLoadTimeMs()  << '\n'
              << "  Kardiologia: " << cardio_.getFullLoadTimeMs() << "\n\n";

    std::cout << CLR_CYAN << "Pelna lista pacjentow: "
              << CLR_WHITE << "out/patients.csv\n"
              << CLR_YELLOW << "===============================================\n"
              << CLR_RESET;
}

std::string DashboardMonitor::hhmmss(std::chrono::seconds s)
{
    int h = int(s.count()/3600),
            m = int((s.count()%3600)/60),
            sec = int(s.count()%60);
    char buf[16];
    std::snprintf(buf,sizeof(buf),"%02d:%02d:%02d",h,m,sec);
    return buf;
}
