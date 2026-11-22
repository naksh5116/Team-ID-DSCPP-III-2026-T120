// WebText.hpp - the wording the web pages show, shared with the console:
// money ("Rs 1234.50"), dates, sentence-case status, ESI names, the route
// footer, and the console's error prefixes ("Not found: ...").
#ifndef VR_WEB_TEXT_HPP
#define VR_WEB_TEXT_HPP

#include "Exceptions.hpp"
#include "Patient.hpp"
#include "Person.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <exception>
#include <string>

namespace WebText {

inline std::string money(double v)
{
    char buf[48];
    std::snprintf(buf, sizeof buf, "Rs %.2f", v);
    return buf;
}

inline std::string date(int ymd) { return ymd ? vrclock::formatDate(ymd) : std::string(); }
inline std::string clock(int minutes) { return vrclock::formatTime(minutes); }

inline std::string status(PatientStatus s)
{
    switch (s) {
    case PatientStatus::Waiting: return "Waiting";
    case PatientStatus::Admitted: return "Admitted";
    case PatientStatus::Treated: return "Treated";
    case PatientStatus::Discharged: return "Discharged";
    }
    return "";
}

inline std::string esiName(int level)
{
    switch (level) {
    case 1: return "Resuscitation";
    case 2: return "Emergent";
    case 3: return "Urgent";
    case 4: return "Less urgent";
    case 5: return "Non-urgent";
    default: return "";
    }
}

inline std::string esiDescription(int level)
{
    switch (level) {
    case 1: return "Needs an immediate life-saving intervention";
    case 2: return "High-risk, confused or lethargic, or in severe pain or distress";
    case 3: return "Stable; expected to need two or more resources";
    case 4: return "Stable; expected to need one resource";
    case 5: return "Stable; expected to need no resources";
    default: return "";
    }
}

// "BED" -> "Bed" (the model's charge categories are upper case).
inline std::string sentenceCase(const std::string& upper)
{
    std::string s;
    for (std::size_t i = 0; i < upper.size(); ++i)
        s += static_cast<char>(i == 0 ? std::toupper(static_cast<unsigned char>(upper[i]))
                                      : std::tolower(static_cast<unsigned char>(upper[i])));
    return s;
}

// "Total distance: 89 m. Estimated walking time: 74 s (1 min 14 s)."
inline std::string routeSummary(double metres, int seconds)
{
    std::string s = "Total distance: " + std::to_string(std::lround(metres)) +
                    " m. Estimated walking time: " + std::to_string(seconds) + " s";
    if (seconds >= 60)
        s += " (" + std::to_string(seconds / 60) + " min " + std::to_string(seconds % 60) + " s)";
    return s + ".";
}

// The console's prefixes, so the web shows the same messages.
inline std::string describe(const std::exception& e)
{
    const std::string what = e.what();
    if (dynamic_cast<const InvalidSeverityException*>(&e))
        return "Registration cancelled: " + what + ".";
    if (dynamic_cast<const PatientNotFoundException*>(&e))
        return "Not found: " + what + ".";
    if (dynamic_cast<const BedUnavailableException*>(&e))
        return "No bed: " + what + ".";
    if (dynamic_cast<const NoRouteFoundException*>(&e))
        return "No route: " + what + ".";
    if (dynamic_cast<const SlotUnavailableException*>(&e))
        return "Theatre: " + what + ".";
    return "Error: " + what + ".";
}

// HTTP status for an exception: 404 for a missing patient, 409 when the
// hospital state does not allow it (no bed, slot taken), 400 otherwise.
inline int statusFor(const std::exception& e)
{
    if (dynamic_cast<const PatientNotFoundException*>(&e))
        return 404;
    if (dynamic_cast<const BedUnavailableException*>(&e) || dynamic_cast<const NoRouteFoundException*>(&e) ||
        dynamic_cast<const SlotUnavailableException*>(&e))
        return 409;
    return 400;
}

} // namespace WebText

#endif // VR_WEB_TEXT_HPP
