#ifndef BROKER_HPP
#define BROKER_HPP

#include "../core/config.hpp"
#include <memory>
#include <string>

namespace mqpp {

class ThreadPool;
class ITransport;
class SubscriptionManager;
class MessageStore;
class MessageDispatcher;

/**
 * Coordinates all components and handles client requests
 */
class Broker {
public:
    explicit Broker(const BrokerConfig& config);
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

    std::unique_ptr<ITransport> transport_;
    std::unique_ptr<SubscriptionManager> subscription_mgr_;
    std::unique_ptr<MessageStore> message_store_;
    std::unique_ptr<MessageDispatcher> dispatcher_;

    bool running_;

    void setup_handlers() const;

    std::string handle_publish(const std::string& request) const;
    std::string handle_subscribe(const std::string& request_str) const;
    std::string handle_acknowledge(const std::string& request_str) const;
};

struct PublishRequest {
    std::string topic;
    std::string payload;

    static PublishRequest from_api(const std::string& request_str);
    static std::string to_response(const MessageId& message_id, std::size_t consumer_count);
};

struct SubscribeRequest {
    UserId consumer_id;
    CallbackUrl callback_url;
    std::string patterns_str;
    std::vector<std::string> patterns;

    static SubscribeRequest from_api(const std::string& request_str);
    std::string to_response();
};

struct AcknowledgeRequest {
    MessageId message_id;
    UserId consumer_id;

    static AcknowledgeRequest from_api(const std::string& request_str);
};

}  // namespace mqpp
#endif // BROKER_HPP