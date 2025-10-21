#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

/**
 * @brief Represents a group of users that can share expenses.
 */
class Group {
public:
    Group() = default;
    Group(std::string id, std::string name, std::vector<std::string> memberIds);

    const std::string &getId() const noexcept;
    const std::string &getName() const noexcept;
    const std::vector<std::string> &getMemberIds() const noexcept;

    bool hasMember(const std::string &userId) const;

    nlohmann::json toJson() const;
    static Group fromJson(const nlohmann::json &j);

private:
    std::string id_{};
    std::string name_{};
    std::vector<std::string> memberIds_{};
};

