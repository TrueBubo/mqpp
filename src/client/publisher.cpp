#include "../core/publisher.hpp"
#include "../network/http_transport.hpp"
#include "../core/string_utils.hpp"
#include <stdexcept>

namespace mqpp {

Publisher::Publisher(const PublisherConfig& config)
    : config_(config)
    , transport_(std::make_unique<HttpTransport>())
{}

Publisher::~Publisher() = default;

MessageId Publisher::publish(const std::string& topic, const std::string& payload) const {
    try {
        std::map<std::string, std::string> request;
        request["type"] = "publish";
        request["topic"] = topic;
        request["payload"] = payload;

        auto&& url = config_.broker_url + "/publish";
        auto&& response = transport_->send_request(url, StringSerializer::serialize(request));

        auto&& response_data = StringSerializer::parse(response);

        if (StringSerializer::get(response_data, "status") != "ok") {
            throw std::runtime_error("Publish failed: " +
                                   StringSerializer::get(response_data, "message", "unknown error"));
        }

        return StringSerializer::get_required(response_data, "message_id");

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to publish message: ") + e.what());
    }
}

}  // namespace mqpp
