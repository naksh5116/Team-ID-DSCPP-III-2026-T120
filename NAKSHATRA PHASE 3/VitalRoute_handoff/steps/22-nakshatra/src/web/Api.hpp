// Api.hpp - the JSON API of VitalRoute Web.
//
// Read calls (GET) return what a page shows, taken from the session in the
// structures' own order (heap order, FIFO order, deque order, merge-sorted
// bills, AVL range walk). Errors use the console's sentences, e.g.
//   404 {"error":"Not found: no patient with ID VR-2026-999999."}
//
//   GET /api/ping                       server is up
//   GET /api/state                      status bar and next patient
//   GET /api/queues                     emergency (heap order), OPD (FIFO)
//   GET /api/patient?id=                record, visits newest first, hash figures
//   GET /api/bill?id=                   one bill with its charge lines
//   GET /api/bills?sort=date|amount     every bill, merge-sorted
//   GET /api/route?id=                  walking route to the patient's bed
//   GET /api/map                        floor plan: landmarks, corridors, beds
//   GET /api/theatres                   theatre slots and free-slot rings
//   GET /api/transfers                  transfer desk, front to back
//   GET /api/doctors                    availability array, specialties
//   GET /api/next-doctor?specialty=     the console's "Next available in ..." sentence
//   GET /api/staff                      staff roster
//   GET /api/admissions?from=&to=       registrations in a date range (AVL)
//   GET /api/indexes                    hash table and AVL tree figures
//
// Actions (POST, form-encoded; see ApiActions.cpp):
//   /api/register-emergency  /api/register-opd  /api/treat-next
//   /api/visit-note  /api/theatre/book  /api/theatre/release
//   /api/transfer  /api/transfer/dispatch  /api/transfer/cancel
//   /api/charge  /api/insurance  /api/discharge  /api/undo  /api/save  /api/reload
//
// Request guard: an action must carry the header "X-VitalRoute: 1", and if
// the browser names the page's origin it must be this server. The pages
// send the header; a page on another website cannot (browsers do not let it
// add headers to a request for a different site without the server's
// permission, which this server never gives). That stops another website
// from changing the hospital through the browser of someone on the network.
#ifndef VR_WEB_API_HPP
#define VR_WEB_API_HPP

#include "HttpServer.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

class WebHub;

class Api {
public:
    explicit Api(WebHub& hub);

    HttpResponse handle(const HttpRequest& req);

    // Sets the number of open event streams reported by /api/ping.
    void setEventClientCounter(std::function<std::size_t()> counter) { eventClients_ = std::move(counter); }
    // Sets the addresses other devices can open, reported by /api/ping
    // (empty when the server runs with --local-only).
    void setNetworkLinks(std::vector<std::string> links) { links_ = std::move(links); }

private:
    using Call = std::function<HttpResponse(const HttpRequest&)>;

    HttpResponse ping(const HttpRequest& req);
    HttpResponse state(const HttpRequest& req);
    HttpResponse queues(const HttpRequest& req);
    HttpResponse patient(const HttpRequest& req);
    HttpResponse bill(const HttpRequest& req);
    HttpResponse bills(const HttpRequest& req);
    HttpResponse route(const HttpRequest& req);
    HttpResponse map(const HttpRequest& req);
    HttpResponse theatres(const HttpRequest& req);
    HttpResponse transfers(const HttpRequest& req);
    HttpResponse doctors(const HttpRequest& req);
    HttpResponse nextDoctor(const HttpRequest& req);
    HttpResponse staff(const HttpRequest& req);
    HttpResponse admissions(const HttpRequest& req);
    HttpResponse indexes(const HttpRequest& req);

    void registerActions();
    HttpResponse registerEmergency(const HttpRequest& req);
    HttpResponse registerOpd(const HttpRequest& req);
    HttpResponse treatNext(const HttpRequest& req);
    HttpResponse visitNote(const HttpRequest& req);
    HttpResponse bookTheatre(const HttpRequest& req);
    HttpResponse releaseTheatre(const HttpRequest& req);
    HttpResponse addTransfer(const HttpRequest& req);
    HttpResponse dispatchTransfer(const HttpRequest& req);
    HttpResponse cancelTransfer(const HttpRequest& req);
    HttpResponse addCharge(const HttpRequest& req);
    HttpResponse recordInsurance(const HttpRequest& req);
    HttpResponse discharge(const HttpRequest& req);
    HttpResponse undo(const HttpRequest& req);
    HttpResponse save(const HttpRequest& req);
    HttpResponse reload(const HttpRequest& req);

    WebHub& hub_;
    std::map<std::string, Call> gets_;
    std::map<std::string, Call> posts_;
    std::function<std::size_t()> eventClients_;
    std::vector<std::string> links_;
};

#endif // VR_WEB_API_HPP
