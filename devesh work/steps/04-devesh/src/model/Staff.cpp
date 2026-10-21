// Staff.cpp
#include "Staff.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>

Staff::Staff(std::string id, std::string name, int age, std::string phone,
             std::string department, std::string shift)
    : Person(std::move(id), std::move(name), age, std::move(phone)),
      department_(std::move(department)), shift_(std::move(shift))
{
}

void Staff::describe(std::ostream& os) const
{
    Person::describe(os);
    os << ", " << department_ << ", " << shift_ << " shift - " << duty();
}

Doctor::Doctor(std::string id, std::string name, int age, std::string phone,
               std::string department, std::string shift,
               std::string specialty, double consultationFee)
    : Staff(std::move(id), std::move(name), age, std::move(phone),
            std::move(department), std::move(shift)),
      specialty_(std::move(specialty)), fee_(consultationFee)
{
    if (fee_ < 0.0)
        throw std::invalid_argument("consultation fee cannot be negative");
}

std::string Doctor::duty() const
{
    return "consults and treats patients in " + specialty_;
}

void Doctor::describe(std::ostream& os) const
{
    Staff::describe(os);
    os << ", fee Rs " << std::fixed << std::setprecision(2) << fee_;
}

int Doctor::beginConsultation(int now, int minutes)
{
    const int start = std::max(now, nextFree_);
    nextFree_ = start + minutes;
    ++seen_;
    return start;
}

Nurse::Nurse(std::string id, std::string name, int age, std::string phone,
             std::string department, std::string shift, std::string ward)
    : Staff(std::move(id), std::move(name), age, std::move(phone),
            std::move(department), std::move(shift)),
      ward_(std::move(ward))
{
}

std::string Nurse::duty() const
{
    return "cares for patients on " + ward_;
}

Admin::Admin(std::string id, std::string name, int age, std::string phone,
             std::string department, std::string shift, std::string office)
    : Staff(std::move(id), std::move(name), age, std::move(phone),
            std::move(department), std::move(shift)),
      office_(std::move(office))
{
}

std::string Admin::duty() const
{
    return "authorises billing and keeps records at " + office_;
}
