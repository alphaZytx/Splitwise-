# Splitwise++

A modern C++17 implementation of a Splitwise-like expense sharing system with clean object-oriented design, strategy patterns, JSON persistence, and a lightweight CLI.

## Features

- Strategy-based splitting (equal, exact, percent) with pluggable factory.
- Thread-safe manager coordinating users, groups, and expenses.
- JSON persistence backed by a lightweight `nlohmann::json`-compatible implementation.
- Greedy settle-up helper to minimise transfers.
- Interactive CLI for day-to-day usage.
- Catch2-style unit tests and GitHub Actions CI.

## Design Overview

- **Strategy Pattern**: `SplitStrategy` hierarchy for equal, exact, and percent splits.
- **Factory Pattern**: `SplitStrategyFactory` instantiates strategies by name.
- **Singleton Friendly**: `SplitwiseManager` can be wrapped as a Meyers singleton if desired.
- **Observer Stub**: `INotifier` and `ConsoleNotifier` allow optional large-expense alerts.
- **Thread Safety**: mutating operations in `SplitwiseManager` guard shared state with `std::mutex`.
- **Persistence**: JSON save/load with automatic balance recomputation for consistency.

## CLI Usage

Build and run the CLI to manage your groups and expenses:

```bash
cmake -S . -B build
cmake --build build -j
./build/splitwise
```

Menu options let you add users, list users, create/list groups, record expenses, view balances, settle up, and persist data. Inputs are validated with friendly error messages and sensible defaults (e.g. blank participant input targets the entire group).

## Architecture

For a deeper dive into the relationships between the core classes, persistence pipeline, and concurrency guarantees, see
[ARCHITECTURE.md](ARCHITECTURE.md).

## Build & Run

```bash
cmake -S . -B build
cmake --build build -j
./build/splitwise
```

## Tests

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
# or
./build/tests
```

## Example JSON

```json
{
  "users": [
    {"id": "USR1", "name": "Alice"}
  ],
  "groups": [
    {"id": "GRP1", "name": "Trip", "members": ["USR1"]}
  ],
  "expenses": [
    {
      "id": "EXP1",
      "groupId": "GRP1",
      "description": "Hotel",
      "payerId": "USR1",
      "amount": 100.0,
      "participants": ["USR1"],
      "exactShares": [100.0],
      "percentShares": [],
      "strategy": "exact"
    }
  ],
  "balances": {
    "USR1": 0.0
  }
}
```

## Future Enhancements

- Observer implementations for push/email alerts on large expenses.
- REST API wrapper around the manager for web/mobile clients.
- Undo/redo stack for expense modifications.
- Richer CLI tooling (CSV import/export, reporting).

