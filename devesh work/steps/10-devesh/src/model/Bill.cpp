// Bill.cpp
#include "Bill.hpp"

#include "Person.hpp"   // vrclock::formatDate

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>

Bill::Bill(std::string billId, std::string patientId, int openedOn)
    : billId_(std::move(billId)), patientId_(std::move(patientId)), openedOn_(openedOn)
{
}

Bill::Bill(const Bill& other)
    : billId_(other.billId_), patientId_(other.patientId_), openedOn_(other.openedOn_),
      insurance_(other.insurance_), settled_(other.settled_),
      settledOn_(other.settledOn_), deduction_(other.deduction_)
{
    charges_.reserve(other.charges_.size());
    for (const auto& c : other.charges_)
        charges_.push_back(c->clone());
}

Bill& Bill::operator=(const Bill& other)
{
    if (this != &other) {
        Bill copy(other);
        *this = std::move(copy);
    }
    return *this;
}

Bill& Bill::operator+=(const Charge& charge)
{
    if (settled_)
        throw std::logic_error("bill " + billId_ + " is settled and cannot take new charges");
    charges_.push_back(charge.clone());
    return *this;
}

std::unique_ptr<Charge> Bill::removeLastCharge()
{
    if (settled_)
        throw std::logic_error("bill " + billId_ + " is settled and cannot be edited");
    if (charges_.empty())
        return nullptr;
    std::unique_ptr<Charge> last = std::move(charges_.back());
    charges_.pop_back();
    return last;
}

double Bill::subtotal() const
{
    double sum = 0.0;
    for (const auto& c : charges_)
        sum += c->calculateCost();      // virtual dispatch, no type checks
    return sum;
}

double Bill::insuranceDeduction() const
{
    return settled_ ? deduction_ : 0.0;
}

double Bill::total() const
{
    return std::max(0.0, subtotal() - insuranceDeduction());
}

double Bill::projectedTotal() const
{
    if (settled_)
        return total();
    const double sub = subtotal();
    return std::max(0.0, sub - (insurance_ ? insurance_->deductionFor(sub) : 0.0));
}

void Bill::setInsurance(const Insurance& cover)
{
    if (settled_)
        throw std::logic_error("bill " + billId_ + " is already settled");
    insurance_ = cover;
}

void Bill::clearInsurance()
{
    if (settled_)
        throw std::logic_error("bill " + billId_ + " is already settled");
    insurance_.reset();
}

void Bill::settle(int date)
{
    if (settled_)
        throw std::logic_error("bill " + billId_ + " was already settled");
    deduction_ = insurance_ ? insurance_->deductionFor(subtotal()) : 0.0;
    settled_ = true;
    settledOn_ = date;
}

void Bill::restoreSettlement(int date, double deduction)
{
    settled_ = true;
    settledOn_ = date;
    deduction_ = std::clamp(deduction, 0.0, subtotal());
}

std::ostream& operator<<(std::ostream& os, const Bill& bill)
{
    const std::string rule(64, '-');
    const auto flags = os.flags();
    const auto precision = os.precision();

    os << rule << '\n'
       << " BILL " << bill.billId_ << "   patient " << bill.patientId_
       << "   opened " << vrclock::formatDate(bill.openedOn_) << '\n'
       << rule << '\n';
    if (bill.charges_.empty())
        os << "  (no charges yet)\n";

    os << std::fixed << std::setprecision(2);
    for (std::size_t i = 0; i < bill.charges_.size(); ++i) {
        const Charge& c = *bill.charges_[i];
        std::string desc = c.description();
        if (desc.size() > 36)
            desc = desc.substr(0, 35) + "~";
        os << ' ' << std::right << std::setw(2) << i + 1 << ". " << std::left << std::setw(10)
           << c.category() << std::setw(37) << desc << std::right << std::setw(12)
           << c.calculateCost() << '\n'
           << "               " << c.detail() << '\n';
    }
    os << rule << '\n'
       << std::left << std::setw(50) << " Subtotal" << std::right << std::setw(14)
       << bill.subtotal() << '\n';

    if (bill.insurance_) {
        if (bill.settled_) {
            os << std::left << std::setw(50) << (" Insurance: " + bill.insurance_->summary())
               << std::right << std::setw(14) << -bill.deduction_ << '\n';
        } else {
            os << " Insurance on file: " << bill.insurance_->summary()
               << "\n   (deducted once, at settlement)\n";
        }
    }
    if (bill.settled_) {
        os << std::left << std::setw(50) << " TOTAL PAID" << std::right << std::setw(14)
           << bill.total() << '\n'
           << " Status: SETTLED on " << vrclock::formatDate(bill.settledOn_) << '\n';
    } else {
        os << std::left << std::setw(50) << " Amount due (after insurance)" << std::right
           << std::setw(14) << bill.projectedTotal() << '\n'
           << " Status: OPEN\n";
    }
    os << rule << '\n';
    os.flags(flags);
    os.precision(precision);
    return os;
}
