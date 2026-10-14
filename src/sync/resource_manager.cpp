#include "sync/resource_manager.h"
#include "core/config.h"

#include <thread>
#include <chrono>
#include <algorithm>

void Semaphore::acquire(int n)
{
    std::unique_lock<std::mutex> lk(mtx_);
    cv_.wait(lk, [&]{ return count_ >= n; });
    count_ -= n;
}

void Semaphore::release(int n)
{
    {
        std::lock_guard<std::mutex> lk(mtx_);
        count_ += n;
    }
    cv_.notify_all();
}

int Semaphore::available() const
{
    std::lock_guard<std::mutex> lk(mtx_);
    return count_;
}


ResourceManager* gResMgr = nullptr;

ResourceManager::ResourceManager(const Config& c)
{
    auto add = [&](const std::string& key, int cnt, int delayMs = -1)
    {
        /*  ← kluczowa zmiana: konstrukcja in-place bez kopiowania  */
        sem_.try_emplace(key, cnt);       // wywoła Semaphore(int cnt)

        total_[key] = cnt;
        if (delayMs > 0) {
            renewalDelayMs_[key] = delayMs;
            pendingRestock_[key] = 0;
            restockedCount_[key] = 0;
        }
    };

    /* sprzęt stały */
    add("CT",         c.ct);
    add("XRAY",       c.xray);
    add("USG",        c.usg);
    add("OR",         c.oroom);
    add("ANEST",      c.anest);
    add("ICU",        c.icuBeds);
    add("DEFIB",      c.defib);
    add("ECHO",       c.echo);
    add("VENT",       c.vent);
    add("NEURO",      c.neuro);
    add("ORTHO_SET",  c.orthoSet);
    add("ENDO",       c.endo);
    add("EKG",        c.ekg);

    /* sprzęt odnawialny (cool-down w ms) */
    add("DIAL",       c.dial,       2000);
    add("LAB",        c.lab,        1500);
    add("BLOOD",      c.blood,      1800);
    add("TRAUMA_KIT", c.traumaKit,  1200);

    gResMgr = this;
}

void ResourceManager::acquire(const std::string& k, int n)
{
    sem_.at(k).acquire(n);
}

/*  release() sam decyduje: zwykły czy odnawialny  */
void ResourceManager::release(const std::string& k, int n)
{
    auto it = renewalDelayMs_.find(k);
    if (it != renewalDelayMs_.end())          // odnawialny
        replenishAfterMs(k, n, it->second);
    else                                      // zwykły
        directRelease(k, n);
}

void ResourceManager::releaseOrReplenish(const std::string& k, int n)
{
    release(k, n);    // alias
}

void ResourceManager::directRelease(const std::string& k, int n)
{
    sem_.at(k).release(n);
}

void ResourceManager::replenishAfterMs(const std::string& k, int n, int delayMs)
{
    {
        std::lock_guard<std::mutex> lk(mtx_);
        pendingRestock_[k] += n;
    }

    std::thread([this, k, n, delayMs]
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

                    {
                        std::lock_guard<std::mutex> lk(mtx_);
                        pendingRestock_[k] -= n;
                        restockedCount_[k] += n;
                    }
                    directRelease(k, n);
                }).detach();
}

int ResourceManager::available(const std::string& k) const  { return sem_.at(k).available(); }
int ResourceManager::total     (const std::string& k) const  { return total_.at(k); }
int ResourceManager::inUse     (const std::string& k) const  { return total(k) - available(k); }

int ResourceManager::restockedCount(const std::string& k) const
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = restockedCount_.find(k);
    return it != restockedCount_.end() ? it->second : 0;
}

int ResourceManager::pendingRestockCount(const std::string& k) const
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = pendingRestock_.find(k);
    return it != pendingRestock_.end() ? it->second : 0;
}

// Watki odnawiajace zasoby sa odlaczone, wiec przed zniszczeniem ResourceManagera czekamy, az skoncza.
void ResourceManager::waitForRestocks(int maxMs) const
{
    for (int waited = 0; waited < maxMs; waited += 50) {
        bool pending = false;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            for (const auto& [k, n] : pendingRestock_) pending = pending || n > 0;
        }
        if (!pending) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void ResourceManager::logUsageTime(const std::string& key, long ms)
{
    std::lock_guard<std::mutex> lk(mtx_);
    usageTimeMs_[key]  += ms;
    usageCount_[key]   += 1;
}

std::vector<std::string> ResourceManager::getAllKeys() const
{
    std::vector<std::string> v; v.reserve(sem_.size());
    for (auto& [k,_] : sem_) v.push_back(k);
    return v;
}

void ResourceManager::getStats(std::map<std::string, long>& totalUse,
                               std::map<std::string, int>&  useCnt,
                               std::map<std::string, long>& avgUse,
                               std::map<std::string, int>&  delays,
                               std::vector<std::string>&    renewable) const
{
    std::lock_guard<std::mutex> lk(mtx_);

    for (auto& [k,t] : usageTimeMs_) {
        totalUse[k] = t;
        useCnt[k]   = usageCount_.at(k);
        avgUse[k]   = useCnt[k] ? t / useCnt[k] : 0;
    }
    for (auto& [k, d] : renewalDelayMs_) {
        delays[k] = d;
        renewable.push_back(k);
    }
}
