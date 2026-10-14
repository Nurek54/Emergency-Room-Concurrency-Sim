#ifndef SOR_RESOURCE_MANAGER_H
#define SOR_RESOURCE_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <map>

struct Config;

class Semaphore {
public:
    explicit Semaphore(int count = 0) : count_(count) {}
    void acquire(int n = 1);
    void release(int n = 1);
    int  available() const;
private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    int count_;
};

class ResourceManager {
public:
    explicit ResourceManager(const Config& c);

    void acquire(const std::string& k, int n = 1);
    void release(const std::string& k, int n = 1);
    void releaseOrReplenish(const std::string& k, int n = 1);

    int  available(const std::string& k) const;
    int  total(const std::string& k)     const;
    bool isRenewable(const std::string& k) const { return renewalDelayMs_.count(k) > 0; }
    int  inUse(const std::string& k)     const;
    int  restockedCount   (const std::string& k) const;
    int  pendingRestockCount(const std::string& k) const;

    void logUsageTime(const std::string& key, long timeMs);
    void waitForRestocks(int maxMs) const;
    std::vector<std::string> getAllKeys() const;

    void getStats(std::map<std::string, long>& totalUse,
                  std::map<std::string, int>&  usageCount,
                  std::map<std::string, long>& avgUseMs,
                  std::map<std::string, int>&  renewableDelays,
                  std::vector<std::string>&    renewableList) const;

private:
    void replenishAfterMs(const std::string& k, int n, int delayMs);
    void directRelease    (const std::string& k, int n);

    /* Dane */
    std::unordered_map<std::string, Semaphore> sem_;
    std::unordered_map<std::string, int> total_;
    std::unordered_map<std::string, int> renewalDelayMs_;
    std::unordered_map<std::string, int> pendingRestock_;
    std::unordered_map<std::string, int> restockedCount_;
    std::unordered_map<std::string, long> usageTimeMs_;
    std::unordered_map<std::string, int>  usageCount_;
    mutable std::mutex mtx_;
};

extern ResourceManager* gResMgr;
#endif
