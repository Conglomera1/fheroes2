# C3D5J — isolated exact local Peasant transfers

Parent I: `7e836c4c2204279dad1a4644d9fa21ae135d5b71`.
Test-only policy/native primitive; no production caller or legacy AI transfer hook.

Intermediate state: **240 native transfer processes / 120 diagnostic pairs pass**
across four profiles and both directions. Compilation/link has no warnings.
Sanitizer runs are pending at this intermediate checkpoint.

## Acceptance and evidence

Reuse the UNCHANGED shared `ActionOwner` lifecycle from I, with a new operation
policy limited to current-day `ToGarrison`/`ToVisitor` Peasant counts in a Knight
town. Explicit program acceptance must bind the actual fixed local visitor ID.
Registry import checks the physical/mission PATROL radius-zero facts, and the
current ticket carries the bound visitor. Missing/wrong binding or mobile visitor
cannot be accepted. Stale/forged tickets, changed actor/mission/physical state,
incomplete coverage or exhausted shared budgets never dispatch the transfer.

`check` requires an exact positive quantity, matching town/day/shape/physical and
projected slots, enough source Peasants, and an engine-order destination slot.
Destination first uses the first matching Peasant stack, otherwise the first
empty slot; overflow of the preferred stack holds, even when another stack could
fit. Source withdrawal consumes Peasant slots in index order, as Project does.
Population calculations use uint64. A hero giving troops must retain at least
one unit in total; retaining a different creature allows giving all Peasants.
An outgoing garrison may become empty. No silent quantity reduction is permitted.

The exact checked action is retained in the prepared primitive. A changed
quantity/direction cannot reuse that plan. `check` or a manually created Checked
value alone is not execution authority; the owner and registry remain required.

## Native primitive and final confirmation

`apply` rechecks complete before-evidence, the exact action and actual visitor.
It builds detached native `Troops` copies, removes precisely the requested source
quantity and calls native destination `JoinTroop`. BOTH copied armies must match
the planned slot results before either live army is changed. It then applies the
two copies using native `Assign` and returns; the owner recaptures the final world.
This is not a claim that two live Assign calls are a general transaction. Any
partial, contradictory or unknown post-write result is retained and blocked for
reconciliation, without restoring a stale world or automatically repeating.

Require exact source/destination slots, all other troops, all seven funds, all
six recruitment stocks, buildings/construction flag, actor/day/config/mission
and full current training-cycle progress. A valid transfer charges nothing and
does not recruit, build, credit income, train units or reset that cycle.
The native runner independently checks numeric populations and tax invariance,
as both local armies keep the same eligibility in this supported setting.

Do not reuse `depositTaxpayerPeasants` or `transferSlowestTroopsToGarrison` as exact
quantity receipts; those general helpers have different semantics. Do not infer
a receipt from a normal AI action or an economy trace. The test primitive is not
installed in `HeroesActionComplete` or its possible later restore boundaries.

## Matrix and fault labels

The default Farm profile covers 45 modes, each in both directions and at diagnostic
levels 0/2. Additional TG, custom-TG and compatibility profiles cover 15 more mode/
profile combinations, again both directions and levels: **240 processes total**.
Custom settings use fee 137, odd/even limits 1/4, success ratio .85 and quota
12/4/1. Fixed-visitor transfers themselves remain free with no funds available.

Cases cover exact/empty slot/full merge, split source/destination stacks, all
Peasants, empty garrison, last hero unit, leave-one, insufficient/absent source,
full/overflow destinations, zero, bindings and mission changes, no acceptance,
unsupported kind, ticket freshness/count/actor, physical changes, shared budget,
coverage, two steps, a round trip, reentrancy and preserving real nonzero progress.

The two UINT32_MAX destination tests set tax rate to zero in their temporary INI
to isolate troop-counter boundaries from unrelated capped-income arithmetic.
They do not establish correct taxation of an enormous artificial population.

26 distinct synthetic pure oracles cover all seven funds and six stocks, visitor,
progress, wrong post-state, missing evidence, wide totals, preferred-slot overflow
and native refusal to reuse a checked plan with another quantity/direction.
Repeated diagnostic/instrumented executions are not new independent oracle designs.

Callbacks inject false returns, true-without-effects, wrong quantity/creature/funds,
rollback, post-effect record invalidation and exception. These are fault tests,
not additional engine bugs. Restoring both armies then returning false is a clean
Failed result with the step retained; restoring then returning true is contradictory
and blocks. The first dispatch is never falsely confirmed as Executed. The tests
simulate a final restore boundary; they are not integration tests of legacy AI
rollback paths. `false_clean` explicitly refreshes/reverifies and invokes a retry;
`rollback_false` prepares a fresh ticket but does not invoke the next action.

Two-step/round-trip tests require one head removed at a time and a new preparation
before the next operation. RESULT status describes the first dispatch; counts and
final state include any later explicitly invoked second action or clean retry.

## Reproduction and stopping boundary

Use the existing matching native C3D5D diagnostic-debug objects and SDL/engine/
Smacker recipe. The unchanged shared build helper accepts
`--source tests/exact_transfer_adapter_engine_test.cpp`, the previous SDL/link
flags and an output outside the checkout. `--sanitize` instruments only fixture/
test-policy/instantiated owner code, not the existing production objects.

Run `run_exact_transfer_adapter.py BINARY --output-dir DIRECTORY`, adding repeated
`--library-dir PATH` as needed. `--profiles` / `--modes` support justified targeted
runs. Every process isolates effective INI and settings caches. The runner checks
independent numeric outcomes, diagnostic neutrality and no purchase/build traces.

No existing owner, recruitment/build policy or production source is modified.
The old H/I results remain preserved; unchanged suites and historical balance
campaigns need not be rerun merely to add this isolated policy.

Stop after the saved final checkpoint and bilan. Live accepted-program ownership,
future-day/multi-town execution, persistence, uncertainty reconciliation and real
map/KingdomTurn/legacy callback integration remain separate work.
