#include "message_store.hpp"
#include "../core/string_utils.hpp"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <print>
#include <mutex>

namespace mqpp {

MessageStore::MessageStore(std::filesystem::path storage_dir) : storage_dir_(std::move(storage_dir)) {
    if (!std::filesystem::exists(storage_dir_)) std::filesystem::create_directories(storage_dir_);
}

void MessageStore::persist_message(const Message& message, const std::vector<std::string>& target_consumers) {
    std::unique_lock lock(mutex_);

    DeliveryState state(
        message.id(),
        message.topic(),
        std::unordered_set(target_consumers.begin(), target_consumers.end())
    );

    write_message_file(message, state);

    delivery_tracking_[message.id()] = std::move(state);
}

void MessageStore::acknowledge(const MessageId& msg_id,
                               const std::string& consumer_id) {
    std::unique_lock lock(mutex_);

    auto it = delivery_tracking_.find(msg_id);
    if (it == delivery_tracking_.end()) return;

    auto& state = it->second;

    state.pending_consumers.erase(consumer_id);
    state.acknowledged_consumers.insert(consumer_id);

    if (state.pending_consumers.empty()) {
        delete_message_file(msg_id);
        delivery_tracking_.erase(it);
    } else {
        auto msg_path = get_message_path(msg_id);
        auto [msg, _] = read_message_file(msg_path);
        update_message_file(msg, state);
    }
}

std::vector<std::pair<Message, DeliveryState>>
MessageStore::get_messages_for_retry(std::chrono::seconds retry_interval) {
    std::shared_lock lock(mutex_);

    std::vector<std::pair<Message, DeliveryState>> messages;
    auto now = std::chrono::system_clock::now();

    for (const auto& [msg_id, state] : delivery_tracking_) {
        if (!state.pending_consumers.empty() && (now - state.last_retry) >= retry_interval) {
            auto msg_path = get_message_path(msg_id);
            auto [msg, current_state] = read_message_file(msg_path);

            messages.emplace_back(std::move(msg), current_state);
        }
    }

    return messages;
}

std::vector<std::pair<Message, DeliveryState>>
MessageStore::load_all_messages() {
    std::unique_lock lock(mutex_);

    std::vector<std::pair<Message, DeliveryState>> messages;

    for (const auto& entry : std::filesystem::directory_iterator(storage_dir_)) {
        if (entry.path().extension() == ".txt") {
            try {
                auto [msg, state] = read_message_file(entry.path());
                messages.emplace_back(msg, state);

                delivery_tracking_[msg.id()] = state;
            } catch (const std::exception& e) {
                std::println(stderr, "[ERROR] Failed to load message from file {}: {}",
                             entry.path().string(), e.what());
            }
        }
    }

    return messages;
}

std::optional<DeliveryState>
MessageStore::get_delivery_state(const MessageId& msg_id) const {
    std::shared_lock lock(mutex_);

    auto it = delivery_tracking_.find(msg_id);
    if (it != delivery_tracking_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void MessageStore::write_message_file(const Message& msg, const DeliveryState& state) {
    std::ostringstream content;

    content << "MESSAGE=" << msg.serialize() << "\n";

    std::vector pending_vec(state.pending_consumers.begin(),
                                         state.pending_consumers.end());
    content << "PENDING=" << StringSerializer::serialize_vector(pending_vec) << "\n";

    std::vector acked_vec(state.acknowledged_consumers.begin(),
                                       state.acknowledged_consumers.end());
    content << "ACKNOWLEDGED=" << StringSerializer::serialize_vector(acked_vec) << "\n";

    auto duration = state.last_retry.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    content << "LAST_RETRY=" << seconds << "\n";

    auto final_path = get_message_path(msg.id());
    auto temp_path = final_path;
    temp_path += ".tmp";

    {
        std::ofstream file(temp_path);
        if (!file) {
            throw std::runtime_error("Failed to open file for writing: " +
                                   temp_path.string());
        }
        file << content.str();
    }

    std::filesystem::rename(temp_path, final_path);
}

void MessageStore::update_message_file(const Message& msg, const DeliveryState& state) {
    write_message_file(msg, state);
}

void MessageStore::delete_message_file(const MessageId& msg_id) {
    auto path = get_message_path(msg_id);
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }
}

std::pair<Message, DeliveryState>
MessageStore::read_message_file(const std::filesystem::path& file) {
    std::ifstream input(file);
    if (!input) {
        throw std::runtime_error("Failed to open file for reading: " + file.string());
    }

    std::map<std::string, std::string> data;
    std::string line;
    while (std::getline(input, line)) {
        auto eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            data[key] = value;
        }
    }

    std::string message_str = StringSerializer::get_required(data, "MESSAGE");
    Message msg = Message::deserialize(message_str);

    DeliveryState state;
    state.message_id = msg.id();
    state.topic = msg.topic();

    std::string pending_str = StringSerializer::get(data, "PENDING", "");
    if (!pending_str.empty()) {
        auto pending_vec = StringSerializer::parse_vector(pending_str);
        state.pending_consumers = std::unordered_set(
            pending_vec.begin(), pending_vec.end());
    }

    std::string acked_str = StringSerializer::get(data, "ACKNOWLEDGED", "");
    if (!acked_str.empty()) {
        auto acked_vec = StringSerializer::parse_vector(acked_str);
        state.acknowledged_consumers = std::unordered_set(
            acked_vec.begin(), acked_vec.end());
    }

    std::string retry_str = StringSerializer::get_required(data, "LAST_RETRY");
    long long seconds = std::stoll(retry_str);
    state.last_retry = std::chrono::system_clock::time_point(std::chrono::seconds(seconds));

    return {std::move(msg), std::move(state)};
}

std::filesystem::path MessageStore::get_message_path(const MessageId& msg_id) const {
    return storage_dir_ / (msg_id + ".txt");
}

}  // namespace mqpp
