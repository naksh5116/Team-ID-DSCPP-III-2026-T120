// Person.hpp - abstract base for everyone the hospital keeps a record of,
// plus the calendar/clock helpers the model shares.
#ifndef VR_PERSON_HPP
#define VR_PERSON_HPP

#include <iosfwd>
#include <string>

// Dates are YYYYMMDD integers (they sort correctly and key the AVL index);
// clock times are minutes since midnight.
namespace vrclock {
int         today();                          // local date, YYYYMMDD
int         minutesNow();                     // local time, minutes since 00:00
bool        validDate(int ymd);
int         parseDate(const std::string& text);   // YYYYMMDD or YYYY-MM-DD; 0 if invalid
std::string formatDate(int ymd);              // 2026-09-30
int         daysBetween(int fromYmd, int toYmd);  // to - from, in days
int         addDays(int ymd, int days);
std::string formatTime(int minutes);          // 14:05 (+1d when past midnight)
}

class Person {
public:
    Person(std::string id, std::string name, int age, std::string phone);
    virtual ~Person() = default;

    Person(const Person&) = default;
    Person& operator=(const Person&) = default;
    Person(Person&&) = default;
    Person& operator=(Person&&) = default;

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    int age() const { return age_; }
    const std::string& phone() const { return phone_; }

    // "Doctor", "Nurse", "Emergency patient", ...
    virtual std::string role() const = 0;
    // One-line summary; subclasses append their own details.
    virtual void describe(std::ostream& os) const;

protected:
    std::string id_;
    std::string name_;
    int age_;
    std::string phone_;
};

// Prints describe() - dispatches to the most-derived class.
std::ostream& operator<<(std::ostream& os, const Person& person);

#endif // VR_PERSON_HPP
