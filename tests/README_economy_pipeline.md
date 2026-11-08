# C3D3: connect frozen planner inputs to observation search

Parent C3D2: 93dc72791c76f648bc3a70ba7d19881ae82144c3.
Intermediate: 8a978bfab3ab7208e0e431e4b8693efab323d32f (21 initial pure checks,
initial game build, before native/final horizon checks).
Branch: knight-ai-lot-c3d3-20261109. Observation only; no 1.1.18 migration.

## One live, opt-in connection

The existing diagnostic-only pre-hero-planning capture now calls
EconomyPipeline::observe. It uses real Kingdom engine force/defense adapters.
All choices remain hypothetical: no purchase, transfer, role change, cancellation
or persistent policy is applied. The returned frame/search/session dies before
legacy AI decisions continue. Runtime diagnostics <2 do not launch this pipeline.
No new INI key or gameplay default is introduced.

## Gate and coordinator

The coordinator accepts ONE frozen Frame and a shared Work object (128 path
queries, 16384 candidate-generation work, 256 projections, 24576 days, 200000
training attempts, 131072 force evaluations, 16384 merge work).

Invalid/incomplete knowledge, unverified local assignments, missing paths or
uncertain enemy recruitment remains held BEFORE generation/projection search.
Do not clear gaps simply to get a score. Complete input coverage is not strategic
omniscience. The ordinary/fiscal/military candidates remain C3A's bounded set.

Generate proposals. If a candidate, retained action, active continuation or known
outside-H threat requires a longer view at H=14, rebuild EVERY candidate at H=28.
All plans use the SAME captured world, effective INI and enemy observations.
Discard the old proposals. No old score is compared to a wider score. No exact
projection was spent on the shorter frame before widening.

The input Frame now retains its observed enemy records, priority order, stationary
visitor evidence and raw path results. Widening reuses those raw routes without
new path queries or extra path credit. Zero remains unknown; an absent cached path
is never invented. Paths are keyed by source/destination and movement, in the same
frozen state. No world recapture or new enemy knowledge can favor only one plan.
Beyond H=28, known effects remain incompletely ranked; no indefinite extension or
claim of durable surplus. Future retained payments can still block the reference.

Verify a common reference through the C3D1 preparer. No project is marked revisable
here: the live planner does not yet supply trusted economic commitment authority.
If the reference fails, return held. Use the SAME verified Risk session and budget
for Search/Stability; keep cached baselines. A verified examined choice may be
reported observed/partial. Missing choices remain held. `actionable` stays false.
Search remains approximate, with its original gaps and proof reserve. Neither an
observed nor a partial result establishes exhausted military opportunities.

## One supported standing mission

Legacy `addHeroToMove` excludes PATROL heroes with patrol distance zero. Capture
can therefore confirm local use for a friendly visitor with exactly that mode.
No visitor in sleep mode, with movement remaining or with merely no current target
is treated as permanently defending. Stationary IDs must match unique captured
visitors; foreign/duplicated references fail. Future costs/combat outcomes are not
changed by the classification. Other mission types remain unconfirmed.

TownArmy still includes garrison+visitor once. Local readiness is not remote
reinforcement. Refresh input/mission evidence before any future execution.
No hero transport, general mission planner, required troop quantity or persistent
commitment is invented. Those remain subsequent integration work.

## Diagnostics

PIPELINE_EVALUATION contains status/reason, searched/widened flags, common end
 date, generation passes/proposal count, reference attempts and ALL remaining
work counters. PIPELINE_GAP keeps incomplete coverage/reasons explicit. Existing
STABILITY records describe the hypothetical chosen plans/proofs. Widened input
records are logged as well, so new deadlines are not hidden behind a changed H.
All rows are kind=evaluation. No observation is labelled an actual purchase.

## Verification

- 23 final targeted oracles, plus ASan/UBSan (leak detection disabled).
- Valid context searched without world changes, ordinary reference, held visitor,
  exact stationary evidence and wrong hero ID, unknown route/no projection work,
  same-snapshot H=28, cached zero/distant paths, horizon overflow, retained date
  20 included, date 30 unverified, failed unrevisable payments, shared projection
  and generation exhaustion, deterministic replay/counters.
- 16 native processes: actual Kingdom/towns/armies/hero modes/INI and force/defense
  adapters; known route, unknown route, visiting hero, zero-radius patrol hero x
  default/custom INI x diagnostics off/on. Capture -> generation -> common reference
  -> risk -> search -> stability. Exact original funds/armies preserved afterwards.
- Route evidence in these NATIVE fixtures is controlled, not a map-path simulation.
  Full real KingdomTurn UI/path/turn-order/Windows performance validation remains
  due. The production hook compiles/links; these tests do not prove full-game
  decision neutrality of additional diagnostic path queries.
- Existing input, recourse, stability and candidate-generation pure checks;
  existing input/stability native processes; full native game compilation/link.

Next: trusted live mission/commitment classification, remaining unknown-risk cases,
then persistence and execution integration with real-game validation. No new
player package is required at this checkpoint. Require new user go-ahead;
Sol High remains recommended.
