// main.cpp - VitalRoute Web server (bin/vitalroute_web).
//
//   vitalroute_web [--port 8080] [--local-only] [--web DIR] [DATA_DIR]
//
// One process holds the hospital session; every browser that opens the
// printed link - on this computer or on another device on the same network -
// works on that same session and is told about every change at once.
//
//   --port N       listen on port N (default 8080)
//   --local-only   accept connections from this computer only (127.0.0.1)
//   --web DIR      folder with the browser pages (default: web/ next to data/)
//   DATA_DIR       hospital data folder (default: data, as the console)
#include "Api.hpp"
#include "HttpServer.hpp"
#include "WebHub.hpp"

#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool fileExists(const std::string& path)
{
    return static_cast<bool>(std::ifstream(path));
}

std::string findDataDir(const std::string& given)
{
    if (!given.empty())
        return given;
    for (const char* dir : {"data", "../data"})
        if (fileExists(std::string(dir) + "/hospital_graph.txt"))
            return dir;
    return "data";
}

std::string findWebDir(const std::string& given, const std::string& dataDir)
{
    if (!given.empty())
        return given;
    for (const std::string& dir : {dataDir + "/../web", std::string("web"), std::string("../web")})
        if (fileExists(dir + "/index.html"))
            return dir;
    return "web";
}

void onInterrupt(int)
{
    HttpServer::requestStop();   // the loop ends within a second
}

} // namespace

int main(int argc, char** argv)
{
    unsigned short port = 8080;
    bool localOnly = false;
    std::string webDir, dataArg;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--port" && i + 1 < argc) {
            const long p = std::strtol(argv[++i], nullptr, 10);
            if (p < 1 || p > 65535) {
                std::cerr << "Port must be 1-65535.\n";
                return 2;
            }
            port = static_cast<unsigned short>(p);
        } else if (a == "--local-only") {
            localOnly = true;
        } else if (a == "--web" && i + 1 < argc) {
            webDir = argv[++i];
        } else if (a == "--help" || a == "-h") {
            std::cout << "Usage: vitalroute_web [--port 8080] [--local-only] [--web DIR] [DATA_DIR]\n";
            return 0;
        } else {
            dataArg = a;
        }
    }
    const std::string dataDir = findDataDir(dataArg);
    webDir = findWebDir(webDir, dataDir);

    try {
        // The hospital: one session for every browser (as the console's startUp).
        WebHub hub(dataDir);
        const HospitalSession::StartupReport report = hub.start();
        for (const std::string& w : report.warnings)
            std::cout << "Warning: " << w << '\n';
        if (report.loadedFromRecords)
            std::cout << "Loaded " << report.loaded.patients << " patients, " << report.loaded.staff << " staff, "
                      << report.loaded.visits << " visits and " << report.loaded.bills << " bills from "
                      << hub.session().recordsDir() << "/\n";
        else
            std::cout << "First run: sample patients and staff created (nothing saved yet).\n";

        HttpServer server;
        Api api(hub);
        api.setEventClientCounter([&server] { return server.eventClients(); });
        hub.setNotifier([&server](const std::string& event, const std::string& data) {
            server.broadcast(event, data);   // every open browser hears about the change
        });
        server.setStaticRoot(webDir);
        server.setEventPath("/api/events");
        server.setApi("/api/", [&api](const HttpRequest& req) { return api.handle(req); });
        server.listen(localOnly ? "127.0.0.1" : "0.0.0.0", port);

        std::cout << "VitalRoute Web is running.\n"
                  << "  On this computer:     http://localhost:" << port << "/\n";
        if (localOnly) {
            std::cout << "  Other devices:        not allowed (--local-only)\n";
        } else {
            const std::vector<std::string> ips = HttpServer::localAddresses();
            if (ips.empty())
                std::cout << "  Other devices:        no network connection found\n";
            std::vector<std::string> links;
            for (const std::string& ip : ips) {
                links.push_back("http://" + ip + ":" + std::to_string(port) + "/");
                std::cout << "  On the same network:  " << links.back() << "\n";
            }
            api.setNetworkLinks(links);
            std::cout << "  There is no login: anyone on this network who opens the link can change\n"
                         "  the data. Use a trusted network, or start with --local-only.\n";
        }
        std::cout << "  Data folder: " << dataDir << "   Pages: " << webDir << "\n"
                  << "Press Ctrl+C to stop.\n"
                  << std::flush;

        std::signal(SIGINT, onInterrupt);
        std::signal(SIGTERM, onInterrupt);
        server.run();
        std::cout << "Stopped.\n";
    } catch (const std::exception& e) {
        std::cerr << "VitalRoute Web could not start: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
