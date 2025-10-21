#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "split_strategy_factory.hpp"
#include "splitwise_manager.hpp"

namespace {
void printMenu() {
    std::cout << "\nSplitwise++ Menu\n"
              << "1. Add user\n"
              << "2. List users\n"
              << "3. Create group\n"
              << "4. List groups\n"
              << "5. Add expense\n"
              << "6. Show balances\n"
              << "7. Settle up (greedy)\n"
              << "8. Save to JSON\n"
              << "9. Load from JSON\n"
              << "10. Exit\n"
              << "Select option: "
              << std::flush;
}

std::vector<std::string> readIds(const std::string &prompt) {
    std::cout << prompt << " (space separated): ";
    std::string line;
    std::getline(std::cin, line);
    std::istringstream iss(line);
    std::vector<std::string> ids;
    std::string token;
    while (iss >> token) {
        ids.push_back(token);
    }
    return ids;
}

void printUsers(const SplitwiseManager &manager) {
    const auto &users = manager.getUsers();
    if (users.empty()) {
        std::cout << "No users have been created yet.\n";
        return;
    }
    for (const auto &[id, user] : users) {
        std::cout << "  " << id << ": " << user.getName() << "\n";
    }
}

void printGroups(const SplitwiseManager &manager) {
    const auto &groups = manager.getGroups();
    if (groups.empty()) {
        std::cout << "No groups have been created yet.\n";
        return;
    }
    const auto &users = manager.getUsers();
    for (const auto &[id, group] : groups) {
        std::cout << "  " << id << ": " << group.getName() << "\n";
        std::cout << "     members: ";
        if (group.getMemberIds().empty()) {
            std::cout << "<none>\n";
            continue;
        }
        bool first = true;
        for (const auto &memberId : group.getMemberIds()) {
            if (!first) {
                std::cout << ", ";
            }
            first = false;
            auto it = users.find(memberId);
            if (it != users.end()) {
                std::cout << it->second.getName() << " (" << memberId << ")";
            } else {
                std::cout << memberId;
            }
        }
        std::cout << "\n";
    }
}

double readAmount(const std::string &prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    try {
        double value = std::stod(line);
        if (value < 0.0) {
            throw std::invalid_argument("Amount cannot be negative");
        }
        return value;
    } catch (const std::exception &) {
        throw std::invalid_argument("Amount must be a non-negative number");
    }
}
}

int main() {
    SplitwiseManager manager;
    manager.setNotifier(std::make_shared<ConsoleNotifier>());
    manager.setNotificationThreshold(std::numeric_limits<double>::infinity());

    bool running = true;
    while (running) {
        printMenu();
        std::string choiceLine;
        if (!std::getline(std::cin, choiceLine)) {
            break;
        }
        int choice = 0;
        try {
            choice = std::stoi(choiceLine);
        } catch (...) {
            std::cout << "Invalid selection.\n";
            continue;
        }

        try {
            switch (choice) {
            case 1: {
                std::cout << "Enter user name: ";
                std::string name;
                std::getline(std::cin, name);
                if (name.empty()) {
                    std::cout << "Name cannot be empty.\n";
                    break;
                }
                std::string userId = manager.addUser(name);
                std::cout << "Created user with id: " << userId << "\n";
                break;
            }
            case 2: {
                printUsers(manager);
                break;
            }
            case 3: {
                if (manager.getUsers().empty()) {
                    std::cout << "Add users before creating groups.\n";
                    break;
                }
                std::cout << "Existing users:\n";
                printUsers(manager);
                std::cout << "Enter group name: ";
                std::string name;
                std::getline(std::cin, name);
                auto memberIds = readIds("Enter member IDs");
                if (memberIds.empty()) {
                    std::cout << "Group must have at least one member.\n";
                    break;
                }
                std::string groupId = manager.addGroup(name, memberIds);
                std::cout << "Created group with id: " << groupId << "\n";
                break;
            }
            case 4: {
                printGroups(manager);
                break;
            }
            case 5: {
                if (manager.getGroups().empty()) {
                    std::cout << "Create a group first.\n";
                    break;
                }
                std::cout << "Available groups:\n";
                printGroups(manager);
                std::cout << "Enter group id: ";
                std::string groupId;
                std::getline(std::cin, groupId);
                std::cout << "Enter payer id: ";
                std::string payerId;
                std::getline(std::cin, payerId);
                double amount = readAmount("Enter amount: ");
                std::cout << "Enter description: ";
                std::string description;
                std::getline(std::cin, description);
                std::cout << "Enter strategy (equal/exact/percent): ";
                std::string strategyType;
                std::getline(std::cin, strategyType);
                auto itGroup = manager.getGroups().find(groupId);
                if (itGroup == manager.getGroups().end()) {
                    std::cout << "Unknown group id.\n";
                    break;
                }
                auto participants = readIds("Enter participant IDs (leave blank for entire group)");
                if (participants.empty()) {
                    participants = itGroup->second.getMemberIds();
                }
                if (std::find(participants.begin(), participants.end(), payerId) == participants.end()) {
                    participants.push_back(payerId);
                }

                SplitInput input;
                input.payerId = payerId;
                input.amount = amount;
                input.participantIds = participants;

                if (strategyType == "exact") {
                    std::cout << "Enter exact shares (" << participants.size() << "): ";
                    std::string sharesLine;
                    std::getline(std::cin, sharesLine);
                    std::istringstream ss(sharesLine);
                    double value;
                    while (ss >> value) {
                        input.exactShares.push_back(value);
                    }
                } else if (strategyType == "percent") {
                    std::cout << "Enter percentage shares (" << participants.size() << "): ";
                    std::string sharesLine;
                    std::getline(std::cin, sharesLine);
                    std::istringstream ss(sharesLine);
                    double value;
                    while (ss >> value) {
                        input.percentShares.push_back(value);
                    }
                }

                auto strategy = SplitStrategyFactory::create(strategyType);
                std::string expenseId = manager.addExpense(groupId, description, input, strategy);
                std::cout << "Expense recorded with id: " << expenseId << "\n";
                break;
            }
            case 6: {
                const auto &balances = manager.getAllBalances();
                if (balances.empty()) {
                    std::cout << "No balances yet.\n";
                    break;
                }
                std::cout << std::fixed << std::setprecision(2);
                for (const auto &[userId, balance] : balances) {
                    auto it = manager.getUsers().find(userId);
                    const std::string &name = it != manager.getUsers().end() ? it->second.getName() : userId;
                    std::cout << name << " (" << userId << "): " << balance << "\n";
                }
                break;
            }
            case 7: {
                auto settlements = manager.settleUpGreedy();
                if (settlements.empty()) {
                    std::cout << "Nothing to settle.\n";
                    break;
                }
                std::cout << std::fixed << std::setprecision(2);
                for (const auto &tx : settlements) {
                    const auto &users = manager.getUsers();
                    std::string fromName = users.count(tx.fromUserId) ? users.at(tx.fromUserId).getName() : tx.fromUserId;
                    std::string toName = users.count(tx.toUserId) ? users.at(tx.toUserId).getName() : tx.toUserId;
                    std::cout << fromName << " -> " << toName << ": " << tx.amount << "\n";
                }
                break;
            }
            case 8: {
                std::cout << "Enter file path to save: ";
                std::string path;
                std::getline(std::cin, path);
                manager.saveToJson(path);
                std::cout << "State saved.\n";
                break;
            }
            case 9: {
                std::cout << "Enter file path to load: ";
                std::string path;
                std::getline(std::cin, path);
                manager.loadFromJson(path);
                std::cout << "State loaded.\n";
                break;
            }
            case 10:
                running = false;
                break;
            default:
                std::cout << "Invalid option.\n";
                break;
            }
        } catch (const std::exception &ex) {
            std::cout << "Error: " << ex.what() << "\n";
        }
    }

    std::cout << "Goodbye!\n";
    return 0;
}