#ifndef MQ_UUID_UTIL_HPP
#define MQ_UUID_UTIL_HPP

#include <uuid.h>
#include <algorithm>

namespace mqpp {

class UuidUtil {
public:
    /**
     * Generate UUID
     * @return String form of the generated UUID
     */
    static std::string generate() {
        std::random_device rd;
        auto seed_data = std::array<int, std::mt19937::state_size>{};
        std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        std::mt19937 generator(seq);
        uuids::uuid_random_generator gen{generator};

        return uuids::to_string(gen());
    }
};

}
#endif //MQ_UUID_UTIL_HPP