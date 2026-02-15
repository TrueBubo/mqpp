#include "http_transport.hpp"
#include <stdexcept>
#include <sstream>

namespace mqpp {

constexpr auto DEFAULT_HTTP_PORT = 80;
constexpr auto CONTENT_TYPE = "application/json";

constexpr auto CONNECTION_TIMEOUT_SEC = 5;
constexpr auto READ_TIMEOUT_SEC = 10;
constexpr auto SERVER_STARTUP_DELAY = std::chrono::milliseconds(100);

constexpr auto HTTP_STATUS_OK = 200;
constexpr auto HTTP_STATUS_INTERNAL_ERROR = 500;

HttpTransport::HttpTransport()
    : server_(std::make_unique<httplib::Server>())
    , running_(false)
    , port_(0)
{}

HttpTransport::~HttpTransport() {
    HttpTransport::stop();
}

std::string HttpTransport::send_request(const std::string& url,
                                       const std::string& body) {

    size_t protocol_end = url.find("://");
    if (protocol_end == std::string::npos) {
        throw std::runtime_error("Invalid URL format: " + url);
    }

    size_t host_start = protocol_end + 3;
    size_t port_start = url.find(':', host_start);
    size_t path_start = url.find('/', host_start);

    auto&& has_port = [](size_t port_start, size_t path_start) {
        return port_start != std::string::npos && port_start < path_start;
    };

    // Format: http://host:port/path
    std::string host;
    uint16_t port = DEFAULT_HTTP_PORT;
    std::string path =  path_start != std::string::npos ? url.substr(path_start) : "/";

    if (has_port(port_start, path_start)) {
        host = url.substr(host_start, port_start - host_start);
        size_t port_end = (path_start != std::string::npos) ? path_start : url.length();
        port = static_cast<uint16_t>(std::stoi(url.substr(port_start + 1, port_end - port_start - 1)));
    } else {
        size_t host_end = (path_start != std::string::npos) ? path_start : url.length();
        host = url.substr(host_start, host_end - host_start);
    }

    httplib::Client client(host, port);
    client.set_connection_timeout(CONNECTION_TIMEOUT_SEC);
    client.set_read_timeout(READ_TIMEOUT_SEC);

    auto res = client.Post(path, body, CONTENT_TYPE);

    if (!res) throw std::runtime_error("HTTP request failed: " + httplib::to_string(res.error()));

    if (res->status != HTTP_STATUS_OK) throw std::runtime_error("HTTP request returned status " + std::to_string(res->status));

    return res->body;
}

void HttpTransport::register_handler(const std::string& endpoint,
                                     RequestHandler handler) {
    if (running_) throw std::runtime_error("Cannot register handlers after server is started");

    server_->Post(endpoint, [handler](const httplib::Request& req,
                                      httplib::Response& res) {
        try {
            std::string response = handler(req.body);
            res.set_content(response, CONTENT_TYPE);
            res.status = HTTP_STATUS_OK;
        } catch (const std::exception& e) {
            res.set_content(R"({"error": ")" + std::string(e.what()) + "\"}",
                          CONTENT_TYPE);
            res.status = HTTP_STATUS_INTERNAL_ERROR;
        }
    });
}

void HttpTransport::start(uint16_t port) {
    if (running_) throw std::runtime_error("Transport already running");

    port_ = port;
    running_ = true;

    server_thread_ = std::thread([this]() {
        server_->listen("0.0.0.0", port_);
    });

    std::this_thread::sleep_for(SERVER_STARTUP_DELAY);
}

void HttpTransport::stop() {
    if (!running_) return;

    running_ = false;
    server_->stop();

    if (server_thread_.joinable()) server_thread_.join();
}

bool HttpTransport::is_running() const {
    return running_;
}

uint16_t HttpTransport::get_port() const {
    return port_;
}

}  // namespace mqpp
