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


    static constexpr auto DEFAULT_HTTP_PORT = 80;
    static constexpr auto CONTENT_TYPE = "application/json";

    static constexpr auto CONNECTION_TIMEOUT_SEC = 5;
    static constexpr auto READ_TIMEOUT_SEC = 10;
    static constexpr auto SERVER_STARTUP_DELAY = std::chrono::milliseconds(100);

    static constexpr auto HTTP_STATUS_OK = 200;
    static constexpr auto HTTP_STATUS_INTERNAL_ERROR = 500;

    static constexpr auto ANY_ADDRESS = "0.0.0.0";
};

}  // namespace mqpp
