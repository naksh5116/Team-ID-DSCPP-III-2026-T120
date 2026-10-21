// Insurance.hpp - a patient's cover, deducted once when the bill is settled.
#ifndef VR_INSURANCE_HPP
#define VR_INSURANCE_HPP

#include <string>

class Insurance {
public:
    enum class Kind { Flat, Percentage };

    // Flat: `value` rupees. Percentage: `value` percent of the bill, limited
    // to `cap` rupees when cap > 0.
    Insurance(std::string provider, std::string policyNumber, Kind kind,
              double value, double cap = 0.0);

    // Amount the insurer pays on a bill of `amount`; never negative and
    // never more than the bill itself, so the patient never goes below zero.
    double deductionFor(double amount) const;

    const std::string& provider() const { return provider_; }
    const std::string& policyNumber() const { return policy_; }
    Kind kind() const { return kind_; }
    double value() const { return value_; }
    double cap() const { return cap_; }
    std::string summary() const;

    static std::string kindName(Kind k);
    static bool parseKind(const std::string& text, Kind& out);

private:
    std::string provider_;
    std::string policy_;
    Kind kind_;
    double value_;
    double cap_;
};

#endif // VR_INSURANCE_HPP
