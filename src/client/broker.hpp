#ifndef BROKER_HPP
#define BROKER_HPP

#include "../core/config.hpp"
#include <memory>
#include <string>

namespace mqpp {

constexpr auto PUBLISH_ENDPOINT     = "/publish";
constexpr auto SUBSCRIBE_ENDPOINT   = "/subscribe";
constexpr auto ACKNOWLEDGE_ENDPOINT = "/acknowledge";

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

    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<ITransport> transport_;
    std::unique_ptr<SubscriptionManager> subscription_mgr_;
    std::unique_ptr<MessageStore> message_store_;
    std::unique_ptr<MessageDispatcher> dispatcher_;

    bool running_;

    void setup_handlers();

    std::string handle_publish(const std::string& request) const;
    std::string handle_subscribe(const std::string& request);
    std::string handle_acknowledge(const std::string& request) const;
};

}  // namespace mqpp
#endif // BROKER_HPP