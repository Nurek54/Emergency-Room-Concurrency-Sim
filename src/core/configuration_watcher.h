#ifndef CONFIGURATION_WATCHER_H
#define CONFIGURATION_WATCHER_H

#include <string>
#include <unordered_map>

class ConfigurationWatcher {
public:
    explicit ConfigurationWatcher(const std::string& path);
    // Zwraca wartość klucza (pusty string, jeśli brak).
    const std::string& get(const std::string& key) const;

private:
    std::unordered_map<std::string, std::string> kv;
};

#endif // CONFIGURATION_WATCHER_H
