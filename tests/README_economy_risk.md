# C3B3: conditional town-loss risk, observation only

Parent C3B2: `078793b7aa66106cf39acf52efac7f764f1409e5`.
Branch: `knight-ai-lot-c3b3-20261108`.
No gameplay defaults, INI keys, save format or live economic choices changed.

## Scope and evidence

`EconomyRisk::Session` evaluates the central trajectory plus at most two supplied
conditional variants. Each loss must refer to a known, reachable C3B2 threat and
its phase-verified last safe OWN day. `firstUnavailableDay = lastSafeDay + 1` is
the first OWN morning/actions for which the town is no longer available. No
already processed income is taken back. No probability, hidden enemy, conquest
revenue, invented ETA or arbitrary route delay is introduced.

Joint losses require explicit verified linkage; alternatives from the same enemy
must be supplied separately unless actual joint/sequential exposure is verified.
Identical impact sets deduplicate. Preselection preserves an income case and a
destination case, using supplied exposure rank then stable ID. The registry is
bounded at 1024 cases/256 towns per case and 4096 total case/loss/evidence
entries. Evidence is indexed, avoiding nested scenario x town x threat x evidence
searches. At most two variants are evaluated.
Unverified cases or additional omitted variants remain gaps; they do not become a
certificate that every relevant risk was examined. Unsupported blocked routes,
external income sources or dependent events must be recorded in registry gaps.

The registry is supplied by the caller; this module does not scan the live
planner/pathfinder or choose threats by itself. `coverageComplete` certifies the
supplied relevant risk coverage, not all possible future situations in the game.

## Dated state changes

C2 now accepts `TownLoss` events. At the missing morning it removes certified
non-tax town income (including income buildings previously built in that plan),
taxable garrison/visitor populations and local military availability. TG fees
and growth stop there; the surviving TGs use the actual shared treasury and
calendar/configuration. Persistent conscription cycle data is not reset.
Captured stocks cannot be recruited; the unavailable town stays in the copied
state to retain stable identities/order/history.

Actions at the lost town after the loss are cancelled, including retained future
commitments, without payments or refunds of earlier investments. Other towns'
actions remain and must actually be paid. A cancelled required action makes the
plan exposed, even if dropping that purchase made its treasury look healthier.
The model currently has only local transfers and purchases; remote delivery or
cross-town prerequisite dependencies are not silently synthesized.

Each variant compares its proposed army to its OWN no-new-investment baseline
under the SAME losses. Existing armies lost in both trajectories are not charged
a second time to the project. New troops only contribute while their useful
position remains available. The central trajectory is always included in the
minimum, so saved TG charges after a capture cannot inflate U_robust above U_central.

No game objects are mutated by these projections. The actual source/captain/hero
contexts are still the C3B1 read-only snapshot assumptions; a trapped visitor is
unavailable in this conditional loss case, not assigned an invented escape.

## Funding, limits, interpretation

Each of the up-to-three variants owns one cached C3B1 baseline. All share the
same external projection/day/attempt/force budgets; a first evaluation costs
up to six C2 projections, later candidates at most three (cached baselines).
There is no reset of the 256-projection limit per case or candidate.

`robustUtility` is the minimum over all certified, funded evaluated trajectories.
Unknown/incomplete/limited/unfunded cases prevent that complete score; a minimum
over the successfully scored subset is separately named `observedMinimum`.
Unfunded cases or cancelled required actions are exposed, never given virtual
cash. Defense vectors remain separate from utility; this module is not the
final strategic comparator or executor.

The result retains the lowest-utility case, earliest actual payment failure,
and the tightest observed resource margin case for each of seven resources.
When all are funded, the gold-margin case is the funding witness. There is no
universal conversion of rare resources into gold or scalar force. A payment gap
is a first-failure diagnostic, NOT an exact full-plan funding requirement or a
sufficient reserve envelope. No monotonic binary search or virtual TG spending
is used. Conservative reserve refinement remains part of later combined planning.

## Income-model limits (must remain explicit before live activation)

Capture attributes town/Statue/Warlock-Dungeon and trapped-visitor artifact/Estates
income exactly without player handicap for humans and AI Normal/Hard (the Hard
flat resource bonus is independent of castle ownership). Native loss replay is
validated on Normal. Easy income scaling, Expert/Impossible ownership-dependent
bonuses and player handicaps are not falsely attributed: their loss forecasts
return incomplete until modeled. Losing the last available town also returns
incomplete; kingdom elimination/grace/continuation is not simulated here.

The native central C2 model retains its earlier scope. Unidentified future losses,
battles, attrition, supply routes, mines and conditional event dependencies are
not proven harmless. Unknown source attribution is not continued free income.

## Traces and validation

Existing diagnostics level 2 emits `RISK_EVALUATION`, `RISK_VARIANT`, `RISK_DAY`
and `RISK_TOWN_LOSS`, all `kind: evaluation`, correlated by parent sequence.
Fields include certified/partial minima, funding, defense completeness/vector,
scenario witnesses, omitted cases/gaps, cancelled/required actions, payment
failure identity/date/gap, minimum resource balances, per-day seven-resource
funds, tax, fees and baseline/proposed force. Unknown values remain null.
Level 0 emits none; no hypothetical payment is recorded as a real purchase.

- `economy_risk_test.cpp`: 33 pure oracles, passing ASan/UBSan with leaks disabled
  (no leak-validation claim), including timing, common losses in M0, required
  cancellations, source funding failure elsewhere, duplicate/unknown/linked
  cases, unsupported attribution, global taxation, persistent cycle, fees,
  last-town uncertainty, no retroactive morning, future cancellations and budgets.
- `economy_risk_engine_test.cpp` with `run_economy_risk_engine_test.py`: six
  processes, three INI profiles x diagnostics off/on. Replay seven real days:
  build Statue on day 2, actually remove the town before day-3 income, recruit
  in the surviving TG town on day 4. Funds match C2 every day; final tax/fee/
  stocks/cycle match. Ten counted replay checks per process, plus observation
  and JSON assertions. Profiles: default, customized tax/fee/calendar/success/
  composition, and global tax with zero successful training. No proprietary data.
- C3B2 (46 pure + four native), C3B1 (22 pure + four native), C2 (89 pure + six
  native), C3A (41 pure) and Taxpayer policy regressions pass.
- Full native game compilation/link passes without warnings in the final log.

Compile pure tests as C++17 with all src/fheroes2 directories and src/engine.
For native tests, link all game objects except main fheroes2.o, engine/Smacker
and the same SDL/image/mixer libraries as the full game; pass the test executable
to its Python runner. Native fixtures are not new balance campaigns or Windows
x86 performance validation. Prior maps/logs/results remain untouched.

Next proposed bounded block: C3C1, combined plan search under the same treasury,
defense priority, risk and work limits. Stability, reserve refinement, unsupported
income/terminal cases, actual planner input assembly and live execution remain
explicit work before releasing a new AI. Obtain a new go-ahead; Sol High advised.
