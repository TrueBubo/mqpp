#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <functional>

namespace mqpp {

class Message;

using MessageId = std::string;  // UUID

using MessageHandler = std::function<void(const Message&)>;

using UserId = std::string;
using CallbackUrl = std::string;

}  // namespace mqpp
#endif // TYPES_HPP