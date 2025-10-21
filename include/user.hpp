#pragma once

#include <string>
#include <nlohmann/json.hpp>

/**
 * @brief Represents a user within the Splitwise++ system.
 */
class User {
public:
    User() = default;
    User(std::string id, std::string name);

    /**
     * @brief Get the identifier of the user.
     */
    const std::string &getId() const noexcept;

    /**
     * @brief Get the human readable name of the user.
     */
    const std::string &getName() const noexcept;

    /**
     * @brief Serialise the user into JSON.
     */
    nlohmann::json toJson() const;

    /**
     * @brief Create a user from JSON data.
     */
    static User fromJson(const nlohmann::json &j);

private:
    std::string id_{};
    std::string name_{};
};

