#include "split_strategy_factory.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {
std::string normalise(std::string type) {
    std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return std::tolower(c); });
    return type;
}
}

std::shared_ptr<SplitStrategy> SplitStrategyFactory::create(const std::string &type) {
    std::string normalised = normalise(type);
    if (normalised == "equal") {
        return std::make_shared<EqualSplitStrategy>();
    }
    if (normalised == "exact") {
        return std::make_shared<ExactSplitStrategy>();
    }
    if (normalised == "percent") {
        return std::make_shared<PercentSplitStrategy>();
    }
    throw std::invalid_argument("Unknown split strategy type: " + type);
}

