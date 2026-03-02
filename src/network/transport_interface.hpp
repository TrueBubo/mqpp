#pragma once

#include <string>
#include <functional>
#include <cstdint>

namespace mqpp {

/**
 * Abstract transport interface
 */
class ITransport {
public:
    virtual ~ITransport() = default;

    /**
     * Send request to remote endpoint, wait for response
     * @param url Full URL or endpoint
     * @param body JSON request body
     * @return JSON response body
     */
    virtual std::string send_request(const std::string& url,
                                     const std::string& body) = 0;

    /**
     * Register handler for incoming requests at an endpoint
     * Handler receives JSON body and returns JSON response
     */
    using RequestHandler = std::function<std::string(const std::string& json)>;

    virtual void register_handler(const std::string& endpoint,
                                  RequestHandler handler) = 0;

    /**
     * Start listening for incoming requests
     * @param port Port to listen on
     */
    virtual void start(uint16_t port) = 0;

    /**
     * Stop the transport layer
     */
    virtual void stop() = 0;

    /**
     * Get the port this transport is listening on
     * Returns 0 if not started
     */
    virtual uint16_t get_port() const = 0;
};

}  // namespace mqpp
