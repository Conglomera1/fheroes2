# C3C2: Farm/TG decision stability, observation only

Parent: d0c21ed71f35483d93f30c8baa505d9d40afe8ff (C3C1).
Branch: knight-ai-lot-c3c2-20261108.

`EconomyStability::select/observe` wraps the combined search using the SAME frozen
Risk::Session, M::Budget and Search::Work. It returns a stability certificate,
NOT permission to change the live game. `actionable` remains false. No planner
call site, INI default, save format, graphics or Windows package changes here.

## Decision gate

Compare only complete, funded central/required-risk plans in the same defense
comparison scope and common horizon. Compare the proposed role change against
the verified current continuation and the best verified alternative that makes
NO optional role transition. Use the stronger of those references. An initial
Farm build is ordinary; TG construction and TG-to-Farm reconversion are role
changes. Retained base commitments remain common and are not charged twice.

Outside a newly viable prioritized defense, require:

- no deterioration of the lexicographically prioritized defense vector;
- robust gain STRICTLY greater than 0.10 * max(1, abs(old), abs(new));
- no central deterioration beyond 1e-6 * max(1, abs(old), abs(new)).

Scores already include all dated construction, conscription and recruitment
costs. No second monetary penalty is added. Unknown defense/funding is not a
negative result or an emergency. Actual financial/legality failures can invalidate
an active commitment, but never waive funding, priority defense or per-conversion
checks. An unresolved common-horizon review prevents new role certificates.

Continuation of the same verified active schedule does not have to beat itself
by 10%. It still has to pass EACH optional conversion's marginal check. This
is conservative: an old commitment does not receive an unlimited authorization.

## Individual proof and legal reconstruction

For every optional conversion in a proposed plan, remove ONLY that conversion
and rebuild its dated graph. Keep unrelated investments, construction dates,
shared payments and military purchases. Reevaluate the full program, never
subtract isolated project scores. Require the same defense/margin/central gate
for each conversion. A first failing proof rejects that combined plan.

If removing the TG reduces available recruits, cap only the affected OPTIONAL
purchase to the exact stock C2 reports at its failure point. If removing a Farm
leaves an optional purchase unfunded, cap it to the exact affordable quantity
across ALL SEVEN current balances. Zero becomes removal of that purchase. Take
the tightest cap reported across failing variants; rerun the common candidate
through central and all required risks. Repeat only while shared work permits.
These caps do NOT modify the original proposed winner or a required purchase.
They create a legal counterfactual, not free troops or virtual cash.

C2 exposes these two exact failure quantities as diagnostics. Its legality,
charging and game rules stay unchanged. Each repair makes a strict reduction;
every full retry consumes the original projection/day/attempt/force counters.

This is a bounded reconstruction, not a second exhaustive search. It does not
reschedule dependent construction, invent routes, remove fixed commitments or
solve every military alternative. A still-illegal, unfunded, unknown or truncated
counterfactual produces NO proof of benefit. Such conversions are rejected.

## Shared ceiling and fallback

Reserve up to one quarter of the remaining projection allowance (at most 64 of
256) for certificates. The search preflight honors this floor for optional
candidates; protected references still get first use of the existing allowance.
No counters are copied/reset/restored. Mechanics, force and merge exhaustion
can still stop a proof. Schedule-equivalent already checked plans are reused.
Counterfactual graph rebuilds and repairs consume shared merge work.

A conversion needs all its proofs. Otherwise choose a fully verified examined
plan without optional role changes. If none exists, return no certificate.
The search's raw `best` is not the stability result: consumers must use
`selected` and `certified`, and must STILL await future execution integration.
A repaired counterfactual is a proof comparator, not an extra searched proposal.
No optimality or persistent multi-turn stability is claimed.

## Diagnostics

Level 2: STABILITY_EVALUATION, PLAN, PROOF, REPAIR, VARIANT, PLAN_VARIANT,
REJECTED and GAP (all prefixed STABILITY_, kind=evaluation, common evaluation_seq).
Summary includes selected/current/fallback indices, certificate, reserved/remaining
work and separate search/proof evaluation counts. Plan rows map indices to full
dated actions, utilities and D/null. Proof rows identify the exact conversion,
its gain/margin/reason, cache use and verified counterfactual utilities. Repair
rows give original/retained quantities; variant rows give funding failures and
minimum seven-resource balances. Gaps remain explicit. Level 0 emits none.
No observation result is logged as an actual purchase.

## Verification

- 48 targeted stability oracles, ASan/UBSan (leaks disabled; no leak claim).
- Strict 10% boundary, negative/small scores, central tolerance, incompatible
  frames, urgent defense, unknown deadlines, individual useless conversions,
  two-town masking, inverse Farm conversion, stock/affordability repairs,
  rare resources, required-purchase preservation, two risk variants, invalid
  active commitment, shared projection/day/merge limits, horizon and determinism.
- 12 native processes: generated C3A proposals / irrelevant bundled TG /
  productive TG, each default/custom INI and diagnostics off/on. Real engine
  strengths, prices, growth and pools. Search itself leaves funds/armies intact.
  Selected CURRENT-DAY actions are then bought with the normal game routines;
  funds and armies match C2. No real multi-day battles or campaign claim.
- Productive native case: 11 Archers on day 4; without TG the legal comparator
  can buy only 8. The marginal benefit passes under both INI profiles. The
  proposed 11 remains intact; only the counterfactual is reduced.
- Rich generated case: 191 candidate evaluations + one baseline use 192
  projections, leaving 64 for proof work. The ordinary winner needs no role proof.
- Existing search/risk/project native regressions plus all existing pure suites.
  Full native game compiles and links. Windows x86 performance remains untested.

Native runner: `python3 tests/run_economy_stability_engine_test.py <binary>`.
Use the same native SDL dependencies as the other C3 fixtures. Header-only pure
suite needs C++17, src/engine and all src/fheroes2 subdirectories included.

Next proposed bounded work: C3C3, reserve/commitment refinement and remaining
prerequisites before live planner assembly. Persistence, unsupported risk/route
cases, last-town outcomes and strategic game validation remain explicit work.
Obtain a new user go-ahead before starting it. Sol High recommended.

## C3C3 follow-up

Search and counterfactual scores now include exact reserve/funding evidence;
see `README_economy_reserve.md`. A computed conditional envelope is never virtual
cash or permission to execute. Live planner integration remains pending.
