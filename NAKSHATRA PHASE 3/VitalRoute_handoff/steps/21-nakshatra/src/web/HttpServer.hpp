// HttpServer.hpp - a small HTTP/1.1 server on the operating system's sockets.
//
// Written for VitalRoute Web without any third-party library: Winsock on
// Windows, BSD sockets on Linux. One thread runs one select() loop over the
// listening socket and every client connection, so requests are handled one
// at a time - exactly like the Qt GUI's single event loop - and the shared
// HospitalSession needs no locks.
//
// What it does:
//   - GET/POST requests with keep-alive; query strings and
//     application/x-www-form-urlencoded bodies are decoded into params
//   - static files from one folder (index.html for "/"), never outside it
//   - Server-Sent Events: GET on the event path keeps the connection open,
//     and broadcast() pushes "event: name / data: ..." to every listener
//   - a comment line to every event listener every 15 s keeps idle
//     connections open through Wi-Fi power saving and proxies
#ifndef VR_WEB_HTTP_SERVER_HPP
#define VR_WEB_HTTP_SERVER_HPP

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct HttpRequest {
    std::string method;                         // GET, POST, ...
    std::string path;                           // decoded, without the query
    std::map<std::string, std::string> headers; // names in lower case
    std::map<std::string, std::string> params;  // query string + form body
    std::string body;
    std::string peer;                           // client address

    // Parameter value, or `fallback` when absent.
    std::string param(const std::string& name, const std::string& fallback = "") const;
    bool has(const std::string& name) const { return params.count(name) != 0; }
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;

    static HttpResponse json(const std::string& body, int status = 200);
    static HttpResponse text(const std::string& body, int status = 200);
};

class HttpServer {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    HttpServer();
    ~HttpServer();
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    // Binds and listens. `address` is "0.0.0.0" (every network) or
    // "127.0.0.1" (this computer only). Throws std::runtime_error with the
    // operating system's reason (port in use, no permission, ...).
    void listen(const std::string& address, unsigned short port);

    // Requests whose path starts with `prefix` go to the handler.
    void setApi(const std::string& prefix, Handler handler);
    // Other GET requests are served from this folder.
    void setStaticRoot(const std::string& folder);
    // GET on this path opens a Server-Sent Events stream.
    void setEventPath(const std::string& path);

    // Sends one event to every open event stream.
    void broadcast(const std::string& event, const std::string& data);
    std::size_t eventClients() const;

    // Runs the loop until stop() (or Ctrl+C, see requestStop()).
    void run();
    void stop();
    // Safe to call from a signal handler: the loop ends within a second.
    static void requestStop();

    // IPv4 addresses of this computer's network interfaces (not 127.0.0.1),
    // for the "open this link on your phone" message.
    static std::vector<std::string> localAddresses();

    // Content type for a file name ("index.html" -> "text/html; ...").
    static std::string contentTypeFor(const std::string& path);
    // Percent-decoding (and '+' to space for form values).
    static std::string urlDecode(const std::string& text, bool plusIsSpace);

private:
    struct Client;
    void acceptClients();
    void readFrom(Client& c);
    void writeTo(Client& c);
    void handleRequests(Client& c);
    void serve(Client& c, HttpRequest& req, bool keepAlive);
    HttpResponse serveFile(const HttpRequest& req) const;
    void queue(Client& c, const HttpResponse& res, bool keepAlive);
    void keepEventStreamsAlive();
    void closeClient(Client& c);

    struct Impl;
    std::unique_ptr<Impl> d_;
};

#endif // VR_WEB_HTTP_SERVER_HPP
