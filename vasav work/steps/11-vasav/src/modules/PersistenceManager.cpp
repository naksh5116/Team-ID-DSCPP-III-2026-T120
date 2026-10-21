// PersistenceManager.cpp
#include "PersistenceManager.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

const char* const kPatients = "patients.dat";
const char* const kStaff = "staff.dat";
const char* const kVisits = "visits.dat";
const char* const kBills = "bills.dat";

// Joins escaped fields with '|'.
class Record {
public:
    explicit Record(const std::string& tag) : line_(tag) {}
    Record& operator<<(const std::string& field)
    {
        line_ += '|';
        line_ += PersistenceManager::escape(field);
        return *this;
    }
    Record& operator<<(const char* field) { return *this << std::string(field); }
    Record& operator<<(int v) { return *this << std::to_string(v); }
    Record& operator<<(long v) { return *this << std::to_string(v); }
    Record& operator<<(double v)
    {
        std::ostringstream os;
        os << std::setprecision(15) << v;
        return *this << os.str();
    }
    const std::string& str() const { return line_; }

private:
    std::string line_;
};

int toInt(const std::string& s)
{
    std::size_t used = 0;
    const int v = std::stoi(s, &used);
    if (used != s.size())
        throw std::invalid_argument("bad integer '" + s + "'");
    return v;
}

long toLong(const std::string& s)
{
    std::size_t used = 0;
    const long v = std::stol(s, &used);
    if (used != s.size())
        throw std::invalid_argument("bad integer '" + s + "'");
    return v;
}

double toDouble(const std::string& s)
{
    std::size_t used = 0;
    const double v = std::stod(s, &used);
    if (used != s.size())
        throw std::invalid_argument("bad number '" + s + "'");
    return v;
}

PatientKind parseKind(const std::string& s)
{
    if (s == Patient::kindName(PatientKind::Emergency))
        return PatientKind::Emergency;
    if (s == Patient::kindName(PatientKind::Outpatient))
        return PatientKind::Outpatient;
    throw std::invalid_argument("unknown patient kind '" + s + "'");
}

PatientStatus parseStatus(const std::string& s)
{
    for (PatientStatus st : {PatientStatus::Waiting, PatientStatus::Admitted,
                             PatientStatus::Treated, PatientStatus::Discharged})
        if (s == Patient::statusName(st))
            return st;
    throw std::invalid_argument("unknown patient status '" + s + "'");
}

void expectFields(const std::vector<std::string>& f, std::size_t n)
{
    if (f.size() != n)
        throw std::invalid_argument("expected " + std::to_string(n) + " fields, found " +
                                    std::to_string(f.size()));
}

// Calls `handle` for every non-comment line; wraps errors with file:line.
void readRecords(const std::string& path,
                 const std::function<void(const std::vector<std::string>&)>& handle)
{
    std::ifstream in(path);
    if (!in)
        return;                         // a missing file means no records
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;
        try {
            handle(PersistenceManager::split(line));
        } catch (const std::exception& e) {
            throw std::runtime_error(path + ":" + std::to_string(lineNo) + ": " + e.what());
        }
    }
}

// Writes via "<file>.tmp" and renames, so a failed save never leaves a
// half-written record file behind.
void writeFile(const std::string& path, const std::string& header,
               const std::vector<std::string>& lines)
{
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out)
            throw std::runtime_error("cannot write " + tmp);
        out << "# " << header << '\n';
        for (const std::string& l : lines)
            out << l << '\n';
        if (!out.flush())
            throw std::runtime_error("error while writing " + tmp);
    }
    std::remove(path.c_str());
    if (std::rename(tmp.c_str(), path.c_str()) != 0)
        throw std::runtime_error("cannot replace " + path);
}

} // namespace

PersistenceManager::PersistenceManager(std::string directory) : dir_(std::move(directory))
{
}

std::string PersistenceManager::pathOf(const std::string& file) const
{
    return dir_ + "/" + file;
}

bool PersistenceManager::hasSavedRecords() const
{
    return std::ifstream(pathOf(kPatients)).good();
}

std::string PersistenceManager::escape(const std::string& field)
{
    std::string out;
    for (char c : field) {
        switch (c) {
        case '|':  out += "\\p"; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': break;
        default:   out += c;
        }
    }
    return out;
}

std::vector<std::string> PersistenceManager::split(const std::string& line)
{
    std::vector<std::string> fields(1);
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c == '|') {
            fields.emplace_back();
        } else if (c == '\\' && i + 1 < line.size()) {
            const char n = line[++i];
            fields.back() += n == 'p' ? '|' : n == 'n' ? '\n' : n;
        } else {
            fields.back() += c;
        }
    }
    return fields;
}

PersistenceManager::Counts PersistenceManager::save(const PatientRegistry& patients,
                                                    const StaffRegistry& staff,
                                                    const BillingEngine& billing) const
{
    Counts counts;
    std::error_code ec;
    std::filesystem::create_directories(dir_, ec);   // fine if it already exists

    std::vector<std::string> patientLines, visitLines, staffLines, billLines;
    for (const Patient* p : patients.all()) {
        Record r("P");
        r << p->id() << p->name() << p->age() << p->phone() << Patient::kindName(p->kind())
          << p->severity() << p->arrivalSeq() << p->admissionDate()
          << Patient::statusName(p->status()) << p->complaint() << p->department()
          << p->bedCode() << p->bedLabel() << p->doctorId() << p->dischargeDate();
        patientLines.push_back(r.str());
        for (const VisitRecord& v : p->visitsOldestFirst()) {
            Record vr("V");
            vr << p->id() << v.date << v.doctor << v.diagnosis << v.prescriptions;
            visitLines.push_back(vr.str());
            ++counts.visits;
        }
        ++counts.patients;
    }

    for (const Doctor* d : staff.doctors()) {
        Record r("D");
        r << d->id() << d->name() << d->age() << d->phone() << d->department() << d->shift()
          << d->specialty() << d->consultationFee() << d->nextFree() << d->patientsSeen();
        staffLines.push_back(r.str());
    }
    for (const Nurse* n : staff.nurses()) {
        Record r("N");
        r << n->id() << n->name() << n->age() << n->phone() << n->department() << n->shift()
          << n->ward();
        staffLines.push_back(r.str());
    }
    for (const Admin* a : staff.admins()) {
        Record r("A");
        r << a->id() << a->name() << a->age() << a->phone() << a->department() << a->shift()
          << a->office();
        staffLines.push_back(r.str());
    }
    counts.staff = staffLines.size();

    for (const Bill* b : billing.listBills(BillingEngine::SortKey::Date)) {
        Record r("B");
        r << b->billId() << b->patientId() << b->openedOn() << (b->settled() ? 1 : 0)
          << b->settledOn() << b->insuranceDeduction();
        if (const Insurance* ins = b->insurance())
            r << ins->provider() << ins->policyNumber() << Insurance::kindName(ins->kind())
              << ins->value() << ins->cap();
        else
            r << "" << "" << "" << "" << "";
        billLines.push_back(r.str());
        for (std::size_t i = 0; i < b->chargeCount(); ++i) {
            const Charge& c = b->charge(i);
            Record cr("C");
            cr << b->billId() << c.category() << c.description() << c.date();
            for (const std::string& param : c.parameters())
                cr << param;
            billLines.push_back(cr.str());
            ++counts.charges;
        }
        ++counts.bills;
    }

    writeFile(pathOf(kStaff), "VitalRoute staff records v1", staffLines);
    writeFile(pathOf(kVisits), "VitalRoute visit records v1", visitLines);
    writeFile(pathOf(kBills), "VitalRoute bill records v1", billLines);
    // patients.dat last: its presence is what marks a complete save.
    writeFile(pathOf(kPatients), "VitalRoute patient records v1", patientLines);
    return counts;
}

PersistenceManager::Counts PersistenceManager::load(PatientRegistry& patients,
                                                    StaffRegistry& staff,
                                                    BillingEngine& billing) const
{
    Counts counts;
    patients.clear();
    staff.clear();
    billing.clear();

    readRecords(pathOf(kStaff), [&](const std::vector<std::string>& f) {
        if (f[0] == "D") {
            expectFields(f, 11);
            Doctor& d = staff.addDoctor(std::unique_ptr<Doctor>(new Doctor(
                f[1], f[2], toInt(f[3]), f[4], f[5], f[6], f[7], toDouble(f[8]))));
            d.setNextFree(toInt(f[9]));
            d.setPatientsSeen(toInt(f[10]));
        } else if (f[0] == "N") {
            expectFields(f, 8);
            staff.addNurse(std::unique_ptr<Nurse>(
                new Nurse(f[1], f[2], toInt(f[3]), f[4], f[5], f[6], f[7])));
        } else if (f[0] == "A") {
            expectFields(f, 8);
            staff.addAdmin(std::unique_ptr<Admin>(
                new Admin(f[1], f[2], toInt(f[3]), f[4], f[5], f[6], f[7])));
        } else {
            throw std::invalid_argument("unknown staff record '" + f[0] + "'");
        }
        ++counts.staff;
    });
    staff.refreshAvailability();

    // Re-registering each patient rebuilds the hash and AVL indexes.
    readRecords(pathOf(kPatients), [&](const std::vector<std::string>& f) {
        if (f[0] != "P")
            throw std::invalid_argument("unknown patient record '" + f[0] + "'");
        expectFields(f, 16);
        std::unique_ptr<Patient> p(new Patient(f[1], f[2], toInt(f[3]), f[4], parseKind(f[5]),
                                               toInt(f[6]), toInt(f[8]), f[10], f[11]));
        p->setArrivalSeq(toLong(f[7]));
        p->setStatus(parseStatus(f[9]));
        if (!f[12].empty())
            p->assignBed(f[12], f[13]);
        p->setDoctorId(f[14]);
        p->setDischargeDate(toInt(f[15]));
        patients.adopt(std::move(p));
        ++counts.patients;
    });

    readRecords(pathOf(kVisits), [&](const std::vector<std::string>& f) {
        if (f[0] != "V")
            throw std::invalid_argument("unknown visit record '" + f[0] + "'");
        expectFields(f, 6);
        patients.find(f[1]).addVisit(VisitRecord::make(toInt(f[2]), f[3], f[4], f[5]));
        ++counts.visits;
    });

    std::map<std::string, std::string> billOwner;      // bill ID -> patient ID
    std::map<std::string, std::pair<int, double>> settlement;
    readRecords(pathOf(kBills), [&](const std::vector<std::string>& f) {
        if (f[0] == "B") {
            expectFields(f, 12);
            Bill b(f[1], f[2], toInt(f[3]));
            if (!f[7].empty()) {
                Insurance::Kind kind = Insurance::Kind::Flat;
                if (!Insurance::parseKind(f[9], kind))
                    throw std::invalid_argument("unknown insurance kind '" + f[9] + "'");
                b.setInsurance(Insurance(f[7], f[8], kind, toDouble(f[10]), toDouble(f[11])));
            }
            if (f[4] == "1")
                settlement[f[1]] = {toInt(f[5]), toDouble(f[6])};
            billOwner[f[1]] = f[2];
            billing.adopt(std::move(b));
            ++counts.bills;
        } else if (f[0] == "C") {
            if (f.size() < 5)
                throw std::invalid_argument("charge record is too short");
            const auto owner = billOwner.find(f[1]);
            if (owner == billOwner.end())
                throw std::invalid_argument("charge for unknown bill " + f[1]);
            const std::vector<std::string> params(f.begin() + 5, f.end());
            billing.billFor(owner->second) += *Charge::restore(f[2], f[3], toInt(f[4]), params);
            ++counts.charges;
        } else {
            throw std::invalid_argument("unknown bill record '" + f[0] + "'");
        }
    });
    // Close settled bills only after all their charges are back.
    for (const auto& s : settlement)
        billing.billFor(billOwner[s.first]).restoreSettlement(s.second.first, s.second.second);

    return counts;
}
