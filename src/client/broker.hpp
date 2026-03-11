#ifndef BROKER_HPP
#define BROKER_HPP

#include "../core/config.hpp"
#include <memory>
#include <string>

namespace mqpp {

class ITransport;
class SubscriptionManager;
class MessageStore;
class MessageDispatcher;

/**
 * Coordinates all components and handles client requests
 */
class Broker {
public:
    /**
     * Setups the broker to a valid state
     * @param config Contains the values used in initialization of Broker members
     */
    explicit Broker(const BrokerConfig& config);

    /**
     * Ensures the Broker and its members are gracefully shutdown
     */
    ~Broker();

    Broker(const Broker&) = delete;
    Broker& operator=(const Broker&) = delete;
    Broker(Broker&&) = delete;
    Broker& operator=(Broker&&) = delete;

    /**
     * Begins listening for requests and starts retry loop
     */
    void start();

    /**
     * Shuts down the broker
     */
    void stop();

private:
    BrokerConfig config_;

    std::shared_ptr<ITransport> transport_;
    std::unique_ptr<SubscriptionManager> subscription_mgr_;
    std::shared_ptr<MessageStore> message_store_;
    std::unique_ptr<MessageDispatcher> dispatcher_;

    bool running_;

    /**
     * Registers handlers for publish, subscribe, and acknowledge endpoints
     */
    void setup_handlers() const;


    /**
     * Parses a publish request, stores the message, and tries to send it to matching consumers
     * @param request_str Serialized publish request
     * @return Serialized response containing the message ID and consumer count who are subscribed to message topic
     */
    std::string handle_publish(const std::string& request_str) const;

    /**
     * Parses a subscribe request, registers the consumer with their endpoint and their topic patterns
     * @param request_str Serialized subscribe request
     * @return Serialized response confirming the subscription
     */
    std::string handle_subscribe(const std::string& request_str) const;

    /**
     * Parses an acknowledge request and marks the message as acknowledged by the consumer
     * @param request_str Serialized acknowledge request
     * @return Serialized response confirming the acknowledgement with the consumer ID and topics registered
     */
    std::string handle_acknowledge(const std::string& request_str) const;
};

/**
 * Structure which holds data parsed from requests coming from Publisher on publish endpoint
 */
struct PublishRequest {
    std::string topic;
    std::string payload;

    /**
     * Handles string received via network and makes a new instance of PublishRequest
     * @param request_str String payload received from publisher
     * @return PublishRequest instance parsed from the request
     */
    static PublishRequest from_api(const std::string& request_str);

    /**
     * Creates a response to publish request from publisher
     * @param message_id Assigned ID
     * @param consumer_count How many consumers were found for the message
     * @return String payload containing status, message_id, and consumer_count
     */
    static std::string to_response(const MessageId& message_id, std::size_t consumer_count);
};

/**
 * Structure which holds data parsed from requests coming from Consumer on subscribe endpoint
 */
struct SubscribeRequest {
    UserId consumer_id;
    CallbackUrl callback_url;
    std::vector<std::string> patterns;

    /**
    * Handles string received via network and makes a new instance of ConsumerRequest
    * @param request_str String payload received from consumer
    * @return SubscribeRequest instance parsed from the request
    */
    static SubscribeRequest from_api(const std::string& request_str);

    /**
    * Creates a response to subscribe request from consumer
    * @return String payload containing status, consumer_id and patterns
    */
    std::string to_response();
};

/**
* Structure which holds data parsed from requests coming from Consumer on acknowledge endpoint
*/
struct AcknowledgeRequest {
    MessageId message_id;
    UserId consumer_id;

    /**
    * Handles string received via network and makes a new instance of ConsumerRequest
    * @param request_str String payload received from consumer
    * @return AcknowledgeRequest instance parsed from the request
    */
    static AcknowledgeRequest from_api(const std::string& request_str);
};

}  // namespace mqpp
#endif // BROKER_HPP