// ApiActions.cpp - the calls that change the hospital (POST).
//
// Every action goes through the shared session, then WebHub announces
// exactly what changed (the same table as the Qt SessionHub):
//
//   register a patient         queue, patients, bills
//   treat next patient         queue, patients, bills, staff, beds; routeAssigned(id)
//   visit note                 patients
//   theatre book / release     theatres (+ bills on booking)
//   transfer desk              transfers (+ bills for a charged ambulance run)
//   manual charge, insurance   bills
//   discharge                  patients, beds, bills
//   undo, save, reload         everything (+ saved / reloaded)
//
// Requests are form-encoded. Answers carry the console's sentence in
// "message"; refusals are an "error" with the console's wording.
#include "Api.hpp"

#include "Json.hpp"
#include "WebHub.hpp"
#include "WebText.hpp"

#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>

namespace {

std::string trimmed(const std::string& s)
{
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos)
        return "";
    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

std::string field(const HttpRequest& req, const std::string& name)
{
    return trimmed(req.param(name));
}

std::string need(const HttpRequest& req, const std::string& name, const std::string& message)
{
    const std::string v = field(req, name);
    if (v.empty())
        throw std::invalid_argument(message);
    return v;
}

bool asInt(const std::string& text, long& out)
{
    if (text.empty())
        return false;
    char* end = nullptr;
    out = std::strtol(text.c_str(), &end, 10);
    return *end == '\0';
}

int intField(const HttpRequest& req, const std::string& name, long lo, long hi, const std::string& message)
{
    long v = 0;
    if (!asInt(field(req, name), v) || v < lo || v > hi)
        throw std::invalid_argument(message);
    return static_cast<int>(v);
}

double amountField(const HttpRequest& req, const std::string& name, const std::string& message)
{
    const std::string text = field(req, name);
    if (text.empty())
        return 0.0;
    char* end = nullptr;
    const double v = std::strtod(text.c_str(), &end);
    if (*end != '\0' || v < 0.0)
        throw std::invalid_argument(message);
    return v;
}

bool flag(const HttpRequest& req, const std::string& name)
{
    const std::string v = field(req, name);
    return v == "1" || v == "true" || v == "on" || v == "yes";
}

// Insurance fields, when an insurer is given (same questions as the console).
std::optional<Insurance> insuranceFrom(const HttpRequest& req)
{
    const std::string provider = field(req, "insurer");
    if (provider.empty())
        return std::nullopt;
    const bool percent = field(req, "coverKind") == "percent";
    const double value = percent ? amountField(req, "percent", "Enter the percentage covered (0-100).")
                                 : amountField(req, "amount", "Enter an amount of zero or more.");
    if (percent && value > 100.0)
        throw std::invalid_argument("Enter the percentage covered (0-100).");
    const double cap = percent ? amountField(req, "cap", "Enter a maximum payout of zero or more.") : 0.0;
    return Insurance(provider, field(req, "policy"), percent ? Insurance::Kind::Percentage : Insurance::Kind::Flat,
                     value, cap);
}

int dateField(const HttpRequest& req, const std::string& name)
{
    const std::string text = field(req, name);
    if (text.empty())
        return vrclock::today();
    const int d = vrclock::parseDate(text);
    if (!d)
        throw std::invalid_argument("Dates look like 2026-09-30 or 20260930.");
    return d;
}

HttpResponse reply(const std::string& message, long version, const std::string& extraKey = "",
                   const std::string& extraValue = "")
{
    JsonWriter j;
    j.beginObject().field("message", message).field("version", version);
    if (!extraKey.empty())
        j.field(extraKey, extraValue);
    j.endObject();
    return HttpResponse::json(j.str());
}

HttpResponse refuse(int status, const std::string& message)
{
    JsonWriter j;
    j.beginObject().field("error", message).endObject();
    return HttpResponse::json(j.str(), status);
}

} // namespace

void Api::registerActions()
{
    auto bind = [this](HttpResponse (Api::*fn)(const HttpRequest&)) {
        return [this, fn](const HttpRequest& req) { return (this->*fn)(req); };
    };
    posts_["/api/register-emergency"] = bind(&Api::registerEmergency);
    posts_["/api/register-opd"] = bind(&Api::registerOpd);
    posts_["/api/treat-next"] = bind(&Api::treatNext);
    posts_["/api/visit-note"] = bind(&Api::visitNote);
    posts_["/api/theatre/book"] = bind(&Api::bookTheatre);
    posts_["/api/theatre/release"] = bind(&Api::releaseTheatre);
    posts_["/api/transfer"] = bind(&Api::addTransfer);
    posts_["/api/transfer/dispatch"] = bind(&Api::dispatchTransfer);
    posts_["/api/transfer/cancel"] = bind(&Api::cancelTransfer);
    posts_["/api/charge"] = bind(&Api::addCharge);
    posts_["/api/insurance"] = bind(&Api::recordInsurance);
    posts_["/api/discharge"] = bind(&Api::discharge);
    posts_["/api/undo"] = bind(&Api::undo);
    posts_["/api/save"] = bind(&Api::save);
    posts_["/api/reload"] = bind(&Api::reload);
}

// ----------------------------------------------------------- reception --

HttpResponse Api::registerEmergency(const HttpRequest& req)
{
    const std::string name = need(req, "name", "Enter the patient's name.");
    const int age = intField(req, "age", 0, 130, "Enter the patient's age (0 to 130).");
    const std::string complaint = need(req, "complaint", "Enter the presenting complaint.");
    long severity = 0;
    if (!asInt(field(req, "severity"), severity))
        throw std::invalid_argument("Choose an ESI level, 1 (most critical) to 5.");
    const std::optional<Insurance> cover = insuranceFrom(req);
    HospitalSession& s = hub_.live();
    // Throws InvalidSeverityException for 0 or 6+ (the console's message).
    const Patient& p = s.registerEmergency(name, age, field(req, "phone"), complaint, static_cast<int>(severity));
    if (cover)
        s.attachInsurance(p.id(), *cover);
    std::string message = "Registered " + p.id() + " (ESI " + std::to_string(p.severity()) +
                          "). Position in the emergency queue: " + std::to_string(s.emergencyPosition(p.id())) +
                          " of " + std::to_string(s.emergencyOrder().size()) + ".";
    if (cover)
        message += " Insurance recorded; it is deducted once, when the bill is settled.";
    const long v = hub_.announce(WebHub::Queue | WebHub::Patients | WebHub::Bills);
    return reply(message, v, "patientId", p.id());
}

HttpResponse Api::registerOpd(const HttpRequest& req)
{
    const std::string name = need(req, "name", "Enter the patient's name.");
    const int age = intField(req, "age", 0, 130, "Enter the patient's age (0 to 130).");
    const std::string complaint = need(req, "complaint", "Enter the reason for the visit.");
    std::string dept = field(req, "department");
    if (dept.empty())
        dept = "General Medicine";   // the console's fallback
    const std::optional<Insurance> cover = insuranceFrom(req);
    HospitalSession& s = hub_.live();
    const Patient& p = s.registerOpd(name, age, field(req, "phone"), complaint, dept);
    if (cover)
        s.attachInsurance(p.id(), *cover);
    std::string message = "Registered " + p.id() + " for " + dept + ". Position in the OPD line: " +
                          std::to_string(s.triage().opdCount()) + ".";
    if (cover)
        message += " Insurance recorded; it is deducted once, when the bill is settled.";
    const long v = hub_.announce(WebHub::Queue | WebHub::Patients | WebHub::Bills);
    return reply(message, v, "patientId", p.id());
}

// ----------------------------------------------------------- treatment --

HttpResponse Api::treatNext(const HttpRequest& req)
{
    using Outcome = HospitalSession::TreatResult::Outcome;
    HospitalSession& s = hub_.live();
    const TriageUnit& t = s.triage();
    std::optional<HospitalSession::MedicineLine> medicine;
    if (!t.hasEmergency() && t.hasOpd() && !field(req, "medicine").empty()) {
        medicine = HospitalSession::MedicineLine{
            field(req, "medicine"), intField(req, "quantity", 1, 1000, "Enter a quantity from 1 to 1000."),
            amountField(req, "unitPrice", "Enter a unit price of zero or more.")};
    }
    const HospitalSession::TreatResult r = s.treatNext(field(req, "diagnosis"), field(req, "prescriptions"), medicine);

    // The console's option 3 lines ("Patient:" in place of "Next patient:",
    // because the page shows them under "Last treatment").
    std::vector<std::string> lines;
    std::string status, outcome;
    const Patient* p = r.patient;
    switch (r.outcome) {
    case Outcome::NobodyWaiting:
        return refuse(409, "Nobody is waiting.");
    case Outcome::Outpatient:
        outcome = "outpatient";
        lines.push_back("Outpatient: " + p->id() + "  " + p->name() + " - " + p->complaint() + " (" +
                        p->department() + ")");
        if (r.doctor)
            lines.push_back("Seen by " + r.doctor->name() + " at " + WebText::clock(r.doctorStart));
        lines.push_back("Consultation recorded. Bill so far: " + WebText::money(r.billSoFar));
        status = p->id() + " seen in OPD. Bill so far: " + WebText::money(r.billSoFar);
        break;
    default:
        lines.push_back("Patient: " + p->id() + "  " + p->name() + ", ESI " + std::to_string(p->severity()) + " - " +
                        p->complaint());
        if (r.outcome == Outcome::NoBed) {
            outcome = "noBed";
            lines.push_back("Cannot admit yet: " + r.error + ". The patient stays at the head of the queue.");
            status = lines.back();
            break;
        }
        if (r.doctor)
            lines.push_back("Attending: " + r.doctor->name() + " from " + WebText::clock(r.doctorStart));
        if (r.outcome == Outcome::ReleasedFromEr) {
            outcome = "released";
            lines.push_back("Treated in the ER and released - ESI " + std::to_string(p->severity()) +
                            " does not need a bed.");
            status = p->id() + " treated in the ER and released.";
            break;
        }
        outcome = "admitted";
        lines.push_back("Bed: " + r.bed.label + " (" + Patient::unitName(r.bed.unit) + ")");
        if (!r.routeError.empty())
            lines.push_back("No route: " + r.routeError + ".");
        status = p->id() + " admitted. Bed " + r.bed.label + ".";
        break;
    }
    const long v = hub_.announce(WebHub::Queue | WebHub::Patients | WebHub::Bills | WebHub::Staff | WebHub::Beds,
                                 outcome == "admitted" ? p->id() : std::string());
    JsonWriter j;
    j.beginObject()
        .field("message", status)
        .field("version", v)
        .field("outcome", outcome)
        .field("patientId", p->id())
        .field("bed", r.outcome == Outcome::Admitted ? r.bed.label : std::string());
    j.key("lines").beginArray();
    for (const std::string& l : lines)
        j.value(l);
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::visitNote(const HttpRequest& req)
{
    const std::string id = need(req, "id", "Look up a patient first.");
    const std::string doctor = need(req, "doctor", "Enter the doctor's name.");
    const std::string diagnosis = need(req, "diagnosis", "Enter the diagnosis.");
    const int date = dateField(req, "date");
    HospitalSession& s = hub_.live();
    const Patient& p = s.findPatient(id);
    s.addVisitNote(p.id(), date, doctor, diagnosis, field(req, "prescriptions"));
    const long v = hub_.announce(WebHub::Patients);
    return reply("Visit added in date order to " + p.id() + ".", v);
}

// ---------------------------------------------- theatres and transfers --

HttpResponse Api::bookTheatre(const HttpRequest& req)
{
    const std::string theatre = need(req, "theatre", "No operation theatres are configured.");
    const std::string patientId = need(req, "patientId", "Enter the patient ID.");
    const std::string procedure = need(req, "procedure", "Enter the procedure.");
    const double fee = amountField(req, "fee", "Enter a surgeon's fee of zero or more.");
    HospitalSession& s = hub_.live();
    const Patient& p = s.findPatient(patientId);
    const OTScheduler::Booking b = s.bookTheatre(theatre, p.id(), procedure, fee);
    const long v = hub_.announce(WebHub::Theatres | WebHub::Bills);
    return reply("Booked " + theatre + " slot " + std::to_string(b.slot + 1) + ", " +
                     s.theatres().slotTime(theatre, b.slot) + " for " + p.name() + ".",
                 v);
}

HttpResponse Api::releaseTheatre(const HttpRequest& req)
{
    const std::string theatre = need(req, "theatre", "Choose a theatre.");
    HospitalSession& s = hub_.live();
    const int slot = intField(req, "slot", 1, 1000, "Choose a slot number.");
    s.releaseTheatreSlot(theatre, slot - 1);   // throws SlotUnavailableException when free
    const long v = hub_.announce(WebHub::Theatres);
    return reply("Released " + theatre + " slot " + std::to_string(slot) + "; it is back in the free-slot ring.", v);
}

HttpResponse Api::addTransfer(const HttpRequest& req)
{
    const std::string from = need(req, "from", "Enter where the request starts (From).");
    const std::string to = need(req, "to", "Enter where the request goes (To).");
    HospitalSession& s = hub_.live();
    const TransferRequest r = s.requestTransfer(flag(req, "urgent"), flag(req, "ambulance"), field(req, "patientId"),
                                                from, to);
    const long v = hub_.announce(WebHub::Transfers);
    return reply("Queued " + TransferDesk::describe(r), v, "code", r.code);
}

HttpResponse Api::dispatchTransfer(const HttpRequest&)
{
    HospitalSession& s = hub_.live();
    const HospitalSession::DispatchResult d = s.dispatchNextTransfer();
    if (!d.dispatched)
        return reply("No pending requests.", hub_.version());
    std::string message = "Dispatched " + TransferDesk::describe(d.request);
    if (!d.chargedBillId.empty())
        message += " Ambulance charge added to " + d.chargedBillId + ".";
    const long v = hub_.announce(WebHub::Transfers | (d.chargedBillId.empty() ? 0u : WebHub::Bills));
    return reply(message, v);
}

HttpResponse Api::cancelTransfer(const HttpRequest&)
{
    HospitalSession& s = hub_.live();
    TransferRequest r;
    if (!s.cancelLastRoutineTransfer(r))
        return reply("The newest request is not a routine one (or none is pending).", hub_.version());
    const long v = hub_.announce(WebHub::Transfers);
    return reply("Cancelled " + TransferDesk::describe(r), v);
}

// --------------------------------------------------------- billing --

HttpResponse Api::addCharge(const HttpRequest& req)
{
    const std::string id = need(req, "id", "Open a bill first.");
    const std::string authoriser = need(req, "authoriser", "Choose who authorises the charge.");
    const std::string description = need(req, "description", "Enter a description for the charge.");
    const std::string type = field(req, "type");
    const int today = vrclock::today();
    std::unique_ptr<Charge> charge;
    if (type == "bed")
        charge = std::make_unique<BedCharge>(description, today, amountField(req, "rate", "Enter a rate of zero or more."),
                                             intField(req, "days", 1, 365, "Enter the days (1 to 365)."));
    else if (type == "medicine")
        charge = std::make_unique<MedicineCharge>(description, today,
                                                  amountField(req, "unitPrice", "Enter a unit price of zero or more."),
                                                  intField(req, "quantity", 1, 10000, "Enter a quantity (1 to 10000)."));
    else if (type == "test")
        charge = std::make_unique<TestCharge>(description, today,
                                              amountField(req, "price", "Enter a price of zero or more."),
                                              flag(req, "urgent"));
    else if (type == "procedure")
        charge = std::make_unique<ProcedureCharge>(
            description, today, amountField(req, "fee", "Enter a professional fee of zero or more."),
            amountField(req, "facility", "Enter a facility fee of zero or more."),
            amountField(req, "consumables", "Enter consumables of zero or more."));
    else
        throw std::invalid_argument("Choose a charge type: bed, medicine, test or procedure.");
    HospitalSession& s = hub_.live();
    const Patient& p = s.findPatient(id);
    s.addManualCharge(p.id(), *charge, authoriser);   // only an administrator may (std::runtime_error)
    const long v = hub_.announce(WebHub::Bills);
    return reply("Added " + WebText::money(charge->calculateCost()) + ". New subtotal " +
                     WebText::money(s.findBill(p.id())->subtotal()) + ".",
                 v);
}

HttpResponse Api::recordInsurance(const HttpRequest& req)
{
    const std::string id = need(req, "id", "Open a bill first.");
    need(req, "insurer", "Enter the insurer.");
    const std::optional<Insurance> cover = insuranceFrom(req);
    HospitalSession& s = hub_.live();
    const Patient& p = s.findPatient(id);
    s.attachInsurance(p.id(), *cover);
    const long v = hub_.announce(WebHub::Bills);
    return reply("Insurance recorded; it is deducted once, when the bill is settled.", v);
}

HttpResponse Api::discharge(const HttpRequest& req)
{
    using Block = HospitalSession::DischargeBlock;
    const std::string id = need(req, "id", "Open a bill first.");
    HospitalSession& s = hub_.live();
    const Patient& p = s.findPatient(id);
    const Block block = s.dischargeBlocker(p.id());
    if (block == Block::StillWaiting)
        return refuse(409, p.id() + " is still waiting to be seen.");
    if (block == Block::AlreadyDischarged)
        return refuse(409, p.id() + " was already discharged.");
    int date = dateField(req, "date");
    const bool clamped = date < p.admissionDate();
    if (clamped)
        date = p.admissionDate();
    const std::string bed = p.bedLabel();
    const Bill& bill = s.discharge(p.id(), date, std::nullopt);
    std::string message;
    if (clamped)
        message += "Discharge cannot be before admission; using the admission date. ";
    if (!bed.empty())
        message += bed + " is free again. ";
    message += "Bill " + bill.billId() + " settled: paid " + WebText::money(bill.total()) + ".";
    const long v = hub_.announce(WebHub::Patients | WebHub::Beds | WebHub::Bills);
    return reply(message, v);
}

// ------------------------------------------------- undo, save, reload --

HttpResponse Api::undo(const HttpRequest&)
{
    using Kind = HospitalSession::UndoResult::Kind;
    HospitalSession& s = hub_.live();
    const HospitalSession::UndoResult r = s.undoLast();
    std::string message;
    switch (r.kind) {   // the console's option 11 sentences
    case Kind::Nothing:
        return reply("Nothing to undo.", hub_.version());
    case Kind::RegistrationDropped:
        message = "Cannot undo the " + r.note + ": " + r.patientId + " has already been seen. Entry dropped.";
        break;
    case Kind::ChargeDropped:
        message = "Cannot undo the " + r.note + ": the bill has changed since (or is settled). Entry dropped.";
        break;
    case Kind::RegistrationUndone:
        message = "Undone: " + r.note + " (" + r.patientId + ").";
        break;
    case Kind::ChargeUndone:
        message = "Undone: " + r.note + " (" + WebText::money(r.amount) + ").";
        break;
    }
    const long v = hub_.announce(WebHub::Everything);
    return reply(message, v);
}

HttpResponse Api::save(const HttpRequest&)
{
    HospitalSession& s = hub_.live();
    const PersistenceManager::Counts c = s.save();
    const long v = hub_.announce(WebHub::Everything);
    hub_.announceEvent("saved");
    return reply("Saved " + std::to_string(c.patients) + " patients, " + std::to_string(c.staff) + " staff, " +
                     std::to_string(c.visits) + " visits, " + std::to_string(c.bills) + " bills (" +
                     std::to_string(c.charges) + " charges) to " + s.recordsDir() + "/",
                 v);
}

HttpResponse Api::reload(const HttpRequest&)
{
    const HospitalSession::StartupReport r = hub_.reload();   // announces everything + "reloaded"
    if (!r.loadedFromRecords)
        return reply("No saved records: sample patients and staff created again.", hub_.version());
    return reply("Loaded " + std::to_string(r.loaded.patients) + " patients, " + std::to_string(r.loaded.staff) +
                     " staff, " + std::to_string(r.loaded.visits) + " visits and " + std::to_string(r.loaded.bills) +
                     " bills from " + hub_.session().recordsDir() + "/",
                 hub_.version());
}
