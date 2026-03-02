#pragma once

#include <string>
#include <vector>
#include <regex>
#include <shared_mutex>
#include "types.hpp"

namespace mqpp {

/**
 * Represents a consumer's subscription
 */
struct Subscription {
    UserId consumer_id;
    std::regex topic_pattern;

    Subscription(std::string id, const std::string& pattern)
        : consumer_id(std::move(id))
        , topic_pattern(pattern)
    {}
};

/**
 * Manages subscriptions and matches topics against consumer patterns
 */
class SubscriptionManager {
public:
    SubscriptionManager() = default;

    /**
     * Add a subscription for a consumer with a regex pattern
     * @param consumer_id
     * @param pattern Regex to match for
     */
    void add_subscription(const UserId& consumer_id,
                         const std::string& pattern);

    /**
     * Remove all subscriptions for a consumer
     * @param consumer_id
     */
    void remove_subscription(const UserId& consumer_id);

    /**
     * Find all consumers whose patterns match the given topic
     * @return consumer IDs
     */
    std::vector<std::string> find_matching_consumers(const std::string& topic) const;

    /**
     * Get all subscriptions
     * @return Active subscriptions
     */
    std::vector<Subscription> get_all_subscriptions() const;

private:
    mutable std::shared_mutex mutex_;
    std::vector<Subscription> subscriptions_;
};

}  // namespace mqpp
