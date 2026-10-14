#include "core/config.h"
#include <cstdlib>
#include <algorithm>

static double getD(const ConfigurationWatcher& w, const std::string& k, double def)
{
    const std::string& v = w.get(k);
    return v.empty() ? def : strtod(v.c_str(), nullptr);
}

static int getI(const ConfigurationWatcher& w, const std::string& k, int def)
{
    const std::string& v = w.get(k);
    return v.empty() ? def : static_cast<int>(strtol(v.c_str(), nullptr, 10));
}

static bool getB(const ConfigurationWatcher& w, const std::string& k, bool def)
{
    const std::string& v = w.get(k);
    if (v.empty()) return def;
    std::string s(v);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s == "1" || s == "true" || s == "yes";
}

Config Config::load(const ConfigurationWatcher& w)
{
    Config c;

    // arrival
    c.lambdaPerSec   = getD(w, "arrival.lambda_per_sec", c.lambdaPerSec);
    c.massEnabled    = getB(w, "arrival.mass_casualty.enabled", c.massEnabled);
    c.massAtSec      = getI(w, "arrival.mass_casualty.at_second", c.massAtSec);
    c.massRatePerSec = getD(w, "arrival.mass_casualty.rate_per_sec", c.massRatePerSec);
    c.maxPatients    = getI(w, "arrival.max_patients", c.maxPatients);  // <== NOWA LINIA

    // triage
    c.pRed    = getD(w, "triage_distribution.red",    c.pRed);
    c.pYellow = getD(w, "triage_distribution.yellow", c.pYellow);
    c.pGreen  = getD(w, "triage_distribution.green",  c.pGreen);
    c.pBlue   = getD(w, "triage_distribution.blue",   c.pBlue);

    // staff
    c.chirurg     = getI(w, "staff.chirurg",  c.chirurg);
    c.ortopeda    = getI(w, "staff.ortopeda", c.ortopeda);
    c.kardiolog   = getI(w, "staff.kardiolog",c.kardiolog);
    c.consultSurg = getI(w, "staff.consultants.surg", c.consultSurg);
    c.consultCard = getI(w, "staff.consultants.card", c.consultCard);

    // equipment
    c.ct       = getI(w, "equipment.ct",   c.ct);
    c.xray     = getI(w, "equipment.xray", c.xray);
    c.usg      = getI(w, "equipment.usg",  c.usg);
    c.oroom    = getI(w, "equipment.or",   c.oroom);
    c.anest    = getI(w, "equipment.anesthesiologist", c.anest);
    c.icuBeds  = getI(w, "equipment.icu_beds",        c.icuBeds);
    c.defib    = getI(w, "equipment.defib",           c.defib);
    c.echo     = getI(w, "equipment.echo",            c.echo);
    c.vent     = getI(w, "equipment.vent",            c.vent);
    c.dial     = getI(w, "equipment.dial",            c.dial);
    c.endo     = getI(w, "equipment.endo",            c.endo);
    c.lab      = getI(w, "equipment.lab",             c.lab);
    c.blood    = getI(w, "equipment.blood",           c.blood);
    c.neuro    = getI(w, "equipment.neuro",           c.neuro);
    c.traumaKit= getI(w, "equipment.trauma_kit",      c.traumaKit);
    c.orthoSet = getI(w, "equipment.ortho_set",       c.orthoSet);
    c.ekg      = getI(w, "equipment.ekg",             c.ekg);

    // timeouts
    c.deadlockMs   = getI(w, "timeouts.deadlock_ms",    c.deadlockMs);
    c.backoffMinMs = getI(w, "timeouts.backoff_min_ms", c.backoffMinMs);
    c.backoffMaxMs = getI(w, "timeouts.backoff_max_ms", c.backoffMaxMs);

    return c;
}
