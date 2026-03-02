#include "http_transport.hpp"
#include <stdexcept>
#include <sstream>

namespace mqpp {

HttpTransport::HttpTransport()
    : server_(std::make_unique<httplib::Server>())
    , running_(false)
    , port_(0)
{}

HttpTransport::~HttpTransport() {
    HttpTransport::stop();
}

std::string HttpTransport::send_request(const std::string& url, const std::string& body) {
    auto&& [host, port, path] = parse_url(url);

    httplib::Client client(host, port);
    client.set_connection_timeout(duration::connection_timeout_sec);
    client.set_read_timeout(duration::read_timeout_sec);

    auto res = client.Post(path, body, http::content_type);

    if (!res) throw std::runtime_error("HTTP request failed: " + httplib::to_string(res.error()));

    if (res->status != http_code::ok) throw std::runtime_error("HTTP request returned status " + std::to_string(res->status));

    return res->body;
}

void HttpTransport::register_handler(const std::string& endpoint,
                                     RequestHandler handler) {
    if (running_) throw std::runtime_error("Cannot register handlers after server is started");

    server_->Post(endpoint, [handler](const httplib::Request& req,
                                      httplib::Response& res) {
        try {
            std::string response = handler(req.body);
            res.set_content(response, http::content_type);
            res.status = http_code::ok;
        } catch (const std::exception& e) {
            res.set_content(R"({"error": ")" + std::string(e.what()) + "\"}",
                          http::content_type);
            res.status = http_code::internal_error;
        }
    });
}

void HttpTransport::start(uint16_t port) {
    if (running_) throw std::runtime_error("Transport already running");

    port_ = port;
    running_ = true;

    server_thread_ = std::thread([this]() {
        server_->listen(http::any_address, port_);
    });

    std::this_thread::sleep_for(duration::server_startup_delay);
}

void HttpTransport::stop() {
    if (!running_) return;

    running_ = false;
    server_->stop();

    if (server_thread_.joinable()) server_thread_.join();
}

uint16_t HttpTransport::get_port() const {
    return port_;
}


HttpTransport::ParsedUrl HttpTransport::parse_url(const std::string& url) {
    size_t protocol_end = url.find("://");
    if (protocol_end == std::string::npos) {
        throw std::runtime_error("Invalid URL format: " + url);
    }

    size_t host_start = protocol_end + 3;
    size_t port_start = url.find(':', host_start);
    size_t path_start = url.find('/', host_start);

    auto has_port = [](size_t port_start, size_t path_start) {
        return port_start != std::string::npos && port_start < path_start;
    };

    ParsedUrl parsed_url;
    parsed_url.port = http::default_port;
    parsed_url.path = path_start != std::string::npos ? url.substr(path_start) : "/";

    if (has_port(port_start, path_start)) {
        parsed_url.host = url.substr(host_start, port_start - host_start);
        size_t port_end = (path_start != std::string::npos) ? path_start : url.length();
        parsed_url.port = static_cast<uint16_t>(std::stoi(url.substr(port_start + 1, port_end - port_start - 1)));
    } else {
        size_t host_end = (path_start != std::string::npos) ? path_start : url.length();
        parsed_url.host = url.substr(host_start, host_end - host_start);
    }

    return parsed_url;
}

}  // namespace mqpp
