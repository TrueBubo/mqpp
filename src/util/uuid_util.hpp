#ifndef MQ_UUID_UTIL_HPP
#define MQ_UUID_UTIL_HPP

#include <uuid.h>

namespace mqpp {

class UuidUtil {
public:
    /**
     * Generate UUID
     * @return String form of the generated UUID
     */
    static std::string generate() {
        size_t seed = std::random_device()();
        std::mt19937 generator(seed);
        uuids::uuid_random_generator gen(generator);
        return uuids::to_string(gen());
    }
};

}
#endif //MQ_UUID_UTIL_HPP