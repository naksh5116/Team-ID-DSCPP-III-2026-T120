// Charge.hpp - one line on a bill.
//
// Charge is abstract: every subtype prices itself through
// calculateCost(), so the billing code only ever holds Charge pointers and
// never asks what kind of charge it has.
#ifndef VR_CHARGE_HPP
#define VR_CHARGE_HPP

#include <memory>
#include <string>
#include <vector>

class Charge {
public:
    Charge(std::string description, int date);
    virtual ~Charge() = default;

    virtual double calculateCost() const = 0;
    virtual std::string category() const = 0;       // BED, MEDICINE, TEST, PROCEDURE
    virtual std::string detail() const = 0;         // how the cost was reached
    virtual std::unique_ptr<Charge> clone() const = 0;
    // The values a subtype needs to be rebuilt (persistence).
    virtual std::vector<std::string> parameters() const = 0;

    const std::string& description() const { return description_; }
    int date() const { return date_; }

    // Rebuilds a saved charge. The saved category picks the subtype from a
    // lookup table; throws std::invalid_argument on bad input.
    static std::unique_ptr<Charge> restore(const std::string& category,
                                           const std::string& description, int date,
                                           const std::vector<std::string>& parameters);

protected:
    Charge(const Charge&) = default;
    Charge& operator=(const Charge&) = default;

private:
    std::string description_;
    int date_;
};

// Bed occupancy: daily rate x days.
class BedCharge final : public Charge {
public:
    BedCharge(std::string description, int date, double ratePerDay, int days);
    double calculateCost() const override;
    std::string category() const override { return "BED"; }
    std::string detail() const override;
    std::unique_ptr<Charge> clone() const override;
    std::vector<std::string> parameters() const override;
    int days() const { return days_; }

private:
    double rate_;
    int days_;
};

// Dispensed medicine: unit price x quantity.
class MedicineCharge final : public Charge {
public:
    MedicineCharge(std::string description, int date, double unitPrice, int quantity);
    double calculateCost() const override;
    std::string category() const override { return "MEDICINE"; }
    std::string detail() const override;
    std::unique_ptr<Charge> clone() const override;
    std::vector<std::string> parameters() const override;

private:
    double unitPrice_;
    int quantity_;
};

// Diagnostic test: base price, plus 25% when run as urgent (STAT).
class TestCharge final : public Charge {
public:
    static constexpr double kUrgentSurcharge = 0.25;

    TestCharge(std::string description, int date, double price, bool urgent);
    double calculateCost() const override;
    std::string category() const override { return "TEST"; }
    std::string detail() const override;
    std::unique_ptr<Charge> clone() const override;
    std::vector<std::string> parameters() const override;

private:
    double price_;
    bool urgent_;
};

// Consultation or procedure: professional fee + facility fee + consumables.
class ProcedureCharge final : public Charge {
public:
    ProcedureCharge(std::string description, int date, double professionalFee,
                    double facilityFee, double consumables);
    double calculateCost() const override;
    std::string category() const override { return "PROCEDURE"; }
    std::string detail() const override;
    std::unique_ptr<Charge> clone() const override;
    std::vector<std::string> parameters() const override;

private:
    double professionalFee_;
    double facilityFee_;
    double consumables_;
};

#endif // VR_CHARGE_HPP
