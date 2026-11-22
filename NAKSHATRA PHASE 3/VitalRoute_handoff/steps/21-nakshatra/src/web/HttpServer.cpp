// HttpServer.cpp - sockets, one select() loop, HTTP parsing, static files, SSE.

// ---- platform sockets (before any other include, for FD_SETSIZE) --------
#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  define FD_SETSIZE 512   // Windows' default of 64 connections is too few for a class demo
#  include <winsock2.h>
#  include <ws2tcpip.h>
using sock_t = SOCKET;
static const sock_t kNoSocket = INVALID_SOCKET;
#else
#  include <arpa/inet.h>
#  include <cerrno>
#  include <csignal>
#  include <cstring>
#  include <fcntl.h>
#  include <ifaddrs.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <unistd.h>
using sock_t = int;
static const sock_t kNoSocket = -1;
#endif

#include "HttpServer.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {

std::atomic<bool> g_stopRequested{false};

constexpr std::size_t kMaxHeader = 64 * 1024;       // bytes of request line + headers
constexpr std::size_t kMaxBody = 1024 * 1024;        // bytes of request body
constexpr std::size_t kMaxClients = FD_SETSIZE - 8;  // keep room for the listener
constexpr auto kPingEvery = std::chrono::seconds(15);

// ------------------------------------------------------- portability --

int lastErrorCode()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

std::string errorText(int code)
{
#ifdef _WIN32
    char* buf = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, static_cast<DWORD>(code), 0, reinterpret_cast<LPSTR>(&buf), 0, nullptr);
    std::string s = buf ? buf : "unknown error";
    if (buf)
        LocalFree(buf);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == '.'))
        s.pop_back();
    return s;
#else
    return std::strerror(code);
#endif
}

bool wouldBlock(int code)
{
#ifdef _WIN32
    return code == WSAEWOULDBLOCK;
#else
    return code == EAGAIN || code == EWOULDBLOCK;
#endif
}

bool addressInUse(int code)
{
#ifdef _WIN32
    return code == WSAEADDRINUSE;
#else
    return code == EADDRINUSE;
#endif
}

void closeSocket(sock_t s)
{
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

void setNonBlocking(sock_t s)
{
#ifdef _WIN32
    u_long on = 1;
    ioctlsocket(s, FIONBIO, &on);
#else
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
#endif
}

int sendSome(sock_t s, const char* data, std::size_t len)
{
#ifdef _WIN32
    return send(s, data, static_cast<int>(std::min<std::size_t>(len, 1 << 20)), 0);
#else
    return static_cast<int>(send(s, data, len, MSG_NOSIGNAL));   // no SIGPIPE on a closed browser tab
#endif
}

// ----------------------------------------------------------- strings --

std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trim(const std::string& s)
{
    const auto b = s.find_first_not_of(" \t");
    if (b == std::string::npos)
        return "";
    return s.substr(b, s.find_last_not_of(" \t") - b + 1);
}

void parseForm(const std::string& text, std::map<std::string, std::string>& out)
{
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t amp = text.find('&', start);
        const std::string pair = text.substr(start, amp == std::string::npos ? std::string::npos : amp - start);
        if (!pair.empty()) {
            const std::size_t eq = pair.find('=');
            const std::string key = HttpServer::urlDecode(pair.substr(0, eq), true);
            const std::string value = eq == std::string::npos ? "" : HttpServer::urlDecode(pair.substr(eq + 1), true);
            if (!key.empty())
                out[key] = value;
        }
        if (amp == std::string::npos)
            break;
        start = amp + 1;
    }
}

const char* statusText(int status)
{
    switch (status) {
    case 200: return "OK";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 409: return "Conflict";
    case 413: return "Payload Too Large";
    case 431: return "Request Header Fields Too Large";
    case 500: return "Internal Server Error";
    case 503: return "Service Unavailable";
    default: return "Unknown";
    }
}

std::string jsonString(const std::string& s)
{
    std::string out = "\"";
    for (char ch : s) {
        switch (ch) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof buf, "\\u%04x", ch);
                out += buf;
            } else {
                out += ch;
            }
        }
    }
    return out + "\"";
}

HttpResponse errorResponse(int status, const std::string& message)
{
    return HttpResponse::json("{\"error\":" + jsonString(message) + "}", status);
}

} // namespace

// ------------------------------------------------------- request/response --

std::string HttpRequest::param(const std::string& name, const std::string& fallback) const
{
    const auto it = params.find(name);
    return it == params.end() ? fallback : it->second;
}

HttpResponse HttpResponse::json(const std::string& body, int status)
{
    HttpResponse r;
    r.status = status;
    r.body = body;
    return r;
}

HttpResponse HttpResponse::text(const std::string& body, int status)
{
    HttpResponse r;
    r.status = status;
    r.contentType = "text/plain; charset=utf-8";
    r.body = body;
    return r;
}

// ---------------------------------------------------------------- server --

struct HttpServer::Client {
    sock_t fd = kNoSocket;
    std::string peer;
    std::string in;
    std::string out;
    bool events = false;          // an open Server-Sent Events stream
    bool closeAfterWrite = false;
    // After a refused request: our side is shut, and what the browser is
    // still sending is read and dropped until it closes (or the deadline),
    // so it receives our error page instead of a "connection reset".
    bool draining = false;
    std::chrono::steady_clock::time_point drainUntil;
};

struct HttpServer::Impl {
    sock_t listener = kNoSocket;
    std::vector<std::unique_ptr<Client>> clients;
    std::vector<std::pair<std::string, Handler>> apis;
    std::string staticRoot;
    std::string eventPath = "/api/events";
    bool running = false;
    std::chrono::steady_clock::time_point lastPing = std::chrono::steady_clock::now();
#ifdef _WIN32
    bool winsockReady = false;
#endif
};

HttpServer::HttpServer() : d_(std::make_unique<Impl>())
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        throw std::runtime_error("could not start Windows networking (WSAStartup)");
    d_->winsockReady = true;
#else
    std::signal(SIGPIPE, SIG_IGN);
#endif
}

HttpServer::~HttpServer()
{
    for (auto& c : d_->clients)
        if (c->fd != kNoSocket)
            closeSocket(c->fd);
    if (d_->listener != kNoSocket)
        closeSocket(d_->listener);
#ifdef _WIN32
    if (d_->winsockReady)
        WSACleanup();
#endif
}

void HttpServer::listen(const std::string& address, unsigned short port)
{
    sock_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == kNoSocket)
        throw std::runtime_error("could not create a socket: " + errorText(lastErrorCode()));
    int on = 1;
#ifdef _WIN32
    // Exclusive use: no other program can take over the same port.
    setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&on), sizeof on);
#else
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);   // restart at once after a stop
#endif
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
        closeSocket(s);
        throw std::runtime_error("not an IPv4 address: " + address);
    }
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0) {
        const int code = lastErrorCode();
        closeSocket(s);
        if (addressInUse(code))
            throw std::runtime_error("port " + std::to_string(port) +
                                     " is already in use (is VitalRoute Web already running? try --port 8081)");
        throw std::runtime_error("could not use port " + std::to_string(port) + ": " + errorText(code));
    }
    if (::listen(s, SOMAXCONN) != 0) {
        const int code = lastErrorCode();
        closeSocket(s);
        throw std::runtime_error("could not listen: " + errorText(code));
    }
    setNonBlocking(s);
    d_->listener = s;
}

void HttpServer::setApi(const std::string& prefix, Handler handler)
{
    d_->apis.emplace_back(prefix, std::move(handler));
}

void HttpServer::setStaticRoot(const std::string& folder)
{
    d_->staticRoot = folder;
}

void HttpServer::setEventPath(const std::string& path)
{
    d_->eventPath = path;
}

std::size_t HttpServer::eventClients() const
{
    return static_cast<std::size_t>(std::count_if(d_->clients.begin(), d_->clients.end(),
                                                  [](const auto& c) { return c->events && c->fd != kNoSocket; }));
}

void HttpServer::broadcast(const std::string& event, const std::string& data)
{
    std::string msg = "event: " + event + "\n";
    std::istringstream lines(data.empty() ? std::string("{}") : data);
    std::string line;
    while (std::getline(lines, line))
        msg += "data: " + line + "\n";
    msg += "\n";
    for (auto& c : d_->clients)
        if (c->events && c->fd != kNoSocket)
            c->out += msg;
}

void HttpServer::requestStop()
{
    g_stopRequested = true;
}

void HttpServer::stop()
{
    d_->running = false;
}

void HttpServer::run()
{
    if (d_->listener == kNoSocket)
        throw std::logic_error("HttpServer::run() before listen()");
    d_->running = true;
    g_stopRequested = false;
    while (d_->running && !g_stopRequested) {
        fd_set readable, writable;
        FD_ZERO(&readable);
        FD_ZERO(&writable);
        FD_SET(d_->listener, &readable);
        sock_t highest = d_->listener;
        for (const auto& c : d_->clients) {
            FD_SET(c->fd, &readable);
            if (!c->out.empty())
                FD_SET(c->fd, &writable);
            highest = std::max(highest, c->fd);
        }
        timeval wait{1, 0};   // wake every second to notice a stop request
        const int ready = select(static_cast<int>(highest) + 1, &readable, &writable, nullptr, &wait);
        if (ready < 0) {
#ifndef _WIN32
            if (errno == EINTR)
                continue;
#endif
            throw std::runtime_error("network wait failed: " + errorText(lastErrorCode()));
        }
        if (ready > 0) {
            if (FD_ISSET(d_->listener, &readable))
                acceptClients();
            // New clients were appended; they were not in the sets this round.
            for (std::size_t i = 0; i < d_->clients.size(); ++i) {
                Client& c = *d_->clients[i];
                if (c.fd == kNoSocket)
                    continue;
                if (FD_ISSET(c.fd, &readable))
                    readFrom(c);
                if (c.fd != kNoSocket && FD_ISSET(c.fd, &writable))
                    writeTo(c);
            }
        }
        // Answers produced while reading go out on the next round; flush
        // what can be sent now so a reply does not wait a full second.
        for (auto& c : d_->clients)
            if (c->fd != kNoSocket && !c->out.empty())
                writeTo(*c);
        keepEventStreamsAlive();
        const auto now = std::chrono::steady_clock::now();
        for (auto& c : d_->clients)
            if (c->draining && c->fd != kNoSocket && now >= c->drainUntil)
                closeClient(*c);
        d_->clients.erase(std::remove_if(d_->clients.begin(), d_->clients.end(),
                                         [](const auto& c) { return c->fd == kNoSocket; }),
                          d_->clients.end());
    }
}

void HttpServer::acceptClients()
{
    for (;;) {
        sockaddr_storage from{};
        socklen_t len = sizeof from;
        const sock_t s = accept(d_->listener, reinterpret_cast<sockaddr*>(&from), &len);
        if (s == kNoSocket)
            return;   // nothing more waiting (or a transient error)
        if (d_->clients.size() >= kMaxClients) {
            closeSocket(s);   // full: the browser will retry
            continue;
        }
        setNonBlocking(s);
        auto c = std::make_unique<Client>();
        c->fd = s;
        char host[INET6_ADDRSTRLEN] = "?";
        if (from.ss_family == AF_INET)
            inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(&from)->sin_addr, host, sizeof host);
        c->peer = host;
        d_->clients.push_back(std::move(c));
    }
}

void HttpServer::closeClient(Client& c)
{
    if (c.fd != kNoSocket)
        closeSocket(c.fd);
    c.fd = kNoSocket;
}

void HttpServer::readFrom(Client& c)
{
    char buf[16384];
    for (;;) {
        const int n = static_cast<int>(recv(c.fd, buf, sizeof buf, 0));
        if (n == 0) {           // the browser closed the connection
            closeClient(c);
            return;
        }
        if (n < 0) {
            if (!wouldBlock(lastErrorCode()))
                closeClient(c);
            break;
        }
        if (c.draining)
            continue;   // dropped: the answer has been sent
        if (!c.events)
            c.in.append(buf, static_cast<std::size_t>(n));
        if (c.in.size() > kMaxHeader + kMaxBody) {
            queue(c, errorResponse(413, "request too large"), false);
            return;
        }
    }
    if (!c.events && !c.draining && c.fd != kNoSocket)
        handleRequests(c);
}

void HttpServer::handleRequests(Client& c)
{
    while (c.fd != kNoSocket && !c.events && !c.closeAfterWrite) {
        const std::size_t headerEnd = c.in.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            if (c.in.size() > kMaxHeader)
                queue(c, errorResponse(431, "request headers too large"), false);
            return;   // wait for the rest
        }
        if (headerEnd > kMaxHeader) {   // complete, but still too large
            queue(c, errorResponse(431, "request headers too large"), false);
            return;
        }
        std::istringstream head(c.in.substr(0, headerEnd));
        std::string requestLine;
        std::getline(head, requestLine);
        if (!requestLine.empty() && requestLine.back() == '\r')
            requestLine.pop_back();
        std::istringstream rl(requestLine);
        HttpRequest req;
        std::string target, version;
        rl >> req.method >> target >> version;
        if (req.method.empty() || target.empty() || version.rfind("HTTP/1.", 0) != 0) {
            queue(c, errorResponse(400, "not an HTTP request"), false);
            return;
        }
        std::string line;
        while (std::getline(head, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            const std::size_t colon = line.find(':');
            if (colon != std::string::npos)
                req.headers[lower(trim(line.substr(0, colon)))] = trim(line.substr(colon + 1));
        }
        std::size_t length = 0;
        if (req.headers.count("content-length")) {
            try {
                length = static_cast<std::size_t>(std::stoul(req.headers["content-length"]));
            } catch (...) {
                queue(c, errorResponse(400, "bad Content-Length"), false);
                return;
            }
            if (length > kMaxBody) {
                queue(c, errorResponse(413, "request body too large"), false);
                return;
            }
        }
        if (c.in.size() < headerEnd + 4 + length)
            return;   // body not complete yet
        req.body = c.in.substr(headerEnd + 4, length);
        c.in.erase(0, headerEnd + 4 + length);

        const std::size_t q = target.find('?');
        req.path = urlDecode(target.substr(0, q), false);
        if (q != std::string::npos)
            parseForm(target.substr(q + 1), req.params);
        if (lower(req.headers["content-type"]).rfind("application/x-www-form-urlencoded", 0) == 0)
            parseForm(req.body, req.params);   // form fields win over the query string
        req.peer = c.peer;

        const std::string connection = lower(req.headers["connection"]);
        const bool keepAlive = version == "HTTP/1.1" ? connection != "close" : connection == "keep-alive";
        serve(c, req, keepAlive);
    }
}

void HttpServer::serve(Client& c, HttpRequest& req, bool keepAlive)
{
    if (req.path == d_->eventPath && req.method == "GET") {
        // Server-Sent Events: headers now, events later via broadcast().
        c.out += "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/event-stream; charset=utf-8\r\n"
                 "Cache-Control: no-store\r\n"
                 "Connection: keep-alive\r\n"
                 "X-Content-Type-Options: nosniff\r\n"
                 "\r\n"
                 "retry: 2000\n\n"
                 ": connected\n\n";
        c.events = true;
        c.in.clear();
        return;
    }
    for (const auto& api : d_->apis) {
        if (req.path.rfind(api.first, 0) != 0)
            continue;
        HttpResponse res;
        try {
            res = api.second(req);
        } catch (const std::exception& e) {
            res = errorResponse(500, std::string("Error: ") + e.what() + ".");
        }
        queue(c, res, keepAlive);
        return;
    }
    if (req.method == "GET" || req.method == "HEAD") {
        HttpResponse res = serveFile(req);
        if (req.method == "HEAD")
            res.body.clear();
        queue(c, res, keepAlive);
        return;
    }
    queue(c, errorResponse(405, "method not allowed"), keepAlive);
}

HttpResponse HttpServer::serveFile(const HttpRequest& req) const
{
    if (d_->staticRoot.empty())
        return HttpResponse::text("Not found", 404);
    std::string path = req.path;
    if (path.empty() || path.back() == '/')
        path += "index.html";
    // Only plain names inside the web folder: no "..", no drive letters,
    // no backslashes, no hidden files.
    if (path.front() != '/' || path.find("..") != std::string::npos || path.find('\\') != std::string::npos ||
        path.find(':') != std::string::npos || path.find("/.") != std::string::npos)
        return HttpResponse::text("Not found", 404);
    std::ifstream file(d_->staticRoot + path, std::ios::binary);
    if (!file)
        return HttpResponse::text("Not found", 404);
    std::ostringstream content;
    content << file.rdbuf();
    HttpResponse res;
    res.contentType = contentTypeFor(path);
    res.body = content.str();
    res.headers.emplace_back("Cache-Control", "no-cache");
    return res;
}

void HttpServer::queue(Client& c, const HttpResponse& res, bool keepAlive)
{
    std::string head = "HTTP/1.1 " + std::to_string(res.status) + " " + statusText(res.status) + "\r\n";
    head += "Content-Type: " + res.contentType + "\r\n";
    head += "Content-Length: " + std::to_string(res.body.size()) + "\r\n";
    bool cacheSet = false;
    for (const auto& h : res.headers) {
        head += h.first + ": " + h.second + "\r\n";
        cacheSet = cacheSet || lower(h.first) == "cache-control";
    }
    if (!cacheSet)
        head += "Cache-Control: no-store\r\n";   // API answers are always live
    head += "X-Content-Type-Options: nosniff\r\n";
    head += keepAlive ? "Connection: keep-alive\r\n" : "Connection: close\r\n";
    head += "\r\n";
    c.out += head;
    c.out += res.body;
    if (!keepAlive)
        c.closeAfterWrite = true;
}

void HttpServer::writeTo(Client& c)
{
    while (!c.out.empty()) {
        const int n = sendSome(c.fd, c.out.data(), c.out.size());
        if (n <= 0) {
            if (n < 0 && wouldBlock(lastErrorCode()))
                return;   // the socket is full; select() says when to go on
            closeClient(c);
            return;
        }
        c.out.erase(0, static_cast<std::size_t>(n));
    }
    if (c.closeAfterWrite && !c.draining) {
#ifdef _WIN32
        shutdown(c.fd, SD_SEND);
#else
        shutdown(c.fd, SHUT_WR);
#endif
        c.draining = true;
        c.drainUntil = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    }
}

void HttpServer::keepEventStreamsAlive()
{
    const auto now = std::chrono::steady_clock::now();
    if (now - d_->lastPing < kPingEvery)
        return;
    d_->lastPing = now;
    for (auto& c : d_->clients)
        if (c->events && c->fd != kNoSocket)
            c->out += ": ping\n\n";   // an SSE comment: browsers ignore it
}

// ---------------------------------------------------------------- helpers --

std::vector<std::string> HttpServer::localAddresses()
{
    std::set<std::string> found;
#ifdef _WIN32
    char host[256] = {};
    if (gethostname(host, sizeof host) == 0) {
        addrinfo hints{};
        hints.ai_family = AF_INET;
        addrinfo* list = nullptr;
        if (getaddrinfo(host, nullptr, &hints, &list) == 0) {
            for (addrinfo* a = list; a; a = a->ai_next) {
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(a->ai_addr)->sin_addr, ip, sizeof ip);
                found.insert(ip);
            }
            freeaddrinfo(list);
        }
    }
#else
    ifaddrs* list = nullptr;
    if (getifaddrs(&list) == 0) {
        for (ifaddrs* a = list; a; a = a->ifa_next) {
            if (!a->ifa_addr || a->ifa_addr->sa_family != AF_INET)
                continue;
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(a->ifa_addr)->sin_addr, ip, sizeof ip);
            found.insert(ip);
        }
        freeifaddrs(list);
    }
#endif
    std::vector<std::string> out;
    for (const std::string& ip : found)
        if (ip.rfind("127.", 0) != 0 && ip.rfind("169.254.", 0) != 0)   // not loopback or unassigned
            out.push_back(ip);
    return out;
}

std::string HttpServer::contentTypeFor(const std::string& path)
{
    const std::size_t dot = path.rfind('.');
    const std::string ext = dot == std::string::npos ? "" : lower(path.substr(dot + 1));
    if (ext == "html") return "text/html; charset=utf-8";
    if (ext == "css") return "text/css; charset=utf-8";
    if (ext == "js") return "text/javascript; charset=utf-8";
    if (ext == "json") return "application/json; charset=utf-8";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "png") return "image/png";
    if (ext == "ico") return "image/x-icon";
    if (ext == "txt") return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

std::string HttpServer::urlDecode(const std::string& text, bool plusIsSpace)
{
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (ch == '+' && plusIsSpace) {
            out += ' ';
        } else if (ch == '%' && i + 2 < text.size() && std::isxdigit(static_cast<unsigned char>(text[i + 1])) &&
                   std::isxdigit(static_cast<unsigned char>(text[i + 2]))) {
            out += static_cast<char>(std::stoi(text.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else {
            out += ch;
        }
    }
    return out;
}
