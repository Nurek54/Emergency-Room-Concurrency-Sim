#ifndef SOR_TYPES_H
#define SOR_TYPES_H
#include <string>

enum class Priority { RED = 0, YELLOW, GREEN, BLUE };
enum class DepartmentID { SURGERY = 0, ORTHOPEDIC, CARDIO };

inline std::string toStr(Priority p)
{
    switch (p) {
        case Priority::RED  : return "CZERWONY";
        case Priority::YELLOW: return "ZOLTY";
        case Priority::GREEN: return "ZIELONY";
        default            : return "NIEBIESKI";
    }
}
inline std::string toStr(DepartmentID d)
{
    switch (d) {
        case DepartmentID::SURGERY  : return "Chirurgia";
        case DepartmentID::ORTHOPEDIC: return "Ortopedia";
        default                     : return "Kardiologia";
    }
}
#endif
