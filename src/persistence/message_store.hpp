#pragma once

#include "../core/message.hpp"
#include "../core/types.hpp"
#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>
#include <chrono>
#include <vector>

namespace mqpp {

/**
 * Tracks message's delivery and acknowledgment state
 */
struct DeliveryState {
    MessageId message_id;
    std::string topic;
    std::unordered_set<std::string> pending_consumers;
    std::unordered_set<std::string> acknowledged_consumers;
    std::chrono::system_clock::time_point last_retry;

    DeliveryState(MessageId id, std::string t,
                  std::unordered_set<std::string> pending)
        : message_id(std::move(id))
        , topic(std::move(t))
        , pending_consumers(std::move(pending))
        , last_retry(std::chrono::system_clock::now())
    {}

    DeliveryState() = default;
};

/**
 * Manages message persistence
 */
class MessageStore {
public:
    explicit MessageStore(std::filesystem::path storage_dir);

    /**
     * Persist message to disk with target consumers
     * @param message Message to be persisted
     * @param target_consumers Customer ids which should receive the message
     */
    void persist_message(const Message& message,
                        const std::vector<std::string>& target_consumers);

    /**
     * Mark message as acknowledged by a consumer
     * Automatically cleans up if all consumers have acked
     */
    void acknowledge(const MessageId& msg_id,
                    const std::string& consumer_id);

    /**
     * Get messages needing retry (not fully acked, past retry interval)
     */
    std::vector<std::pair<Message, DeliveryState>> get_messages_for_retry(
        std::chrono::seconds retry_interval);

    /**
     * Load all persisted messages on startup
     * Returns list of delivery states for recovery
     */
    std::vector<std::pair<Message, DeliveryState>> load_all_messages();

    /**
     * Get delivery state for a message (for testing/monitoring)
     */
    std::optional<DeliveryState> get_delivery_state(const MessageId& msg_id) const;

private:
    std::filesystem::path storage_dir_;
    mutable std::shared_mutex mutex_; // allows locking/unlocking in const context

    std::unordered_map<MessageId, DeliveryState> delivery_tracking_;

    void write_message_file(const Message& msg, const DeliveryState& state);
    void delete_message_file(const MessageId& msg_id);
    void update_message_file(const Message& msg, const DeliveryState& state);
    std::pair<Message, DeliveryState> read_message_file(const std::filesystem::path& file);

    std::filesystem::path get_message_path(const MessageId& msg_id) const;
};

}  // namespace mqpp
