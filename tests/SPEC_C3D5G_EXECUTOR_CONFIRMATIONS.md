# C3D5G — pre-execution checks and synchronous confirmations

10 November 2026. **Design and source audit only; no executor implementation.**
Based on production C3D5D (`9e84baeb37a814d5487b126ad2b76231ec99fd3f`) and
recovered C3D5F evidence (`48b02c89576f0abcf3bb2f3bb527def481846fe7`).
This specification does not promote hypothetical programs into live commitments.

## 1. Evidence and scope

C3D5F reproduces two raw recruitment inconsistencies: request 9 from stock 8,
and request 1 from stock 0. In both, the API accepts, charges for and adds the
requested troops while removing only available stock. Each is observed with
diagnostics 0 and 2. The test-local stock guard skips both calls without effects.
See `RESULTS_C3D5F_FINAL.md` for all 18 observations and their limits.

These are deliberate direct API calls, not proof of a broken normal AI path.
No global recruitment correction is selected here. All other checks below are
requirements derived from source/contracts, unless explicitly covered by C3D5F.

The first implementation should cover **one exact local Recruit action in an
isolated native fixture**, with explicit test-owner acceptance. Production
acceptance, KingdomTurn integration, persistence, Build and transfer execution
remain later work. It must not call the normal observer's `stage()` as a source
of live authorization. No extra projection budget or private funds may be used.

## 2. Admission, freshness and identity

Before any world mutation, the future owner must establish all of these:

1. A concrete program was explicitly accepted by that owner. A verified copied
   plan, `Staged`, `ready()`, a ticket or a matching legacy AI action is insufficient.
   The accepted program, registry token and game/load lifetime must be bound.
2. Capture current physical state, effective INI, mission state and shared
   verification budgets; refresh the owned registry. **Refresh first, issue a new
   ticket second:** even a failed refresh invalidates old tickets. A ticket from
   the pre-refresh revision must never be reused.
3. Validate registry/player/day/revision, complete action identity (project, id,
   town, kind, count, pool, building, date, required flag) and bound visitor.
   The town still exists, belongs to that player and refers to the same actor.
4. Reverify the retained accepted schedule with the existing common Project /
   Risk / Recourse budgets and applicable safety constraints. `Ready` does not
   prove funding or legality. Future dates and incomplete/unsupported checks hold.
5. Select the next action in the accepted schedule's preserved same-day order,
   including order across programs. `Registry::ticket()` only gates a program's
   head and dependencies; it does not by itself enforce global execution order.
6. Require a usable revision counter and an unblocked owner. A prior uncertain
   effect or failed post-action recording blocks subsequent execution, including
   a normal fresh snapshot alone. Duplicate callbacks cannot grant a second call.

Use a synchronous interval with no unrelated AI work between final precheck,
engine call, final effect capture and recording. If freshness changes before
the call, skip it and require a new capture/ticket. If it changes after possible
effects, treat the result as uncertain. An invalid/foreign/stale ticket is never
submitted as a valid Failed or Uncertain receipt for another action.

## 3. Recruitment preconditions

Resolve `Action.pool` (0..5) through the fresh town's actual built dwelling and
upgrade, as Project does. Do not treat the pool integer as a Monster ID. Verify
the dwelling exists, the creature matches the town race and effective upgrade,
and the resolved engine creature and fresh projection agree. The action carries
no Monster ID; keep the resolved ID in the operation's local evidence.

Require `0 < count <= fresh stock`. This is an exact request: no silent clamping,
substitution or affordable partial recruitment. `getRecruitLimit()` is a useful
stock/funds bound for ordinary callers; it does not establish destination room,
complete upgrade consistency or execution authority for this contract.

Compute all seven costs with checked wide arithmetic **before** constructing a
large Troop request or calling an engine cost multiplication. Require representable
counts, per-resource totals and remaining balances, current payment availability,
and equality with the effective projected unit costs. Never use a cached price
or a private reserve to bypass the shared accepted-program budget.

Resolve placement using the engine's garrison-first, same-monster-stack-first
order. If the garrison cannot take the entire request, require the current
visitor to take the entire request. This path promises no split between armies
and no arbitrary/exact-visitor delivery. Record the current visitor even if the
program has no transfer binding. A bound visitor must match both the ticket and
the fresh physical/mission context.

`CanJoinTroop()` checks a matching creature or empty slot; it does not check
addition overflow. Check the precise destination stack's resulting count in a
wide type. If engine placement selects an overflowing stack, hold the action;
do not invent a different destination. Repeat equivalent range checks for
transfers. Overflow is a source-derived requirement, not a C3D5F reproduction.

## 4. Recruitment evidence and outcome

Before the one call, capture all seven funds, all six dwelling stocks, building
mask/construction-day availability, both armies by slot (IDs and counts), town
owner/day, visitor identity and bound ticket/config revision. Resolve and retain
the expected cost and destination. No trace row serves as the receipt authority.

After `RecruitMonster(request, false)` returns, recapture in the same interval:

- Success requires return `true`, precisely `count` units added to the resolved
  destination, precisely `count` removed from the corresponding stock, and all
  seven balances reduced by the checked cost exactly once.
- Require the other army, other dwelling stocks, unrelated slots/creatures,
  buildings, construction flag, player/day/config and visitor to be unchanged.
  Check slot results in addition to aggregated counts, so an unrelated
  reorganization is not silently accepted. Do not require unspecified commander
  caches to equal a copied forecast; widen evidence explicitly if later needed.
- Return `false` counts as Failed only if all captured affected state is proven
  unchanged. Refusal alone is insufficient proof of no effect.
- Return `true` with missing/extra/wrong effects, return `false` with effects,
  incomplete post-capture, changed actor or ambiguous interleaving is Uncertain.
  Even `true` with no effects is contradictory; never automatically retry it.

| Situation | Engine call | Registry handling | Next action |
|---|---|---|---|
| No accepted authority, invalid/stale ticket, unsupported context | skipped | no fabricated receipt | owner holds; capture/resolve first |
| Current valid ticket, local precheck refusal, proven unchanged | skipped | Failed may be recorded for this ticket | pending retained; refresh/reverify |
| Return false, complete unchanged evidence | once | record Failed | pending retained; refresh/reverify |
| Return true, complete exact final effects and identity | once | record Executed | remove one head only, then refresh |
| Contradictory, partial or unobservable effects | at most once | record Uncertain if current ticket still valid; otherwise block owner separately | reconciliation required; no automatic retry |
| Exact engine effects but `record()` rejects or cannot complete | once | never relabel as clean failure or undo payment | block owner separately; reconcile before more actions |

`Registry::record()` trusts its caller's Outcome; it does not inspect the engine.
The adapter must classify effects before calling it. A successful Executed record
returns **Status::Held** with reason `execution_recorded_refresh_required`; Held
alone is not an execution failure. The owner must check the complete Update and
the expected pending-step removal. Failed retains the step; Uncertain blocks
normal refresh. Registry has no reconciliation/unblock method today: do not
recreate an empty registry to retry or discard accepted pending programs.

After Executed, refresh from actual post-state; do not simulate the payment into
the world again, refund it, replay the action or replace acquired buildings or
troops with a pre-action snapshot. If recording fails after effects, preservation
and reconciliation take precedence over making a test pass.

## 5. Building and transfer boundaries for later blocks

| Operation | Mandatory precheck | Final confirmation and isolation |
|---|---|---|
| Build | Accepted supported single building; fresh `AllowBuyBuilding`, actual `getBuildingCost`, all seven funds, effective TG prerequisites/INI; checked projected effects | Confirm after `BuyBuilding` returns: payment once, exact building transition and construction-day flag consumed; verify initial stock for base dwellings and Farm/TG mutual exclusion. Include building-specific side effects before claiming support. |
| ToGarrison | Exact bound fixed local visitor, positive Peasant count, source population, destination capacity/range; hero retains at least one unit | Exact conservation between both armies, unchanged unrelated slots/resources/stocks and same visitor; confirm only after final keep/rollback decision. |
| ToVisitor | Same bound fixed local visitor and direction, enough garrison Peasants, representable destination | Exact inverse conservation with no unrelated transfer; confirm only after final keep/rollback decision. |

For Build, town/shipyard map changes are already excluded by Project capture;
other catalogued buildings can have engine-only effects (mage-guild training,
captain state). A projected catalog entry alone does not certify a complete live
receipt. Hold a building until its evidence includes those actual effects.

`depositTaxpayerPeasants()` transfers the first found Peasant stack and may retain
the hero's final unit; it does not implement a freely requested exact quantity.
`transferSlowestTroopsToGarrison()` merges/transfers multiple stacks. Neither is
an exact-action confirmation primitive. `HeroesActionComplete()` can restore
both armies after the slow-transfer attempt; `reinforceCastle()` has another
restore boundary. Do not confirm inside either tentative operation or attribute
all of HeroesActionComplete to a single ticket. Prefer a dedicated isolated
exact local transfer for a future transfer block, with final-state verification.

## 6. Verification matrix before any live connection

Existing C3D5F results are evidence to preserve, not 18 executor tests that pass.
New adapter logic will need its own tests; unchanged historical suites need not
be rerun solely because of interruption or relocation.

| Block | Required scenarios / independent expected results | Current evidence |
|---|---|---|
| Recruitment guard | exact stock, overstock, empty stock, zero count, invalid pool/foreign creature/unbuilt or changed upgrade, insufficient funds in each resource | Archer raw exact/overstock/empty/zero/poor observed; two test-local stock guards observed. Full adapter still absent. |
| Placement | garrison empty/merge, full garrison + room/merge in visitor, both full, no visitor, replacement visitor, count/cost/stack overflow | Full garrison without visitor refuses; visitor fallback succeeds. Other cases untested here. |
| Classification | exact success; false/no effects; true/no effects; false/partial effects; wrong stock, cost or destination; missing post-state | Raw two inconsistencies measured; no classifier implemented. Injected outcomes must be labelled synthetic, not engine reproductions. |
| Registry integration | explicit accepted owner vs observer; stale/wrong identities; global same-day order; exactly one pending step removed; double receipt; Failed refresh; Uncertain held; recording rejection after success | Registry source reviewed; old registry unit results preserved. No native accepted-owner adapter test. |
| Buildings later | refusal/no effects, success, new dwelling stock, Farm/TG conversion both ways with custom INI, unsupported side effects held | Source audit only for these confirmations. |
| Transfers later | exact deposit/withdrawal, multiple Peasant stacks, last hero unit, saturation, visitor changes, rollback causes no false success | Source audit only for these confirmations. |

Run diagnostic off/on comparison for a new native adapter fixture after it exists;
diagnostics must not grant authority or change measured state. A first adapter
block may test registry lifecycle in isolation, but must still document that the
production observer creates hypothetical programs only. Real-map strategic
campaigns and player release validation are separate, later work.

## 7. Source anchors and stopping point

Paths below are relative to production C3D5D (unchanged in C3D5F/C3D5G):

- `src/fheroes2/castle/castle.cpp`: `RecruitMonster`, `getRecruitLimit`,
  `CheckBuyBuilding`, `BuyBuilding`, `getMonstersInDwelling`.
- `src/fheroes2/army/army.cpp`: `CanJoinTroop`, `JoinTroop`.
- `src/fheroes2/ai/economy_project.h` and `.cpp`: copied actions, capture,
  effective creature/stock/cost checks, supported building effects and transfers.
- `src/fheroes2/ai/economy_registry.h`: refresh invalidation, ticket identity,
  record's trusted Outcome, Held returns and uncertain-result block.
- `src/fheroes2/ai/economy_registry_observation.h`: hypothetical snapshot-only stage.
- `src/fheroes2/ai/economy_commitments.h`: bound local fixed visitor contract.
- `src/fheroes2/ai/ai_common.cpp`, `ai_planner_hero.cpp` and
  `ai_planner_castle.cpp`: generic transfers and their restore boundaries.

Stop after preserving C3D5F, this specification and a recovery/audit bilan.
No production source, INI/default, translation, save, resource, release or player
package changes. No new compilation, simulation or engine run in this block.
Next proposed block: isolated exact-Recruit adapter and targeted native tests,
without the normal AI live connection. **Sol, High** recommended; start only
after the user's next explicit go-ahead.
