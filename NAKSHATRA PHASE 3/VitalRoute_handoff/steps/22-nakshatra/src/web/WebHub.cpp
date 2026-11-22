// WebHub.cpp
#include "WebHub.hpp"

#include "Json.hpp"

#include <stdexcept>
#include <utility>

WebHub::WebHub(std::string dataDir) : dataDir_(std::move(dataDir)) {}

HospitalSession& WebHub::live()
{
    if (!session_)
        throw std::logic_error("the session has not been started");
    return *session_;
}

HospitalSession::StartupReport WebHub::start()
{
    auto fresh = std::make_unique<HospitalSession>(dataDir_);
    const HospitalSession::StartupReport report = fresh->startUp();
    session_ = std::move(fresh);
    return report;
}

HospitalSession::StartupReport WebHub::reload()
{
    auto fresh = std::make_unique<HospitalSession>(dataDir_);
    const HospitalSession::StartupReport report = fresh->startUp();   // throws: old session kept
    session_ = std::move(fresh);
    announce(Everything);
    announceEvent("reloaded");
    return report;
}

long WebHub::announce(unsigned changes, const std::string& routePatient)
{
    ++version_;
    if (!notifier_)
        return version_;
    JsonWriter data;
    data.beginObject().field("version", version_).endObject();
    static const std::pair<Change, const char*> names[] = {
        {Patients, "patientsChanged"}, {Queue, "queueChanged"},       {Beds, "bedsChanged"},
        {Bills, "billsChanged"},       {Theatres, "theatresChanged"}, {Transfers, "transfersChanged"},
        {Staff, "staffChanged"},
    };
    for (const auto& n : names)
        if (changes & n.first)
            notifier_(n.second, data.str());
    if (!routePatient.empty()) {
        JsonWriter route;
        route.beginObject().field("patientId", routePatient).field("version", version_).endObject();
        notifier_("routeAssigned", route.str());
    }
    return version_;
}

void WebHub::announceEvent(const std::string& event)
{
    if (!notifier_)
        return;
    JsonWriter data;
    data.beginObject().field("version", version_).endObject();
    notifier_(event, data.str());
}
