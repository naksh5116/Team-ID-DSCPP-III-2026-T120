// Insurance.cpp
#include "Insurance.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

Insurance::Insurance(std::string provider, std::string policyNumber, Kind kind,
                     double value, double cap)
    : provider_(std::move(provider)), policy_(std::move(policyNumber)),
      kind_(kind), value_(value), cap_(cap)
{
    if (provider_.empty())
        throw std::invalid_argument("insurance needs a provider name");
    if (value_ < 0.0 || cap_ < 0.0)
        throw std::invalid_argument("insurance amounts cannot be negative");
    if (kind_ == Kind::Percentage && value_ > 100.0)
        throw std::invalid_argument("insurance percentage cannot exceed 100");
}

double Insurance::deductionFor(double amount) const
{
    if (amount <= 0.0)
        return 0.0;
    double d = kind_ == Kind::Flat ? value_ : amount * value_ / 100.0;
    if (kind_ == Kind::Percentage && cap_ > 0.0)
        d = std::min(d, cap_);
    return std::clamp(d, 0.0, amount);
}

std::string Insurance::summary() const
{
    std::ostringstream os;
    os << provider_;
    if (!policy_.empty())
        os << " (" << policy_ << ")";
    os << ", " << std::fixed << std::setprecision(2);
    if (kind_ == Kind::Flat) {
        os << "flat Rs " << value_;
    } else {
        os << std::setprecision(value_ == static_cast<int>(value_) ? 0 : 2) << value_ << "%";
        if (cap_ > 0.0)
            os << " up to Rs " << std::setprecision(2) << cap_;
    }
    return os.str();
}

std::string Insurance::kindName(Kind k)
{
    return k == Kind::Flat ? "FLAT" : "PERCENT";
}

bool Insurance::parseKind(const std::string& text, Kind& out)
{
    if (text == "FLAT") {
        out = Kind::Flat;
        return true;
    }
    if (text == "PERCENT") {
        out = Kind::Percentage;
        return true;
    }
    return false;
}
