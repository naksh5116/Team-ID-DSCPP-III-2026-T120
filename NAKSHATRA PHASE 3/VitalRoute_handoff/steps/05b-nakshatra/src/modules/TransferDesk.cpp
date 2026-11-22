// TransferDesk.cpp
#include "TransferDesk.hpp"

#include <cstdio>
#include <new>

TransferDesk::TransferDesk()
{
    if (vr_deque_init(&queue_, sizeof(TransferRequest), 8) != 0)
        throw std::bad_alloc();
}

TransferDesk::~TransferDesk()
{
    vr_deque_free(&queue_);
}

TransferRequest TransferDesk::request(bool urgent, bool ambulance, const std::string& patientId,
                                      const std::string& from, const std::string& to)
{
    TransferRequest r{};
    std::snprintf(r.code, sizeof r.code, "TR-%04d", nextCode_++);
    std::snprintf(r.patientId, sizeof r.patientId, "%s", patientId.c_str());
    std::snprintf(r.from, sizeof r.from, "%s", from.c_str());
    std::snprintf(r.to, sizeof r.to, "%s", to.c_str());
    r.urgent = urgent ? 1 : 0;
    r.ambulance = ambulance ? 1 : 0;

    const int rc = urgent ? vr_deque_push_front(&queue_, &r) : vr_deque_push_back(&queue_, &r);
    if (rc != 0)
        throw std::bad_alloc();
    return r;
}

bool TransferDesk::dispatchNext(TransferRequest& out)
{
    return vr_deque_pop_front(&queue_, &out) == 0;
}

bool TransferDesk::cancelLastRoutine(TransferRequest& out)
{
    TransferRequest back;
    if (vr_deque_back(&queue_, &back) != 0 || back.urgent)
        return false;
    return vr_deque_pop_back(&queue_, &out) == 0;
}

std::vector<TransferRequest> TransferDesk::pending() const
{
    std::vector<TransferRequest> out;
    for (std::size_t i = 0; i < vr_deque_size(&queue_); ++i)
        out.push_back(*static_cast<const TransferRequest*>(vr_deque_at(&queue_, i)));
    return out;
}

std::size_t TransferDesk::size() const
{
    return vr_deque_size(&queue_);
}

std::string TransferDesk::describe(const TransferRequest& r)
{
    std::string s = std::string(r.code) + "  " + (r.urgent ? "URGENT " : "routine") + "  " +
                    (r.ambulance ? "ambulance" : "transfer ") + "  " + r.from + " -> " + r.to;
    if (r.patientId[0])
        s += "  (" + std::string(r.patientId) + ")";
    return s;
}
