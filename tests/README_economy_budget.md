# C1: dated shared reservations

`economy_budget.h` is a pure component. It is not called by the live AI yet.
It validates the supplied accounting assumptions, not full action legality,
military value, hidden map events, routes or future battle outcomes.

Compile/run from the repository root:

```
g++ -std=c++17 -Wall -Wextra -Wconversion -Wshadow -pedantic \
    -Isrc/fheroes2/castle tests/economy_budget_test.cpp -o economy_budget_test
./economy_budget_test
```

Use the same test with `-fsanitize=address,undefined -fno-omit-frame-pointer`
for memory/undefined-behaviour checking. No SDL or original game assets are needed.
If the host runs under ptrace, use `ASAN_OPTIONS=detect_leaks=0` for that run:
LeakSanitizer cannot inspect processes there. This does not test memory leaks.
The B2 scheduler is exercised on copied states; these are mechanical unit cases,
not full games or a new balance campaign.

## Input contract for the next planner block

- Today's morning is already processed. Steps may spend today; morning entries
  must be later. Morning income precedes automatic charges, then supplied steps
  run in their stable input order for that day. There is at most one aggregated
  morning per date. The caller must provide all relevant dated flows.
- All projects use the same seven-resource account. Use effective INI/game
  prices when generating steps; there is no hardcoded exchange rate or virtual
  income. A construction chain is multiple steps belonging to one project.
- Step/morning IDs are nonzero and globally unique within the evaluation.
  Reusing an existing commitment means referring to it, not inserting it twice.
- Stock keys identify canonical sources, across all factions. Register even a
  currently empty source with quantity zero. Withdrawals are shared and dated.
  Only verified arrivals may replenish stocks. Source/destination effects on
  tax, growth, armies and automatic training must be generated separately by
  the future plan projector. A stock claim alone does not simulate a delivery.
- Register every available exclusive asset. Use a canonical per-town build key
  for construction steps, with a one-day interval; use a canonical hero key and
  inclusive mission interval for heroes. Supply today's spent building action
  as an initial busy interval. Unknown assets and overlapping claims fail.
- `required` describes a chosen commitment; it does not secretly reorder steps,
  choose military responses or cancel discretionary projects. A plan is covered
  only when every prefix is financed. The caller can compare a plan with/without
  a discretionary expense to protect a later required commitment.
- Commitments after the common horizon remain in `pending` and prevent full
  certification. They are not silently discarded or reserved forever today.
- Amounts are nonnegative, at most INT32_MAX. Horizon is at most 28 future days.
  There are at most 4096 total input entries/claims/assets. Exceeding a limit
  yields an explicit incomplete/invalid result, never a cheap wrapped cost.

## TG costs and conservative results

The bound helper reads the effective odd/even calendar limits, enabled flag,
formation price and peasant price. An active training period describes paid
**mornings**, so a TG built today normally starts tomorrow. The bound charges
every permitted attempt at hut-source price. Success rate and composition do
not reduce this upper bound: failed attempts still cost money.

Mark these morning charges conservative. A covered trajectory is sufficient
under the supplied fixed flows and bound. A deficit is `ConservativeGap`, not
proof of infeasibility: poverty, missing Peasants or cheaper garrison conscripts
may reduce actual fees. The next block must replay conditional mechanics with
the proposed actions before using such a gap to discard a useful investment.

`extraNeeded` and possibly negative accounting balances are diagnostic numbers.
They are never injected into game funds and are not actual debt, actual fees,
actual training or an order to hoard. No exchanges are invented. With fixed,
uncapped flows and no conditional charges, prefix deficits are the exact cash
shortfall; with conservative charges they are an envelope only. Capped incomes
also prevent interpreting the deficit as a universally minimal initial grant.

This component cannot certify the full financial consequences of a Farm/TG
conversion, transport or purchase until the caller supplies its verified effects.
That integration, candidate generation and scoring are not C1.
