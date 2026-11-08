# C3D2: frozen input collection from the existing AI planner

Parent C3D1: 188b74d13868b341370ee411e195d6cd2503a724.
Intermediate savepoint: 0140d1aca728567cb6a00fcd61f864b1821f3eed
(25 initial pure oracles + game build, before native and visitor checks).
Final branch: knight-ai-lot-c3d2-20261109. No 1.1.18 migration or gameplay defaults.

## Live connection and phase

Planner::KingdomTurn now performs ONE diagnostic-only input capture on its first
pre-hero-planning pass, immediately after the existing castle-danger assessment.
The initial visible-map/View All scan has populated _enemyArmies. The collector
uses that knowledge, NOT a global enumeration of hidden enemy heroes. Priorities
come from the existing getSortedCastleList(castles,castlesInDanger) ordering.
The capture is opt-in (diagnostics-enabled compilation and runtime level >=2).
Normal AI decisions/legacy purchases remain unchanged. This is NOT activation
of the economic optimizer and does not rank or execute Farm/TG decisions in-game.

Capture effective INI, money, stock, buildings, available build actions, garrisons
and visitors with EconomyProject::capture. The collection is a fresh local
snapshot, not persistence and not a cache reusable after heroes move or buy.

## Route evidence and limits

For each known enemy/town pair, reuse the SAME getDistance(start,town,ourColor,
strength) model as legacy castle-danger analysis, with the same optimistic enemy
settings. Unlike its 3000-distance/3-day threat filter, input collection considers
known paths within the common 14/28-day window. It does NOT apply the legacy
50%-per-delayed-day reduction to the enemy's actual observed strength.

Enemy hero daily movement comes from the existing maximum-move-points estimate.
Last safe own day = today + ceil(distance / movement) - 1. Arrival within one day
therefore allows only today's response. The loss variant starts the next own
morning. This remains a conditional estimate using current paths/speed, not a
battle forecast, exact enemy-turn simulation or prediction of future bonuses.

A zero distance is UNKNOWN path evidence, not proof that the enemy can never
arrive. Missing movement, insufficient query quota, unknown strength or an
unverified observation prevents complete input coverage. An enemy castle's
potential future recruitment is not a known mobile hero; it gets no invented
arrival date. Those cases remain explicit gaps. Known dates outside H are counted;
a later policy layer must consider common horizon review before interpreting
these as absence of future threats or claiming surplus.

Work.paths is shared, decreases only and is capped at 128 per kingdom collection.
No per-town reset, hidden second quota or random selection. Stable enemy-tile and
town-ID ordering makes query truncation deterministic. At most 1024 enemy/town
pairs, 1024 input enemies and 256 towns; omitted coverage is signalled. Query
configuration and temporarily removed friendly map heroes are restored by RAII.
IMPORTANT: capture the visitor/army snapshot BEFORE path-only hero erasure.

## Military assignments and risks

Only local castle garrison/readiness is registered. No troops are credited at a
distant front and no route/transport/army merger is invented. A visiting hero at
this early phase has no committed future mission: its TownArmy assignment is
unverified and the resulting frame remains incomplete. It is NOT silently kept
in that castle for 14 days. Capture still preserves that hero and its real army.
Other local garrisons are a static readiness reference, not a promise that later
legacy AI actions will leave them unchanged. Refresh before future execution.

Known threat/deadline pairs create separate single-town conditional loss
scenarios with evidence IDs; fiscal/other recurring-income towns receive Income
focus, other destinations Destination focus. Risk::Session still validates these,
merges equivalent impacts and enforces its supported-variant limit. Frame input
coverage is NOT a certificate of robust funding, utility or strategic safety.
Unknown income attribution, terminal ownership cases etc remain earlier-module
limitations. No old commitment, request quantity or revisability permission is
fabricated from the legacy heuristic budget. Such planner/persistence integration
remains pending. No synthetic urgent troop quantity is guessed.

## Diagnostics

PLANNER_INPUTS: phase, validity, scope/end date, town/assignment/threat counts,
coverage, queries, remaining path budget and out-of-horizon count.
PLANNER_ROUTE: each actual query's enemy/town, raw movement-cost distance and
movement allowance. PLANNER_THREAT: stable identity, full strength, priority,
reachability and last-safe date or null. PLANNER_ASSIGNMENT: local-only use and
verification flag. PLANNER_INPUT_GAP: missing evidence/coverage/limits.
All are kind=evaluation and parent-linked. No cancellation or purchase is logged
as actual by collection. Level 0 emits no new rows and runs no new live capture.

## Verification and remaining limitations

- 26 targeted pure oracles: own-day deadlines and rounding, strength unchanged,
  no enemies, missing/zero routes, stationary recruitment risk, zero movement,
  unverified enemy not queried, incomplete scan, exhausted/shared path budget,
  outside/boundary horizon, deterministic two-town rank, duplicate IDs/priorities,
  invalid window/nonfinite strength, 128-query truncation, UINT32 day arithmetic,
  visiting hero's assignment explicitly uncommitted.
- ASan/UBSan on pure suite, leak detection disabled (no leak claim).
- 12 native snapshot processes: actual world, Knight/Warlock towns, hero visitor,
  observed attacker, game stock/prices/INI, engine army/defense scorers and Risk
  session. Known/unknown route evidence and uncommitted visitor, each with default
  and custom INI, diagnostics off/on. World funds and armies remain unchanged.
- The NATIVE route provider uses controlled distances to isolate extraction and
  unknown propagation. This does NOT exercise the whole KingdomTurn UI/hero
  cycle or certify real-map path estimates. The production getDistance hook is
  compiled/linked; campaign-level route/turn-order/performance checks remain due.
- Recourse and siege-defense native/pure regression suites; full native build.

Next bounded block should connect frozen frames to the observation search and
trusted mission/commitment classification, with refresh/horizon/unknown handling.
Do not enable autonomous Farm/TG decisions yet. Persistent economic policy,
recurrent-surplus proof, unsupported routes/risks and strategic Windows testing
remain separate work. Require new user go-ahead. Sol High recommended.
