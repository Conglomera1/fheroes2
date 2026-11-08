# C3A: bounded proposals and honest coverage

`EconomyCandidates::generate(context, sharedBudget)` creates proposals without
calling the C2 projector, evaluating military utility, or modifying the game.
`observe(kingdom, throughDay, budget)` captures real engine/INI data and returns
both that context and its proposals. It is not called by the live planner yet.

## Presort, limits, references

- At most 12 ordinary local proposals per town and 64 common proposals. Local
  selection starts with two representatives per available family: a strong
  nominal reinforcement and a cheaper alternative, or a known training milestone.
  Spare places preserve missing action kinds before filling by the presort.
- Families: immediate recruitment, existing-army upgrades, building chains,
  fiscal development, TG conversion, supply/wait. Existing-army upgrades remain
  explicitly unsupported; zero proposals in that family is not a search proof.
- Recruiting samples one unit and the current nominal affordable/stock/slot
  maximum, respecting all seven resources. Supplied quantities/deadlines are
  also represented. Later dates and financing remain proposals for C2 to check.
- Chains preserve every supported prerequisite and one construction per day,
  including retained construction commitments. They are not cut after six steps.
  Earliest and delayed schedules are sampled, not exhaustive calendar searches.
- Fiscal chains include a subsequent military purchase, possibly in a different
  faction's town. Targets are reduced to two representatives, explicitly logged.
  No universal gold-to-strength conversion or final monetary military score is used.
- The first possible TG success uses calendar limits, failure remainder and
  the persistent current composition cycle. Paid attempts/population are not
  guaranteed by this lower-bound date: C2 must validate them. Existing TGs also
  retain a next-success milestone; no cycle is reset during generation.
- Quick unit strength comes from `Monster::GetMonsterStrength()` without hero
  bonuses. It is a presort estimate only, not utility, combat viability or arrival.
- Urgent supplied requests precede optional cuts, ordered by deadline, caller
  priority and stable town/quantity identifiers. Overflow is counted, never called
  impossible. Ordinary families alternate towns with a day-rotated starting point.
- The no-new-investment reference and optional active continuation are kept
  separately from local/common cuts. The former is NOT the best ordinary spending
  reference; that still requires the next comparison stage.

Inputs are capped at 256 towns and 1,024 quantity requests. At most 96 unique raw
proposals are retained temporarily per town. The shared optional proposal budget
starts at 16,384; supplied urgent requests and references are independently bounded
and preserved first. Repeated calls must not reset the budget in the same turn.
Full C2 plan projections are a later budget (including the shared day/attempt
limits); generation itself runs zero such projections. The later complete-plan
search must enforce its 256-evaluation cap, including references and variants.

## Materialization and interpretation

`materialize(context, candidate, output)` retains base commitments, assigns new
noncolliding step IDs and inserts active commitments already named by the same ID
only once. A conflicting reused ID fails. Different IDs are not deduplicated merely
because they buy the same quantity: two actual purchases may be distinct expenses.

Every proposal still needs C2 financing/legality checks and the planned military
evaluation. `Result.valid` only means generation prechecks succeeded. Horizon
overflow, omitted families/quantities/schedules, limits and unsupported options
remain in `gaps`. Nothing in C3A establishes durable surplus or an optimized plan.
It requests a common horizon review when an effect/action exceeds the supplied
window; it never scores one plan at 28 days against another at 14 days.

Per-family counters track generated proposals, merged duplicates, local/common
cuts and retained family membership. A single deduplicated proposal can represent
multiple families, so these counters are not an additive unique-plan equation.

## Observation records

With existing diagnostics at level 2, `observe` writes `CANDIDATE_COVERAGE`,
`CANDIDATE_FAMILY`, `CANDIDATE_BLOCKED` and `CANDIDATE_PROPOSAL`. All use
`kind: proposal`, distinct from actual transactions and forecasts. Child records
reference their coverage record's sequence through `batch_seq` (within run/segment).
Proposal rows include labels, family masks, urgency, milestones, nominal estimates
and dated actions. They do not claim that costs have been paid or troops acquired.
Action type values follow C2: 0 build, 1 recruit, 2 local deposit, 3 local withdrawal.

## Tests

Compile `economy_candidates_test.cpp` and `economy_project_test.cpp` as C++17 with
all `src/fheroes2/*` directories and `src/engine` on the include path. The shared
fixture is now `economy_project_fixture.h`. The former checks bounded generation,
coverage, fair rotation, urgent overflow, references, persistent cycles, long
chains and cross-faction fiscal projects; selected proposals are replayed in C2.
Run the same generation test under ASan/UBSan; under ptrace disable leak detection
with `ASAN_OPTIONS=detect_leaks=0` (no leak-validation claim).

The existing native engine fixture additionally exercises `observe` on real
captured data, replays selected proposals with a shared C2 budget and checks the
new JSON record structure. Its six fresh processes still compare real purchases
and conversions with C2 under three INI profiles and diagnostics off/on. This is
not a campaign of full games, a performance validation on Windows x86 or a proof
that the final AI chooses the best proposal.
