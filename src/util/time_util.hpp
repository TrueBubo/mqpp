#ifndef TIME_UTIL_HPP
#define TIME_UTIL_HPP
#include <chrono>
#include <string>

namespace mqpp {

class TimeUtil {
public:
    /**
     * Convert time_point to timestamp string (seconds since epoch)
     * @param time_point Instant wanted to be converted to string
     * @return timestamp string
     */
    static std::string time_point_to_string(std::chrono::system_clock::time_point time_point) {
        auto&& duration = time_point.time_since_epoch();
        auto&& seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        return std::to_string(seconds);
    }

    /**
     * Parse timestamp string to time_point
     * @param str String of the seconds from epoch
     * @return time point
     */
    static std::chrono::system_clock::time_point string_to_time_point(const std::string& str) {
        auto&& seconds = std::stoll(str);
        return std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
    }
};
}

#endif //TIME_UTIL_HPP