#include "balance_sheet.hpp"

#include <cmath>

void BalanceSheet::applyDelta(const BalanceMap &delta) {
    for (const auto &[userId, change] : delta) {
        balances_[userId] += change;
        if (std::abs(balances_[userId]) < 1e-9) {
            balances_[userId] = 0.0;
        }
    }
}

void BalanceSheet::clear() { balances_.clear(); }

const BalanceSheet::BalanceMap &BalanceSheet::getBalances() const noexcept { return balances_; }

nlohmann::json BalanceSheet::toJson() const {
    nlohmann::json j;
    for (const auto &[userId, balance] : balances_) {
        j[userId] = balance;
    }
    return j;
}

BalanceSheet BalanceSheet::fromJson(const nlohmann::json &j) {
    BalanceSheet sheet;
    sheet.balances_ = j.get<BalanceMap>();
    return sheet;
}

