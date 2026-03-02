#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "../core/types.hpp"
#include <string>
#include <chrono>
#include <map>

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

    std::string serialize() const;
    std::map<std::string, std::string> to_map() const;

    static Message deserialize(const std::string& str);
    static Message from_map(const std::map<std::string, std::string>& data);

private:
    MessageId id_;
    std::string topic_;
    std::string payload_;
    std::chrono::system_clock::time_point timestamp_;
};

}  // namespace mqpp
#endif // MESSAGE_HPP