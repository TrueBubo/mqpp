#include "core/publisher.hpp"
#include <iostream>
#include <thread>
#include <chrono>

constexpr auto SUCCESS_EXIT_CODE = 0;
constexpr auto ERROR_EXIT_CODE = 1;

int main(int argc, char* argv[]) {
    try {
        mqpp::PublisherConfig config;

        if (argc > 1) {
            config.broker_url = argv[1];
        }

        std::print("Connecting to broker at {}...\n", config.broker_url);

        mqpp::Publisher publisher(config);

        std::print("Publisher ready. Publishing test messages...\n---\n");

        for (int i = 1; i <= 5; ++i) {
            std::string topic = i % 2 == 0 ? "user.login" : "order.created";
            std::string payload = "{\"id\": " + std::to_string(i) + ", \"timestamp\": " +
                                std::to_string(std::time(nullptr)) + "}";

            std::print("Publishing to \"{}\": {}\n", topic, payload);

            auto msg_id = publisher.publish_with_id(topic, payload);

            std::print("\t-> Acknowledged with ID: {}\n", msg_id);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::print("---\n");
        std::print("All messages published successfully\n");

    } catch (const std::exception& e) {
        std::print(std::cerr, "Error: {}\n", e.what());
        return ERROR_EXIT_CODE;
    }

    return SUCCESS_EXIT_CODE;
}
