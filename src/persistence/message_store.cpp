#include "message_store.hpp"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <print>
#include <mutex>
#include <ranges>

#include "../core/shared.hpp"

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
        auto&& msg_path = get_message_path(msg_id);
        auto [msg, _] = read_message_file(msg_path);
        write_message_file(msg, state);
    }
}

std::vector<std::pair<Message, DeliveryState>>
MessageStore::get_messages_for_retry(std::chrono::seconds retry_interval) {
    std::unique_lock lock(mutex_);

    std::vector<std::pair<Message, DeliveryState>> messages;
    auto&& now = std::chrono::system_clock::now();

    for (auto&& [msg_id, state] : delivery_tracking_) {
        if (!state.pending_consumers.empty() && (now - state.last_retry) >= retry_interval) {
            auto&& msg_path = get_message_path(msg_id);
            auto&& [msg, _] = read_message_file(msg_path);

            state.last_retry = now;
            write_message_file(msg, state);

            messages.emplace_back(std::move(msg), state);
        }
    }

    return messages;
}

std::vector<std::pair<Message, DeliveryState>>
MessageStore::load_all_messages() {
    std::unique_lock lock(mutex_);

    std::vector<std::pair<Message, DeliveryState>> messages;

    for (const auto& entry : std::filesystem::directory_iterator(storage_dir_)) {
        if (entry.path().extension() == FILE_EXTENSION) {
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

void MessageStore::write_message_file(const Message& msg, const DeliveryState& state) const {
    std::ostringstream content;

    content << field::message << '=' << msg.serialize() << '\n';

    std::vector pending_vec(state.pending_consumers.begin(),
                                         state.pending_consumers.end());
    content << field::pending << '=' << StringSerializer::serialize_vector(pending_vec) << '\n';

    std::vector acked_vec(state.acknowledged_consumers.begin(),
                                       state.acknowledged_consumers.end());
    content << field::acknowledged << '=' << StringSerializer::serialize_vector(acked_vec) << '\n';

    auto duration = state.last_retry.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    content << field::last_retry << '=' << seconds << '\n';

    auto final_path = get_message_path(msg.id());
    auto temp_path = final_path;
    temp_path += TEMP_FILE_EXTENSION;

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

void MessageStore::delete_message_file(const MessageId& msg_id) const {
    auto path = get_message_path(msg_id);
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }
}

std::pair<Message, DeliveryState> MessageStore::read_message_file(const std::filesystem::path& file) {
    auto data = parse_file_data(file);
    Message msg = parse_message(data);
    DeliveryState state = parse_delivery_state(data, msg);
    return {std::move(msg), std::move(state)};
}

std::vector<std::pair<Message, DeliveryState>> MessageStore::get_pending_messages_for_consumer(const std::string& consumer_id) {
    std::shared_lock lock(mutex_);

    auto&& pending = delivery_tracking_
        | std::views::filter([&](const auto& entry) {
            auto&& [msg_id, current_state] = entry;
            return current_state.pending_consumers.contains(consumer_id);
          })
        | std::views::transform([&](const auto& entry) {
            auto&& [msg_id, current_state] = entry;
            return read_message_file(get_message_path(msg_id));
          })
        | std::ranges::to<std::vector>();

    return pending;
}

std::vector<std::pair<MessageId, std::string>> MessageStore::get_all_message_topics() const {
    std::shared_lock lock(mutex_);

    return delivery_tracking_ | std::views::transform([](auto&& entry) {
        auto&& [message_id, state] = entry;
        return std::pair{message_id, state.topic};
    }) | std::ranges::to<std::vector>();
}

void MessageStore::add_pending_consumer(const MessageId& msg_id, const UserId& consumer_id) {
    std::unique_lock lock(mutex_);

    auto it = delivery_tracking_.find(msg_id);
    if (it == delivery_tracking_.end()) return;

    auto& state = it->second;
    if (state.pending_consumers.contains(consumer_id) ||
        state.acknowledged_consumers.contains(consumer_id)) return;

    state.pending_consumers.insert(consumer_id);

    auto [msg, _] = read_message_file(get_message_path(msg_id));
    write_message_file(msg, state);
}

std::filesystem::path MessageStore::get_message_path(const MessageId& msg_id) const {
    return storage_dir_ / (msg_id + FILE_EXTENSION);
}

Data MessageStore::parse_file_data(const std::filesystem::path& file) {
    std::ifstream input(file);
    if (!input) {
        throw std::runtime_error("Failed to open file for reading: " + file.string());
    }

    Data data;
    std::string line;
    while (std::getline(input, line)) {
        auto eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            data[line.substr(0, eq_pos)] = line.substr(eq_pos + 1);
        }
    }
    return data;
}

Message MessageStore::parse_message(const Data& data) {
    std::string message_str = StringSerializer::get_required(data, field::message);
    return Message::deserialize(message_str);
}

DeliveryState MessageStore::parse_delivery_state(const Data& data, const Message& msg) {
    DeliveryState state;
    state.message_id = msg.id();
    state.topic = msg.topic();

    std::string pending_str = StringSerializer::get(data, field::pending, "");
    if (!pending_str.empty()) {
        auto pending_vec = StringSerializer::parse_vector(pending_str);
        state.pending_consumers = std::unordered_set(pending_vec.begin(), pending_vec.end());
    }

    std::string acked_str = StringSerializer::get(data, field::acknowledged, "");
    if (!acked_str.empty()) {
        auto acked_vec = StringSerializer::parse_vector(acked_str);
        state.acknowledged_consumers = std::unordered_set(acked_vec.begin(), acked_vec.end());
    }

    std::string retry_str = StringSerializer::get_required(data, field::last_retry);
    long long seconds = std::stoll(retry_str);
    state.last_retry = std::chrono::system_clock::time_point(std::chrono::seconds(seconds));

    return state;
}

}  // namespace mqpp
