// Charge.cpp
#include "Charge.hpp"

#include <functional>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

std::string money(double amount)
{
    std::ostringstream os;
    os << "Rs " << std::fixed << std::setprecision(2) << amount;
    return os.str();
}

std::string number(double value)
{
    std::ostringstream os;
    os << std::setprecision(15) << value;
    return os.str();
}

void requireNonNegative(double value, const char* what)
{
    if (value < 0.0)
        throw std::invalid_argument(std::string(what) + " cannot be negative");
}

double toDouble(const std::string& s)
{
    std::size_t used = 0;
    const double v = std::stod(s, &used);
    if (used != s.size())
        throw std::invalid_argument("bad number '" + s + "'");
    return v;
}

int toInt(const std::string& s)
{
    std::size_t used = 0;
    const int v = std::stoi(s, &used);
    if (used != s.size())
        throw std::invalid_argument("bad integer '" + s + "'");
    return v;
}

} // namespace

Charge::Charge(std::string description, int date)
    : description_(std::move(description)), date_(date)
{
    if (description_.empty())
        throw std::invalid_argument("a charge needs a description");
}

std::unique_ptr<Charge> Charge::restore(const std::string& category,
                                        const std::string& description, int date,
                                        const std::vector<std::string>& p)
{
    using Maker = std::function<std::unique_ptr<Charge>()>;
    const std::map<std::string, std::pair<std::size_t, Maker>> makers = {
        {"BED", {2, [&] { return std::unique_ptr<Charge>(new BedCharge(
                              description, date, toDouble(p.at(0)), toInt(p.at(1)))); }}},
        {"MEDICINE", {2, [&] { return std::unique_ptr<Charge>(new MedicineCharge(
                              description, date, toDouble(p.at(0)), toInt(p.at(1)))); }}},
        {"TEST", {2, [&] { return std::unique_ptr<Charge>(new TestCharge(
                              description, date, toDouble(p.at(0)), p.at(1) == "1")); }}},
        {"PROCEDURE", {3, [&] { return std::unique_ptr<Charge>(new ProcedureCharge(
                              description, date, toDouble(p.at(0)), toDouble(p.at(1)),
                              toDouble(p.at(2)))); }}},
    };
    const auto it = makers.find(category);
    if (it == makers.end())
        throw std::invalid_argument("unknown charge category '" + category + "'");
    if (p.size() != it->second.first)
        throw std::invalid_argument("wrong parameter count for " + category + " charge");
    return it->second.second();
}

// ---------------------------------------------------------------- Bed --

BedCharge::BedCharge(std::string description, int date, double ratePerDay, int days)
    : Charge(std::move(description), date), rate_(ratePerDay), days_(days)
{
    requireNonNegative(rate_, "bed rate");
    if (days_ < 1)
        throw std::invalid_argument("a bed charge covers at least one day");
}

double BedCharge::calculateCost() const
{
    return rate_ * days_;
}

std::string BedCharge::detail() const
{
    return std::to_string(days_) + (days_ == 1 ? " day x " : " days x ") + money(rate_);
}

std::unique_ptr<Charge> BedCharge::clone() const
{
    return std::unique_ptr<Charge>(new BedCharge(*this));
}

std::vector<std::string> BedCharge::parameters() const
{
    return {number(rate_), std::to_string(days_)};
}

// ----------------------------------------------------------- Medicine --

MedicineCharge::MedicineCharge(std::string description, int date, double unitPrice,
                               int quantity)
    : Charge(std::move(description), date), unitPrice_(unitPrice), quantity_(quantity)
{
    requireNonNegative(unitPrice_, "unit price");
    if (quantity_ < 1)
        throw std::invalid_argument("quantity must be at least 1");
}

double MedicineCharge::calculateCost() const
{
    return unitPrice_ * quantity_;
}

std::string MedicineCharge::detail() const
{
    return std::to_string(quantity_) + " x " + money(unitPrice_);
}

std::unique_ptr<Charge> MedicineCharge::clone() const
{
    return std::unique_ptr<Charge>(new MedicineCharge(*this));
}

std::vector<std::string> MedicineCharge::parameters() const
{
    return {number(unitPrice_), std::to_string(quantity_)};
}

// --------------------------------------------------------------- Test --

TestCharge::TestCharge(std::string description, int date, double price, bool urgent)
    : Charge(std::move(description), date), price_(price), urgent_(urgent)
{
    requireNonNegative(price_, "test price");
}

double TestCharge::calculateCost() const
{
    return price_ * (urgent_ ? 1.0 + kUrgentSurcharge : 1.0);
}

std::string TestCharge::detail() const
{
    return money(price_) + (urgent_ ? " + 25% urgent" : " routine");
}

std::unique_ptr<Charge> TestCharge::clone() const
{
    return std::unique_ptr<Charge>(new TestCharge(*this));
}

std::vector<std::string> TestCharge::parameters() const
{
    return {number(price_), urgent_ ? "1" : "0"};
}

// ---------------------------------------------------------- Procedure --

ProcedureCharge::ProcedureCharge(std::string description, int date, double professionalFee,
                                 double facilityFee, double consumables)
    : Charge(std::move(description), date),
      professionalFee_(professionalFee), facilityFee_(facilityFee), consumables_(consumables)
{
    requireNonNegative(professionalFee_, "professional fee");
    requireNonNegative(facilityFee_, "facility fee");
    requireNonNegative(consumables_, "consumables");
}

double ProcedureCharge::calculateCost() const
{
    return professionalFee_ + facilityFee_ + consumables_;
}

std::string ProcedureCharge::detail() const
{
    return "fee " + money(professionalFee_) + " + facility " + money(facilityFee_) +
           " + consumables " + money(consumables_);
}

std::unique_ptr<Charge> ProcedureCharge::clone() const
{
    return std::unique_ptr<Charge>(new ProcedureCharge(*this));
}

std::vector<std::string> ProcedureCharge::parameters() const
{
    return {number(professionalFee_), number(facilityFee_), number(consumables_)};
}
