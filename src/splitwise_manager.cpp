#include "splitwise_manager.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

namespace {
constexpr double EPSILON = 1e-6;
}

SplitwiseManager::SplitwiseManager() = default;

std::string SplitwiseManager::addUser(const std::string &name) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string id = generateId("USR");
    users_.emplace(id, User{id, name});
    return id;
}

std::string SplitwiseManager::addGroup(const std::string &name, const std::vector<std::string> &memberIds) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &member : memberIds) {
        if (!users_.count(member)) {
            throw std::invalid_argument("Unknown user id: " + member);
        }
    }
    std::string id = generateId("GRP");
    groups_.emplace(id, Group{id, name, memberIds});
    return id;
}

std::string SplitwiseManager::addExpense(const std::string &groupId,
                                         const std::string &description,
                                         const SplitInput &input,
                                         const std::shared_ptr<SplitStrategy> &strategy) {
    if (!strategy) {
        throw std::invalid_argument("Strategy must not be null");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!groups_.count(groupId)) {
        throw std::invalid_argument("Unknown group id: " + groupId);
    }
    const auto &group = groups_.at(groupId);
    if (input.participantIds.empty()) {
        throw std::invalid_argument("Expense must include at least one participant");
    }
    if (!group.hasMember(input.payerId)) {
        throw std::invalid_argument("Payer must be part of the group");
    }
    if (std::find(input.participantIds.begin(), input.participantIds.end(), input.payerId) ==
        input.participantIds.end()) {
        throw std::invalid_argument("Participants must include the payer");
    }
    for (const auto &participant : input.participantIds) {
        if (!group.hasMember(participant)) {
            throw std::invalid_argument("Participant not in group: " + participant);
        }
    }

    BalanceSheet::BalanceMap delta = strategy->computeSplits(input);
    std::string id = generateId("EXP");
    expenses_.emplace(id, Expense{id, groupId, description, input, strategy});
    balanceSheet_.applyDelta(delta);

    if (notifier_ && input.amount > notificationThreshold_) {
        notifier_->notifyLargeExpense(expenses_.at(id), notificationThreshold_);
    }

    return id;
}

const std::map<std::string, User> &SplitwiseManager::getUsers() const noexcept { return users_; }

const std::map<std::string, Group> &SplitwiseManager::getGroups() const noexcept { return groups_; }

const std::map<std::string, Expense> &SplitwiseManager::getExpenses() const noexcept { return expenses_; }

const BalanceSheet::BalanceMap &SplitwiseManager::getAllBalances() const noexcept {
    return balanceSheet_.getBalances();
}

void SplitwiseManager::saveToJson(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json j;
    j["users"] = nlohmann::json::array();
    for (const auto &[id, user] : users_) {
        j["users"].push_back(user.toJson());
    }
    j["groups"] = nlohmann::json::array();
    for (const auto &[id, group] : groups_) {
        j["groups"].push_back(group.toJson());
    }
    j["expenses"] = nlohmann::json::array();
    for (const auto &[id, expense] : expenses_) {
        j["expenses"].push_back(expense.toJson());
    }
    j["balances"] = balanceSheet_.toJson();

    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Failed to open file for writing: " + path);
    }
    out << std::setw(2) << j;
}

void SplitwiseManager::loadFromJson(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open file for reading: " + path);
    }
    nlohmann::json j;
    in >> j;

    if (!j.is_object()) {
        throw std::runtime_error("Invalid JSON format: expected an object at the root");
    }

    auto ensureArray = [](const nlohmann::json &value, const std::string &name) {
        if (!value.is_array()) {
            throw std::runtime_error("Invalid JSON format: '" + name + "' must be an array");
        }
    };

    for (const auto &key : {"users", "groups", "expenses"}) {
        if (!j.contains(key)) {
            throw std::runtime_error("Invalid JSON: missing key '" + std::string(key) + "'");
        }
        ensureArray(j.at(key), key);
    }

    users_.clear();
    groups_.clear();
    expenses_.clear();
    balanceSheet_.clear();

    for (const auto &userJson : j.at("users")) {
        User user = User::fromJson(userJson);
        users_.emplace(user.getId(), user);
    }
    for (const auto &groupJson : j.at("groups")) {
        Group group = Group::fromJson(groupJson);
        for (const auto &member : group.getMemberIds()) {
            if (!users_.count(member)) {
                throw std::runtime_error("Group '" + group.getId() + "' references unknown user '" + member + "'");
            }
        }
        groups_.emplace(group.getId(), group);
    }
    for (const auto &expenseJson : j.at("expenses")) {
        std::string strategyType = expenseJson.at("strategy").get<std::string>();
        auto strategy = SplitStrategyFactory::create(strategyType);
        Expense expense = Expense::fromJson(expenseJson, strategy);
        const auto groupIt = groups_.find(expense.getGroupId());
        if (groupIt == groups_.end()) {
            throw std::runtime_error("Expense '" + expense.getId() + "' references unknown group");
        }
        const auto &group = groupIt->second;
        const auto &input = expense.getInput();
        if (!users_.count(input.payerId)) {
            throw std::runtime_error("Expense '" + expense.getId() + "' references unknown payer");
        }
        if (input.participantIds.empty()) {
            throw std::runtime_error("Expense '" + expense.getId() + "' must include participants");
        }
        if (std::find(input.participantIds.begin(), input.participantIds.end(), input.payerId) ==
            input.participantIds.end()) {
            throw std::runtime_error("Expense '" + expense.getId() + "' participants must include payer");
        }
        for (const auto &participant : input.participantIds) {
            if (!group.hasMember(participant)) {
                throw std::runtime_error(
                    "Expense '" + expense.getId() + "' includes participant not in group: " + participant);
            }
        }
        expenses_.emplace(expense.getId(), expense);
    }

    auto updateCounter = [this](const auto &container, const std::string &prefix) {
        std::size_t maxCounter = 0;
        for (const auto &entry : container) {
            const std::string &id = entry.first;
            if (id.rfind(prefix, 0) == 0) {
                try {
                    std::size_t value = std::stoul(id.substr(prefix.size()));
                    maxCounter = std::max(maxCounter, value);
                } catch (const std::exception &) {
                    // Ignore ids that do not follow the expected pattern.
                }
            }
        }
        counters_[prefix] = maxCounter;
    };
    updateCounter(users_, "USR");
    updateCounter(groups_, "GRP");
    updateCounter(expenses_, "EXP");

    recomputeBalances();
}

std::vector<SettlementTransaction> SplitwiseManager::settleUpGreedy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    struct Entry {
        std::string userId;
        double amount;
    };

    std::vector<Entry> creditors;
    std::vector<Entry> debtors;
    for (const auto &[userId, balance] : balanceSheet_.getBalances()) {
        if (balance > EPSILON) {
            creditors.push_back({userId, balance});
        } else if (balance < -EPSILON) {
            debtors.push_back({userId, balance});
        }
    }

    auto creditorCmp = [](const Entry &a, const Entry &b) { return a.amount < b.amount; };
    auto debtorCmp = [](const Entry &a, const Entry &b) { return a.amount > b.amount; };
    std::priority_queue<Entry, std::vector<Entry>, decltype(creditorCmp)> creditorQueue(creditorCmp, creditors);
    std::priority_queue<Entry, std::vector<Entry>, decltype(debtorCmp)> debtorQueue(debtorCmp, debtors);

    std::vector<SettlementTransaction> result;
    while (!creditorQueue.empty() && !debtorQueue.empty()) {
        Entry creditor = creditorQueue.top();
        creditorQueue.pop();
        Entry debtor = debtorQueue.top();
        debtorQueue.pop();

        double settlement = std::min(creditor.amount, -debtor.amount);
        creditor.amount -= settlement;
        debtor.amount += settlement;
        result.push_back({debtor.userId, creditor.userId, settlement});

        if (creditor.amount > EPSILON) {
            creditorQueue.push(creditor);
        }
        if (debtor.amount < -EPSILON) {
            debtorQueue.push(debtor);
        }
    }
    return result;
}

void SplitwiseManager::setNotifier(std::shared_ptr<INotifier> notifier) {
    std::lock_guard<std::mutex> lock(mutex_);
    notifier_ = std::move(notifier);
}

void SplitwiseManager::setNotificationThreshold(double threshold) {
    std::lock_guard<std::mutex> lock(mutex_);
    notificationThreshold_ = threshold;
}

void ConsoleNotifier::notifyLargeExpense(const Expense &expense, double threshold) {
    std::cout << "[Alert] Expense '" << expense.getDescription() << "' exceeded threshold " << threshold
              << "\n";
}

std::string SplitwiseManager::generateId(const std::string &prefix) {
    std::size_t count = ++counters_[prefix];
    return prefix + std::to_string(count);
}

void SplitwiseManager::recomputeBalances() {
    balanceSheet_.clear();
    for (const auto &[id, expense] : expenses_) {
        balanceSheet_.applyDelta(expense.getStrategy()->computeSplits(expense.getInput()));
    }
}