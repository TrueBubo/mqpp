#ifndef PUBLISHER_HPP
#define PUBLISHER_HPP

#include "config.hpp"
#include "types.hpp"
#include <memory>
#include <string>

namespace mqpp {

class ITransport;

/**
 * Publisher client for sending messages to the broker
 */
class Publisher {
public:
    explicit Publisher(const PublisherConfig& config);
    ~Publisher();

    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;

    /**
     * Publish a message to a topic
     */
    MessageId publish(const std::string& topic, const std::string& payload) const;

    /**
     * Publish a message and return the assigned message ID
     */
    MessageId publish_with_id(const std::string& topic, const std::string& payload) const;

private:
    PublisherConfig config_;
    std::unique_ptr<ITransport> transport_;
};

}  // namespace mqpp
#endif // PUBLISHER_HPP