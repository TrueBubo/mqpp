#include "message_dispatcher.hpp"

#include <mutex>

#include "../core/string_utils.hpp"
#include <print>

namespace mqpp {

constexpr auto RETRY_LOOP_MAX_ATTEMPTS  = 10;
constexpr auto RETRY_LOOP_DELAY = std::chrono::seconds(1);

MessageDispatcher::MessageDispatcher(std::shared_ptr<ITransport> transport,
                                   std::shared_ptr<MessageStore> store,
                                   std::chrono::seconds retry_interval)
    : transport_(std::move(transport))
    , store_(std::move(store))
    , retry_interval_(retry_interval)
    , running_(false)
{}

MessageDispatcher::~MessageDispatcher() {
    stop();
}

void MessageDispatcher::dispatch(const Message& message,
                                const std::vector<UserId>& consumer_ids) {
    for (auto&& consumer_id : consumer_ids) {
        deliver_to_consumer(message, consumer_id);
    }
}

void MessageDispatcher::start_retry_loop() {
    if (running_) return;
    running_ = true;
    retry_thread_ = std::thread(&MessageDispatcher::retry_loop, this);
}

void MessageDispatcher::stop() {
    if (!running_) return;
    running_ = false;
    if (retry_thread_.joinable()) {
        retry_thread_.join();
    }
}

void MessageDispatcher::register_consumer_endpoint(const UserId& consumer_id,
                                                  const CallbackUrl& callback_url) {
    std::lock_guard lock(endpoints_mutex_);
    consumer_endpoints_[consumer_id] = callback_url;
}

void MessageDispatcher::unregister_consumer_endpoint(const UserId& consumer_id) {
    std::lock_guard lock(endpoints_mutex_);
    consumer_endpoints_.erase(consumer_id);
}

bool MessageDispatcher::deliver_to_consumer(const Message& message,
                                           const UserId& consumer_id) const {
    try {
        std::string callback_url;
        {
            std::lock_guard lock(endpoints_mutex_);
            auto&& it = consumer_endpoints_.find(consumer_id);
            if (it == consumer_endpoints_.end()) return false;
            callback_url = it->second;
        }

        std::map<std::string, std::string> delivery_data;
        delivery_data["type"] = "message";
        delivery_data["message"] = message.serialize();

        std::string response = transport_->send_request(
            callback_url,
            StringSerializer::serialize(delivery_data)
        );

        if (auto&& response_data = StringSerializer::parse(response);
            StringSerializer::get(response_data, "status") != "ok"
            ) return false;

        store_->acknowledge(message.id(), consumer_id);
        return true;
    } catch (const std::exception& e) {
        std::println(stderr, "[ERROR] Message delivery failed for consumer {} (msg_id: {}): {}",
                     consumer_id, message.id(), e.what());
        return false;
    }
}

void MessageDispatcher::retry_loop() const {
    while (running_) {
        try {
            auto&& messages = store_->get_messages_for_retry(retry_interval_);

            for (auto&& [msg, state] : messages) {
                deliver_to_consumers(msg, state.pending_consumers);
            }

        } catch (const std::exception& e) {
            std::println(stderr, "[ERROR] Retry loop error: {}", e.what());
        }

        for (int i = 0; i < RETRY_LOOP_MAX_ATTEMPTS && running_; ++i) {
            std::this_thread::sleep_for(RETRY_LOOP_DELAY);
        }
    }
}

void MessageDispatcher::dispatch_pending_for_consumer(const UserId& consumer_id) {
    auto pending = store_->get_pending_messages_for_consumer(consumer_id);
    for (auto&& [msg, state] : pending) {
        deliver_to_consumer(msg, consumer_id);
    }
}

void MessageDispatcher::deliver_to_consumers(const Message& msg, const std::unordered_set<UserId>& consumers) const {
    for (auto&& consumer_id : consumers) {
        deliver_to_consumer(msg, consumer_id);
    }
}

}  // namespace mqpp
