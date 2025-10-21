#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "split_strategy.hpp"

/**
 * @brief Represents an expense recorded in the system.
 */
class Expense {
public:
    Expense() = default;
    Expense(std::string id,
            std::string groupId,
            std::string description,
            SplitInput input,
            std::shared_ptr<SplitStrategy> strategy);

    const std::string &getId() const noexcept;
    const std::string &getGroupId() const noexcept;
    const std::string &getDescription() const noexcept;
    const SplitInput &getInput() const noexcept;
    const std::shared_ptr<SplitStrategy> &getStrategy() const noexcept;

    nlohmann::json toJson() const;
    static Expense fromJson(const nlohmann::json &j, const std::shared_ptr<SplitStrategy> &strategy);

private:
    std::string id_{};
    std::string groupId_{};
    std::string description_{};
    SplitInput input_{};
    std::shared_ptr<SplitStrategy> strategy_{};
};

