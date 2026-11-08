# C3C1: bounded combined-plan search, observation only

Parent C3B3: `e075cd79bc8f785e2a12b3364704771e1a5204f4`.
Branch: `knight-ai-lot-c3c1-20261108`.
No live planner execution, gameplay/INI defaults, graphics or save-format changes.
Every result has `actionable=false`: stability, per-role counterfactuals and
actual planner integration remain required before autonomous conversions.

## Input, references, common state

`EconomySearch::search` takes ONE existing EconomyRisk::Session, C3A proposals,
the same external M::Budget and a shared Work merge counter. Its initial context
comes from that evaluator's frozen context, not a potentially different snapshot.
The world must remain unchanged through this read-only observation pass.

The no-new-investment reference retains base commitments; optional active
continuation is built from context.active. Both are checked first and protected
from optional merge-budget exhaustion (never from the common projection limits).
The best ORDINARY program is derived from fully checked ordinary proposals and
combinations, not equated to the no-new-investment reference. Ordinary means no
new optional TG build; already retained base commitments remain common.

At most 64 optional proposals/4096 total retained-active-source action entries
are accepted; excess/invalid optional inputs are counted and do not erase the
references. The common comparison horizon remains the evaluator's 14/28 days.
A C3A longer-horizon flag triggers explicit common horizon review/partial status;
this function does not silently extend only one candidate. Distant unsupported
plans are not scored as zero. The caller must recreate the COMMON comparison
when an extension is required.

## Composition and identities

`combine` merges the entire dated action sequences, not their presort gains or
standalone military scores. Same town/day/building prerequisites are shared and
paid once. Explicit shared step IDs are preserved once only if compatible;
conflicting reused IDs fail. Different recruitment IDs or separate zero-ID
purchases remain separate real expenses. One source component is not added to
itself indefinitely.

The per-day partial order of each component is retained with a deterministic
acyclic ordering. Retained actions stay before optional actions on that day,
consistent with C2 materialization. Contradictory shared order or different
buildings using the same town's construction action are rejected before a full
projection. Stronger required flags cannot silently rewrite a retained optional
commitment. Chains can contain more than six buildings; the six-pass bound is
NOT a chain-length limit.

Actual resources, stock, troop slots, hero-last-stack protection, prerequisites,
calendar income and TG charges are checked by the full C2/Risk projection of
the MERGED program. There is no summed standalone-budget or summed-force proof.
Existing unsupported transport/upgrade/market operations remain unsupported.

## Ranking and bounded search

Only a complete funded risk result with finite central/robust utilities and a
complete CENTRAL defense vector in the common comparison scope can be ranked.
Unknown, partial, exposed or unverified results never replace a verified reference.

Compare in CP7A order: lexicographic D, robust utility, central utility, active
continuation, additional gold paid TODAY, stable generated plan ID. Utility
numerical ties use 1e-6 * max(1, abs(a), abs(b)). That relation need not be
transitive, so beam selection uses deterministic scans, never a std::sort
predicate that would violate strict weak ordering. Common retained payments are
excluded from the extra immediate-cost tie break; no new monetary penalty is
subtracted from utility.

After isolated proposals, retain at most eight promising plans; preserve the
best ordinary and valid active continuation within that beam. For at most six
passes, try additions and substitutions of one component by another whole
proposal (which may itself be a multi-action complementary chain). Reevaluate
full schedules, not marginal gains. Equal schedules reuse a compact score cache.
Alternative decompositions of the same schedule are not exhaustively expanded;
calendar rescheduling and arbitrary multi-component replacement packets remain
outside this initial heuristic. Zero-gain stepping stones can be lost in beam
reduction; C3A's complete fiscal chains help but do not prove exhaustive coverage.

All references, risk variants and combined evaluations share the same remaining
256-projection/day/attempt/force counters. Read-only cost accessors allow a
projection preflight before starting a candidate that cannot fit. Full projections
may still stop on a deeper day/attempt/force limit; they remain unranked. Raw merge
attempts share a 16384 bound, so rejected/cached combinations cannot cause an
unbounded search. Up to 256 compact schedule records are retained.

When a limit is reached, return the best VERIFIED examined plan and references
with partial status/reason. Never manufacture a winner from partial utilities.
No result certifies a global optimum, durable surplus or complete action coverage.
Unresolved defense deadlines are reported, not treated as an impossible defense;
futile-spending/recourse review remains before live execution.

## Compact records and diagnostics

Cached plans keep actions, member indices, scalar scores, defense bits, comparison
scope and small variant funding witnesses. Full C2 end-state/catalog copies and
full risk trajectories are discarded after evaluation, avoiding multiplicative
kingdom-sized state storage across all candidate records.

Diagnostic level 2 adds SEARCH_EVALUATION, SEARCH_SOURCE, SEARCH_PLAN and
SEARCH_VARIANT, all kind=evaluation and linked by evaluation_seq. Source rows map
member indices to source IDs/labels/families (active continuation is explicit).
Plans include actions, D or null when not complete, utilities, ordinary/active
flags, extra immediate gold, variants and gaps. Variant rows include actual
funding, failed payment day/step/reason, cancellations, required cancellations and
minimum balances for all seven resources. Summary includes protected references,
attempts, rejected merges, cache hits, beam cuts, substitutions, omissions, passes,
stop reason and every shared remaining work counter. These are hypotheses, not
real purchases or authorizations. Level 0 produces none of these records.

## Verification

- economy_search_test.cpp: 32 targeted cases, passing ASan/UBSan with leaks
  disabled (no leak-validation claim). Covers shared gold/rare resources, Knight
  Farm financing Warlock troops, shared prerequisites, distinct/shared/conflicting
  step identities, order cycles, defense deadlines, robust-before-central ranking,
  active ties, protected references, common limits, stable repeat, horizon review,
  immediate-cost commitments, beam/substitutions and a nine-action chain.
- economy_search_engine_test.cpp + run_economy_search_engine_test.py: actual
  C3A-generated proposals for two Knight towns and a Warlock town, full engine
  force/INI forecasts and bounded search; four fresh processes (default/custom
  INI x diagnostics off/on). In this fixture, 255 unique evaluations + one cached
  reference projection consume the 256 bound; 263 schedule cache hits, one pass.
  It returns a verified best examined plan and ordinary reference, not an optimum.
  Winner rematerialization passes C2 and today's actions are actually bought with
  engine routines, matching funds and armies. Future special weeks/battles are
  not replayed or claimed validated. Search itself leaves the world unchanged.
- C3B3: 33 pure cases and six native processes still pass; C3B2 46 + four,
  C3B1 22 + four, C2 89 + six, C3A 41 and Taxpayer policy checks still pass.
- Full native game compiles/links with no warnings in the final log. No Windows
  x86 performance/game-campaign claim; no old balance simulation was rerun.

Pure tests: C++17 with all src/fheroes2 directories and src/engine included.
Native tests: same libraries/config as full game, all game objects EXCEPT main
fheroes2.o, engine/Smacker/SDL/image/mixer. Pass the native fixture binary to its
Python runner. No proprietary game graphics/data are needed.

Next proposed bounded block: C3C2, decision stability/role-change margin and
counterfactual checks within the SAME evaluation budget. Reserve/refinement,
unsupported income/terminal/route cases, planner input assembly and live execution
remain explicit work. Search can exhaust the budget; the next block must preserve
a verified ordinary fallback instead of granting extra proof evaluations. Obtain
explicit go-ahead; Sol High recommended.
