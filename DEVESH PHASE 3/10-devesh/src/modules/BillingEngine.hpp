// BillingEngine.hpp - one bill per registered patient.
//
// Charges accrue through Bill::operator+= as resources are consumed. The
// engine only ever handles Charge references - it never asks which kind of
// charge it holds. Listings are ordered with the C engine's merge sort.
#ifndef VR_BILLING_ENGINE_HPP
#define VR_BILLING_ENGINE_HPP

#include "Bill.hpp"
#include "Staff.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

class BillingEngine {
public:
    enum class SortKey { Date, Amount };

    // Opens a bill for the patient (or returns the one already open).
    Bill& open(const std::string& patientId, int date);
    Bill* find(const std::string& patientId);
    const Bill* find(const std::string& patientId) const;
    Bill& billFor(const std::string& patientId);          // throws PatientNotFoundException

    // Automatic accrual as resources are used: bill += charge.
    void charge(const std::string& patientId, const Charge& charge);
    // A charge entered by hand; only staff who may authorise charges
    // (administrators) can add one. Throws std::runtime_error otherwise.
    void manualCharge(const std::string& patientId, const Charge& charge, const Staff& authoriser);
    // Removes the newest charge on the patient's bill (nullptr if none).
    std::unique_ptr<Charge> undoLastCharge(const std::string& patientId);

    void attachInsurance(const std::string& patientId, const Insurance& cover);
    // Applies insurance once and closes the bill.
    const Bill& settle(const std::string& patientId, int date);
    void remove(const std::string& patientId);

    // All bills merge-sorted by opening date (oldest first) or by amount
    // (largest first).
    std::vector<const Bill*> listBills(SortKey key) const;

    // Used by the persistence loader.
    Bill& adopt(Bill bill);
    std::vector<const Bill*> all() const;
    void clear();
    std::size_t size() const { return bills_.size(); }

private:
    std::map<std::string, Bill> bills_;    // patient ID -> bill
    long nextSerial_ = 1;
};

#endif // VR_BILLING_ENGINE_HPP
