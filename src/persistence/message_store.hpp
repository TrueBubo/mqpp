#ifndef MESSAGE_STORE_HPP
#define MESSAGE_STORE_HPP

#include "../message/message.hpp"
#include "../core/types.hpp"
#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>
#include <chrono>
#include <vector>
#include "../util/string_utils.hpp"

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
    /**
     * Constructs MessageStore
     * @param storage_dir Where should message
     */
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
     * @param msg_id Acknowledged message
     * @param consumer_id ID of the consumer who sent acknowledge
     */
    void acknowledge(const MessageId& msg_id,
                    const std::string& consumer_id);

    /**
     * Get messages needing retry (not acked by everyone, last retry older than retry interval)
     * @param retry_interval How often to retry
     * @return Messages with their delivery states
     */
    std::vector<std::pair<Message, DeliveryState>> get_messages_for_retry(
        std::chrono::seconds retry_interval);

    /**
     * Load all persisted messages on startup
     * @return list of messages and their delivery states for recovery
     */
    std::vector<std::pair<Message, DeliveryState>> load_all_messages();

    /**
     * Get all messages that are pending delivery for a specific consumer
     * @param consumer_id ID of the requested consumer
     * @return List of pending messages and their states
     */
    std::vector<std::pair<Message, DeliveryState>> get_pending_messages_for_consumer(
        const std::string& consumer_id);

    /**
     * Used to match stored messages against a newly subscribed consumer's patterns.
     * @return messageId and topic pair for every tracked message.
     */
    std::vector<std::pair<MessageId, std::string>> get_all_message_topics() const;

    /**
     * Add consumer_id to a message's pending set if not already pending/acknowledged.
     * Persists the updated state to disk.
     * @param msg_id ID of the message to be updated with the new consumer
     * @param consumer_id ID of the consumer who wants to receive the message
     */
    void add_pending_consumer(const MessageId& msg_id, const UserId& consumer_id);

private:
    std::filesystem::path storage_dir_;
    mutable std::shared_mutex mutex_; // allows locking/unlocking in const context

    std::unordered_map<MessageId, DeliveryState> delivery_tracking_;

    /**
     * Persists the message to file
     * @param msg Message to be stored
     * @param state State of the message
     */
    void write_message_file(const Message& msg, const DeliveryState& state) const;

    /**
     * Deletes the message by ID from disk
     * @param msg_id ID of the message to be deleted
     */
    void delete_message_file(const MessageId& msg_id) const;

    /**
     * Reads message file from disk
     * @param file path to the file
     * @return Message with its delivery state
     */
    static std::pair<Message, DeliveryState> read_message_file(const std::filesystem::path& file);
    static Data parse_file_data(const std::filesystem::path& file);
    static Message parse_message(const Data& data);
    static DeliveryState parse_delivery_state(const Data& data, const Message& msg);

    std::filesystem::path get_message_path(const MessageId& msg_id) const;

    static constexpr auto FILE_EXTENSION = ".mqpp";
    static constexpr auto TEMP_FILE_EXTENSION = ".tmp";
};

}  // namespace mqpp

#endif // MESSAGE_STORE_HPP