#include "publisher.hpp"
#include "../network/http_transport.hpp"
#include "../util/string_utils.hpp"
#include <stdexcept>

#include "../shared.hpp"

namespace mqpp {

Publisher::Publisher(const PublisherConfig& config)
    : config_(config)
    , transport_(std::make_unique<HttpTransport>())
{}

Publisher::~Publisher() = default;

MessageId Publisher::publish(const std::string& topic, const std::string& payload) const {
    try {
        auto&& request = create_publish_request(topic, payload);

        auto&& url = config_.broker_url + endpoint::publish;
        auto&& response = transport_->send_request(url, request);

        auto&& response_data = StringSerializer::parse(response);

        if (StringSerializer::get(response_data, field::status) != status::ok) {
            throw std::runtime_error("Publish failed: " +
                                   StringSerializer::get(response_data, field::message, "unknown error"));
        }

        return StringSerializer::get_required(response_data, field::message_id);

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to publish message: ") + e.what());
    }
}

std::string Publisher::create_publish_request(const std::string &topic, const std::string &payload) {
    std::map<std::string, std::string> request;
    request[field::type] = type::publish;
    request[field::topic] = topic;
    request[field::payload] = payload;

    return StringSerializer::serialize(request);
}
}  // namespace mqpp
