#ifndef TRANSPORT_INTERFACE_HPP
#define TRANSPORT_INTERFACE_HPP

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
     * @param body string request body
     * @return string response body
     */
    virtual std::string send_request(const std::string& url,
                                     const std::string& body) = 0;

    /**
     * Register handler for incoming requests at an endpoint
     * Handler receives string body and returns string response
     */
    using RequestHandler = std::function<std::string(const std::string& body)>;

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
     */
    virtual uint16_t port() const = 0;
};

}  // namespace mqpp

#endif // TRANSPORT_INTERFACE_HPP