#include "../third_party/catch2.hpp"

#include "split_strategy_factory.hpp"
#include "splitwise_manager.hpp"

#include <cstdio>
#include <limits>
#include <memory>

TEST_CASE("Balances reconcile across multiple expenses", "[manager]") {
    SplitwiseManager manager;
    manager.setNotifier(std::make_shared<ConsoleNotifier>());
    manager.setNotificationThreshold(1000.0);

    std::string alice = manager.addUser("Alice");
    std::string bob = manager.addUser("Bob");
    std::string carol = manager.addUser("Carol");
    std::string groupId = manager.addGroup("Friends", {alice, bob, carol});

    SplitInput dinner;
    dinner.payerId = alice;
    dinner.amount = 120.0;
    dinner.participantIds = {alice, bob, carol};
    auto equalStrategy = SplitStrategyFactory::create("equal");
    manager.addExpense(groupId, "Dinner", dinner, equalStrategy);

    SplitInput movie;
    movie.payerId = bob;
    movie.amount = 60.0;
    movie.participantIds = {alice, bob, carol};
    movie.percentShares = {30.0, 30.0, 40.0};
    auto percentStrategy = SplitStrategyFactory::create("percent");
    manager.addExpense(groupId, "Movie", movie, percentStrategy);

    SplitInput taxi;
    taxi.payerId = carol;
    taxi.amount = 45.0;
    taxi.participantIds = {bob, carol};
    taxi.exactShares = {20.0, 25.0};
    auto exactStrategy = SplitStrategyFactory::create("exact");
    manager.addExpense(groupId, "Taxi", taxi, exactStrategy);

    const auto &balances = manager.getAllBalances();
    double total = 0.0;
    for (const auto &[user, balance] : balances) {
        total += balance;
    }
    REQUIRE(total == Approx(0.0).margin(1e-6));
    REQUIRE(balances.at(alice) == Approx(62.0));
    REQUIRE(balances.at(bob) == Approx(-18.0));
    REQUIRE(balances.at(carol) == Approx(-44.0));

    auto settlements = manager.settleUpGreedy();
    double settlementTotal = 0.0;
    for (const auto &tx : settlements) {
        settlementTotal += tx.amount;
    }
    REQUIRE(settlementTotal >= 0.0);
}

TEST_CASE("Persistence round trip restores balances", "[manager][persistence]") {
    SplitwiseManager manager;
    manager.setNotifier(nullptr);
    manager.setNotificationThreshold(std::numeric_limits<double>::infinity());

    std::string alice = manager.addUser("Alice");
    std::string bob = manager.addUser("Bob");
    std::string groupId = manager.addGroup("Trip", {alice, bob});

    SplitInput hotel;
    hotel.payerId = alice;
    hotel.amount = 200.0;
    hotel.participantIds = {alice, bob};
    auto equal = SplitStrategyFactory::create("equal");
    manager.addExpense(groupId, "Hotel", hotel, equal);

    manager.saveToJson("test_data.json");

    SplitwiseManager loaded;
    loaded.loadFromJson("test_data.json");

    REQUIRE(loaded.getAllBalances().at(alice) == Approx(manager.getAllBalances().at(alice)));
    REQUIRE(loaded.getAllBalances().at(bob) == Approx(manager.getAllBalances().at(bob)));

    std::remove("test_data.json");
}

