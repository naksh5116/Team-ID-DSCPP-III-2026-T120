// TransferDesk.hpp - ambulance and inter-department transfer requests.
//
// Requests live in the C deque (vr_deque): urgent ones are pushed at the
// front, routine ones at the back, and the desk always dispatches from the
// front. The newest routine request can be withdrawn from the back.
#ifndef VR_TRANSFER_DESK_HPP
#define VR_TRANSFER_DESK_HPP

#include "vr_deque.h"

#include <cstddef>
#include <string>
#include <vector>

// Plain struct so the deque can copy it by value.
struct TransferRequest {
    char code[12];          // TR-0001
    char patientId[24];     // may be empty for an external pick-up
    char from[48];
    char to[48];
    int  urgent;            // 1 = urgent, 0 = routine
    int  ambulance;         // 1 = ambulance run, 0 = internal transfer
};

class TransferDesk {
public:
    TransferDesk();
    ~TransferDesk();
    TransferDesk(const TransferDesk&) = delete;
    TransferDesk& operator=(const TransferDesk&) = delete;

    TransferRequest request(bool urgent, bool ambulance, const std::string& patientId,
                            const std::string& from, const std::string& to);
    bool dispatchNext(TransferRequest& out);       // pop_front
    bool cancelLastRoutine(TransferRequest& out);  // pop_back, routine only
    std::vector<TransferRequest> pending() const;  // front to back
    std::size_t size() const;

    static std::string describe(const TransferRequest& r);

private:
    vr_deque queue_;
    int nextCode_ = 1;
};

#endif // VR_TRANSFER_DESK_HPP
