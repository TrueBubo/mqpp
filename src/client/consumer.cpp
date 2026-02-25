#include "../core/consumer.hpp"
#include "../network/http_transport.hpp"
#include "../core/string_utils.hpp"
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

    transport_->register_handler("/receive",
        [this](const std::string& data) {
            return handle_incoming_message(data);
    });

    transport_->start(config_.listen_port);

    if (config_.listen_port == LISTENER_DEFAULT_PORT) {
        config_.listen_port = transport_->get_port();
    }

    running_ = true;

    try {
        std::map<std::string, std::string> subscribe_request;
        subscribe_request["type"] = "subscribe";
        subscribe_request["consumer_id"] = config_.consumer_id;
        subscribe_request["patterns"] = StringSerializer::serialize_vector(config_.topic_patterns);

        std::stringstream callback_url;
        callback_url << "http://localhost:" << config_.listen_port << "/receive";
        subscribe_request["callback_url"] = callback_url.str();

        std::string url = config_.broker_url + "/subscribe";
        std::string response = transport_->send_request(url, StringSerializer::serialize(subscribe_request));

        if (auto response_data = StringSerializer::parse(response);
            StringSerializer::get(response_data, "status") != "ok"
        ) {
            throw std::runtime_error("Subscription failed: " +
                StringSerializer::get(response_data, "message", "unknown error"));
        }

    } catch (const std::exception& e) {
        stop();
        throw std::runtime_error(std::string("Failed to subscribe: ") + e.what());
    }
}

void Consumer::stop() {
    if (!running_) return;

    running_ = false;
    transport_->stop();
}


std::string Consumer::handle_incoming_message(const std::string& request_str) const {
    try {
        auto req = StringSerializer::parse(request_str);

        std::string message_str = StringSerializer::get_required(req, "message");
        Message msg = Message::deserialize(message_str);

        if (handler_) handler_(msg);

        send_acknowledgment(msg.id());

        std::map<std::string, std::string> response;
        response["status"] = "ok";
        return StringSerializer::serialize(response);

    } catch (const std::exception& e) {
        std::map<std::string, std::string> response;
        response["status"] = "error";
        response["message"] = e.what();
        return StringSerializer::serialize(response);
    }
}

void Consumer::send_acknowledgment(const MessageId& msg_id) const {
    try {
        std::map<std::string, std::string> ack_request;
        ack_request["type"] = "acknowledge";
        ack_request["message_id"] = msg_id;
        ack_request["consumer_id"] = config_.consumer_id;

        std::string url = config_.broker_url + "/acknowledge";
        transport_->send_request(url, StringSerializer::serialize(ack_request));

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to consume message: ") + e.what());
    }
}

}  // namespace mqpp
