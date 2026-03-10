#include "broker.hpp"
#include "../core/shared.hpp"
#include "../message/message.hpp"
#include "../util/string_utils.hpp"
#include "../network/http_transport.hpp"
#include "../subscription/subscription_manager.hpp"
#include "../persistence/message_store.hpp"
#include "../dispatching/message_dispatcher.hpp"
#include <stdexcept>
#include <algorithm>

namespace mqpp {

Broker::Broker(const BrokerConfig& config)
    : config_(config)
    , running_(false)
{
    transport_ = std::make_shared<HttpTransport>();
    subscription_mgr_ = std::make_unique<SubscriptionManager>();
    message_store_ = std::make_shared<MessageStore>(config_.storage_dir);

    dispatcher_ = std::make_unique<MessageDispatcher>(
        transport_,
        message_store_,
        config_.retry_interval
    );

    setup_handlers();
}

Broker::~Broker() {
    stop();
}

void Broker::start() {
    if (running_) throw std::runtime_error("Broker already running");

    auto persisted = message_store_->load_all_messages();

    transport_->start(config_.port);

    dispatcher_->start_retry_loop();

    running_ = true;
}

void Broker::stop() {
    if (!running_) return;

    running_ = false;
    dispatcher_->stop();
    transport_->stop();
}


void Broker::setup_handlers() const {
    transport_->register_handler(endpoint::publish,
        [this](const std::string& data) {
            return handle_publish(data);
        });

    transport_->register_handler(endpoint::subscribe,
        [this](const std::string& data) {
            return handle_subscribe(data);
        });

    transport_->register_handler(endpoint::acknowledge,
        [this](const std::string& data) {
            return handle_acknowledge(data);
        });
}

std::string Broker::handle_publish(const std::string& request_str) const {
    try {
        auto&& request = PublishRequest::from_api(request_str);
        auto&& [topic, payload] = request;

        Message message(topic, payload);

        auto&& consumers = subscription_mgr_->find_matching_consumers(topic);

        message_store_->persist_message(message, consumers);

        if (!consumers.empty()) {
            dispatcher_->dispatch(message, consumers);
        }

        return PublishRequest::to_response(message.id(), consumers.size());
    } catch (const std::exception& exception) {
        return create_error_response(exception);
    }
}

std::string Broker::handle_subscribe(const std::string& request_str) const {
    try {
        auto&& request = SubscribeRequest::from_api(request_str);
        auto&& [consumer_id, callback_url, patterns_str, patterns] = request;

        dispatcher_->register_consumer_endpoint(consumer_id, callback_url);

        for (auto&& pattern : patterns) subscription_mgr_->add_subscription(consumer_id, pattern);

        for (const auto& [msg_id, topic] : message_store_->get_all_message_topics()) {
            auto matching = subscription_mgr_->find_matching_consumers(topic);
            if (std::ranges::find(matching, consumer_id) != matching.end()) {
                message_store_->add_pending_consumer(msg_id, consumer_id);
            }
        }

        dispatcher_->dispatch_pending_for_consumer(consumer_id);

        return request.to_response();
    } catch (const std::exception& exception) {
        return create_error_response(exception);
    }
}

std::string Broker::handle_acknowledge(const std::string& request_str) const {
    try {
        auto&& [message_id, consumer_id] = AcknowledgeRequest::from_api(request_str);
        message_store_->acknowledge(message_id, consumer_id);
        return create_ok_response();
    } catch (const std::exception& exception) {
        return create_error_response(exception);
    }
}

PublishRequest PublishRequest::from_api(const std::string& request_str) {
    auto&& req = StringSerializer::parse(request_str);

    auto&& topic = StringSerializer::get_required(req, field::topic);
    auto&& payload = StringSerializer::get_required(req, field::payload);

    return {
    std::move(topic),
    std::move(payload),
    };
}

std::string PublishRequest::to_response(const MessageId& message_id, std::size_t consumer_count) {
    std::map<std::string, std::string> response;
    response[field::status]     = status::ok;
    response[field::message_id] = message_id;
    response[field::consumers]  = std::to_string(consumer_count);
    return StringSerializer::serialize(response);
}

SubscribeRequest SubscribeRequest::from_api(const std::string& request_str) {
    auto&& request_map = StringSerializer::parse(request_str);

    auto&& consumer_id = StringSerializer::get_required(request_map, field::consumer_id);
    auto&& callback_url = StringSerializer::get_required(request_map, field::callback_url);

    auto&& patterns_str = StringSerializer::get_required(request_map, field::patterns);
    auto&& patterns = StringSerializer::parse_vector(patterns_str);

    return {
        std::move(consumer_id),
        std::move(callback_url),
        std::move(patterns_str),
        patterns
    };
}

std::string SubscribeRequest::to_response() {
    std::map<std::string, std::string> response;
    response[field::status]      = status::ok;
    response[field::consumer_id] = std::move(consumer_id);
    response[field::patterns]    = std::move(patterns_str);

    return StringSerializer::serialize(response);
}

AcknowledgeRequest AcknowledgeRequest::from_api(const std::string& request_str) {
    auto&& req = StringSerializer::parse(request_str);

    std::string message_id = StringSerializer::get_required(req, field::message_id);
    std::string consumer_id = StringSerializer::get_required(req, field::consumer_id);

    return {
    std::move(message_id),
        std::move(consumer_id),
    };
}
}  // namespace mqpp
