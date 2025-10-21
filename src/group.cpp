#include "group.hpp"

#include <algorithm>

Group::Group(std::string id, std::string name, std::vector<std::string> memberIds)
    : id_(std::move(id)), name_(std::move(name)), memberIds_(std::move(memberIds)) {}

const std::string &Group::getId() const noexcept { return id_; }

const std::string &Group::getName() const noexcept { return name_; }

const std::vector<std::string> &Group::getMemberIds() const noexcept { return memberIds_; }

bool Group::hasMember(const std::string &userId) const {
    return std::find(memberIds_.begin(), memberIds_.end(), userId) != memberIds_.end();
}

nlohmann::json Group::toJson() const {
    nlohmann::json j;
    j["id"] = id_;
    j["name"] = name_;
    auto members = nlohmann::json::array();
    for (const auto &member : memberIds_) {
        members.push_back(member);
    }
    j["members"] = members;
    return j;
}

Group Group::fromJson(const nlohmann::json &j) {
    return Group{j.at("id").get<std::string>(),
                 j.at("name").get<std::string>(),
                 j.at("members").get<std::vector<std::string>>()};
}

