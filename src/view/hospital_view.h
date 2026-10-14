#ifndef SOR_HOSPITAL_VIEW_H
#define SOR_HOSPITAL_VIEW_H

#include "output/patient_logger.h"
#include <array>
#include <functional>
#include <string>
#include <vector>

struct DepartmentSnapshot {
    std::array<int, 4> queue{};  // RED, YELLOW, GREEN, BLUE
    int busy = 0;
    int doctors = 0;
    int served = 0;
};

struct ResourceSnapshot {
    std::string key;
    int total = 0;
    int inUse = 0;
    int cooling = 0;  // zasoby odnawialne w trakcie odnawiania
    bool renewable = false;
};

struct HospitalSnapshot {
    double elapsedSec = 0;
    double lambdaPerSec = 0;
    int arrived = 0;
    int triageQueue = 0;
    int triageBusy = 0;
    int nurses = 0;
    int triaged = 0;
    std::array<DepartmentSnapshot, 3> dept{};
    std::vector<ResourceSnapshot> resources;
    bool massEnabled = false;
    bool massFired = false;
    int massAtSec = 0;
    int massSize = 0;
    int deadlockAlarms = 0;
    PatientStats stats;
};

// Okno raylib z widokiem SOR na zywo. Dziala w watku glownym i tylko czyta migawki stanu.
class HospitalView {
public:
    HospitalView(std::function<HospitalSnapshot()> snapshot, std::function<void()> massCasualtyNow);
    ~HospitalView();

    bool isOpen() const { return open_; }
    bool frame();  // rysuje jedna klatke; false, gdy uzytkownik zamknal okno

private:
    struct State;
    std::function<HospitalSnapshot()> snapshot_;
    std::function<void()> massNow_;
    bool open_ = false;
    State *st_ = nullptr;
};

#endif // SOR_HOSPITAL_VIEW_H
