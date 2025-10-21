#pragma once

#include <memory>
#include <string>

#include "split_strategy.hpp"

/**
 * @brief Factory for constructing split strategies by identifier.
 */
class SplitStrategyFactory {
public:
    /**
     * @brief Create a split strategy instance for the given type string.
     */
    static std::shared_ptr<SplitStrategy> create(const std::string &type);
};

