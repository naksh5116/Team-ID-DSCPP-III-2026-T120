// Text.hpp - explicit conversions between the session's std::string and Qt,
// and the formats shared with the console (money, dates, times).
#ifndef VR_GUI_TEXT_HPP
#define VR_GUI_TEXT_HPP

#include "Patient.hpp"   // PatientStatus
#include "Person.hpp"    // vrclock

#include <QDate>
#include <QString>

#include <string>

namespace Text {

inline QString qs(const std::string& s) { return QString::fromStdString(s); }
inline QString qs(const char* s) { return QString::fromUtf8(s); }
inline std::string ss(const QString& s) { return s.trimmed().toStdString(); }

// "Rs 1234.50", exactly as the console prints amounts.
inline QString money(double v)
{
    return QStringLiteral("Rs %1").arg(v, 0, 'f', 2);
}

inline QString date(int ymd) { return qs(vrclock::formatDate(ymd)); }       // 2026-09-30
inline QString clock(int minutes) { return qs(vrclock::formatTime(minutes)); } // 14:05

// Sentence-case status for the GUI (the model's names are upper case for
// the console and the record files).
inline QString statusText(PatientStatus s)
{
    switch (s) {
    case PatientStatus::Waiting: return QStringLiteral("Waiting");
    case PatientStatus::Admitted: return QStringLiteral("Admitted");
    case PatientStatus::Treated: return QStringLiteral("Treated");
    case PatientStatus::Discharged: return QStringLiteral("Discharged");
    }
    return QString();
}

// YYYYMMDD <-> QDate (the session keys dates as YYYYMMDD integers).
inline int ymd(const QDate& d) { return d.year() * 10000 + d.month() * 100 + d.day(); }
inline QDate qdate(int ymd) { return QDate(ymd / 10000, (ymd / 100) % 100, ymd % 100); }

// "Total distance: 89 m. Estimated walking time: 74 s (1 min 14 s)." -
// the console's route footer as one sentence.
inline QString routeSummary(double metres, int seconds)
{
    QString s = QStringLiteral("Total distance: %1 m. Estimated walking time: %2 s")
                    .arg(qRound64(metres)).arg(seconds);
    if (seconds >= 60)
        s += QStringLiteral(" (%1 min %2 s)").arg(seconds / 60).arg(seconds % 60);
    return s + QLatin1Char('.');
}

} // namespace Text

#endif // VR_GUI_TEXT_HPP
