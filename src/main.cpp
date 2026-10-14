#include "core/configuration_watcher.h"
#include "core/config.h"
#include "core/hospital.h"
#include <filesystem>
#include <iostream>

// Kolejnosc szukania konfiguracji: argument programu, config.txt w katalogu roboczym,
// config/config.txt w katalogu roboczym, a na koncu config/config.txt w katalogu projektu.
static std::string findConfig(int argc, char* argv[])
{
    if (argc > 1) return argv[1];
    for (const char* p : {"config.txt", "config/config.txt"})
        if (std::filesystem::exists(p)) return p;
    return SOR_DEFAULT_CONFIG;
}

int main(int argc, char* argv[])
{
    const std::string cfgPath = findConfig(argc, argv);
    if (!std::filesystem::exists(cfgPath))
        std::cout << "Nie znaleziono " << cfgPath << ", uzywam wartosci domyslnych.\n";
    else
        std::cout << "Konfiguracja: " << cfgPath << "\n";

    ConfigurationWatcher watcher(cfgPath);
    Config cfg = Config::load(watcher);

    Hospital h(cfg);
    h.run();
    return 0;
}
