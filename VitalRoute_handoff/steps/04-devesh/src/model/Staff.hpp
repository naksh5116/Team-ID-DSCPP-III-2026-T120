// Staff.hpp - hospital employees: Staff -> Doctor, Nurse, Admin.
//
// Role behaviour is virtual: only doctors prescribe, only administrators
// may authorise manual bill entries, and each role describes its own duty.
#ifndef VR_STAFF_HPP
#define VR_STAFF_HPP

#include "Person.hpp"

#include <string>

class Staff : public Person {
public:
    Staff(std::string id, std::string name, int age, std::string phone,
          std::string department, std::string shift);

    const std::string& department() const { return department_; }
    const std::string& shift() const { return shift_; }

    virtual std::string duty() const = 0;
    virtual bool canPrescribe() const { return false; }
    virtual bool canAuthorizeCharges() const { return false; }

    void describe(std::ostream& os) const override;

protected:
    std::string department_;
    std::string shift_;
};

class Doctor final : public Staff {
public:
    Doctor(std::string id, std::string name, int age, std::string phone,
           std::string department, std::string shift,
           std::string specialty, double consultationFee);

    std::string role() const override { return "Doctor"; }
    std::string duty() const override;
    bool canPrescribe() const override { return true; }
    void describe(std::ostream& os) const override;

    const std::string& specialty() const { return specialty_; }
    double consultationFee() const { return fee_; }

    // Minutes since midnight when the doctor is next free.
    int nextFree() const { return nextFree_; }
    void setNextFree(int minutes) { nextFree_ = minutes; }
    int patientsSeen() const { return seen_; }
    void setPatientsSeen(int n) { seen_ = n; }

    // Book a consultation at or after `now`; returns its start time.
    int beginConsultation(int now, int minutes);

private:
    std::string specialty_;
    double fee_;
    int nextFree_ = 0;
    int seen_ = 0;
};

class Nurse final : public Staff {
public:
    Nurse(std::string id, std::string name, int age, std::string phone,
          std::string department, std::string shift, std::string ward);

    std::string role() const override { return "Nurse"; }
    std::string duty() const override;
    const std::string& ward() const { return ward_; }

private:
    std::string ward_;
};

class Admin final : public Staff {
public:
    Admin(std::string id, std::string name, int age, std::string phone,
          std::string department, std::string shift, std::string office);

    std::string role() const override { return "Administrator"; }
    std::string duty() const override;
    bool canAuthorizeCharges() const override { return true; }
    const std::string& office() const { return office_; }

private:
    std::string office_;
};

#endif // VR_STAFF_HPP
