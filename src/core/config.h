#ifndef SOR_CONFIG_H
#define SOR_CONFIG_H

#include "core/configuration_watcher.h"
#include <string>

struct Config
{
    /* ---------- arrival ---------- */
    double lambdaPerSec      = 11.0;
    bool   massEnabled       = false;
    int    massAtSec         = 5;
    double massRatePerSec    = 1000.0;
    int    maxPatients       = -1;

    /* ---------- triage_distribution ---------- */
    double pRed    = 0.10;
    double pYellow = 0.20;
    double pGreen  = 0.50;
    double pBlue   = 0.20;

    /* ---------- staff ---------- */
    int chirurg      = 6;
    int ortopeda     = 8;


    int kardiolog    = 6;
    int consultSurg  = 5;
    int consultCard  = 4;

    /* ---------- equipment ---------- */
    int ct           = 3;
    int ekg          = 3;
    int vent         = 2;
    int dial         = 2;
    int lab          = 4;
    int blood        = 4;
    int xray         = 3;
    int orthoSet     = 3;

    int usg          = 3;
    int oroom        = 3;
    int anest        = 2;
    int icuBeds      = 1;
    int defib        = 3;
    int echo         = 2;
    int endo         = 2;
    int neuro        = 1;
    int traumaKit    = 3;

    /* ---------- timeouts ---------- */
    int deadlockMs   = 20000;
    int backoffMinMs = 1000;
    int backoffMaxMs = 3000;

    static Config load(const ConfigurationWatcher& w);
};

#endif /* SOR_CONFIG_H */
