#include "view/hospital_view.h"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

constexpr int W = 1280;
constexpr int H = 760;

Color hex(unsigned v, unsigned char a = 255) {
    return Color{static_cast<unsigned char>(v >> 16), static_cast<unsigned char>(v >> 8),
                 static_cast<unsigned char>(v), a};
}

const Color BG = hex(0x0B0F17);
const Color CARD = hex(0x131A24);
const Color CARD2 = hex(0x18212E);
const Color EDGE = hex(0x263041);
const Color FG = hex(0xE6EDF3);
const Color MUTED = hex(0x8B949E);
const Color DIM = hex(0x4B5563);
const Color ACCENT = hex(0x6366F1);
const Color COOL = hex(0x22D3EE);
const Color ALERT = hex(0xF43F5E);

const Color PRIO[4] = {hex(0xEF4444), hex(0xF59E0B), hex(0x10B981), hex(0x3B82F6)};
const Color DEPT[3] = {hex(0xA78BFA), hex(0x38BDF8), hex(0xF472B6)};

struct Lang {
    const char *title, *subtitle;
    const char *time, *inSystem, *served, *avgWait, *avgTreat;
    const char *entrance, *perSec, *triage, *nurses, *queue, *discharged, *waitShort;
    const char *dept[3];
    const char *prio[4];
    const char *massOff, *massIn, *massDone, *massBanner, *patients;
    const char *resources, *resourcesSub, *inUse, *cooling, *free;
    const char *chartTitle, *chartInSystem, *waitTitle, *noData;
    const char *keys, *deadlock;
    const char *res[17];
    const char *windowTitle;
};

const Lang PL = {
    "SOR · Szpitalny Oddział Ratunkowy", "wielowątkowa symulacja na żywo · C++20",
    "czas", "w systemie", "wypisani", "śr. oczekiwanie", "śr. leczenie",
    "Wejście", "pacjentów / s", "Triage", "pielęgniarki", "kolejka", "wypisani", "śr. czekanie",
    {"Chirurgia", "Ortopedia", "Kardiologia"},
    {"czerwony", "żółty", "zielony", "niebieski"},
    "zdarzenie masowe: wyłączone", "zdarzenie masowe za", "zdarzenie masowe: wystąpiło",
    "ZDARZENIE MASOWE", "pacjentów",
    "Sprzęt i zasoby", "każdy typ to semafor; odnawialne wracają po czasie",
    "zajęte", "odnawia się", "wolne",
    "Kolejki do lekarzy (ostatnie 2 min)", "w systemie",
    "Średnie oczekiwanie wg priorytetu z triage", "brak danych",
    "[M] zdarzenie masowe teraz     [L] English     [Esc] zakończ",
    "alarmy detektora zakleszczeń",
    {"Tomograf (CT)", "RTG", "USG", "Sale operacyjne", "Anestezjolodzy", "Łóżka OIOM", "Defibrylatory",
     "Echo serca", "Respiratory", "Dializator", "Endoskop", "Laboratorium", "Zestawy krwi",
     "Konsultant neuro", "Zestawy urazowe", "Zestawy ortop.", "EKG"},
    "SOR - symulacja na żywo",
};

const Lang EN = {
    "ER · Emergency Room", "live multithreaded simulation · C++20",
    "time", "in system", "discharged", "avg wait", "avg treatment",
    "Entrance", "patients / s", "Triage", "nurses", "queue", "discharged", "avg wait",
    {"Surgery", "Orthopedics", "Cardiology"},
    {"red", "yellow", "green", "blue"},
    "mass casualty: off", "mass casualty in", "mass casualty: happened",
    "MASS CASUALTY", "patients",
    "Equipment and resources", "each type is a semaphore; renewable ones come back after a cooldown",
    "in use", "cooling down", "free",
    "Queues for doctors (last 2 min)", "in system",
    "Average wait by triage priority", "no data yet",
    "[M] mass casualty now     [L] Polski     [Esc] quit",
    "deadlock detector alarms",
    {"CT scanner", "X-ray", "Ultrasound", "Operating rooms", "Anesthesiologists", "ICU beds",
     "Defibrillators", "Echocardiograph", "Ventilators", "Dialysis", "Endoscope", "Lab", "Blood units",
     "Neuro consult", "Trauma kits", "Ortho sets", "ECG"},
    "ER - live simulation",
};

const char *RES_KEYS[17] = {"CT", "XRAY", "USG", "OR", "ANEST", "ICU", "DEFIB", "ECHO", "VENT",
                            "DIAL", "ENDO", "LAB", "BLOOD", "NEURO", "TRAUMA_KIT", "ORTHO_SET", "EKG"};

struct Fonts {
    Font regular{}, bold{}, mono{};
    bool custom = true;
};

Font loadFirst(const std::vector<const char *> &paths, int size, std::vector<int> &cps, bool &ok) {
    for (const char *p : paths) {
        if (!FileExists(p)) continue;
        Font f = LoadFontEx(p, size, cps.data(), static_cast<int>(cps.size()));
        if (f.texture.id != 0 && f.glyphCount > 0) {
            GenTextureMipmaps(&f.texture);
            SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);
            return f;
        }
    }
    ok = false;
    return GetFontDefault();
}

Fonts loadFonts() {
    std::vector<int> cps;
    for (int c = 32; c < 127; ++c) cps.push_back(c);
    int n = 0;
    int *more = LoadCodepoints("ąćęłńóśźżĄĆĘŁŃÓŚŹŻ·−×≤λ→", &n);
    for (int i = 0; i < n; ++i) cps.push_back(more[i]);
    UnloadCodepoints(more);

    Fonts f;
    f.regular = loadFirst({"C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf",
                           "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                           "/System/Library/Fonts/Supplemental/Arial.ttf"}, 48, cps, f.custom);
    f.bold = loadFirst({"C:/Windows/Fonts/segoeuib.ttf", "C:/Windows/Fonts/arialbd.ttf",
                        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
                        "/System/Library/Fonts/Supplemental/Arial Bold.ttf"}, 56, cps, f.custom);
    f.mono = loadFirst({"C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/cour.ttf",
                        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                        "/System/Library/Fonts/Supplemental/Courier New.ttf"}, 40, cps, f.custom);
    return f;
}

// Domyslna czcionka raylib nie ma polskich znakow, wiec bez czcionki systemowej zamieniamy je na ASCII.
std::string ascii(const std::string &s) {
    static const std::vector<std::pair<std::string, std::string>> map = {
        {"ą", "a"}, {"ć", "c"}, {"ę", "e"}, {"ł", "l"}, {"ń", "n"}, {"ó", "o"}, {"ś", "s"}, {"ź", "z"},
        {"ż", "z"}, {"Ą", "A"}, {"Ć", "C"}, {"Ę", "E"}, {"Ł", "L"}, {"Ń", "N"}, {"Ó", "O"}, {"Ś", "S"},
        {"Ź", "Z"}, {"Ż", "Z"}, {"·", "-"}, {"−", "-"}, {"×", "x"}, {"λ", "l"}, {"→", "->"}};
    std::string out = s;
    for (const auto &[from, to] : map)
        for (size_t p = out.find(from); p != std::string::npos; p = out.find(from, p + to.size()))
            out.replace(p, from.size(), to);
    return out;
}

enum Align { LEFT, CENTER, RIGHT };

struct Painter {
    Fonts f;

    void text(const Font &font, const std::string &raw, float x, float y, float size, Color c,
              Align a = LEFT) const {
        const std::string s = f.custom ? raw : ascii(raw);
        const float sp = f.custom ? 0.0f : size / 10.0f;
        Vector2 m = MeasureTextEx(font, s.c_str(), size, sp);
        if (a == CENTER) x -= m.x / 2;
        if (a == RIGHT) x -= m.x;
        DrawTextEx(font, s.c_str(), {std::round(x), std::round(y)}, size, sp, c);
    }
    float width(const Font &font, const std::string &raw, float size) const {
        const std::string s = f.custom ? raw : ascii(raw);
        return MeasureTextEx(font, s.c_str(), size, f.custom ? 0.0f : size / 10.0f).x;
    }
    void card(Rectangle r, Color fill = CARD) const {
        DrawRectangleRounded(r, 12.0f / std::min(r.width, r.height), 8, fill);
        DrawRectangleRoundedLinesEx(r, 12.0f / std::min(r.width, r.height), 8, 1.0f, EDGE);
    }
    void pill(Rectangle r, Color fill) const { DrawRectangleRounded(r, 1.0f, 12, fill); }
};

std::string clock(double s) {
    int t = static_cast<int>(s);
    char b[16];
    std::snprintf(b, sizeof b, "%02d:%02d:%02d", t / 3600, (t % 3600) / 60, t % 60);
    return b;
}

std::string secs(double ms) {
    char b[24];
    if (ms < 1000) std::snprintf(b, sizeof b, "%d ms", static_cast<int>(ms + 0.5));
    else std::snprintf(b, sizeof b, "%.2f s", ms / 1000.0);
    return b;
}

struct Particle {
    Vector2 from, to;
    double t0, dur;
    Color color;
};

struct Sample {
    double t;
    int queue[3];
    int inSystem;
};

Vector2 lerp(Vector2 a, Vector2 b, float k) { return {a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k}; }

float ease(float k) { return k < 0.5f ? 2 * k * k : 1 - std::pow(-2 * k + 2, 2.0f) / 2; }

}  // namespace

struct HospitalView::State {
    Painter p;
    const Lang *L = &PL;
    HospitalSnapshot prev;
    bool hasPrev = false;
    std::vector<Particle> particles;
    std::deque<Sample> history;
    double lastSample = -1;
    double massFlashUntil = -1;
    int massFlashCount = 0;
    double now = 0;
};

HospitalView::HospitalView(std::function<HospitalSnapshot()> snapshot, std::function<void()> massCasualtyNow)
    : snapshot_(std::move(snapshot)), massNow_(std::move(massCasualtyNow)) {
    bool screen = true;
#if defined(__linux__)
    auto set = [](const char *n) {
        const char *v = std::getenv(n);
        return v != nullptr && *v != '\0';
    };
    screen = set("DISPLAY") || set("WAYLAND_DISPLAY");
#endif
    SetTraceLogLevel(LOG_WARNING);
    if (screen) {
        SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
        InitWindow(W, H, PL.windowTitle);
    }
    if (!screen || !IsWindowReady()) {
        std::cout << "Nie udalo sie otworzyc okna wizualizacji, symulacja dziala tylko w konsoli.\n";
        return;
    }
    SetTargetFPS(60);
    st_ = new State();
    st_->p.f = loadFonts();
    open_ = true;
}

HospitalView::~HospitalView() {
    if (!open_) return;
    for (Font *f : {&st_->p.f.regular, &st_->p.f.bold, &st_->p.f.mono})
        if (f->texture.id != GetFontDefault().texture.id) UnloadFont(*f);
    delete st_;
    CloseWindow();
}

bool HospitalView::frame() {
    if (!open_) return true;
    if (WindowShouldClose()) return false;

    State &S = *st_;
    const Painter &P = S.p;
    const Lang &L = *S.L;
    if (IsKeyPressed(KEY_L)) {
        S.L = (S.L == &PL) ? &EN : &PL;
        SetWindowTitle(S.L->windowTitle);
    }
    if (IsKeyPressed(KEY_M) && massNow_) massNow_();

    HospitalSnapshot s = snapshot_();
    S.now = s.elapsedSec;
    const double now = s.elapsedSec;

    int queueTotal[3], inDept[3];
    int inSystem = s.triageQueue + s.triageBusy;
    for (int d = 0; d < 3; ++d) {
        queueTotal[d] = 0;
        for (int q : s.dept[d].queue) queueTotal[d] += q;
        inDept[d] = queueTotal[d] + s.dept[d].busy;
        inSystem += inDept[d];
    }

    // ---------- geometria ----------
    const Rectangle flow{16, 88, W - 32, 320};
    const Vector2 door{106, 268};
    const Vector2 triageIn{304, 300};
    const float laneX = 448, laneW = W - 32 - laneX + 16 - 16, laneH = 92, laneGap = 10;
    auto laneRect = [&](int d) { return Rectangle{laneX, 104 + d * (laneH + laneGap), laneW, laneH}; };
    auto laneQueuePos = [&](int d) {
        Rectangle r = laneRect(d);
        return Vector2{r.x + 30, r.y + 62};
    };
    auto laneExit = [&](int d) {
        Rectangle r = laneRect(d);
        return Vector2{r.x + r.width - 40, r.y + r.height / 2};
    };
    auto doctorPos = [&](int d, int i) {
        Rectangle r = laneRect(d);
        return Vector2{r.x + 520 + i * 44.0f, r.y + 56};
    };

    // ---------- czasteczki z roznic miedzy migawkami ----------
    if (S.hasPrev) {
        const HospitalSnapshot &o = S.prev;
        int arrivedNow = s.arrived - o.arrived;
        for (int i = 0; i < std::min(arrivedNow, 12); ++i)
            S.particles.push_back({{door.x + 40, door.y - 10}, triageIn, now + i * 0.06, 0.7, FG});
        if (arrivedNow >= std::max(3, s.massSize) && s.massSize > 0) {
            S.massFlashUntil = now + 4.0;
            S.massFlashCount = s.massSize;
        }
        for (int d = 0; d < 3; ++d) {
            int oldQueue = 0;
            for (int q : o.dept[d].queue) oldQueue += q;
            int entered = (queueTotal[d] + s.dept[d].busy + s.dept[d].served) -
                          (oldQueue + o.dept[d].busy + o.dept[d].served);
            for (int i = 0; i < std::min(entered, 8); ++i) {
                Color c = FG;
                for (int pr = 0; pr < 4; ++pr)
                    if (s.dept[d].queue[pr] > o.dept[d].queue[pr]) c = PRIO[pr];
                if (entered > 0 && queueTotal[d] == oldQueue) c = DEPT[d];  // od razu do wolnego lekarza
                S.particles.push_back({{triageIn.x + 70, triageIn.y - 40}, laneQueuePos(d), now + i * 0.05, 0.8, c});
            }
            int left = s.dept[d].served - o.dept[d].served;
            for (int i = 0; i < std::min(left, 6); ++i)
                S.particles.push_back({doctorPos(d, 0), laneExit(d), now + i * 0.05, 0.6, DEPT[d]});
        }
    }
    S.prev = s;
    S.hasPrev = true;
    if (S.lastSample < 0 || now - S.lastSample >= 0.5) {
        S.lastSample = now;
        S.history.push_back({now, {queueTotal[0], queueTotal[1], queueTotal[2]}, inSystem});
        while (!S.history.empty() && now - S.history.front().t > 120) S.history.pop_front();
    }

    BeginDrawing();
    ClearBackground(BG);

    // ================= naglowek =================
    DrawRectangleRounded({16, 16, 10, 58}, 1.0f, 8, ALERT);
    P.text(P.f.bold, L.title, 38, 18, 26, FG);
    P.text(P.f.regular, L.subtitle, 38, 50, 15, MUTED);
    {
        struct Kpi { std::string label, value; Color c; };
        std::vector<Kpi> k = {
            {L.time, clock(now), FG},
            {L.inSystem, std::to_string(inSystem), inSystem > 20 ? ALERT : FG},
            {L.served, std::to_string(s.stats.count), PRIO[2]},
            {L.avgWait, secs(s.stats.avgWaitMs), FG},
            {L.avgTreat, secs(s.stats.avgTreatMs), FG},
        };
        float x = W - 16;
        for (auto it = k.rbegin(); it != k.rend(); ++it) {
            float w = std::max(P.width(P.f.bold, it->value, 20), P.width(P.f.regular, it->label, 12)) + 28;
            x -= w;
            Rectangle r{x, 18, w, 56};
            P.card(r, CARD);
            P.text(P.f.regular, it->label, r.x + 14, r.y + 8, 12, MUTED);
            P.text(P.f.bold, it->value, r.x + 14, r.y + 24, 20, it->c);
            x -= 8;
        }
    }

    // ================= przeplyw pacjentow =================
    P.card(flow);
    // wejscie
    P.text(P.f.bold, L.entrance, 36, 104, 17, FG);
    DrawRectangleRounded({door.x - 34, door.y - 74, 68, 96}, 0.25f, 8, CARD2);
    DrawRectangleRoundedLinesEx({door.x - 34, door.y - 74, 68, 96}, 0.25f, 8, 2, EDGE);
    DrawRectangle(static_cast<int>(door.x - 2), static_cast<int>(door.y - 70), 4, 88, EDGE);
    DrawCircle(static_cast<int>(door.x + 22), static_cast<int>(door.y - 26), 3, MUTED);
    P.text(P.f.bold, std::to_string(s.arrived), door.x, 136, 30, FG, CENTER);
    {
        char lam[32];
        std::snprintf(lam, sizeof lam, "λ = %.2f %s", s.lambdaPerSec, L.perSec);
        P.text(P.f.regular, lam, door.x, 302, 13, MUTED, CENTER);
    }
    {
        std::string m;
        Color c = MUTED;
        if (!s.massEnabled && !s.massFired) m = L.massOff;
        else if (s.massFired) { m = L.massDone; c = ALERT; }
        else {
            int left = std::max(0, s.massAtSec - static_cast<int>(now));
            char b[16];
            std::snprintf(b, sizeof b, " %d:%02d", left / 60, left % 60);
            m = std::string(L.massIn) + b;
            c = left < 30 ? PRIO[1] : MUTED;
        }
        float w = P.width(P.f.regular, m, 12) + 20;
        Rectangle r{door.x - w / 2, 330, w, 22};
        P.pill(r, Fade(c, 0.14f));
        P.text(P.f.regular, m, door.x, 333, 12, c, CENTER);
    }

    // triage
    P.text(P.f.bold, L.triage, 214, 104, 17, FG);
    P.text(P.f.regular, std::string(L.nurses) + " " + std::to_string(s.triageBusy) + "/" + std::to_string(s.nurses),
           214, 126, 13, MUTED);
    for (int i = 0; i < s.nurses; ++i) {
        float x = 214 + (i % 2) * 74.0f, y = 152 + (i / 2) * 74.0f;
        bool busy = i < s.triageBusy;
        Rectangle r{x, y, 64, 64};
        DrawRectangleRounded(r, 0.25f, 8, busy ? Fade(PRIO[1], 0.22f) : CARD2);
        DrawRectangleRoundedLinesEx(r, 0.25f, 8, busy ? 2.0f : 1.0f, busy ? PRIO[1] : EDGE);
        // prosty krzyz
        Color cc = busy ? PRIO[1] : DIM;
        DrawRectangle(static_cast<int>(x + 27), static_cast<int>(y + 18), 10, 28, cc);
        DrawRectangle(static_cast<int>(x + 18), static_cast<int>(y + 27), 28, 10, cc);
    }
    P.text(P.f.regular, std::string(L.queue) + ": " + std::to_string(s.triageQueue), 214, 308, 13, MUTED);
    for (int i = 0; i < std::min(s.triageQueue, 24); ++i)
        DrawCircle(214 + (i % 12) * 12, 334 + (i / 12) * 12, 4, FG);
    if (s.triageQueue > 24) P.text(P.f.regular, "+" + std::to_string(s.triageQueue - 24), 362, 340, 12, MUTED);

    // oddzialy
    for (int d = 0; d < 3; ++d) {
        Rectangle r = laneRect(d);
        DrawRectangleRounded(r, 10.0f / r.height, 8, CARD2);
        DrawRectangleRounded({r.x, r.y, 6, r.height}, 1.0f, 6, DEPT[d]);
        P.text(P.f.bold, L.dept[d], r.x + 20, r.y + 10, 17, FG);
        P.text(P.f.regular, std::string(L.queue) + ": " + std::to_string(queueTotal[d]), r.x + 20, r.y + 33, 13, MUTED);

        // kolejka: kropki w kolorach priorytetu (najpilniejsi pierwsi)
        int shown = 0;
        const int maxDots = 28;
        for (int pr = 0; pr < 4; ++pr) {
            for (int i = 0; i < s.dept[d].queue[pr] && shown < maxDots; ++i, ++shown) {
                Vector2 c{r.x + 30 + (shown % 14) * 17.0f, r.y + 62 + (shown / 14) * 14.0f - (shown >= 14 ? 7 : 0)};
                DrawCircleV(c, 6, PRIO[pr]);
            }
        }
        if (queueTotal[d] > maxDots)
            P.text(P.f.bold, "+" + std::to_string(queueTotal[d] - maxDots), r.x + 272, r.y + 52, 14, FG);
        // licznik wg priorytetu
        for (int pr = 0; pr < 4; ++pr) {
            float x = r.x + 330 + pr * 40;
            DrawCircle(static_cast<int>(x), static_cast<int>(r.y + 20), 5, PRIO[pr]);
            P.text(P.f.bold, std::to_string(s.dept[d].queue[pr]), x + 10, r.y + 12, 14, FG);
        }
        // lekarze
        for (int i = 0; i < s.dept[d].doctors; ++i) {
            Vector2 c = doctorPos(d, i);
            bool busy = i < s.dept[d].busy;
            if (busy) {
                float pulse = 19 + 2.5f * static_cast<float>(std::sin(GetTime() * 5 + i));
                DrawRing(c, pulse, pulse + 2, 0, 360, 40, Fade(DEPT[d], 0.7f));
                DrawCircleV(c, 16, DEPT[d]);
            } else {
                DrawCircleV(c, 16, CARD);
                DrawRing(c, 15, 16.5f, 0, 360, 40, DIM);
            }
            DrawCircleV({c.x, c.y - 5}, 5, busy ? BG : DIM);
            DrawRectangleRounded({c.x - 8, c.y + 2, 16, 8}, 1.0f, 6, busy ? BG : DIM);
        }
        P.text(P.f.regular, std::to_string(s.dept[d].busy) + "/" + std::to_string(s.dept[d].doctors),
               doctorPos(d, 0).x - 16, r.y + 12, 13, MUTED);
        // wypis i czas oczekiwania
        P.text(P.f.bold, std::to_string(s.dept[d].served), r.x + r.width - 20, r.y + 22, 24, FG, RIGHT);
        P.text(P.f.regular, L.discharged, r.x + r.width - 20, r.y + 50, 12, MUTED, RIGHT);
        P.text(P.f.regular, std::string(L.waitShort) + " " + secs(s.stats.deptAvgWaitMs[d]), r.x + r.width - 20,
               r.y + 66, 12, MUTED, RIGHT);
    }

    // czasteczki
    for (auto it = S.particles.begin(); it != S.particles.end();) {
        float k = static_cast<float>((now - it->t0) / it->dur);
        if (k >= 1.0f) {
            it = S.particles.erase(it);
            continue;
        }
        if (k >= 0.0f) {
            Vector2 p = lerp(it->from, it->to, ease(k));
            p.y -= std::sin(k * PI) * 18;
            DrawCircleV(p, 5, it->color);
            DrawCircleV(p, 9, Fade(it->color, 0.18f));
        }
        ++it;
    }

    // baner zdarzenia masowego
    if (now < S.massFlashUntil) {
        float a = static_cast<float>(std::min(1.0, (S.massFlashUntil - now) / 0.6));
        Rectangle b{flow.x + 14, flow.y + flow.height - 52, 404, 40};
        DrawRectangleRounded(b, 0.4f, 8, Fade(ALERT, 0.95f * a));
        P.text(P.f.bold, std::string(L.massBanner) + "  ·  +" + std::to_string(S.massFlashCount) + " " + L.patients,
               b.x + b.width / 2, b.y + 10, 16, Fade(WHITE, a), CENTER);
    }

    // ================= zasoby =================
    const Rectangle resCard{16, 420, 684, 296};
    P.card(resCard);
    P.text(P.f.bold, L.resources, resCard.x + 20, resCard.y + 14, 17, FG);
    P.text(P.f.regular, L.resourcesSub, resCard.x + 20, resCard.y + 36, 12, MUTED);
    {
        std::map<std::string, const ResourceSnapshot *> byKey;
        for (const auto &r : s.resources) byKey[r.key] = &r;
        for (int i = 0; i < 17; ++i) {
            auto found = byKey.find(RES_KEYS[i]);
            if (found == byKey.end()) continue;
            const ResourceSnapshot &r = *found->second;
            const int col = i % 3, row = i / 3;
            Rectangle t{resCard.x + 16 + col * 222.0f, resCard.y + 60 + row * 34.0f, 214, 29};
            const bool full = r.total > 0 && r.inUse >= r.total - r.cooling && r.inUse > 0;
            DrawRectangleRounded(t, 0.3f, 6, full ? Fade(ALERT, 0.12f) : CARD2);
            P.text(P.f.regular, L.res[i], t.x + 10, t.y + 7, 13, full ? FG : MUTED);
            // pipsy: zajete / odnawiane / wolne
            const int units = std::min(r.total, 6);
            for (int u = 0; u < units; ++u) {
                Rectangle pip{t.x + t.width - 12 - (units - u) * 15.0f, t.y + 9, 11, 11};
                Color c = EDGE;
                if (u < r.inUse) c = full ? ALERT : PRIO[1];
                else if (u < r.inUse + r.cooling) c = COOL;
                DrawRectangleRounded(pip, 0.35f, 4, c);
            }
        }
        float lx = resCard.x + 20;
        const float ly = resCard.y + resCard.height - 22;
        for (auto [label, c] : {std::pair<const char *, Color>{L.inUse, PRIO[1]}, {L.cooling, COOL}, {L.free, EDGE}}) {
            DrawRectangleRounded({lx, ly + 2, 11, 11}, 0.35f, 4, c);
            P.text(P.f.regular, label, lx + 17, ly, 12, MUTED);
            lx += P.width(P.f.regular, label, 12) + 40;
        }
        P.text(P.f.regular, std::string(L.deadlock) + ": " + std::to_string(s.deadlockAlarms),
               resCard.x + resCard.width - 20, ly, 12, MUTED, RIGHT);
    }

    // ================= wykres kolejek =================
    const Rectangle chart{716, 420, W - 732, 160};
    P.card(chart);
    P.text(P.f.bold, L.chartTitle, chart.x + 20, chart.y + 14, 15, FG);
    {
        const Rectangle plot{chart.x + 44, chart.y + 60, chart.width - 64, chart.height - 76};
        int maxV = 4;
        for (const auto &h : S.history) {
            maxV = std::max(maxV, h.inSystem);
            for (int q : h.queue) maxV = std::max(maxV, q);
        }
        maxV = ((maxV + 3) / 4) * 4;
        for (int g = 0; g <= 4; ++g) {
            float y = plot.y + plot.height - plot.height * g / 4.0f;
            DrawLineEx({plot.x, y}, {plot.x + plot.width, y}, 1, Fade(EDGE, 0.7f));
            P.text(P.f.mono, std::to_string(maxV * g / 4), plot.x - 8, y - 7, 11, DIM, RIGHT);
        }
        auto px = [&](double t) { return plot.x + plot.width * static_cast<float>(1.0 - (now - t) / 120.0); };
        auto py = [&](int v) { return plot.y + plot.height - plot.height * v / static_cast<float>(maxV); };
        // w systemie jako wypelnione pole
        for (size_t i = 1; i < S.history.size(); ++i) {
            const auto &a = S.history[i - 1], &b = S.history[i];
            float x0 = px(a.t), x1 = px(b.t);
            if (x1 < plot.x) continue;
            x0 = std::max(x0, plot.x);
            Vector2 p0{x0, py(a.inSystem)}, p1{x1, py(b.inSystem)};
            Vector2 b0{x0, plot.y + plot.height}, b1{x1, plot.y + plot.height};
            DrawTriangle(p0, b0, b1, Fade(MUTED, 0.12f));
            DrawTriangle(p0, b1, p1, Fade(MUTED, 0.12f));
        }
        for (int d = 0; d < 3; ++d)
            for (size_t i = 1; i < S.history.size(); ++i) {
                const auto &a = S.history[i - 1], &b = S.history[i];
                if (px(b.t) < plot.x) continue;
                DrawLineEx({std::max(px(a.t), plot.x), py(a.queue[d])}, {px(b.t), py(b.queue[d])}, 2.0f, DEPT[d]);
            }
        float lx = chart.x + 36;
        for (int d = -1; d < 3; ++d) {
            const char *name = d >= 0 ? L.dept[d] : L.chartInSystem;
            Color c = d >= 0 ? DEPT[d] : Fade(MUTED, 0.5f);
            DrawRectangleRounded({lx - 16, chart.y + 42, 10, d >= 0 ? 4.0f : 8.0f}, 1.0f, 4, c);
            P.text(P.f.regular, name, lx, chart.y + 36, 12, MUTED);
            lx += P.width(P.f.regular, name, 12) + 34;
        }
    }

    // ================= oczekiwanie wg priorytetu =================
    const Rectangle waitCard{716, 592, W - 732, 124};
    P.card(waitCard);
    P.text(P.f.bold, L.waitTitle, waitCard.x + 20, waitCard.y + 12, 15, FG);
    {
        double maxW = 1;
        for (double w : s.stats.prioAvgWaitMs) maxW = std::max(maxW, w);
        for (int pr = 0; pr < 4; ++pr) {
            float y = waitCard.y + 40 + pr * 19.0f;
            P.text(P.f.regular, L.prio[pr], waitCard.x + 20, y, 12, MUTED);
            Rectangle track{waitCard.x + 110, y + 3, waitCard.width - 250, 10};
            DrawRectangleRounded(track, 1.0f, 6, CARD2);
            if (s.stats.prioCount[pr] > 0) {
                float w = std::max(10.0f, track.width * static_cast<float>(s.stats.prioAvgWaitMs[pr] / maxW));
                DrawRectangleRounded({track.x, track.y, w, track.height}, 1.0f, 6, PRIO[pr]);
                P.text(P.f.mono, secs(s.stats.prioAvgWaitMs[pr]) + "  n=" + std::to_string(s.stats.prioCount[pr]),
                       track.x + track.width + 12, y, 12, FG);
            } else {
                P.text(P.f.regular, L.noData, track.x + track.width + 12, y, 12, DIM);
            }
        }
    }

    // ================= stopka =================
    {
        float lx = 20;
        for (int pr = 0; pr < 4; ++pr) {
            DrawCircle(static_cast<int>(lx + 5), 738, 5, PRIO[pr]);
            P.text(P.f.regular, L.prio[pr], lx + 14, 730, 13, MUTED);
            lx += P.width(P.f.regular, L.prio[pr], 13) + 34;
        }
        P.text(P.f.regular, L.keys, W - 20, 730, 13, Fade(MUTED, 0.85f), RIGHT);
    }

    EndDrawing();
    return true;
}
