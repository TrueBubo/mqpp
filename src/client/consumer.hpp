#ifndef CONSUMER_HPP
#define CONSUMER_HPP

#include "../core/config.hpp"
#include "../core/types.hpp"
#include "../message/message.hpp"
#include "../core/shared.hpp"
#include <memory>
#include <string>

namespace mqpp {

class ITransport;

/**
 * Consumer client for receiving messages from the broker
 */
class Consumer {
public:
    /**
     * Setups the consumer to a valid state
     * @param config Contains the values used in initialization of Consumer members
     */
    explicit Consumer(const ConsumerConfig& config);

    /**
     * Ensures the Broker and its members are gracefully shutdown
     */
    ~Consumer();

    Consumer(const Consumer&) = delete;
    Consumer& operator=(const Consumer&) = delete;
    Consumer(Consumer&&) = delete;
    Consumer& operator=(Consumer&&) = delete;

    /**
     * Set the message handler callback
     * @param handler Function invoked on each message received
     */
    void set_handler(MessageHandler handler);

    /**
     * Registers with broker and begins listening for messages
     */
    void start();

    /**
     * Stop the consumer
     */
    void stop();

private:
    ConsumerConfig config_;
    std::unique_ptr<ITransport> transport_;
    MessageHandler handler_;
    bool running_;

    /**
     * Handle incoming message delivery from broker
     * @param request_str Request from broker received on receive endpoint
     * @return Status of message processing
     */
    std::string handle_incoming_message(const std::string& request_str) const;

    /**
     * Deserializes the payloads from broker containing messages
     * @param request_str Serialized request from broker
     * @return Parsed message
     */
    static Message parse_message(const std::string& request_str);

    /**
     * Helper method for creating subscription request string payload
     * @return Serialized request
     */
    std::string create_subscribe_request() const;

    /**
     * Handles the message with user provided handler and acknowledges the message
     * @param msg Message to be processed
     */
    void process_message(const Message& msg) const;

    /**
     * Send acknowledgment back to broker with our ID that Consumer received a message
     * @param msg_id ID of the message the Consumer received
     */
    void send_acknowledgment(const MessageId& msg_id) const;
};

}  // namespace mqpp
#endif // CONSUMER_HPP