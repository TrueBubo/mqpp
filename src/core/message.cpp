#include "message.hpp"
#include "string_utils.hpp"
#include <uuid.h>
#include <random>

namespace mqpp {

namespace {
    /**
     * Generate UUID
     * @return String form of the generated UUID
     */
    std::string generate_uuid() {
        std::random_device rd;
        auto seed_data = std::array<int, std::mt19937::state_size>{};
        std::ranges::generate(seed_data, std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        std::mt19937 generator(seq);
        uuids::uuid_random_generator gen{generator};

        return uuids::to_string(gen());
    }

    /**
     * Convert time_point to timestamp string (seconds since epoch)
     * @param time_point Instant wanted to be converted to string
     * @return timestamp string
     */
    std::string time_point_to_string(std::chrono::system_clock::time_point time_point) {
        auto duration = time_point.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        return std::to_string(seconds);
    }

    /**
     * Parse timestamp string to time_point
     * @param str String of the seconds from epoch
     * @return time point
     */
    std::chrono::system_clock::time_point string_to_time_point(const std::string& str) {
        long long seconds = std::stoll(str);
        return std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
    }
}

Message::Message(std::string topic, std::string payload)
    : id_(generate_uuid())
    , topic_(std::move(topic))
    , payload_(std::move(payload))
    , timestamp_(std::chrono::system_clock::now())
{}

Message::Message(MessageId id, std::string topic, std::string payload,
                 std::chrono::system_clock::time_point timestamp)
    : id_(std::move(id))
    , topic_(std::move(topic))
    , payload_(std::move(payload))
    , timestamp_(timestamp)
{}

std::string Message::serialize() const {
    return StringSerializer::serialize(to_map());
}

std::map<std::string, std::string> Message::to_map() const {
    return {
        {"id", id_},
        {"topic", topic_},
        {"payload", payload_},
        {"timestamp", time_point_to_string(timestamp_)}
    };
}

Message Message::deserialize(const std::string& str) {
    return from_map(StringSerializer::parse(str));
}

Message Message::from_map(const std::map<std::string, std::string>& data) {
    return {
        StringSerializer::get_required(data, "id"),
        StringSerializer::get_required(data, "topic"),
        StringSerializer::get_required(data, "payload"),
        string_to_time_point(StringSerializer::get_required(data, "timestamp"))
    };
}

}  // namespace mqpp
