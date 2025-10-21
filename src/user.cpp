#include "user.hpp"

User::User(std::string id, std::string name)
    : id_(std::move(id)), name_(std::move(name)) {}

const std::string &User::getId() const noexcept { return id_; }

const std::string &User::getName() const noexcept { return name_; }

nlohmann::json User::toJson() const {
    nlohmann::json j;
    j["id"] = id_;
    j["name"] = name_;
    return j;
}

User User::fromJson(const nlohmann::json &j) {
    return User{j.at("id").get<std::string>(), j.at("name").get<std::string>()};
}

