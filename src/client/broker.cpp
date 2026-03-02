#include "broker.hpp"
#include "../core/message.hpp"
#include "../core/string_utils.hpp"
#include "../threading/thread_pool.hpp"
#include "../network/http_transport.hpp"
#include "../core/subscription_manager.hpp"
#include "../persistence/message_store.hpp"
#include "../dispatching/message_dispatcher.hpp"
#include <stdexcept>

namespace mqpp {

Broker::Broker(const BrokerConfig& config)
    : config_(config)
    , running_(false)
{
    thread_pool_ = std::make_unique<ThreadPool>(config_.num_threads);
    transport_ = std::make_unique<HttpTransport>();
    subscription_mgr_ = std::make_unique<SubscriptionManager>();
    message_store_ = std::make_unique<MessageStore>(config_.storage_dir);

    dispatcher_ = std::make_unique<MessageDispatcher>(
        std::shared_ptr<ITransport>(transport_.get(), [](ITransport*){}),
        std::shared_ptr<MessageStore>(message_store_.get(), [](MessageStore*){}),
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


void Broker::setup_handlers() {
    transport_->register_handler(PUBLISH_ENDPOINT,
        [this](const std::string& data) {
            return handle_publish(data);
        });

    transport_->register_handler(SUBSCRIBE_ENDPOINT,
        [this](const std::string& data) {
            return handle_subscribe(data);
        });

    transport_->register_handler(ACKNOWLEDGE_ENDPOINT,
        [this](const std::string& data) {
            return handle_acknowledge(data);
        });
}

std::string Broker::handle_publish(const std::string& request_str) const {
    try {
        auto&& req = StringSerializer::parse(request_str);

        auto&& topic = StringSerializer::get_required(req, "topic");
        auto&& payload = StringSerializer::get_required(req, "payload");

        Message msg(topic, payload);

        auto consumers = subscription_mgr_->find_matching_consumers(topic);

        if (!consumers.empty()) {
            message_store_->persist_message(msg, consumers);
            dispatcher_->dispatch(msg, consumers);
        }

        std::map<std::string, std::string> response;
        response["status"] = "ok";
        response["message_id"] = msg.id();
        response["consumers"] = std::to_string(consumers.size());

        return StringSerializer::serialize(response);

    } catch (const std::exception& e) {
        std::map<std::string, std::string> error;
        error["status"] = "error";
        error["message"] = e.what();
        return StringSerializer::serialize(error);
    }
}

std::string Broker::handle_subscribe(const std::string& request_str) {
    try {
        auto&& req = StringSerializer::parse(request_str);

        auto&& consumer_id = StringSerializer::get_required(req, "consumer_id");
        auto&& patterns_str = StringSerializer::get_required(req, "patterns");
        auto&& callback_url = StringSerializer::get_required(req, "callback_url");

        auto&& patterns = StringSerializer::parse_vector(patterns_str);

        dispatcher_->register_consumer_endpoint(consumer_id, callback_url);

        for (auto&& pattern : patterns) subscription_mgr_->add_subscription(consumer_id, pattern);

        dispatcher_->dispatch_pending_for_consumer(consumer_id);

        std::map<std::string, std::string> response;
        response["status"] = "ok";
        response["consumer_id"] = consumer_id;
        response["patterns"] = patterns_str;

        return StringSerializer::serialize(response);

    } catch (const std::exception& e) {
        std::map<std::string, std::string> error;
        error["status"] = "error";
        error["message"] = e.what();
        return StringSerializer::serialize(error);
    }
}

std::string Broker::handle_acknowledge(const std::string& request_str) const {
    try {
        auto&& req = StringSerializer::parse(request_str);

        std::string message_id = StringSerializer::get_required(req, "message_id");
        std::string consumer_id = StringSerializer::get_required(req, "consumer_id");

        message_store_->acknowledge(message_id, consumer_id);

        std::map<std::string, std::string> response;
        response["status"] = "ok";

        return StringSerializer::serialize(response);

    } catch (const std::exception& e) {
        std::map<std::string, std::string> error;
        error["status"] = "error";
        error["message"] = e.what();
        return StringSerializer::serialize(error);
    }
}

}  // namespace mqpp
