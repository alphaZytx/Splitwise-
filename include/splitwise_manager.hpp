#pragma once

#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "balance_sheet.hpp"
#include "expense.hpp"
#include "group.hpp"
#include "split_strategy_factory.hpp"
#include "user.hpp"

/**
 * @brief Interface for notification observers.
 */
class INotifier {
public:
    virtual ~INotifier() = default;
    virtual void notifyLargeExpense(const Expense &expense, double threshold) = 0;
};

/**
 * @brief Notifier implementation that prints to stdout.
 */
class ConsoleNotifier : public INotifier {
public:
    void notifyLargeExpense(const Expense &expense, double threshold) override;
};

/**
 * @brief Represents a settlement transfer between two users.
 */
struct SettlementTransaction {
    std::string fromUserId;
    std::string toUserId;
    double amount{0.0};
};

/**
 * @brief Central orchestrator responsible for managing users, groups and expenses.
 */
class SplitwiseManager {
public:
    SplitwiseManager();

    /**
     * @brief Add a new user to the system.
     */
    std::string addUser(const std::string &name);

    /**
     * @brief Add a new group with the provided members.
     */
    std::string addGroup(const std::string &name, const std::vector<std::string> &memberIds);

    /**
     * @brief Record a new expense and update balances according to the strategy.
     */
    std::string addExpense(const std::string &groupId,
                           const std::string &description,
                           const SplitInput &input,
                           const std::shared_ptr<SplitStrategy> &strategy);

    /**
     * @brief Access users.
     */
    const std::map<std::string, User> &getUsers() const noexcept;

    /**
     * @brief Access groups.
     */
    const std::map<std::string, Group> &getGroups() const noexcept;

    /**
     * @brief Access expenses.
     */
    const std::map<std::string, Expense> &getExpenses() const noexcept;

    /**
     * @brief Retrieve all balances.
     */
    const BalanceSheet::BalanceMap &getAllBalances() const noexcept;

    /**
     * @brief Save the current state to a JSON file.
     */
    void saveToJson(const std::string &path);

    /**
     * @brief Load the state from a JSON file.
     */
    void loadFromJson(const std::string &path);

    /**
     * @brief Compute settlement transactions using a greedy strategy.
     */
    std::vector<SettlementTransaction> settleUpGreedy() const;

    /**
     * @brief Configure an observer notifier.
     */
    void setNotifier(std::shared_ptr<INotifier> notifier);

    /**
     * @brief Update the large expense notification threshold.
     */
    void setNotificationThreshold(double threshold);

private:
    std::string generateId(const std::string &prefix);
    void recomputeBalances();

    mutable std::mutex mutex_{};
    std::map<std::string, User> users_{};
    std::map<std::string, Group> groups_{};
    std::map<std::string, Expense> expenses_{};
    BalanceSheet balanceSheet_{};
    std::shared_ptr<INotifier> notifier_{};
    double notificationThreshold_{std::numeric_limits<double>::infinity()};
    std::map<std::string, std::size_t> counters_{};
};

