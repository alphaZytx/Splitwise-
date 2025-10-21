# Splitwise++ Architecture

This document provides a high-level map of the core building blocks in Splitwise++, how data moves between them, and the
constraints that guided the implementation.

## Module Overview

| Module | Responsibility |
| --- | --- |
| `User` | Immutable value object representing a participant (id + display name). |
| `Group` | Maintains membership (user ids) and provides membership lookups used by expense validation. |
| `SplitStrategy` hierarchy | Encapsulates the math for equal/exact/percent distributions; returns per-user deltas. |
| `SplitStrategyFactory` | Resolves a runtime string to a concrete `SplitStrategy` implementation. |
| `Expense` | Records an applied strategy, its parameters (`SplitInput`), and contextual metadata. |
| `BalanceSheet` | Aggregates per-user running balances and exposes JSON serialisation helpers. |
| `SplitwiseManager` | Thread-safe façade that coordinates users, groups, expenses, and persistence. |
| `ConsoleNotifier` | Minimal observer used to demonstrate the notification extension point. |
| CLI (`src/main.cpp`) | User-facing loop that translates menu selections into manager calls. |

## Control Flow

### Recording an Expense

1. The CLI gathers input (group id, payer, amount, strategy type, participants) and requests a strategy instance from the factory.
2. `SplitwiseManager::addExpense` validates:
   - the group and payer exist,
   - at least one participant was provided, all belong to the group, and the payer is included,
   - the strategy pointer is non-null.
3. The chosen `SplitStrategy` computes a `BalanceSheet::BalanceMap` delta that is applied to the shared `BalanceSheet`.
4. If the amount breaches the configured threshold, `ConsoleNotifier` is invoked (no-op by default except for console output).

### Persistence Pipeline

Saving (`saveToJson`):

1. Acquire the manager mutex to freeze concurrent mutations.
2. Serialise users, groups, and expenses via their `toJson` helpers and dump the snapshot to disk alongside the current balances.

Loading (`loadFromJson`):

1. Lock the mutex and parse the JSON document.
2. Validate top-level sections, rehydrate users and groups, and ensure all membership references remain valid.
3. Reconstruct expenses by pulling a fresh strategy from the factory, then verifying payer/participants against the owning group.
4. Regenerate id counters to keep future inserts monotonic.
5. Recompute balances solely from the expense list to guarantee consistency.

## Thread Safety

`SplitwiseManager` guards all mutating operations (`addUser`, `addGroup`, `addExpense`, `saveToJson`, `loadFromJson`, notifier setters)
with an internal `std::mutex`. Read-only accessors return references to internal maps but rely on external callers to avoid mutation.
The greedy settle-up algorithm also takes the mutex to snapshot balances safely before deriving settlement transactions.

## Extensibility Hooks

- **Strategies**: implement `SplitStrategy::computeSplits`, register with the factory, and the CLI automatically accepts the new type.
- **Notifiers**: provide an `INotifier` implementation and call `SplitwiseManager::setNotifier` to enable richer alerting (e.g. email).
- **Persistence**: the load/save helpers intentionally separate entity serialisation logic, easing alternative backends (e.g. SQLite).
- **CLI Enhancements**: menu handlers live in `src/main.cpp` inside a small helper namespace, making it straightforward to add
  additional commands such as reporting or editing workflows.

## Testing Strategy

- `tests/strategy_tests.cpp` ensures each strategy enforces its invariants (sum checks, vector lengths) and computes correct deltas.
- `tests/reconciliation_tests.cpp` exercises the manager end-to-end: expense combinations, persistence round-trip, and settle-up flow.
- GitHub Actions (`.github/workflows/cpp.yml`) runs the full CMake build + Catch2 test suite to keep regressions out of the main branch.

## Relationships Diagram (Textual)

```
CLI (main.cpp)
   │
   └──> SplitwiseManager ───┬──> User / Group registries
                            ├──> Expense ledger ──┬──> SplitStrategy (Equal/Exact/Percent)
                            │                     └──> BalanceSheet (apply deltas)
                            ├──> SplitStrategyFactory (runtime resolution)
                            └──> Optional INotifier observers
```

