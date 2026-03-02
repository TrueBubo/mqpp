#pragma once

#include "transport_interface.hpp"
#include <httplib.h>
#include <atomic>
#include <memory>
#include <thread>

namespace mqpp {

/**
 * HTTP-based transport implementation using cpp-httplib
 */
class HttpTransport : public ITransport {
public:
    HttpTransport();
    ~HttpTransport() override;

    std::string send_request(const std::string& url,
                            const std::string& body) override;

    void register_handler(const std::string& endpoint,
                         RequestHandler handler) override;

    void start(uint16_t port) override;
    void stop() override;

    uint16_t get_port() const override;

private:
    struct ParsedUrl {
        std::string host;
        uint16_t port;
        std::string path;
    };

    static ParsedUrl parse_url(const std::string& url);

    std::unique_ptr<httplib::Server> server_;
    std::unique_ptr<httplib::Client> client_;
    std::thread server_thread_;
    std::atomic<bool> running_;
    uint16_t port_;


    struct http {
        static constexpr auto default_port = 80;
        static constexpr auto content_type = "text/plain";
        static constexpr auto any_address = "0.0.0.0";
    };

    struct duration {
        static constexpr auto connection_timeout_sec = 5;
        static constexpr auto read_timeout_sec = 10;
        static constexpr auto server_startup_delay = std::chrono::milliseconds(100);
    };

    struct http_code {
        static constexpr auto ok = 200;
        static constexpr auto internal_error = 500;
    };

};

}  // namespace mqpp
