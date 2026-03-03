#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "../core/types.hpp"
#include <string>
#include <chrono>

#include "util/string_utils.hpp"

namespace mqpp {

/**
 * Represents a message in the system
 */
class Message {
public:
    /**
     * Create a new message with generated UUID
     */
    Message(std::string topic, std::string payload);

    /**
     * Create message with explicit ID (for deserialization)
     */
    Message(MessageId id, std::string topic, std::string payload,
            std::chrono::system_clock::time_point timestamp);

    const MessageId& id() const { return id_; }
    const std::string& topic() const { return topic_; }
    const std::string& payload() const { return payload_; }
    std::chrono::system_clock::time_point timestamp() const { return timestamp_; }

    /**
     * Transforms the message using @class StringSerializer
     * @return String representation of the message
     */
    std::string serialize() const;

    /**
     * Changes the object properties to key value pairs for easier serialization
     * @return Map of keys and values
     */
    Data to_map() const;

    /**
     *
     * @param str Serialized message
     * @return Parsed message
     */
    static Message deserialize(const std::string& str);

    /**
     * Creates a message object from key value map
     * @param data Map of message properties as keys and values
     * @return Converted Message
     */
    static Message from_map(const Data& data);

private:
    MessageId id_;
    std::string topic_;
    std::string payload_;
    std::chrono::system_clock::time_point timestamp_;

    struct keys {
        static constexpr auto id = "id";
        static constexpr auto topic = "topic";
        static constexpr auto payload = "payload";
        static constexpr auto timestamp = "timestamp";
    };
};

}  // namespace mqpp
#endif // MESSAGE_HPP