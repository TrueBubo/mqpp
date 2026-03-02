#include "message.hpp"
#include "../util/string_utils.hpp"
#include "../util/uuid_util.hpp"
#include "../util/time_util.hpp"
#include <algorithm>

namespace mqpp {

Message::Message(std::string topic, std::string payload)
    : id_(UuidUtil::generate())
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
        {keys::id, id_},
        {keys::topic, topic_},
        {keys::payload, payload_},
        {keys::timestamp, TimeUtil::time_point_to_string(timestamp_)}
    };
}

Message Message::deserialize(const std::string& str) {
    return from_map(StringSerializer::parse(str));
}

Message Message::from_map(const std::map<std::string, std::string>& data) {
    return {
        StringSerializer::get_required(data, keys::id),
        StringSerializer::get_required(data, keys::topic),
        StringSerializer::get_required(data, keys::payload),
        TimeUtil::string_to_time_point(StringSerializer::get_required(data, keys::timestamp))
    };
}

}  // namespace mqpp
