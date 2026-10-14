#include "core/configuration_watcher.h"
#include <fstream>
#include <sstream>

ConfigurationWatcher::ConfigurationWatcher(const std::string& path)
{
    std::ifstream in(path);
    std::string   line;

    while (std::getline(in, line))
    {
        // usuń komentarze i nadmiarowe spacje
        if (auto pos = line.find('#'); pos != std::string::npos) line.erase(pos);
        line.erase(0,  line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;        // zła linia

        auto key = line.substr(0, pos);
        auto val = line.substr(pos + 1);
        // obetnij spacje
        key.erase(key.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));

        kv.emplace(std::move(key), std::move(val));
    }
}

const std::string& ConfigurationWatcher::get(const std::string& key) const
{
    static const std::string empty;
    auto it = kv.find(key);
    return it == kv.end() ? empty : it->second;
}
