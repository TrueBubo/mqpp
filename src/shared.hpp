#ifndef ENDPOINTS_HPP
#define ENDPOINTS_HPP
#include "util/string_utils.hpp"

namespace mqpp {

struct endpoint {
    static constexpr auto publish = "/publish";
    static constexpr auto subscribe = "/subscribe";
    static constexpr auto acknowledge = "/acknowledge";
    static constexpr auto receive = "/receive";
};

struct field {
    static constexpr auto type = "type";
    static constexpr auto status = "status";
    static constexpr auto message = "message";
    static constexpr auto message_id = "message_id";
    static constexpr auto consumer_id = "consumer_id";
    static constexpr auto patterns = "patterns";
    static constexpr auto callback_url = "callback_url";
    static constexpr auto topic = "topic";
    static constexpr auto payload = "payload";
    static constexpr auto consumers = "consumers";
    static constexpr auto pending = "PENDING";
    static constexpr auto acknowledged = "ACKNOWLEDGED";
    static constexpr auto last_retry = "LAST_RETRY";
};

struct type {
    static constexpr auto subscribe = "subscribe";
    static constexpr auto publish = "publish";
    static constexpr auto acknowledge = "acknowledge";
    static constexpr auto message = "message";
};

struct status {
    static constexpr auto ok = "ok";
    static constexpr auto error = "error";
};

inline std::string create_error_response(const std::exception& exception) {
    Data error;
    error[field::status] = status::error;
    error[field::message] = exception.what();
    return StringSerializer::serialize(error);
}

inline std::string create_ok_response() {
    Data response;
    response[field::status] = status::ok;

    return StringSerializer::serialize(response);
}

}  // namespace mqpp
#endif // ENDPOINTS_HPP