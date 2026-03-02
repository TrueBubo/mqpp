#ifndef ENDPOINTS_HPP
#define ENDPOINTS_HPP

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
};

struct type {
    static constexpr auto subscribe = "subscribe";
    static constexpr auto publish = "publish";
    static constexpr auto acknowledge = "acknowledge";
};

struct status {
    static constexpr auto ok = "ok";
    static constexpr auto error = "error";
};

}  // namespace mqpp
#endif // ENDPOINTS_HPP