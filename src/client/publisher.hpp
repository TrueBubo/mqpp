#ifndef PUBLISHER_HPP
#define PUBLISHER_HPP

#include "../core/config.hpp"
#include "../core/types.hpp"
#include <memory>
#include <string>

namespace mqpp {

class ITransport;

/**
 * Publisher client for sending messages to the broker
 */
class Publisher {
public:
    /**
     * Setups the publisher to a valid state
     * @param config Contains the values used in initialization of Publisher members
     */
    explicit Publisher(const PublisherConfig& config);

    /**
     * Ensures the Publisher and its members are gracefully shutdown
     */
    ~Publisher();

    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;
    Publisher(Publisher&&) = delete;
    Publisher& operator=(Publisher&&) = delete;

    /**
     * Publish a message and return the assigned message ID
     * @param topic Topic to publish to
     * @param payload Data to be sent
     * @return MessageId of the message created
     */
    MessageId publish(const std::string& topic, const std::string& payload) const;

private:
    PublisherConfig config_;
    std::unique_ptr<ITransport> transport_;

    /**
     * Helper method for creating subscription request string payload
     * @param topic Topic to create a message for
     * @param payload Data to be sent
     * @return Serialized request
     */
    static std::string create_publish_request(const std::string& topic, const std::string& payload) ;
};

}  // namespace mqpp
#endif // PUBLISHER_HPP