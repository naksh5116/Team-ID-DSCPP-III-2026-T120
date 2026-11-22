// BillingEngine.cpp
#include "BillingEngine.hpp"

#include "Exceptions.hpp"
#include "vr_sort.h"

#include <cstdio>
#include <cstdlib>
#include <utility>

Bill& BillingEngine::open(const std::string& patientId, int date)
{
    auto it = bills_.find(patientId);
    if (it != bills_.end())
        return it->second;
    char id[16];
    std::snprintf(id, sizeof id, "BL-%06ld", nextSerial_++);
    return bills_.emplace(patientId, Bill(id, patientId, date)).first->second;
}

Bill* BillingEngine::find(const std::string& patientId)
{
    auto it = bills_.find(patientId);
    return it == bills_.end() ? nullptr : &it->second;
}

const Bill* BillingEngine::find(const std::string& patientId) const
{
    auto it = bills_.find(patientId);
    return it == bills_.end() ? nullptr : &it->second;
}

Bill& BillingEngine::billFor(const std::string& patientId)
{
    Bill* b = find(patientId);
    if (!b)
        throw PatientNotFoundException(patientId);
    return *b;
}

void BillingEngine::charge(const std::string& patientId, const Charge& charge)
{
    billFor(patientId) += charge;
}

void BillingEngine::manualCharge(const std::string& patientId, const Charge& charge,
                                 const Staff& authoriser)
{
    if (!authoriser.canAuthorizeCharges())
        throw std::runtime_error(authoriser.name() + " (" + authoriser.role() +
                                 ") is not authorised to add manual charges");
    billFor(patientId) += charge;
}

std::unique_ptr<Charge> BillingEngine::undoLastCharge(const std::string& patientId)
{
    return billFor(patientId).removeLastCharge();
}

void BillingEngine::attachInsurance(const std::string& patientId, const Insurance& cover)
{
    billFor(patientId).setInsurance(cover);
}

const Bill& BillingEngine::settle(const std::string& patientId, int date)
{
    Bill& b = billFor(patientId);
    b.settle(date);
    return b;
}

void BillingEngine::remove(const std::string& patientId)
{
    bills_.erase(patientId);
}

std::vector<const Bill*> BillingEngine::all() const
{
    std::vector<const Bill*> out;
    out.reserve(bills_.size());
    for (const auto& entry : bills_)
        out.push_back(&entry.second);
    return out;
}

std::vector<const Bill*> BillingEngine::listBills(SortKey key) const
{
    std::vector<const Bill*> out = all();
    if (key == SortKey::Date) {
        vr_merge_sort(out.data(), out.size(), sizeof(const Bill*), [](const void* a, const void* b) {
            const Bill* x = *static_cast<const Bill* const*>(a);
            const Bill* y = *static_cast<const Bill* const*>(b);
            if (x->openedOn() != y->openedOn())
                return x->openedOn() < y->openedOn() ? -1 : 1;
            return x->billId().compare(y->billId());
        });
    } else {
        vr_merge_sort(out.data(), out.size(), sizeof(const Bill*), [](const void* a, const void* b) {
            const double x = (*static_cast<const Bill* const*>(a))->total();
            const double y = (*static_cast<const Bill* const*>(b))->total();
            return (x < y) - (x > y);              // largest first
        });
    }
    return out;
}

Bill& BillingEngine::adopt(Bill bill)
{
    const std::string patientId = bill.patientId();
    long serial = 0;
    if (std::sscanf(bill.billId().c_str(), "BL-%ld", &serial) == 1 && serial >= nextSerial_)
        nextSerial_ = serial + 1;
    bills_.erase(patientId);
    return bills_.emplace(patientId, std::move(bill)).first->second;
}

void BillingEngine::clear()
{
    bills_.clear();
    nextSerial_ = 1;
}
