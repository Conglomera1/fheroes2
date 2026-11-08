# C3D1: explicit preparation of a feasible common reference

Validated parent C3C3: f043ac9c8bb6d3f7034c213c681a4846ba780556.
Intermediate source checkpoint: db0e3f947adaa01eb1234c3a39b185756d76fe4f
(native validation still pending at that savepoint).
Final branch: knight-ai-lot-c3d1-20261109.
No upstream 1.1.18 migration, gameplay/INI/save changes, autonomous actions or
Windows package. Preparation and all "releases" below are hypothetical only.

## Problem and boundary

An unpaid retained action made the common military baseline unavailable, so no
candidate could receive a comparable utility score. The financial failure was
also discarded from its diagnostic result. Military now keeps the exact C2
financial failure, while STILL providing no utility for an impossible baseline.

`EconomyRecourse::prepare/observe` constructs a revised comparison frame only
when the CALLER explicitly marks complete unexecuted projects as revisable.
A permission contains the project ID and snapshots of EVERY retained action in
that project. IDs, dates, required flags, counts and operation details must match.
Unknown, duplicate, partial or stale permissions fail. A required step is not
implicitly revisable: the caller must establish that abandoning its project is
allowed. The live planner does not yet supply these classifications automatically.

## Procedure

1. Freeze the original world/config/context; validate permissions without paying
   anything or changing that world. Actions before the current decision are not
   accepted as future commitments; previously spent money stays in the snapshot.
2. Build a fresh Risk session and check its no-new-investment reference across
   the central and supported required scenarios, with the original shared budget.
3. Return it untouched if already fully verified. Otherwise require concrete C2
   evidence: an unpaid purchase, a supported materially illegal operation, or a
   required destination cancelled by a verified ownership loss.
4. Missing catalog/income/route/defense information, an unknown horizon, work
   exhaustion or absence of evidence does NOT authorize project abandonment.
5. If any failing project lacks explicit permission, stop blocked. Otherwise
   remove those projects' future actions AS WHOLE PROJECTS in the copied context.
   Preserve unrelated commitments and ALL initial money, populations, troops and
   already built infrastructure. No refund, gift, debt or cancellation of TG fees.
6. Drop the old optional active continuation, which may depend on a removed
   prerequisite. Generate fresh proposals from the resulting context. Do not
   replay old candidate scores, merges or defense-scope comparisons.
7. Repeat only while a project was explicitly removed and shared work permits.
   Return a session ONLY when the new common reference is fully verified and
   funded centrally and in the required risk variants. The caller then uses this
   SAME session with Search/Stability, preserving cached baselines and counters.

Maximum 64 permissions / 1024 base actions / 2048 total base+permission entries.
Every retry spends the same <=256 projection, day, training-attempt and force
allowances; there is no private replacement budget. Preflight may conservatively
stop with a little unusable allowance instead of starting half an evaluation.
At most number-of-permissions+1 attempts; rejected/unknown states give no score.
The returned session holds a fresh frozen context and defense comparison scope.
`actionable` remains false, including when `status==Ready`.

## Conservative limitation

This block does not find the optimal subset of old commitments. If an EARLIER
revisable investment causes a LATER protected payment to fail, it stops blocked;
it does not infer permission to erase the protected payment or guess which earlier
investment to undo. Rescheduling, alternative defense/evacuation choices, last
stand and richer recourse generation remain later planner work.

An unknown/exposed required scenario can still prevent a final reference even
after justified hypothetical removals. Such partial results contain diagnostics
but NO usable session or authorization. The caller must never apply a partial
release list to the game. Obligations outside the common horizon remain unresolved.

## Diagnostics and validation

Level 2 adds REFERENCE_PREPARATION, REFERENCE_RELEASE and REFERENCE_FAILURE,
all kind=evaluation and linked by evaluation_seq. They identify status/reason,
trial count, project/action IDs, observed scenario/date/failure, regeneration flag,
funding protection and remaining shared counters. No false actual cancellation.
Level 0 emits none; no random draws or wall-clock selection introduced.

- 28 targeted oracles: blocked/authorized failure, full/stale/duplicate permission,
  original reference retained if feasible, no refund, unrelated commitments,
  active continuation invalidation, successive project failures, dynamic disabled
  TG prerequisite, unknown data/horizon, shared projection/day limits, destination
  loss in a required scenario, coverage uncertainty and follow-on real scoring.
- Native fixture: actual game prices/stocks and engine force, intentionally
  overcommitted recruitment. Without permission it remains blocked; with explicit
  permission it releases only that future project, regenerates C3A proposals,
  runs stability in the SAME session/budget and replays selected CURRENT-DAY
  actions with normal game routines. Original world remains intact in observation.
- Native default/custom INI x approval denied/allowed x diagnostics off/on:
  8 fresh processes. Diagnostics must not change decisions/work counters.
- ASan/UBSan on targeted suite, leaks disabled (no leak-validation claim).
- Relevant existing pure and native suites remain regression checks. No balance
  campaign is rerun and no Windows performance or strategic-game success claimed.

Next bounded work: assemble trusted live planner inputs and revisability data,
then address the still-uncovered risks/recourse/persistence before execution.
Require fresh user go-ahead. Sol High remains recommended; no user test needed.
