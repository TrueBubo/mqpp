#include "../src/client/broker.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <print>


constexpr auto SUCCESS_EXIT_CODE = 0;
constexpr auto ERROR_EXIT_CODE = 1;

bool should_stop = false;

void signal_handler(int signal) {
    std::print("Received signal {}, shutting down...", signal);
    should_stop = true;
}

void register_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

int main() {
    register_handlers();

    try {
        const mqpp::BrokerConfig config;

        std::print("Starting message broker\n");

        mqpp::Broker broker(config);
        broker.start();

        std::print("Broker started successfully\n");

        while (!should_stop) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        broker.stop();
        std::print("Broker stopped\n");
    } catch (const std::exception& e) {
        std::print(std::cerr, "Error: {}\n", e.what());
        return ERROR_EXIT_CODE;
    }

    return SUCCESS_EXIT_CODE;
}
