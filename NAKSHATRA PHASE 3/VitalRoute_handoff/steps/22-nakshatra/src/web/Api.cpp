// Api.cpp - routing, the request guard, and the read calls (GET).
// The actions (POST) are in ApiActions.cpp.
#include "Api.hpp"

#include "Json.hpp"
#include "RouteSteps.hpp"
#include "TheatreSchedule.hpp"
#include "WebHub.hpp"
#include "WebText.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace {

HttpResponse error(int status, const std::string& message)
{
    JsonWriter j;
    j.beginObject().field("error", message).endObject();
    return HttpResponse::json(j.str(), status);
}

std::string required(const HttpRequest& req, const std::string& name, const std::string& message)
{
    std::string v = req.param(name);
    const auto b = v.find_first_not_of(" \t");
    v = b == std::string::npos ? "" : v.substr(b, v.find_last_not_of(" \t") - b + 1);
    if (v.empty())
        throw std::invalid_argument(message);
    return v;
}

// Every node id present in the map (ids need not be contiguous).
std::vector<int> nodeIds(const HospitalMap& map)
{
    std::vector<int> ids;
    for (int id = 0; static_cast<int>(ids.size()) < map.nodeCount() && id < 100000; ++id)
        if (map.hasNode(id))
            ids.push_back(id);
    return ids;
}

std::string nodeLabel(const vr_node& n)
{
    if (n.type == NODE_LIFT)
        return std::string(n.name).rfind("Lift B", 0) == 0 ? "Lift B" : "Lift A";
    if (n.type == NODE_STAIRS)
        return "Stairs";
    return n.name;
}

// What the next patient needs (the Doctor's "Care" line).
std::string needsText(const HospitalSession& s, const Patient& p)
{
    if (!p.isEmergency())
        return "OPD consultation";
    switch (p.requiredUnit()) {
    case CareUnit::ICU:
        return "Needs an ICU bed (" + std::to_string(s.router().freeBeds(CareUnit::ICU)) + " free)";
    case CareUnit::Ward:
        return "Needs a ward bed (" + std::to_string(s.router().freeBeds(CareUnit::Ward)) + " free)";
    case CareUnit::None:
        return "Treated in the emergency room, no bed";
    }
    return "";
}

void writeBillSummary(JsonWriter& j, const Bill* b)
{
    if (!b) {
        j.null();
        return;
    }
    j.beginObject()
        .field("billId", b->billId())
        .field("total", b->projectedTotal())
        .field("totalText", WebText::money(b->projectedTotal()))
        .field("settled", b->settled())
        .endObject();
}

int countBeds(const ResourceRouter& r, CareUnit unit)
{
    return static_cast<int>(std::count_if(r.beds().begin(), r.beds().end(),
                                          [unit](const Bed& b) { return b.unit == unit; }));
}

} // namespace

Api::Api(WebHub& hub) : hub_(hub)
{
    auto bind = [this](HttpResponse (Api::*fn)(const HttpRequest&)) {
        return [this, fn](const HttpRequest& req) { return (this->*fn)(req); };
    };
    gets_["/api/ping"] = bind(&Api::ping);
    gets_["/api/state"] = bind(&Api::state);
    gets_["/api/queues"] = bind(&Api::queues);
    gets_["/api/patient"] = bind(&Api::patient);
    gets_["/api/bill"] = bind(&Api::bill);
    gets_["/api/bills"] = bind(&Api::bills);
    gets_["/api/route"] = bind(&Api::route);
    gets_["/api/map"] = bind(&Api::map);
    gets_["/api/theatres"] = bind(&Api::theatres);
    gets_["/api/transfers"] = bind(&Api::transfers);
    gets_["/api/doctors"] = bind(&Api::doctors);
    gets_["/api/next-doctor"] = bind(&Api::nextDoctor);
    gets_["/api/staff"] = bind(&Api::staff);
    gets_["/api/admissions"] = bind(&Api::admissions);
    gets_["/api/indexes"] = bind(&Api::indexes);
    registerActions();
}

HttpResponse Api::handle(const HttpRequest& req)
{
    const bool isGet = req.method == "GET" || req.method == "HEAD";
    const auto get = gets_.find(req.path);
    const auto post = posts_.find(req.path);
    if (get == gets_.end() && post == posts_.end())
        return error(404, "Not found: no such API call.");
    try {
        if (isGet && get != gets_.end())
            return get->second(req);
        if (req.method == "POST" && post != posts_.end()) {
            // Request guard (see Api.hpp): our pages only.
            if (req.headers.count("x-vitalroute") == 0 || req.headers.at("x-vitalroute") != "1")
                return error(403, "Refused: changes must come from the VitalRoute pages.");
            const auto origin = req.headers.find("origin");
            const auto host = req.headers.find("host");
            if (origin != req.headers.end() && host != req.headers.end() &&
                origin->second != "http://" + host->second)
                return error(403, "Refused: changes must come from the VitalRoute pages.");
            return post->second(req);
        }
        return error(405, get != gets_.end() ? "Use GET for this call." : "Use POST for this call.");
    } catch (const std::invalid_argument& e) {
        return error(400, e.what());   // a missing or malformed field, worded for the page
    } catch (const std::exception& e) {
        return error(WebText::statusFor(e), WebText::describe(e));
    }
}

// ------------------------------------------------------------- reading --

HttpResponse Api::ping(const HttpRequest&)
{
    JsonWriter j;
    j.beginObject()
        .field("ok", true)
        .field("name", "VitalRoute Web")
        .field("version", hub_.version())
        .field("eventClients", eventClients_ ? static_cast<unsigned long long>(eventClients_()) : 0ULL)
        .key("links").beginArray();
    for (const std::string& link : links_)
        j.value(link);
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::state(const HttpRequest&)
{
    const HospitalSession& s = hub_.session();
    const TriageUnit& t = s.triage();
    const Patient* next = nullptr;
    if (t.hasEmergency())
        next = s.patients().tryFind(t.peekEmergency());
    else if (t.hasOpd())
        next = s.patients().tryFind(t.opdLine().front());

    JsonWriter j;
    j.beginObject()
        .field("version", hub_.version())
        .field("today", WebText::date(vrclock::today()))
        .field("waitingEmergency", static_cast<unsigned long long>(t.emergencyCount()))
        .field("waitingOpd", static_cast<unsigned long long>(t.opdCount()))
        .field("icuFree", s.router().freeBeds(CareUnit::ICU))
        .field("wardFree", s.router().freeBeds(CareUnit::Ward))
        .field("icuTotal", countBeds(s.router(), CareUnit::ICU))
        .field("wardTotal", countBeds(s.router(), CareUnit::Ward))
        .field("canUndo", s.canUndo())
        .field("undoNote", s.nextUndoNote())
        .field("recordsDir", s.recordsDir());
    j.key("next");
    if (!next) {
        j.null();
    } else {
        j.beginObject()
            .field("id", next->id())
            .field("name", next->name())
            .field("age", next->age())
            .field("kind", next->isEmergency() ? "emergency" : "opd")
            .field("severity", next->severity())
            .field("esiName", WebText::esiName(next->severity()))
            .field("complaint", next->complaint())
            .field("department", next->department())
            .field("needs", needsText(s, *next))
            .endObject();
    }
    j.endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::queues(const HttpRequest&)
{
    const HospitalSession& s = hub_.session();
    JsonWriter j;
    j.beginObject().key("emergency").beginArray();
    int position = 1;
    for (const Patient* p : s.emergencyOrder()) {   // heap order
        j.beginObject()
            .field("position", position++)
            .field("id", p->id())
            .field("name", p->name())
            .field("age", p->age())
            .field("severity", p->severity())
            .field("esiName", WebText::esiName(p->severity()))
            .field("complaint", p->complaint())
            .field("registered", WebText::date(p->admissionDate()))
            .endObject();
    }
    j.endArray().key("opd").beginArray();
    position = 1;
    for (const std::string& id : s.triage().opdLine()) {   // FIFO order
        const Patient* p = s.patients().tryFind(id);
        if (!p)
            continue;
        j.beginObject()
            .field("position", position++)
            .field("id", p->id())
            .field("name", p->name())
            .field("age", p->age())
            .field("department", p->department())
            .field("reason", p->complaint())
            .endObject();
    }
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::patient(const HttpRequest& req)
{
    const std::string id = required(req, "id", "Enter a patient ID, for example VR-2026-000001.");
    const HospitalSession& s = hub_.session();
    const Patient& p = s.findPatient(id);   // hash table lookup; throws PatientNotFoundException
    const Doctor* d = s.findDoctor(p.doctorId());
    const Bill* b = s.findBill(p.id());
    const vr_hash_stats st = s.patients().hashStats();

    JsonWriter j;
    j.beginObject()
        .field("id", p.id())
        .field("name", p.name())
        .field("age", p.age())
        .field("phone", p.phone())
        .field("kind", p.isEmergency() ? "emergency" : "opd")
        .field("severity", p.severity())
        .field("esiName", WebText::esiName(p.severity()))
        .field("status", WebText::status(p.status()))
        .field("complaint", p.complaint())
        .field("department", p.department())
        .field("registered", WebText::date(p.admissionDate()))
        .field("bedCode", p.bedCode())
        .field("bed", p.bedLabel())
        .field("discharged", WebText::date(p.dischargeDate()));
    j.key("doctor");
    if (d)
        j.beginObject().field("id", d->id()).field("name", d->name()).field("specialty", d->specialty()).endObject();
    else
        j.null();
    j.key("bill");
    writeBillSummary(j, b);
    j.field("insurance", b && b->insurance() ? b->insurance()->summary() : std::string());
    j.key("visits").beginArray();
    for (const VisitRecord& v : p.visitsNewestFirst()) {   // the list walked from its tail
        j.beginObject()
            .field("date", WebText::date(v.date))
            .field("doctor", std::string(v.doctor))
            .field("diagnosis", std::string(v.diagnosis))
            .field("prescriptions", std::string(v.prescriptions))
            .endObject();
    }
    j.endArray();
    j.key("hashIndex").beginObject()
        .field("entries", static_cast<unsigned long long>(st.entries))
        .field("buckets", static_cast<unsigned long long>(st.buckets))
        .field("load", st.load_factor)
        .field("collisions", static_cast<unsigned long long>(st.collisions))
        .field("resizes", static_cast<unsigned long long>(st.resizes))
        .endObject();
    j.endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::bill(const HttpRequest& req)
{
    const std::string id = required(req, "id", "Enter a patient ID.");
    const HospitalSession& s = hub_.session();
    const Patient& p = s.findPatient(id);
    const Bill* b = s.findBill(p.id());
    if (!b)
        return error(404, "Not found: no bill for " + p.id() + ".");
    const double deduction = b->settled() ? b->insuranceDeduction() : b->subtotal() - b->projectedTotal();
    const double total = b->settled() ? b->total() : b->projectedTotal();

    JsonWriter j;
    j.beginObject()
        .field("billId", b->billId())
        .field("patientId", b->patientId())
        .field("opened", WebText::date(b->openedOn()))
        .field("settled", b->settled())
        .field("settledOn", WebText::date(b->settledOn()));
    j.key("lines").beginArray();
    for (std::size_t i = 0; i < b->chargeCount(); ++i) {
        const Charge& c = b->charge(i);   // polymorphic: priced by calculateCost()
        j.beginObject()
            .field("date", WebText::date(c.date()))
            .field("category", WebText::sentenceCase(c.category()))
            .field("description", c.description())
            .field("detail", c.detail())
            .field("amount", c.calculateCost())
            .field("amountText", WebText::money(c.calculateCost()))
            .endObject();
    }
    j.endArray()
        .field("subtotal", b->subtotal())
        .field("subtotalText", WebText::money(b->subtotal()))
        .field("insurance", b->insurance() ? b->insurance()->summary() : std::string())
        .field("deduction", deduction)
        .field("deductionText", WebText::money(deduction))
        .field("total", total)
        .field("totalText", WebText::money(total))
        .field("totalLabel", b->settled() ? "Paid" : "Due (estimate until discharge)")
        .endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::bills(const HttpRequest& req)
{
    const bool byAmount = req.param("sort") == "amount";
    JsonWriter j;
    j.beginObject().field("sort", byAmount ? "amount" : "date").key("bills").beginArray();
    for (const Bill* b : hub_.session().listBills(byAmount)) {   // merge-sorted
        j.beginObject()
            .field("billId", b->billId())
            .field("patientId", b->patientId())
            .field("opened", WebText::date(b->openedOn()))
            .field("total", b->total())
            .field("totalText", WebText::money(b->total()))
            .field("settled", b->settled())
            .endObject();
    }
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::route(const HttpRequest& req)
{
    const std::string id = required(req, "id", "Enter a patient ID.");
    const HospitalSession& s = hub_.session();
    const Patient& p = s.findPatient(id);
    if (p.bedCode().empty())
        return error(409, p.id() + " has no bed assigned.");
    const Route r = s.routeForPatient(p.id());   // Dijkstra path + Navigator steps
    const RouteSteps steps = mapRouteSteps(s.navigator(), r);
    const HospitalMap& map = s.map();

    JsonWriter j;
    j.beginObject()
        .field("patientId", p.id())
        .field("origin", r.origin)
        .field("destination", r.destination)
        .field("distance", r.distance)
        .field("seconds", r.seconds)
        .field("summary", WebText::routeSummary(r.distance, r.seconds));
    j.key("steps").beginArray();
    for (const std::string& step : r.steps)
        j.value(step);
    j.endArray().key("path").beginArray();
    for (int v : r.path)
        j.value(v);
    j.endArray().key("stepEdges").beginArray();
    for (const std::vector<int>& edges : steps.stepEdges) {
        j.beginArray();
        for (int k : edges)
            j.value(k);
        j.endArray();
    }
    j.endArray().key("stepVertex").beginArray();
    for (int v : steps.stepVertex)
        j.value(v);
    j.endArray().key("stepFloor").beginArray();
    for (int i = 0; i < static_cast<int>(r.steps.size()); ++i)
        j.value(steps.floorOfStep(map, r, i));
    j.endArray().key("floors").beginArray();
    std::set<int> floors;
    for (int v : r.path)
        floors.insert(map.node(v).floor);
    for (int f : floors)
        j.value(f);
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::map(const HttpRequest&)
{
    const HospitalSession& s = hub_.session();
    const HospitalMap& map = s.map();
    const std::vector<int> ids = nodeIds(map);
    double minX = 0, maxX = 0, minY = 0, maxY = 0;
    std::set<int> floors;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const vr_node& n = map.node(ids[i]);
        minX = i == 0 ? n.x : std::min(minX, n.x);
        maxX = i == 0 ? n.x : std::max(maxX, n.x);
        minY = i == 0 ? n.y : std::min(minY, n.y);
        maxY = i == 0 ? n.y : std::max(maxY, n.y);
        floors.insert(n.floor);
    }

    JsonWriter j;
    j.beginObject().key("bounds").beginObject()
        .field("minX", minX).field("maxX", maxX).field("minY", minY).field("maxY", maxY)
        .endObject();
    j.key("floors").beginArray();
    for (int f : floors)
        j.value(f);
    j.endArray().key("nodes").beginArray();
    for (int id : ids) {
        const vr_node& n = map.node(id);
        j.beginObject()
            .field("id", n.id)
            .field("floor", n.floor)
            .field("x", n.x)
            .field("y", n.y)
            .field("type", vr_node_type_name(n.type))
            .field("name", std::string(n.name))
            .field("label", nodeLabel(n))
            .endObject();
    }
    j.endArray().key("edges").beginArray();
    for (std::size_t a = 0; a < ids.size(); ++a)
        for (std::size_t b = a + 1; b < ids.size(); ++b) {
            const vr_edge* e = map.edge(ids[a], ids[b]);
            if (!e)
                e = map.edge(ids[b], ids[a]);
            if (!e)
                continue;
            j.beginObject()
                .field("from", ids[a])
                .field("to", ids[b])
                .field("type", vr_edge_type_name(e->type))
                .endObject();
        }
    j.endArray().key("beds").beginArray();
    for (const Bed& b : s.router().beds()) {
        j.beginObject()
            .field("code", b.code)
            .field("node", b.node)
            .field("number", b.number)
            .field("unit", Patient::unitName(b.unit))
            .field("free", b.occupant.empty())
            .endObject();
    }
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::theatres(const HttpRequest&)
{
    const HospitalSession& s = hub_.session();
    JsonWriter j;
    j.beginObject().key("theatres").beginArray();
    for (const TheatreDayInfo& day : readTheatreSchedule(s.theatres())) {
        j.beginObject()
            .field("name", day.name)
            .field("freeCount", day.freeCount)
            .field("total", static_cast<int>(day.slots.size()));
        j.key("ring").beginArray();
        for (int n : day.ringOrder)
            j.value(n);
        j.endArray().key("slots").beginArray();
        for (std::size_t i = 0; i < day.slots.size(); ++i) {
            const TheatreSlotInfo& slot = day.slots[i];
            const Patient* p = slot.booked ? s.patients().tryFind(slot.patientId) : nullptr;
            j.beginObject()
                .field("slot", static_cast<int>(i) + 1)
                .field("time", slot.time)
                .field("booked", slot.booked)
                .field("patientId", slot.patientId)
                .field("patientName", p ? p->name() : std::string())
                .field("procedure", slot.procedure)
                .endObject();
        }
        j.endArray().endObject();
    }
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::transfers(const HttpRequest&)
{
    JsonWriter j;
    j.beginObject().key("pending").beginArray();
    int position = 1;
    for (const TransferRequest& r : hub_.session().transfers().pending()) {   // deque, front first
        j.beginObject()
            .field("position", position++)
            .field("code", std::string(r.code))
            .field("urgent", r.urgent != 0)
            .field("priority", r.urgent ? "Urgent" : "Routine")
            .field("type", r.ambulance ? "Ambulance" : "Internal transfer")
            .field("patientId", std::string(r.patientId))
            .field("from", std::string(r.from))
            .field("to", std::string(r.to))
            .endObject();
    }
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::doctors(const HttpRequest&)
{
    const HospitalSession& s = hub_.session();
    const int now = vrclock::minutesNow();
    JsonWriter j;
    j.beginObject().key("specialties").beginArray();
    for (const std::string& sp : s.specialties())
        j.value(sp);
    j.endArray().key("opdDepartments").beginArray();
    for (const std::string& d : s.opdDepartments())
        j.value(d);
    j.endArray().key("availability").beginArray();
    for (const Doctor* d : s.staff().availability()) {   // array order: specialty, then next free
        const bool free = d->nextFree() <= now;
        j.beginObject()
            .field("id", d->id())
            .field("name", d->name())
            .field("specialty", d->specialty())
            .field("free", free)
            .field("status", free ? "Free now" : "Busy until " + WebText::clock(d->nextFree()))
            .field("seen", d->patientsSeen())
            .field("fee", d->consultationFee())
            .endObject();
    }
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::nextDoctor(const HttpRequest& req)
{
    const std::string spec = required(req, "specialty", "Choose a specialty.");
    const HospitalSession& s = hub_.session();
    const int now = vrclock::minutesNow();
    const Doctor* d = s.nextAvailableDoctor(spec);   // lower-bound binary search
    std::string text;
    if (d) {
        // The console's option 6 sentence.
        text = "Next available in " + spec + ": " + d->name() + " - ";
        if (d->nextFree() <= now)
            text += "free now";
        else
            text += "free at " + WebText::clock(d->nextFree()) + " (in " + std::to_string(d->nextFree() - now) + " min)";
    } else {
        text = "Nobody on the roster has the specialty " + spec + ".";
    }
    JsonWriter j;
    j.beginObject().field("text", text).field("doctorId", d ? d->id() : std::string()).endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::staff(const HttpRequest&)
{
    const StaffRegistry& reg = hub_.session().staff();
    JsonWriter j;
    j.beginObject().key("staff").beginArray();
    auto add = [&j](const Staff* st, const std::string& detail, bool authorises) {
        j.beginObject()
            .field("id", st->id())
            .field("name", st->name())
            .field("role", st->role())
            .field("department", st->department())
            .field("shift", st->shift())
            .field("detail", detail)
            .field("canAuthoriseCharges", authorises)
            .endObject();
    };
    for (const Doctor* d : reg.doctors())
        add(d, d->specialty() + ", consultation " + WebText::money(d->consultationFee()), d->canAuthorizeCharges());
    for (const Nurse* n : reg.nurses())
        add(n, "Ward " + n->ward(), n->canAuthorizeCharges());
    for (const Admin* a : reg.admins())
        add(a, a->office() + "; may authorise manual charges", a->canAuthorizeCharges());
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::admissions(const HttpRequest& req)
{
    const int today = vrclock::today();
    int from = vrclock::addDays(today, -30);   // the console's defaults
    int to = today;
    for (auto [name, target] : {std::pair<const char*, int*>{"from", &from}, {"to", &to}}) {
        const std::string text = req.param(name);
        if (text.empty())
            continue;
        const int d = vrclock::parseDate(text);
        if (!d)
            throw std::invalid_argument("Dates look like 2026-09-30 or 20260930.");
        *target = d;
    }
    if (from > to)
        std::swap(from, to);   // as the console does
    const HospitalSession& s = hub_.session();
    const std::vector<Patient*> found = s.admittedBetween(from, to);   // AVL range walk
    const vr_avl& idx = s.patients().dateIndex();

    JsonWriter j;
    j.beginObject()
        .field("from", WebText::date(from))
        .field("to", WebText::date(to))
        .field("summary", std::to_string(found.size()) + " patient(s) registered " + WebText::date(from) + " to " +
                              WebText::date(to) + ".")
        .field("avl", "AVL index: " + std::to_string(idx.nodes) + " dates, height " +
                          std::to_string(vr_avl_height(&idx)) + ", rotations LL " + std::to_string(idx.rot_ll) +
                          " RR " + std::to_string(idx.rot_rr) + " LR " + std::to_string(idx.rot_lr) + " RL " +
                          std::to_string(idx.rot_rl) + ".");
    j.key("patients").beginArray();
    for (const Patient* p : found) {
        j.beginObject()
            .field("registered", WebText::date(p->admissionDate()))
            .field("id", p->id())
            .field("name", p->name())
            .field("kind", p->isEmergency() ? "Emergency" : "Outpatient")
            .field("status", WebText::status(p->status()))
            .endObject();
    }
    j.endArray().endObject();
    return HttpResponse::json(j.str());
}

HttpResponse Api::indexes(const HttpRequest&)
{
    const PatientRegistry& reg = hub_.session().patients();
    const vr_hash_stats st = reg.hashStats();
    const vr_avl& idx = reg.dateIndex();
    JsonWriter j;
    j.beginObject().key("hash").beginObject()
        .field("entries", static_cast<unsigned long long>(st.entries))
        .field("buckets", static_cast<unsigned long long>(st.buckets))
        .field("usedBuckets", static_cast<unsigned long long>(st.used_buckets))
        .field("load", st.load_factor)
        .field("longestChain", static_cast<unsigned long long>(st.longest_chain))
        .field("collisions", static_cast<unsigned long long>(st.collisions))
        .field("resizes", static_cast<unsigned long long>(st.resizes))
        .endObject();
    j.key("avl").beginObject()
        .field("nodes", static_cast<unsigned long long>(idx.nodes))
        .field("values", static_cast<unsigned long long>(idx.values))
        .field("height", vr_avl_height(&idx))
        .field("ll", static_cast<unsigned long long>(idx.rot_ll))
        .field("rr", static_cast<unsigned long long>(idx.rot_rr))
        .field("lr", static_cast<unsigned long long>(idx.rot_lr))
        .field("rl", static_cast<unsigned long long>(idx.rot_rl))
        .field("balanced", vr_avl_validate(&idx) != 0)
        .endObject();
    j.endObject();
    return HttpResponse::json(j.str());
}
