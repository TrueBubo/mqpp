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

    bool is_running() const override;
    uint16_t get_port() const override;

private:
    std::unique_ptr<httplib::Server> server_;
    std::unique_ptr<httplib::Client> client_;
    std::thread server_thread_;
    std::atomic<bool> running_;
    uint16_t port_;
};

}  // namespace mqpp
