# C3C3: funding evidence, commitments and conditional reserve envelopes

Parent validated C3C2: 0819f22a371781c1e77913ab3353e9615e132af6.
Intermediate source-only WIP: 20a487a6a3b81e2c3631c006cd45f541c9e9994c.
Final branch: knight-ai-lot-c3c3-20261108.
Observation only. No live planner execution, new INI, gameplay/save changes,
Windows package or balance-campaign rerun.

## Exact evidence first

`EconomyReserve::assess` inspects an EXISTING full Risk result. It spends no
projection and gives no counters back. A protected trajectory requires a Scored
risk result, central first, unique scenarios, at most three variants, every actual
C2 financial result AND dated C1 reconciliation covered, no cancelled required
action. Incomplete military/risk coverage stays uncertified. Search and stability
score summaries now carry this evidence and require it for a verified plan.

For a protected sequence, the additional uncertainty funding need is ZERO: its
payments, including retained future purchases and actual automatic charges,
already fit in real funds in every required scenario. Those payments are not
added as a second reserve. Variant minimum balances remain OBSERVED headroom,
not a fixed amount that can safely be spent now. TG cash-dependent charges can
make feasibility non-monotone. Every changed investment still needs the normal
full central/risk evaluation and stability gate.

`commitments(context,candidate)` materializes existing IDs once, retains base
payments and newly required steps, and reports all actions beyond the common
horizon with the next known milestone. Conflicting IDs fail; no new debit is
created. A future milestone is not discarded, assigned a certified zero cost or
reserved indefinitely. It requires common horizon/plan review. This audit helper
does not serialize projects or itself assemble future planner commitments.

## Conditional envelope, never an execution certificate

`envelope` takes a C1 ledger and verified TG morning periods. The caller MUST
explicitly certify the fixed income and schedule/ownership assumptions in that
scenario. These flags default false. Do NOT mark taxes from cash-dependent TG
consumption as fixed merely because one real replay observed them. Do NOT invent
market trades, deliveries or victorious battle income. Automatic assembly of
these conservative ledgers from all world situations is NOT implemented here.

The helper rebuilds dated fee ceilings using actual effective INI odd/even
calendar limits, enabled flag, fee and Peasant price. It charges the maximum
permitted attempts at hut-source cost, starting on the provided paid mornings.
No success-rate or composition discount: failed attempts are still paid. Existing
automatic-bound entries are refused to prevent double charging. Synthetic morning
IDs avoid all supplied payment/morning IDs. C1 validates stocks and exclusive use.

The maximum seven-resource prefix deficits form a candidate sufficient envelope.
The helper checks resource capacity and REPLAYS that candidate against the same
fixed-flow/bound ledger, since clipping income can discard an early cash grant.
If that recheck fails, the envelope remains unresolved. This is no binary search
on real TG mechanics, no minimum grant claim and no real money injection.

`refine` gives exact funded evidence priority over ANY excessive bound: zero
additional need, even if maximum fees would have produced a gap. Otherwise it
requires a complete supplied ledger for every actual exposed scenario, common
starting funds/date/horizon and explicit assumption verification. It returns the
coordinatewise maximum envelope, NOT an optimal resource trade or a minimum
reserve. Unknown coverage, missing variants, limits, capacity or legality failures
remain unknown. `actionable` is always false, even with a computed envelope.

A positive envelope calls for reducing/deferring discretionary spending or
acknowledging exposure. It never authorizes an unfunded program and never pauses
automatic conscription. The helper does not turn a limited search into evidence
that every ordinary military outlet has been exhausted.

## Verification

- 44 targeted oracles: dated/shared commitments, seven resources, conflicting IDs,
  horizon milestone, no double reserve, conditional envelope, calendar reset,
  custom fee/limits, success/composition independence of paid attempts, disabled
  training, duplicate periods, pre-existing charge refusal, ID collisions,
  extreme settings/capacity, clipped-income recheck, partial/missing risk variants,
  common scenario funds, exact evidence overriding an excessive bound, limits.
- Real copied scheduler case: at 99 gold a 99-gold payment on day 2 is possible;
  at 100 gold automatic training consumes 100 and the payment fails. Both have
  the same shared evaluation cost. No monotonic funding assumption is used.
- Native Search/Stability runners additionally require verified winners' retained
  payments to be protected; Search also requires known additional need. These
  use actual INI, engine army strength/prices, normal growth and existing real
  day-1 replay. Diagnostics off/on remain decision-neutral.
- ASan/UBSan on the new suite, leak detection disabled (no leak claim).
- Regressions: C3C2, C3C1, C3B3, C2, C1 pure/native as relevant. Full native game.

Important remaining limitation: a financially infeasible common M0 baseline is
still reported `baseline_unavailable` by the military evaluator. It gives no fake
military score or reserve certificate. Preparing a feasible emergency reference
and explicit recourse belongs to subsequent integration work, not an invented
reserve top-up. Persistent policy, planner threat/path assembly, unsupported
income/terminal/route cases, surplus/recurrent-outlet certification and strategic
Windows game validation are still required before live autonomous conversions.
