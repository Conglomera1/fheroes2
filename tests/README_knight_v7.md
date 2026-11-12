# Knight AI v7

Farm/TG conversions remain separate mobilisation decisions. Starting TG is weekly;
safe TG towns are reviewed daily. Affordable Farm recovery below 2000 gold plus
its configured return cost no longer has the 10% local-force veto. Unsafe towns
keep the military safety gate. Starting TG must not delay primary buildings
fundable within seven days of known income. The previous 25%-of-treasury entry
limit and 2000-gold reserve also remain.

The 14-day comparator now recruits in ALL towns against one treasury. Purchases
are staged on copied states, checked for stocks, seven resources and army slots,
then applied through the dated projector. Non-recruit expenditure uses a rolling
average of the previous seven completed AI-turn days. Negative trade legs do not
mint funds. Conscription and role conversions are excluded from that allowance
because they are modeled explicitly. History resets on load/new-map segments;
initial comparisons have no past expenditure allowance. Future battles, captures,
loot, orders and construction sequences remain unmodeled.

The common construction scope covers hero movement and purchases, not just the
last castle-development pass. Routine recruitment, upgrades, hero purchases and
market trades preserve commitments and a 2000-gold buffer plus the actual Farm
return costs of active TG towns. A first hero can still be hired. Known military
emergencies can bypass routine restrictions and are traced. Construction can use
the buffer. Human kingdoms and copied mechanical simulations are exempt.

The five-wood rule applies only before Marketplace and without a sawmill.
Marketplace itself is exempt. Accessible candidates retain their relative
priorities. A building blocked by this wood rule cannot book a reservation that
would itself prevent Marketplace construction.

Multiple Knight towns receive roles from existing AI knowledge. The closest to
known enemies is the military hub; existing TG/building value break ties.
Endangered towns also remain military. Other towns prioritize Statue, Marketplace,
Hut, Well and Farm, without developing another complete base-dwelling chain.
Marketplace can precede an inaccessible Statue only when tomorrow's common
funding remains valid. Roles are recomputed after movement/capture.

Shipyard and boat purchases require an identifiable use: a hero who can reach
the coast and a reachable visible reward or known water edge for exploration.
Native financial thresholds still apply, together with the common budget.
A seven-day bounded mission keeps the destination across boarding and bypasses
routine reinforcement sleep, while defense, ownership changes and completed or
changed targets can cancel it. Hidden objects are not prospective rewards.
General hero defense/pathfinding remains native; waiting reasons are now traced.

New traces include revision 7, budget refusals/emergencies, town roles, forecast
town count/overhead/failures, boat city/tile/cost and missions, unit upgrades,
hero waits, defense selection and capture garrisons. The older passive FORECAST
is labeled as passive income/conscription, distinct from the role comparison.

## Verification

- 17 v7 generated-state cases, diagnostics off/on: 34 native engine processes.
  Tests include actual recruitment budgets, human and emergency exemptions,
  safe/unsafe recovery, eleven-town comparison, measured overhead, market/wood/
  sawmill rules, economic-town order, an actual purchased boat with a native
  reachable route, and a hidden reward excluded from mission candidates.
- Existing construction suite: 20 processes. Its fixtures now have Marketplace
  so the original funding cases are independent of the new wood-rule tests.
- Existing live Farm/TG adapter: 20 processes, including configurable costs,
  unsuccessful training, danger, mobile visitors, rich/poor and recovery cases.
- Eight shared-resource ledger scenarios.

Diagnostics must leave decisions and balances unchanged. These are headless
Linux engine fixtures, not full AoW/Utopeia replays or manual Windows 7 tests.
The distribution executable is built and checked by Windows x86 CI.
