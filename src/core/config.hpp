#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <chrono>

#include "types.hpp"

namespace mqpp {

constexpr auto BROKER_DEFAULT_PORT = 8080;
constexpr auto LISTENER_DEFAULT_PORT = 9090;

/**
 * Configuration for the Broker
 */
struct BrokerConfig {
    uint16_t port = BROKER_DEFAULT_PORT;
    std::string storage_dir = "./.data/messages";
    std::chrono::seconds retry_interval{30};
};

/**
 * Configuration for Publisher clients
 */
struct PublisherConfig {
    std::string broker_url;
    std::chrono::seconds timeout{5};

    explicit PublisherConfig(const std::string& broker_url = std::format("http://localhost:{}", BROKER_DEFAULT_PORT))
        : broker_url(broker_url)
    {}
};

/**
 * Configuration for Consumer clients
 */
struct ConsumerConfig {
    std::string broker_url;
    UserId consumer_id;
    int listen_port;
    std::vector<std::string> topic_patterns;

    explicit ConsumerConfig(
        const UserId& consumer_id,
        std::vector<std::string>& topic_patterns,
        const std::string& broker_url = std::format("http://localhost:{}", BROKER_DEFAULT_PORT),
        int listen_port = LISTENER_DEFAULT_PORT
        ) : broker_url(broker_url), consumer_id(consumer_id), listen_port(listen_port), topic_patterns(topic_patterns)
    {}
};

}  // namespace mqpp
#endif // CONFIG_HPP