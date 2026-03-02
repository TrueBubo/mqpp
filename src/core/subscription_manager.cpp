#include "subscription_manager.hpp"
#include <algorithm>
#include <ranges>
#include <mutex>

namespace mqpp {

void SubscriptionManager::add_subscription(const UserId& consumer_id,
                                           const std::string& pattern) {
    std::unique_lock lock(mutex_);

    subscriptions_.emplace_back(consumer_id, pattern);
}

void SubscriptionManager::remove_subscription(const UserId& consumer_id) {
    std::unique_lock lock(mutex_);

    std::erase_if(subscriptions_,
                  [&consumer_id](const Subscription& sub) {
                      return sub.consumer_id == consumer_id;
                  });
}

std::vector<std::string>
SubscriptionManager::find_matching_consumers(const std::string& topic) const {
    std::shared_lock lock(mutex_);

    auto&& matching = subscriptions_
        | std::views::filter([&topic](auto&& sub) {
            return std::regex_match(topic, sub.topic_pattern);
        })
        | std::views::transform(&Subscription::consumer_id)
        | std::ranges::to<std::vector<std::string>>();

    return matching;
}

std::vector<Subscription>
SubscriptionManager::get_all_subscriptions() const {
    std::shared_lock lock(mutex_);
    return subscriptions_;
}

}  // namespace mqpp
