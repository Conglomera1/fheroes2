# C3D4: frozen legacy mission and resource-intent evidence

Parent C3D3: 5025439c0dffa0c3abbdfa4935bfd44323b93981.
Observation only. No gameplay/default/save/INI/resource/upstream change.

Read actual friendly hero roles (Scout, Courier, Hunter, Fighter, Champion),
current position, visitor identity, movement, sleep/patrol settings and existing
path destination. Do not select/recalculate targets, move heroes or query routes.
Collect existing planner ATTACK/DEFEND/REINFORCE priorities with related tiles,
and its seven BudgetEntry hints, mapped to Project resource order.

Only a zero-radius PATROL visitor is fixed local use, per legacy addHeroToMove.
A mobile patrol, sleeping/exhausted visitor, Courier or DEFEND target is not a
permanent defender. A path destination is neither a funded delivery nor an ETA.
Priority tasks are not allocated to heroes. BudgetEntry.missing is a maximum
heuristic across towns, not a sum, payment schedule or reserved resource amount.
Do not insert these hints into Project actions or grant recourse permissions.

Validation bounds: 256 towns/heroes, 1024 priority targets, 2048 related links,
seven unique nonnegative resource hints; reject malformed/duplicate identities,
unknown roles/task types, missing or mismatched visitors. Invalid mission capture
stops before route work. Mobile visitors retain the existing coverage gap.
Field heroes/priority/budget limitations are recorded separately from local
coverage: the existing scorer still evaluates local readiness only. It does not
claim all external military outlets or future commitments have been modeled.

Frame retains the classification during 14->28 widening without recapturing live
heroes, budget hints or targets. A later live mission change needs a fresh frame;
the old frame remains the old evidence. No execution authorization follows.
PLANNER_MISSIONS explicitly states future_commitments_complete=false and
military_outlets_exhausted=false. Detailed hero, target, relation, resource and
limitation rows are evaluation-only and diagnostics-level-2-only.

Intermediate source checkpoint: 9fd714c3558a0cf1b0c65e69bcee4e3c5d7d0188
on knight-ai-lot-c3d4-wip1-20261109 (25 mission checks and input/pipeline checks;
native validation pending at that point).

Completed validation:
- 25 pure mission oracles; warning-free, ASan/UBSan (detect_leaks=0).
- Existing inputs 26 and pipeline 23 pure checks, as the capture/frame changed.
- Four new native processes: default/custom effective INI x diagnostics 0/2,
  six real friendly heroes across all five roles, actual path/movement/sleep/
  patrol fields, three priorities and seven resource hints. Stable widening and
  fresh capture after a changed live patrol mode. No new route queries.
- Existing pipeline 16 and inputs 12 native processes, rebuilt against the new
  capture. Same observations, shared budgets, held visitors and exact original
  kingdom funds/town armies. Diagnostic JSON valid; off/on summaries identical.
- Full native game compilation/link, no warnings in final build log.

These fixtures do not run a whole KingdomTurn or map campaign. The private planner
priority/budget adapter is compiled/linked; supplied fixture hints exercise its
destination contract, not the strategic decisions that generated those hints.
No strategic/performance equivalence or Windows build is claimed.

Remaining: date/quantity/beneficiary/transport feasibility for real missions,
real persistent economic commitments and recourse authority, then refresh and
execution policy. No new player package; wait for the next user go-ahead.
