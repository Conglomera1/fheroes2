# C3B2: known military deadlines and defense priority (observation only)

Parent C3B1: `60c21fc00876a693f7b2bedd3442cf0cef32b9e4`.
Checkpoint branch: `knight-ai-lot-c3b2-20261108`.
No live economic selection/execution, new INI settings or save-format changes.

## Threat input and phase contract

`EconomyDefense::snapshotThreat` freezes a supplied `AI::EnemyArmy` observation:
full recorded strength, source tile, hero ID and the melee/no-Ballistics profile
used by `Castle::GetGarrisonStrength`. It does not read an unverified hero.
Do not halve the enemy army's strength because it is several days away: the
existing planner's panic-distance discount is not its strength at battle time.

The caller supplies a known/reachable flag, current defense-priority rank (lower
first), and optional `lastSafeDay`. This is the last OWN action phase guaranteed
to occur BEFORE the enemy can attack, not the calendar label of enemy arrival.
All actions on that own day are included; purchases on a later day cannot help.
The current day may be the last safe phase. Never invent an ETA, assume turn
order or reuse a stale path to fill a missing phase. An absent/stale/outside-H
phase remains incomplete. Extend the same horizon to 28 for every compared plan
when the contract requires it; do not silently score different horizons.

The registry's `coverageComplete` must certify that all current known defense
requests are represented. Empty and certified is distinct from absent/unknown.
This block does not call the planner or pathfinder automatically. Wiring these
inputs into the live planner, including verified path/turn-order information,
is a later integration step. The current functions are explicit observation
entry points, not evidence that an unchecked caller-supplied route exists.

## Projection and ranking

Pass the optional registry and defense scorer into `EconomyMilitary::Session`.
The C2 completed-day callback collects deadline checks during the SAME baseline
and proposal trajectories as C3B1. No extra projection is run for a deadline;
the cached baseline also retains its defense checks. Each full defense-strength
call consumes the same external force budget used by military utility. Limits,
invalid inputs and incomplete observations never become a false numeric zero.

A defense is provisionally viable at the shared Taxpayer-policy ratio >= 1.10.
`localDefenseValue` uses C3B1's full hypothetical combined army/commander, then
the engine's existing tower, turret and moat arithmetic. Building counts and
mage-guild tower attack use the projected building mask. Empty armies receive no
free tower strength. Knight Fortifications contribute exactly as the existing
approximation does, including their building count; no invented wall multiplier
is added. This is an engine heuristic, not a simulated battle or guaranteed win.

The siege arithmetic and count-by-mask helper are shared with the real castle
getter, preserving its existing behavior. Native tests retain the OLD siege
formula using actual `Battle::Tower` objects as an independent regression oracle.
No live recruitment, Taxpayer preservation exception or last-stand rule changes.

D contains viable-defense bits ordered by deadline, existing planner rank,
then stable town tile. Several known threats at the same town/deadline share a
bit: defense must meet each observed profile separately. They are NOT summed
into a fictional combined enemy army. Distinct deadlines at one town remain
distinct entries: a day-4 threat must not borrow that town's day-2 priority over
another town threatened on day 3. Future force after earlier battles remains
conditional: attrition or victories are not simulated.

`compareDefense` compares funded, complete D vectors lexicographically, before
military utility. A larger late U cannot rescue a missed earlier defense bit.
Equal false bits are not ranked by small improvements in an insufficient ratio.
Incomplete coverage/deadlines/strength, invalid or unfunded plans are incomparable.
The comparison scope belongs to ONE read-only Session; complete checks from
another Session/state cannot accidentally be compared. The registry/window key
must also match. This is defense priority ONLY, not the final strategic chooser.

`coverage` reports towns unresolved in all supplied comparable alternatives,
plus compared/omitted counts. This is not a proof that no feasible defense exists,
a complete search certificate or proof that all separately solvable requests can
be solved simultaneously. Later search must consider legal regrouping/evacuation
and avoid futile spending; this block executes no purchases itself.

## Diagnostics

Existing diagnostic level 2 adds `DEFENSE_EVALUATION`, `DEFENSE_CITY` and
`DEFENSE_CHECK`, all `kind: evaluation`. They include candidate, funding, D,
baseline D, missing information, ordered town/deadline/priority, enemy source and
profile, force, ratio, shared viability threshold and nullable viability. Children
refer to their parent `evaluation_seq` within the existing run/segment.
Missing deadline/force/viability stays JSON null, not zero/false. Existing C3B1
records still report central utility separately; `Scored` there does NOT mean
that defense/risk/combined strategic validation has been completed. Level 0
emits no new records; simulations do not log fake actual payments or recruits.

## Validation

- `economy_defense_test.cpp`: 46 targeted oracles, including exact/on-time versus
  late purchases, lexicographic deadlines/ranks/stable ties, multiple and unknown
  threats, H14/H28, stale phases, empty certified registry, unfunded/illegal
  candidates, shared budgets, cached baseline, NaN/overflow, cross-session
  rejection and unresolved alternatives. Automatic TG conscription can remove
  defenders; unpurchased trainees do not replace them in the defense score.
- `economy_defense_engine_test.cpp` and `run_economy_defense_engine_test.py`:
  970 counted checks per process. Six factions x captain absent/present x five
  structure states x empty/populated army x four attacker profiles, comparing
  copied predictions and the live getter to the pre-extraction Tower oracle.
  Projected turret/moat/Fortifications purchases match real construction results.
  The real funding/commander model prefers the smaller on-time defense over a
  stronger late army; an unknown deadline remains incomplete/null.
- Two INI profiles x diagnostics off/on = four native defense processes.
- C3B1: 22 pure oracles and four native processes (161 checks each) still pass.
- C2: 89 pure cases and six native processes with nine real actions/seven days
  still pass. C3A's 41 cases and existing Taxpayer defense-policy checks pass.
- New defense suite passes ASan/UBSan with leak detection disabled; no leak claim.
- Full native game compilation/link succeeds, with no warnings in the final log.

Pure tests: C++17 with every `src/fheroes2/*` directory and `src/engine` included.
Native tests: link all game objects except main `fheroes2.o`, engine/Smacker and
the same SDL/mixer/image libraries as the game. Run the native binary through its
Python runner. No proprietary graphics/data are needed for these fixtures.
These are calculation/integration tests, not new balance campaigns. Prior map
simulations are preserved. Windows x86 performance/player testing is still open.

## Next bounded block

C3B3: explicit prudent risk variants founded on known threats, sharing resources
and evaluation limits. Combined-plan search, stability, actual planner input
assembly and AI execution remain later work. Obtain a new explicit go-ahead.
Sol High recommended; keep source and report checkpoints at the block boundary.

## C3B3 follow-up

Conditional town-loss variants now reuse these same deadline checks through
EconomyRisk::Session. See README_economy_risk.md for the completed scope,
comparison baselines, funding witnesses and explicit income/terminal limits.
