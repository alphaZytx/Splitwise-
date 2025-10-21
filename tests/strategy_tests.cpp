#define CATCH_CONFIG_MAIN
#include "../third_party/catch2.hpp"

#include "split_strategy.hpp"

TEST_CASE("Equal split distributes amounts evenly", "[strategy]") {
    EqualSplitStrategy strategy;
    SplitInput input;
    input.payerId = "payer";
    input.amount = 100.0;
    input.participantIds = {"payer", "a", "b", "c"};

    auto delta = strategy.computeSplits(input);
    REQUIRE(delta["payer"] == Approx(75.0));
    REQUIRE(delta["a"] == Approx(-25.0));
    REQUIRE(delta["b"] == Approx(-25.0));
    REQUIRE(delta["c"] == Approx(-25.0));
}

TEST_CASE("Exact split validates totals", "[strategy]") {
    ExactSplitStrategy strategy;
    SplitInput input;
    input.payerId = "payer";
    input.amount = 50.0;
    input.participantIds = {"payer", "friend"};
    input.exactShares = {25.0, 25.0};

    auto delta = strategy.computeSplits(input);
    REQUIRE(delta["payer"] == Approx(25.0));
    REQUIRE(delta["friend"] == Approx(-25.0));

    input.exactShares = {30.0, 10.0};
    REQUIRE_THROWS_AS(strategy.computeSplits(input), std::invalid_argument);
}

TEST_CASE("Percent split enforces percentages", "[strategy]") {
    PercentSplitStrategy strategy;
    SplitInput input;
    input.payerId = "payer";
    input.amount = 200.0;
    input.participantIds = {"payer", "friend"};
    input.percentShares = {40.0, 60.0};

    auto delta = strategy.computeSplits(input);
    REQUIRE(delta["payer"] == Approx(120.0));
    REQUIRE(delta["friend"] == Approx(-120.0));

    input.percentShares = {30.0};
    REQUIRE_THROWS_AS(strategy.computeSplits(input), std::invalid_argument);
}

TEST_CASE("Empty participants cause errors", "[strategy]") {
    EqualSplitStrategy equal;
    SplitInput input;
    REQUIRE_THROWS_AS(equal.computeSplits(input), std::invalid_argument);
}

