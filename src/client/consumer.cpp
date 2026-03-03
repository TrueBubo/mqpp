#include "consumer.hpp"
#include "../network/http_transport.hpp"
#include "../util/string_utils.hpp"
#include <stdexcept>
#include <sstream>

namespace mqpp {

Consumer::Consumer(const ConsumerConfig& config)
    : config_(config)
    , transport_(std::make_unique<HttpTransport>())
    , running_(false)
{
    if (config_.consumer_id.empty()) throw std::invalid_argument("consumer_id cannot be empty");

    if (config_.topic_patterns.empty()) throw std::invalid_argument("topic_patterns cannot be empty");
}

Consumer::~Consumer() {
    stop();
}

void Consumer::set_handler(MessageHandler handler) {
    if (running_) throw std::runtime_error("Cannot set handler while consumer is running");
    handler_ = std::move(handler);
}

void Consumer::start() {
    if (running_) throw std::runtime_error("Consumer already running");

    if (!handler_) throw std::runtime_error("Message handler not set");

    transport_->register_handler(endpoint::receive,
        [this](const std::string& data) {
            return handle_incoming_message(data);
    });

    transport_->start(config_.listen_port);

    if (config_.listen_port == LISTENER_DEFAULT_PORT) {
        config_.listen_port = transport_->port();
    }

    running_ = true;

    try {
        auto&& subscribe_request = create_subscribe_request();
        std::string url = config_.broker_url + endpoint::subscribe;
        std::string response = transport_->send_request(url, subscribe_request);

        if (auto response_data = StringSerializer::parse(response);
            StringSerializer::get(response_data, field::status) != status::ok
        ) {
            throw std::runtime_error("Subscription failed: " +
                StringSerializer::get(response_data, field::message, "unknown error"));
        }

    } catch (const std::exception& e) {
        stop();
        throw std::runtime_error(std::string("Failed to subscribe: ") + e.what());
    }
}

std::string Consumer::create_subscribe_request() const {
    Data request;
    request[field::type] = type::subscribe;
    request[field::consumer_id] = config_.consumer_id;
    request[field::patterns] = StringSerializer::serialize_vector(config_.topic_patterns);

    auto&& callback_url = std::format("http://localhost:{}{}", config_.listen_port, endpoint::receive);
    request[field::callback_url] = callback_url;

    return StringSerializer::serialize(request);
}

void Consumer::stop() {
    if (!running_) return;

    running_ = false;
    transport_->stop();
}


Message Consumer::parse_message(const std::string& request_str) {
    auto request = StringSerializer::parse(request_str);
    std::string message_str = StringSerializer::get_required(request, field::message);
    return Message::deserialize(message_str);
}

void Consumer::process_message(const Message& msg) const {
    if (handler_) handler_(msg);
    send_acknowledgment(msg.id());
}

std::string Consumer::handle_incoming_message(const std::string& request_str) const {
    try {
        auto&& message = parse_message(request_str);
        process_message(message);

        std::map<std::string, std::string> response;
        response[field::status] = status::ok;
        return StringSerializer::serialize(response);

    } catch (const std::exception& e) {
        std::map<std::string, std::string> response;
        response[field::status] = status::error;
        response[field::message] = e.what();
        return StringSerializer::serialize(response);
    }
}

void Consumer::send_acknowledgment(const MessageId& msg_id) const {
    try {
        std::map<std::string, std::string> ack_request;
        ack_request[field::type] = type::acknowledge;
        ack_request[field::message_id] = msg_id;
        ack_request[field::consumer_id] = config_.consumer_id;

        std::string url = config_.broker_url + endpoint::acknowledge;
        transport_->send_request(url, StringSerializer::serialize(ack_request));

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to consume message: ") + e.what());
    }
}

}  // namespace mqpp
