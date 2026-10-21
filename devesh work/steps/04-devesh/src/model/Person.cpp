// Person.cpp
#include "Person.hpp"

#include <cctype>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

bool isLeap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

int daysInMonth(int y, int m)
{
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return days[m - 1] + (m == 2 && isLeap(y) ? 1 : 0);
}

// Days since 1970-01-01 (civil calendar, proleptic Gregorian).
long toDayNumber(int ymd)
{
    int y = ymd / 10000, m = ymd / 100 % 100, d = ymd % 100;
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const long yoe = y - era * 400;
    const long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

int fromDayNumber(long z)
{
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const long doe = z - era * 146097;
    const long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long mp = (5 * doy + 2) / 153;
    const int d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    const int m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    const int y = static_cast<int>(yoe + era * 400 + (m <= 2));
    return y * 10000 + m * 100 + d;
}

std::tm localNow()
{
    std::time_t t = std::time(nullptr);
    std::tm out{};
    const std::tm* local = std::localtime(&t);
    if (local)
        out = *local;
    return out;
}

} // namespace

namespace vrclock {

int today()
{
    const std::tm now = localNow();
    return (now.tm_year + 1900) * 10000 + (now.tm_mon + 1) * 100 + now.tm_mday;
}

int minutesNow()
{
    const std::tm now = localNow();
    return now.tm_hour * 60 + now.tm_min;
}

bool validDate(int ymd)
{
    const int y = ymd / 10000, m = ymd / 100 % 100, d = ymd % 100;
    return y >= 1900 && y <= 2999 && m >= 1 && m <= 12 && d >= 1 && d <= daysInMonth(y, m);
}

int parseDate(const std::string& text)
{
    std::string digits;
    for (char c : text) {
        if (std::isdigit(static_cast<unsigned char>(c)))
            digits += c;
        else if (c != '-' && c != '/' && !std::isspace(static_cast<unsigned char>(c)))
            return 0;
    }
    if (digits.size() != 8)
        return 0;
    const int ymd = std::stoi(digits);
    return validDate(ymd) ? ymd : 0;
}

std::string formatDate(int ymd)
{
    std::ostringstream os;
    os << ymd / 10000 << '-' << std::setw(2) << std::setfill('0') << ymd / 100 % 100
       << '-' << std::setw(2) << std::setfill('0') << ymd % 100;
    return os.str();
}

int daysBetween(int fromYmd, int toYmd)
{
    return static_cast<int>(toDayNumber(toYmd) - toDayNumber(fromYmd));
}

int addDays(int ymd, int days)
{
    return fromDayNumber(toDayNumber(ymd) + days);
}

std::string formatTime(int minutes)
{
    const int day = minutes / 1440, m = minutes % 1440;
    std::ostringstream os;
    os << std::setw(2) << std::setfill('0') << m / 60 << ':' << std::setw(2)
       << std::setfill('0') << m % 60;
    if (day > 0)
        os << " (+" << day << "d)";
    return os.str();
}

} // namespace vrclock

Person::Person(std::string id, std::string name, int age, std::string phone)
    : id_(std::move(id)), name_(std::move(name)), age_(age), phone_(std::move(phone))
{
    if (name_.empty())
        throw std::invalid_argument("a person needs a name");
    if (age_ < 0 || age_ > 130)
        throw std::invalid_argument("age must be between 0 and 130");
}

void Person::describe(std::ostream& os) const
{
    os << id_ << "  " << name_ << " (" << age_ << "), " << role();
    if (!phone_.empty())
        os << ", tel " << phone_;
}

std::ostream& operator<<(std::ostream& os, const Person& person)
{
    person.describe(os);
    return os;
}
