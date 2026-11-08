# C3B1: central, dated military utility (observation only)

Parent: C3A `bd5ba37fcbd2ccb53cad96183b9598a14a91e19d`.
Checkpoint branch: `knight-ai-lot-c3b1-20261108`.
No live planner calls this module. No gameplay/INI/save-format changes.

## Contract

`EconomyMilitary::Session` owns a frozen C3A context and one common assignment
registry. Pass the SAME external budget across sessions in one planning pass;
never reset it per candidate. It permits 256 C2 projections (including baselines)
and 131072 full force evaluations, in addition to C2's shared day/attempt limits.
The baseline is cached per session and retains actual commitments and automatic
mechanisms; it is NOT the best ordinary spending competitor.

The horizon must contain exactly 14 or 28 FUTURE days. The observer also records
today after actions; today's row is diagnostic only. For each completed day:
`M = sum(full engine strength of verified useful assigned armies)`.
`U = sum(M_plan - M_baseline, tomorrow..throughDay) / H`.
No additional delay multiplier, gold-to-strength conversion, stock credit or
automatic conquest income. Negative differences are preserved.

Each owned town must have an explicit usefulness record, possibly `useful=false`.
Unknown/unverified usefulness prevents a complete score; it is not silently zero.
Overlapping uses of the same town are rejected, since `TownArmy` includes its
visitor. All compared plans share this registry. It does not establish routes,
arrival dates or mission usefulness: verified dates MUST come from the caller's
military knowledge. This local adapter does not support remote deliveries.

C2 checks dated legality and funding before a score is returned. Unfunded,
unsupported, unknown or budget-exhausted cases have no numeric utility, with
status/reason gaps. The system does not choose the winning candidate yet.

## Engine adapter

`localArmyValue` uses real attack, defense, archery, morale, luck, spells, mana
and power. It evaluates the WHOLE hypothetical army, with raw composition deltas
applied before morale/luck clamps and maximum-value artifacts. Castle bonus
changes (including Necromancer power) are applied before clamping too.
A present hero commands combined local troops selected with the existing
`ArrangeForCastleDefense` routine, on copies. The visitor-only slot evaluates
only that hero's army. Without a hero, the existing or newly projected captain
is used; legacy behavior with absent quarters is preserved.

A new captain receives engine defaults, initial mana and spells from an already
visible guild. No spells stored for unbuilt guild/library levels are read;
projects requiring their military value are incomplete, not assigned invented
magic. Existing commanders retain observed skills, artifacts, visited bonuses,
spells and mana throughout the trajectory. No future battle, experience change,
visited-bonus expiration or mana regeneration is simulated. A session must be
used within one read-only observation pass, before the live world changes.
These are CENTRAL conditional force trajectories, not battle outcome predictions.

The small engine extraction keeps ordinary `Army::GetStrength` on the same
formula and commander getters. Native frozen-formula oracles cover that change.
The optional const-state C2 day observer runs after completed day actions only;
it does not rerun a morning or execute a real transaction.

## Telemetry

`evaluateObserved` emits `MILITARY_EVALUATION` and `MILITARY_DAY` at existing
diagnostics level 2, with `kind: evaluation`, candidate, status, funded flag,
optional utility, shared budget remaining, gaps and the force-model label.
Daily rows contain projected date, baseline, proposal and difference, linked by
`evaluation_seq` within the existing run/segment. No fake real purchases or
payments are logged. Level 0 emits no new records. No automatic live call yet.

## Verification and continuation

- `economy_military_test.cpp`: 22 pure oracles: timing, H14/H28, baseline cache,
  retained commitments, missing/overlapping assignments, losses, no stock/tax
  credit, unknown force, illegal/unfunded projects and shared limits.
- `economy_military_engine_test.cpp` + `run_economy_military_engine_test.py`:
  161 numerical/context checks per process across six factions, captain present
  or absent, hero leadership/archery/magic, living/undead mixed forces, maximum
  morale/luck artifacts, projected tavern/special buildings, new captain with
  known guild, unknown guild/library, combined army and world-state neutrality.
  Two INI profiles x diagnostics off/on = four fresh processes. In the final
  fixture, the same purchase gives U=9.9 at day 2 and 4.95 at day 9 (H14).
- C2's 89 and C3A's 41 pure cases remain passing.
- C2's existing native runner still passes its six processes comparing nine
  actual actions over seven days under three profiles with diagnostics off/on.
- New pure suite passes ASan/UBSan with `detect_leaks=0`; no leak-test claim.

Pure tests: C++17, include all `src/fheroes2/*` directories and `src/engine`.
Native tests: compile with the same configuration as the full game, link all
game objects EXCEPT the main `fheroes2.o`, engine and Smacker libraries and the
same SDL/image/mixer dependencies. Pass that test executable to its Python
runner. Run the full game build after changes to shared headers/strength code.
Tests need no original Heroes II graphics/data. Skip gettext template generation
when xgettext is unavailable; no translations are changed in this block.

Not implemented: urgency D vector, fortification/viability deadline checks,
prudent risk scenarios, enemy/path verification, combined search, persistent
active-plan selection or live AI execution. Missing C3A proposal families and
search gaps are still present. Do not interpret a scored local project as a
fully validated strategic plan or durable surplus. Windows x86 performance and
player campaigns remain untested. Previous balance simulations are not rerun.

Suggested next bounded block: C3B2, known military deadlines and priority defense
viability, before risk variants and combined search. Obtain explicit permission
before starting it. Keep source/report checkpoints at each block.

## C3B2 follow-up

Optional deadline/defense checks are now attached to the same Session projections.
See `README_economy_defense.md` for the completed scope, priority comparator,
phase/visibility contracts, shared budgets and remaining integration limits.
The C3B1 central utility itself still does not select or execute an AI plan.
