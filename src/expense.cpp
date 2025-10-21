#include "expense.hpp"

Expense::Expense(std::string id,
                 std::string groupId,
                 std::string description,
                 SplitInput input,
                 std::shared_ptr<SplitStrategy> strategy)
    : id_(std::move(id)),
      groupId_(std::move(groupId)),
      description_(std::move(description)),
      input_(std::move(input)),
      strategy_(std::move(strategy)) {}

const std::string &Expense::getId() const noexcept { return id_; }

const std::string &Expense::getGroupId() const noexcept { return groupId_; }

const std::string &Expense::getDescription() const noexcept { return description_; }

const SplitInput &Expense::getInput() const noexcept { return input_; }

const std::shared_ptr<SplitStrategy> &Expense::getStrategy() const noexcept { return strategy_; }

nlohmann::json Expense::toJson() const {
    nlohmann::json j;
    j["id"] = id_;
    j["groupId"] = groupId_;
    j["description"] = description_;
    j["payerId"] = input_.payerId;
    j["amount"] = input_.amount;
    auto participants = nlohmann::json::array();
    for (const auto &participant : input_.participantIds) {
        participants.push_back(participant);
    }
    j["participants"] = participants;
    auto exactShares = nlohmann::json::array();
    for (double share : input_.exactShares) {
        exactShares.push_back(share);
    }
    j["exactShares"] = exactShares;
    auto percentShares = nlohmann::json::array();
    for (double share : input_.percentShares) {
        percentShares.push_back(share);
    }
    j["percentShares"] = percentShares;
    j["strategy"] = strategy_ ? strategy_->name() : std::string{};
    return j;
}

Expense Expense::fromJson(const nlohmann::json &j, const std::shared_ptr<SplitStrategy> &strategy) {
    SplitInput input;
    input.payerId = j.at("payerId").get<std::string>();
    input.amount = j.at("amount").get<double>();
    input.participantIds = j.at("participants").get<std::vector<std::string>>();
    if (j.contains("exactShares")) {
        input.exactShares = j.at("exactShares").get<std::vector<double>>();
    }
    if (j.contains("percentShares")) {
        input.percentShares = j.at("percentShares").get<std::vector<double>>();
    }
    return Expense{j.at("id").get<std::string>(),
                   j.at("groupId").get<std::string>(),
                   j.value("description", std::string{}),
                   input,
                   strategy};
}

