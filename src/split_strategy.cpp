#include "split_strategy.hpp"

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>

namespace {
constexpr double EPSILON = 1e-6;
}

BalanceSheet::BalanceMap EqualSplitStrategy::computeSplits(const SplitInput &input) const {
    if (input.participantIds.empty()) {
        throw std::invalid_argument("Equal split requires at least one participant");
    }
    if (input.amount < 0) {
        throw std::invalid_argument("Expense amount cannot be negative");
    }

    double share = input.amount / static_cast<double>(input.participantIds.size());
    BalanceSheet::BalanceMap delta;
    delta[input.payerId] += input.amount;
    for (const auto &participant : input.participantIds) {
        delta[participant] -= share;
    }
    return delta;
}

std::string EqualSplitStrategy::name() const { return "equal"; }

BalanceSheet::BalanceMap ExactSplitStrategy::computeSplits(const SplitInput &input) const {
    if (input.participantIds.size() != input.exactShares.size()) {
        throw std::invalid_argument("Exact split requires values for each participant");
    }
    double total = std::accumulate(input.exactShares.begin(), input.exactShares.end(), 0.0);
    if (std::abs(total - input.amount) > EPSILON) {
        throw std::invalid_argument("Exact split shares must sum to the total amount");
    }

    BalanceSheet::BalanceMap delta;
    delta[input.payerId] += input.amount;
    for (std::size_t i = 0; i < input.participantIds.size(); ++i) {
        delta[input.participantIds[i]] -= input.exactShares[i];
    }
    return delta;
}

std::string ExactSplitStrategy::name() const { return "exact"; }

BalanceSheet::BalanceMap PercentSplitStrategy::computeSplits(const SplitInput &input) const {
    if (input.participantIds.size() != input.percentShares.size()) {
        throw std::invalid_argument("Percent split requires percentages for each participant");
    }
    double totalPercent = std::accumulate(input.percentShares.begin(), input.percentShares.end(), 0.0);
    if (std::abs(totalPercent - 100.0) > EPSILON) {
        throw std::invalid_argument("Percent split shares must sum to 100");
    }

    BalanceSheet::BalanceMap delta;
    delta[input.payerId] += input.amount;
    for (std::size_t i = 0; i < input.participantIds.size(); ++i) {
        delta[input.participantIds[i]] -= input.amount * (input.percentShares[i] / 100.0);
    }
    return delta;
}

std::string PercentSplitStrategy::name() const { return "percent"; }

