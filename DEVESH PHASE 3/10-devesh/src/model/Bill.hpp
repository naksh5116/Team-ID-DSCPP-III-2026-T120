// Bill.hpp - a patient's bill: a list of polymorphic charges plus optional
// insurance, which is applied exactly once, at settlement.
#ifndef VR_BILL_HPP
#define VR_BILL_HPP

#include "Charge.hpp"
#include "Insurance.hpp"

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Bill {
public:
    Bill(std::string billId, std::string patientId, int openedOn);
    ~Bill() = default;
    Bill(const Bill& other);                 // deep copy via Charge::clone()
    Bill& operator=(const Bill& other);
    Bill(Bill&&) noexcept = default;
    Bill& operator=(Bill&&) noexcept = default;

    // Appends a copy of the charge. Throws std::logic_error once settled.
    Bill& operator+=(const Charge& charge);
    // Removes and returns the newest charge (nullptr when there is none).
    std::unique_ptr<Charge> removeLastCharge();

    double subtotal() const;                  // sum of calculateCost()
    double insuranceDeduction() const;        // 0 until settled
    double total() const;                     // subtotal - deduction, never < 0
    double projectedTotal() const;            // what total() will be at settlement

    void setInsurance(const Insurance& cover);
    void clearInsurance();
    const Insurance* insurance() const { return insurance_ ? &*insurance_ : nullptr; }

    // Applies insurance (once) and closes the bill. Throws std::logic_error
    // if it is already settled.
    void settle(int date);
    // Used by the persistence loader to restore a settled bill as saved.
    void restoreSettlement(int date, double deduction);

    bool settled() const { return settled_; }
    int settledOn() const { return settledOn_; }
    int openedOn() const { return openedOn_; }
    const std::string& billId() const { return billId_; }
    const std::string& patientId() const { return patientId_; }
    std::size_t chargeCount() const { return charges_.size(); }
    const Charge& charge(std::size_t i) const { return *charges_.at(i); }

    friend std::ostream& operator<<(std::ostream& os, const Bill& bill);

private:
    std::string billId_;
    std::string patientId_;
    int openedOn_;
    std::vector<std::unique_ptr<Charge>> charges_;
    std::optional<Insurance> insurance_;
    bool settled_ = false;
    int settledOn_ = 0;
    double deduction_ = 0.0;
};

#endif // VR_BILL_HPP
