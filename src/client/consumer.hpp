#ifndef CONSUMER_HPP
#define CONSUMER_HPP

#include "../core/config.hpp"
#include "../core/types.hpp"
#include "../message/message.hpp"
#include "shared.hpp"
#include <map>
#include <memory>
#include <string>

namespace mqpp {

class ITransport;

/**
 * Consumer client for receiving messages from the broker
 */
class Consumer {
public:
    explicit Consumer(const ConsumerConfig& config);
    ~Consumer();

    Consumer(const Consumer&) = delete;
    Consumer& operator=(const Consumer&) = delete;

    /**
     * Set the message handler callback
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
     * @param request_json
     * @return acknowledgment response
     */
    std::string handle_incoming_message(const std::string& request_json) const;
    static Message parse_message(const std::string& request_str);
    std::map<std::string, std::string> create_subscribe_request() const;
    void process_message(const Message& msg) const;

    /**
     * Send acknowledgment back to broker
     */
    void send_acknowledgment(const MessageId& msg_id) const;
};

}  // namespace mqpp
#endif // CONSUMER_HPP