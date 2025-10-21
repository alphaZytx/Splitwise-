#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "balance_sheet.hpp"

/**
 * @brief Represents the input parameters for splitting an expense.
 */
struct SplitInput {
    std::string payerId;
    double amount{0.0};
    std::vector<std::string> participantIds;
    std::vector<double> exactShares;
    std::vector<double> percentShares;
};

/**
 * @brief Strategy interface describing how an expense amount is split.
 */
class SplitStrategy {
public:
    virtual ~SplitStrategy() = default;

    /**
     * @brief Calculate the balance delta to apply for the provided split input.
     */
    virtual BalanceSheet::BalanceMap computeSplits(const SplitInput &input) const = 0;

    /**
     * @brief Returns the human readable name for the strategy.
     */
    virtual std::string name() const = 0;
};

/**
 * @brief Evenly splits the expense across participants.
 */
class EqualSplitStrategy : public SplitStrategy {
public:
    BalanceSheet::BalanceMap computeSplits(const SplitInput &input) const override;
    std::string name() const override;
};

/**
 * @brief Splits the expense using exact values per participant.
 */
class ExactSplitStrategy : public SplitStrategy {
public:
    BalanceSheet::BalanceMap computeSplits(const SplitInput &input) const override;
    std::string name() const override;
};

/**
 * @brief Splits the expense using percentage values per participant.
 */
class PercentSplitStrategy : public SplitStrategy {
public:
    BalanceSheet::BalanceMap computeSplits(const SplitInput &input) const override;
    std::string name() const override;
};

