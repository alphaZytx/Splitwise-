#pragma once

#include <cctype>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace nlohmann {

class json {
public:
    using object_t = std::map<std::string, json>;
    using array_t = std::vector<json>;
    using variant_t = std::variant<std::nullptr_t, bool, double, std::string, array_t, object_t>;

    json() : data_(nullptr) {}
    json(std::nullptr_t) : data_(nullptr) {}
    json(bool value) : data_(value) {}
    json(double value) : data_(value) {}
    json(int value) : data_(static_cast<double>(value)) {}
    json(const char *value) : data_(std::string(value)) {}
    json(std::string value) : data_(std::move(value)) {}
    json(array_t value) : data_(std::move(value)) {}
    json(object_t value) : data_(std::move(value)) {}

    json(std::initializer_list<std::pair<const std::string, json>> init) : data_(object_t{}) {
        auto &obj = std::get<object_t>(data_);
        for (const auto &item : init) {
            obj[item.first] = item.second;
        }
    }

    static json array() { return json(array_t{}); }

    bool is_object() const { return std::holds_alternative<object_t>(data_); }
    bool is_array() const { return std::holds_alternative<array_t>(data_); }
    bool is_string() const { return std::holds_alternative<std::string>(data_); }

    json &operator[](const std::string &key) {
        ensure_object();
        return std::get<object_t>(data_)[key];
    }

    const json &at(const std::string &key) const {
        if (!is_object()) {
            throw std::out_of_range("json value is not an object");
        }
        const auto &obj = std::get<object_t>(data_);
        auto it = obj.find(key);
        if (it == obj.end()) {
            throw std::out_of_range("key not found: " + key);
        }
        return it->second;
    }

    json &at(const std::string &key) {
        return const_cast<json &>(std::as_const(*this).at(key));
    }

    bool contains(const std::string &key) const {
        if (!is_object()) {
            return false;
        }
        const auto &obj = std::get<object_t>(data_);
        return obj.find(key) != obj.end();
    }

    void push_back(const json &value) {
        ensure_array();
        std::get<array_t>(data_).push_back(value);
    }

    array_t::const_iterator begin() const {
        if (!is_array()) {
            throw std::logic_error("json value is not an array");
        }
        return std::get<array_t>(data_).begin();
    }

    array_t::const_iterator end() const {
        if (!is_array()) {
            throw std::logic_error("json value is not an array");
        }
        return std::get<array_t>(data_).end();
    }

    template <typename T> T get() const;

    template <typename T> T value(const std::string &key, T defaultValue) const {
        if (!contains(key)) {
            return defaultValue;
        }
        return at(key).get<T>();
    }

    std::string dump(int indent = -1) const {
        std::ostringstream oss;
        dumpInternal(oss, indent, 0);
        return oss.str();
    }

    friend std::ostream &operator<<(std::ostream &os, const json &j) {
        int indent = static_cast<int>(os.width());
        if (indent > 0) {
            os << j.dump(indent);
            os.width(0);
        } else {
            os << j.dump();
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, json &j) {
        std::ostringstream buffer;
        buffer << is.rdbuf();
        std::string content = buffer.str();
        std::size_t pos = 0;
        j = parseValue(content, pos);
        skipWhitespace(content, pos);
        if (pos != content.size()) {
            throw std::runtime_error("Unexpected trailing data in JSON");
        }
        return is;
    }

private:
    variant_t data_;

    void ensure_object() {
        if (!is_object()) {
            if (std::holds_alternative<std::nullptr_t>(data_)) {
                data_ = object_t{};
            } else {
                throw std::logic_error("json value is not an object");
            }
        }
    }

    void ensure_array() {
        if (!is_array()) {
            if (std::holds_alternative<std::nullptr_t>(data_)) {
                data_ = array_t{};
            } else {
                throw std::logic_error("json value is not an array");
            }
        }
    }

    static void skipWhitespace(const std::string &s, std::size_t &pos) {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
            ++pos;
        }
    }

    static json parseValue(const std::string &s, std::size_t &pos) {
        skipWhitespace(s, pos);
        if (pos >= s.size()) {
            throw std::runtime_error("Unexpected end of JSON");
        }
        char c = s[pos];
        if (c == 'n') {
            if (s.compare(pos, 4, "null") == 0) {
                pos += 4;
                return json(nullptr);
            }
        } else if (c == 't') {
            if (s.compare(pos, 4, "true") == 0) {
                pos += 4;
                return json(true);
            }
        } else if (c == 'f') {
            if (s.compare(pos, 5, "false") == 0) {
                pos += 5;
                return json(false);
            }
        } else if (c == '"') {
            return json(parseString(s, pos));
        } else if (c == '[') {
            return parseArray(s, pos);
        } else if (c == '{') {
            return parseObject(s, pos);
        } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return json(parseNumber(s, pos));
        }
        throw std::runtime_error("Invalid JSON value");
    }

    static std::string parseString(const std::string &s, std::size_t &pos) {
        if (s[pos] != '"') {
            throw std::runtime_error("Expected string");
        }
        ++pos;
        std::string result;
        while (pos < s.size()) {
            char c = s[pos++];
            if (c == '"') {
                return result;
            }
            if (c == '\\') {
                if (pos >= s.size()) {
                    throw std::runtime_error("Invalid escape sequence");
                }
                char esc = s[pos++];
                switch (esc) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                case 'u': result += parseUnicode(s, pos); break;
                default: throw std::runtime_error("Unsupported escape sequence");
                }
            } else {
                result.push_back(c);
            }
        }
        throw std::runtime_error("Unterminated string");
    }

    static std::string parseUnicode(const std::string &s, std::size_t &pos) {
        if (pos + 4 > s.size()) {
            throw std::runtime_error("Invalid unicode escape");
        }
        unsigned int code = 0;
        for (int i = 0; i < 4; ++i) {
            char c = s[pos++];
            code <<= 4;
            if (c >= '0' && c <= '9') {
                code |= static_cast<unsigned int>(c - '0');
            } else if (c >= 'A' && c <= 'F') {
                code |= static_cast<unsigned int>(c - 'A' + 10);
            } else if (c >= 'a' && c <= 'f') {
                code |= static_cast<unsigned int>(c - 'a' + 10);
            } else {
                throw std::runtime_error("Invalid unicode escape");
            }
        }
        std::string result;
        if (code <= 0x7F) {
            result.push_back(static_cast<char>(code));
        } else if (code <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 | ((code >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xE0 | ((code >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
        return result;
    }

    static double parseNumber(const std::string &s, std::size_t &pos) {
        std::size_t start = pos;
        if (s[pos] == '-') {
            ++pos;
        }
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
            ++pos;
        }
        if (pos < s.size() && s[pos] == '.') {
            ++pos;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
                ++pos;
            }
        }
        if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
            ++pos;
            if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) {
                ++pos;
            }
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
                ++pos;
            }
        }
        double value = std::strtod(s.c_str() + start, nullptr);
        return value;
    }

    static json parseArray(const std::string &s, std::size_t &pos) {
        if (s[pos] != '[') {
            throw std::runtime_error("Expected array");
        }
        ++pos;
        json result = array();
        skipWhitespace(s, pos);
        if (pos < s.size() && s[pos] == ']') {
            ++pos;
            return result;
        }
        while (true) {
            result.push_back(parseValue(s, pos));
            skipWhitespace(s, pos);
            if (pos >= s.size()) {
                throw std::runtime_error("Unterminated array");
            }
            if (s[pos] == ',') {
                ++pos;
                continue;
            }
            if (s[pos] == ']') {
                ++pos;
                break;
            }
            throw std::runtime_error("Expected ',' or ']' in array");
        }
        return result;
    }

    static json parseObject(const std::string &s, std::size_t &pos) {
        if (s[pos] != '{') {
            throw std::runtime_error("Expected object");
        }
        ++pos;
        json result(object_t{});
        skipWhitespace(s, pos);
        if (pos < s.size() && s[pos] == '}') {
            ++pos;
            return result;
        }
        while (true) {
            skipWhitespace(s, pos);
            std::string key = parseString(s, pos);
            skipWhitespace(s, pos);
            if (pos >= s.size() || s[pos] != ':') {
                throw std::runtime_error("Expected ':' in object");
            }
            ++pos;
            json value = parseValue(s, pos);
            result[key] = value;
            skipWhitespace(s, pos);
            if (pos >= s.size()) {
                throw std::runtime_error("Unterminated object");
            }
            if (s[pos] == ',') {
                ++pos;
                continue;
            }
            if (s[pos] == '}') {
                ++pos;
                break;
            }
            throw std::runtime_error("Expected ',' or '}' in object");
        }
        return result;
    }

    void dumpInternal(std::ostream &os, int indent, int depth) const {
        if (std::holds_alternative<std::nullptr_t>(data_)) {
            os << "null";
        } else if (std::holds_alternative<bool>(data_)) {
            os << (std::get<bool>(data_) ? "true" : "false");
        } else if (std::holds_alternative<double>(data_)) {
            os << std::get<double>(data_);
        } else if (std::holds_alternative<std::string>(data_)) {
            os << '"' << escape(std::get<std::string>(data_)) << '"';
        } else if (std::holds_alternative<array_t>(data_)) {
            const auto &arr = std::get<array_t>(data_);
            os << '[';
            if (!arr.empty()) {
                if (indent > 0) {
                    os << '\n';
                }
                for (std::size_t i = 0; i < arr.size(); ++i) {
                    if (indent > 0) {
                        os << std::string((depth + 1) * indent, ' ');
                    }
                    arr[i].dumpInternal(os, indent, depth + 1);
                    if (i + 1 < arr.size()) {
                        os << ',';
                    }
                    if (indent > 0) {
                        os << '\n';
                    }
                }
                if (indent > 0) {
                    os << std::string(depth * indent, ' ');
                }
            }
            os << ']';
        } else {
            const auto &obj = std::get<object_t>(data_);
            os << '{';
            if (!obj.empty()) {
                if (indent > 0) {
                    os << '\n';
                }
                std::size_t count = 0;
                for (const auto &kv : obj) {
                    if (indent > 0) {
                        os << std::string((depth + 1) * indent, ' ');
                    }
                    os << '"' << escape(kv.first) << '"' << ':';
                    if (indent > 0) {
                        os << ' ';
                    }
                    kv.second.dumpInternal(os, indent, depth + 1);
                    if (++count < obj.size()) {
                        os << ',';
                    }
                    if (indent > 0) {
                        os << '\n';
                    }
                }
                if (indent > 0) {
                    os << std::string(depth * indent, ' ');
                }
            }
            os << '}';
        }
    }

    static std::string escape(const std::string &value) {
        std::ostringstream oss;
        for (char c : value) {
            switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\u" << std::hex << std::uppercase << static_cast<int>(c);
                } else {
                    oss << c;
                }
            }
        }
        return oss.str();
    }
};

inline json operator"" _json(const char *str, std::size_t) {
    std::istringstream iss(str);
    json j;
    iss >> j;
    return j;
}

template <> inline std::string json::get<std::string>() const {
    if (!is_string()) {
        throw std::logic_error("json value is not a string");
    }
    return std::get<std::string>(data_);
}

template <> inline double json::get<double>() const {
    if (!std::holds_alternative<double>(data_)) {
        throw std::logic_error("json value is not a number");
    }
    return std::get<double>(data_);
}

template <> inline int json::get<int>() const {
    return static_cast<int>(get<double>());
}

template <> inline bool json::get<bool>() const {
    if (!std::holds_alternative<bool>(data_)) {
        throw std::logic_error("json value is not a boolean");
    }
    return std::get<bool>(data_);
}

template <> inline json json::get<json>() const { return *this; }

template <> inline std::vector<std::string> json::get<std::vector<std::string>>() const {
    if (!is_array()) {
        throw std::logic_error("json value is not an array");
    }
    std::vector<std::string> result;
    for (const auto &item : std::get<array_t>(data_)) {
        result.push_back(item.get<std::string>());
    }
    return result;
}

template <> inline std::vector<double> json::get<std::vector<double>>() const {
    if (!is_array()) {
        throw std::logic_error("json value is not an array");
    }
    std::vector<double> result;
    for (const auto &item : std::get<array_t>(data_)) {
        result.push_back(item.get<double>());
    }
    return result;
}

template <> inline std::map<std::string, double> json::get<std::map<std::string, double>>() const {
    if (!is_object()) {
        throw std::logic_error("json value is not an object");
    }
    std::map<std::string, double> result;
    for (const auto &kv : std::get<object_t>(data_)) {
        result[kv.first] = kv.second.get<double>();
    }
    return result;
}

} // namespace nlohmann

