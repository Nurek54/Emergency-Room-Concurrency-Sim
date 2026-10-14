#include "output/patient_logger.h"
#include "core/types.h"
#include <filesystem>
#include <sstream>

PatientLogger gPatientLogger;

PatientLogger::PatientLogger() {
    std::filesystem::create_directory("out");
    out_.open("out/patients.csv", std::ios::trunc);
    out_ << "id;choroby;priorytet;oddzial;sprzet;czas_leczenia_ms;czas_oczek_ms\n";
}

PatientLogger::~PatientLogger() {
    stop();
    out_.close();
}

void PatientLogger::start() {
    running_ = true;
    th_ = std::thread(&PatientLogger::run, this);
}

void PatientLogger::stop() {
    running_ = false;
    cv_.notify_all();
    if (th_.joinable()) th_.join();
}

void PatientLogger::log(const Patient& p,
                        const std::vector<std::string>& equip,
                        long waitMs,
                        long treatMs)
{
    {
        std::lock_guard<std::mutex> lk(mtx_);
        q_.emplace(LogEntry{p, equip, waitMs, treatMs});
    }
    cv_.notify_one();
}

void PatientLogger::run() {
    while (running_ || !q_.empty()) {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&]{ return !q_.empty() || !running_; });

        while (!q_.empty()) {
            LogEntry e = q_.front(); q_.pop();
            lk.unlock();

            auto join = [](const auto& v){
                std::ostringstream ss;
                for (size_t i = 0; i < v.size(); ++i) {
                    if (i) ss << '|';
                    ss << v[i];
                }
                return ss.str();
            };

            out_ << e.p.id << ';'
                 << join(e.p.diseases) << ';'
                 << toStr(e.p.originalPriority) << ';'
                 << toStr(e.p.dept) << ';'
                 << join(e.equip) << ';'
                 << e.treatMs << ';'
                 << e.waitMs << '\n';
            out_.flush();

            lk.lock();
            ++count_;
            totalWait_  += e.waitMs;
            totalTreat_ += e.treatMs;
            for (const auto& k : e.equip) equipCnt_[k]++;
            const int d = static_cast<int>(e.p.dept);
            const int pr = static_cast<int>(e.p.originalPriority);
            ++deptCount_[d];
            deptWait_[d] += e.waitMs;
            ++prioCount_[pr];
            prioWait_[pr] += e.waitMs;
        }
    }
}

PatientStats PatientLogger::stats() const {
    std::lock_guard<std::mutex> lock(mtx_);
    PatientStats s;
    s.count = count_;
    if (count_) {
        s.avgWaitMs = double(totalWait_) / count_;
        s.avgTreatMs = double(totalTreat_) / count_;
    }
    for (int i = 0; i < 3; ++i) {
        s.deptCount[i] = deptCount_[i];
        s.deptAvgWaitMs[i] = deptCount_[i] ? double(deptWait_[i]) / deptCount_[i] : 0.0;
    }
    for (int i = 0; i < 4; ++i) {
        s.prioCount[i] = prioCount_[i];
        s.prioAvgWaitMs[i] = prioCount_[i] ? double(prioWait_[i]) / prioCount_[i] : 0.0;
    }
    return s;
}

void PatientLogger::summary(std::ostream& os) const {
    std::lock_guard<std::mutex> lock(mtx_);
    os << "================ PODSUMOWANIE =================\n"
       << "Liczba pacjentow: " << count_ << '\n'
       << "Sredni czas oczekiwania: " << (count_ ? totalWait_ / count_ : 0) << " ms\n"
       << "Sredni czas leczenia:    " << (count_ ? totalTreat_ / count_ : 0) << " ms\n\n"
       << "Uzycie sprzetu (liczba zajec):\n";
    for (const auto& [k, v] : equipCnt_)
        os << "  " << k << " : " << v << '\n';
    os << "==============================================\n";
}
