#pragma once

#include <map>
#include <string>
#include <nlohmann/json.hpp>

/**
 * @brief Aggregates the net balances for users within the system.
 */
class BalanceSheet {
public:
    using BalanceMap = std::map<std::string, double>;

    /**
     * @brief Apply a set of delta values to the balance sheet.
     */
    void applyDelta(const BalanceMap &delta);

    /**
     * @brief Reset all balances to zero.
     */
    void clear();

    /**
     * @brief Access all stored balances.
     */
    const BalanceMap &getBalances() const noexcept;

    /**
     * @brief Serialise the balance sheet.
     */
    nlohmann::json toJson() const;

    /**
     * @brief Populate the balance sheet from JSON data.
     */
    static BalanceSheet fromJson(const nlohmann::json &j);

private:
    BalanceMap balances_{};
};

