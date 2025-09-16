// Minimal single-header HTTP/1.0 server for local development (Windows Winsock).
// NOTE: This is not production-grade. It only supports basic GET/POST with
// Content-Length and exact path matching, single request per connection.

#pragma once

#include <string>
#include <functional>
#include <map>
#include <thread>
#include <atomic>
#include <vector>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
#endif

namespace minihttplib {

class Response {
public:
    int status = 200;
    std::string body;
    std::map<std::string, std::string> headers;
};

class Request {
public:
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
};

class Server {
public:
    using Handler = std::function<void(const Request&, Response&)>;

    void Post(const char* path, Handler h) { post_handlers_[path] = std::move(h); }
    void Get(const char* path, Handler h) { get_handlers_[path] = std::move(h); }

    bool listen(const char* host, int port) {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

        SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSock == INVALID_SOCKET) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(port));
        if (!host || std::strcmp(host, "0.0.0.0") == 0) {
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
        } else {
            inet_pton(AF_INET, host, &addr.sin_addr);
        }

        int opt = 1;
        setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        if (bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(listenSock); WSACleanup(); return false;
        }
        if (::listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(listenSock); WSACleanup(); return false;
        }

        running_ = true;
        while (running_) {
            SOCKET client = ::accept(listenSock, nullptr, nullptr);
            if (client == INVALID_SOCKET) break;
            handle_client(client);
            closesocket(client);
        }
        closesocket(listenSock);
        WSACleanup();
        return true;
#else
        (void)host; (void)port; return false;
#endif
    }

    void stop() { running_ = false; }

private:
    void handle_client(SOCKET client) {
        // Read request (very naive, up to 64KB)
        std::vector<char> buf(65536);
        int recvd = recv(client, buf.data(), (int)buf.size(), 0);
        if (recvd <= 0) return;
        std::string req_str(buf.data(), recvd);

        Request req; Response res;
        parse_request(req_str, req);
        dispatch(req, res);
        std::string resp = build_response(res);
        send(client, resp.data(), (int)resp.size(), 0);
    }

    static void parse_request(const std::string& s, Request& req) {
        std::istringstream iss(s);
        std::string line;
        // request line
        if (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::istringstream ls(line);
            ls >> req.method >> req.path; // ignore HTTP version
        }
        // headers
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) break;
            auto p = line.find(':');
            if (p != std::string::npos) {
                std::string k = line.substr(0, p);
                std::string v = line.substr(p + 1);
                // trim leading spaces
                size_t i = 0; while (i < v.size() && (v[i] == ' ' || v[i] == '\t')) ++i; v = v.substr(i);
                req.headers[k] = v;
            }
        }
        // body by Content-Length
        size_t content_len = 0;
        if (auto it = req.headers.find("Content-Length"); it != req.headers.end()) {
            content_len = (size_t)std::stoul(it->second);
        }
        if (content_len > 0) {
            std::string remaining;
            remaining.assign(std::istreambuf_iterator<char>(iss), std::istreambuf_iterator<char>());
            if (remaining.size() >= content_len) req.body = remaining.substr(0, content_len);
            else req.body = remaining;
        }
    }

    void dispatch(const Request& req, Response& res) {
        if (req.method == "GET") {
            auto it = get_handlers_.find(req.path);
            if (it != get_handlers_.end()) { it->second(req, res); return; }
        } else if (req.method == "POST") {
            auto it = post_handlers_.find(req.path);
            if (it != post_handlers_.end()) { it->second(req, res); return; }
        }
        res.status = 404; res.body = "Not Found";
    }

    static std::string build_response(const Response& res) {
        std::ostringstream oss;
        oss << "HTTP/1.0 " << (res.status == 0 ? 200 : res.status) << "\r\n";
        bool has_ct = false;
        for (auto& kv : res.headers) {
            if (_stricmp(kv.first.c_str(), "Content-Type") == 0) has_ct = true;
            oss << kv.first << ": " << kv.second << "\r\n";
        }
        if (!has_ct) oss << "Content-Type: text/plain; charset=utf-8\r\n";
        oss << "Content-Length: " << res.body.size() << "\r\n";
        oss << "Connection: close\r\n\r\n";
        oss << res.body;
        return oss.str();
    }

    std::map<std::string, Handler> post_handlers_;
    std::map<std::string, Handler> get_handlers_;
    std::atomic<bool> running_{false};
};

} // namespace minihttplib



