#pragma once

#include "../core/message.hpp"
#include "../network/transport_interface.hpp"
#include "../persistence/message_store.hpp"
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <unordered_map>

namespace mqpp {

/**
 * Handles async message delivery to consumers with retry logic
 * Manages background retry thread
 */
class MessageDispatcher {
public:
    MessageDispatcher(std::shared_ptr<ITransport> transport,
                     std::shared_ptr<MessageStore> store,
                     std::chrono::seconds retry_interval);

    ~MessageDispatcher();

    MessageDispatcher(const MessageDispatcher&) = delete;
    MessageDispatcher& operator=(const MessageDispatcher&) = delete;

    /**
     * Dispatch message to selected consumers
     * Does not wait for acknowledgement
     */
    void dispatch(const Message& message,
                 const std::vector<UserId>& consumer_ids);

    /**
     * Start the retry background thread
     */
    void start_retry_loop();

    /**
     * Stop the retry thread
     */
    void stop();

    /**
     * Register consumer endpoint for delivery
     */
    void register_consumer_endpoint(const CallbackUrl& consumer_id,
                                   const CallbackUrl& callback_url);

    /**
     * Unregister consumer endpoint
     */
    void unregister_consumer_endpoint(const UserId& consumer_id);

    /**
     * Dispatch all pending (unacknowledged) messages for a consumer.
     * Called on reconnect to flush messages missed during disconnection.
     */
    void dispatch_pending_for_consumer(const UserId& consumer_id);

private:
    std::shared_ptr<ITransport> transport_;
    std::shared_ptr<MessageStore> store_;
    std::chrono::seconds retry_interval_;

    std::unordered_map<UserId, CallbackUrl> consumer_endpoints_;
    mutable std::mutex endpoints_mutex_;

    std::thread retry_thread_;
    std::atomic<bool> running_;

    /**
     * Deliver message to single consumer
     * @param message Message to be delivered
     * @param consumer_id Customer to which the message is sent
     * @return Whether customer acknowledged
     */
    bool deliver_to_consumer(const Message& message,
                            const UserId& consumer_id) const;

    /**
     * Sends the same message to all the consumers
     * @param message Message to be sent
     * @param consumers Receivers of the message
     */
    void deliver_to_consumers(const Message& message, const std::unordered_set<UserId>& consumers) const;

    /**
     * Retry loop - periodically checks for unacknowledged messages
     */
    void retry_loop() const;
};

}  // namespace mqpp
