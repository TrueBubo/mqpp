#include "../src/client/consumer.hpp"
#include <print>
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <print>

constexpr auto SUCCESS_EXIT_CODE = 0;
constexpr auto ERROR_EXIT_CODE = 1;

constexpr auto DEFAULT_CONSUMER_ID = "consumer-1";

bool should_stop = false;

void signal_handler(int signal) {
    std::print("Received signal {}, shutting down...", signal);
    should_stop = true;
}

void register_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

int main(int argc, char* argv[]) {
    register_handlers();

    try {
        std::vector<std::string> topic_patterns{"user\\..*", "order\\..*"};
        mqpp::ConsumerConfig config(DEFAULT_CONSUMER_ID, topic_patterns);

        if (argc > 1) {
            config.consumer_id = argv[1];
        }

        if (argc > 2) {
            config.listen_port = std::stoi(argv[2]);
        }

        std::print("Starting consumer...\n");
        std::print("  Consumer ID: {}\n", config.consumer_id);
        std::print("  Broker: {}\n", config.broker_url);
        std::print("  Listen port: {}\n", config.listen_port);
        std::print("  Patterns:\n");
        for (const auto& pattern : config.topic_patterns) {
            std::print("    - {}\n", pattern);
        }

        mqpp::Consumer consumer(config);

        consumer.set_handler([](const mqpp::Message& msg) {
            std::print("\n=== Message Received ===\n");
            std::print("  ID: {}\n", msg.id());
            std::print("  Topic: {}\n", msg.topic());
            std::print("  Payload: {}\n", msg.payload());
            std::print("========================\n");
        });

        consumer.start();

        std::print("Consumer started successfully!\n");
        std::print("Waiting for messages... Press Ctrl+C to stop.\n");
        std::print("---\n");

        while (!should_stop) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::print("\nStopping consumer...\n");
        consumer.stop();
        std::print("Consumer stopped.\n");

    } catch (const std::exception& e) {
        std::print(std::cerr, "Error: {}\n", e.what());
        return ERROR_EXIT_CODE;
    }

    return SUCCESS_EXIT_CODE;
}
