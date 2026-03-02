#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

namespace mqpp {
using Data = std::map<std::string, std::string>;
/**
 * String serialization utility
 * Format: key1=value1|key2=value2|key3=value3
 * Use \| to escape pipe character in values
 * Use \= to escape equals sign in values
 */
class StringSerializer {
public:
    static Data parse(const std::string& str) {
        Data result;
        std::string current_key;
        std::string current_value;
        bool reading_key = true;
        bool escaped = false;

        for (auto&& c : str) {
            if (escaped) {
                if (reading_key) current_key += c;
                else current_value += c;
                escaped = false;
                continue;
            }

            if (c == ESCAPE_CHAR) {
                escaped = true;
                continue;
            }

            if (c == KEY_VALUE_ASSOCIATOR_CHAR && reading_key) {
                reading_key = false;
            } else if (c == ENTRY_SEPARATOR_CHAR) {
                if (!current_key.empty()) {
                    result[current_key] = current_value;
                }
                current_key.clear();
                current_value.clear();
                reading_key = true;
            } else {
                if (reading_key) current_key += c; else current_value += c;
            }
        }

        if (!current_key.empty()) result[current_key] = current_value;

        return result;
    }

    static std::string serialize(const Data& data) {
        std::ostringstream oss;
        bool first = true;

        for (const auto& [key, value] : data) {
            if (!first) oss << ENTRY_SEPARATOR_CHAR;
            first = false;
            oss << escape(key) << KEY_VALUE_ASSOCIATOR_CHAR << escape(value);
        }

        return oss.str();
    }

    static std::string get(const Data& data,
                          const std::string& key,
                          const std::string& default_value = "") {
        auto&& entry = data.find(key);
        return (entry != data.end()) ? entry->second : default_value;
    }

    static std::string get_required(const Data& data,
                                    const std::string& key) {
        auto&& entry = data.find(key);
        if (entry == data.end()) {
            throw missing_required_field_exception(key);
        }
        return entry->second;
    }

    static std::string serialize_vector(const std::vector<std::string>& vec) {
        std::ostringstream oss;
        bool first = true;
        for (const auto& item : vec) {
            if (!first) oss << VECTOR_ITEM_SEPARATOR_CHAR;
            first = false;
            oss << escape_item_separators(item);
        }
        return oss.str();
    }

    static std::vector<std::string> parse_vector(const std::string& str) {
        std::vector<std::string> result;
        std::string current;
        bool escaped = false;

        for (char c : str) {
            if (escaped) {
                current += c;
                escaped = false;
                continue;
            }

            if (c == ESCAPE_CHAR) {
                escaped = true;
                continue;
            }

            if (c == VECTOR_ITEM_SEPARATOR_CHAR) {
                if (!current.empty()) {
                    result.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }

        if (!current.empty()) {
            result.push_back(current);
        }

        return result;
    }

private:
    static std::string escape(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (c == ENTRY_SEPARATOR_CHAR || c == KEY_VALUE_ASSOCIATOR_CHAR || c == ESCAPE_CHAR) {
                result += ESCAPE_CHAR;
            }
            result += c;
        }
        return result;
    }

    static std::string escape_item_separators(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (c == VECTOR_ITEM_SEPARATOR_CHAR || c == ESCAPE_CHAR) {
                result += ESCAPE_CHAR;
            }
            result += c;
        }
        return result;
    }

    static std::runtime_error missing_required_field_exception(const std::string& key) {
        return std::runtime_error("Missing required field: " + key);
    }

    static constexpr auto KEY_VALUE_ASSOCIATOR_CHAR = '=';
    static constexpr auto ENTRY_SEPARATOR_CHAR = '|';
    static constexpr auto ESCAPE_CHAR = '\\';

    static constexpr auto VECTOR_ITEM_SEPARATOR_CHAR = ',';
};

}  // namespace mqpp

#endif // STRING_UTILS_HPP