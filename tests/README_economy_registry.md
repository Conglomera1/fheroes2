# C3D5B: in-memory observation-program registry lifecycle

Parent C3D5A: 52fe4d5b4488e67d593ae63b428cefdce6b60b2f.
Isolated header-only registry, no engine callback/production owner, persistence,
save format, policy execution or gameplay/default change in this block.

Refresh accepts a raw fresh Context (no retained/active actions; the registry owns
its pending programs) and the current mission snapshot. It increments revision
on EVERY attempt and invalidates old tickets even if recapture fails. Import all
remaining programs against the supplied world/INI; missing towns, overdue steps
or incompatible input hold the registry rather than erase/reschedule obligations.
Day rewinds require a new instance. The owner must issue a unique nonzero registry
token per game/load, and refresh after EVERY relevant change. There is no automatic
game-change detection or real KingdomTurn hook yet.

Stage explicit zero-ID drafts: allocate monotonic project/action IDs, preserve
them across refreshes, validate with C3D5A before publishing. Failed staging does
not consume published identities or alter an already valid book. No budget/role/
priority is interpreted as authority. Ready still means descriptive, not funded.

A reporting ticket identifies only today's first remaining step, with no pending
dependency. It binds registry token, snapshot stamp and exact Action. It is NOT a
purchase authorization. A future engine callback must confirm ACTUAL full success,
not a projected action or elapsed date. Stale/foreign/mutated/duplicate receipts
fail. Executed removes only that step; its remaining project gains executedPrefix
and stays protected. Completed project dependency links are removed only after
confirmation. Executed/Failed/Uncertain all invalidate the frame; unconfirmed
steps remain. Refresh from the actual resulting state before any new view/ticket.
Failed means confirmed NOT executed, including no partial success. An Uncertain
outcome quarantines the registry even after raw refresh: the action could already
have spent resources. No future reference or retry ticket is returned until an
explicit reconciliation adapter is implemented. A raw state cannot identify the
operation responsible for changes. Registry identity cannot be copied or moved.
No funds, stock, troop or building are changed by the registry, and no refund is
created. Completed steps are not paid twice in the next projected reference.

Bounds inherit C3D5A: 64 programs, 1024 pending actions; low-half non-reused IDs,
revision overflow fails. No implicit reset/pruning, abandonment or rescheduling.
Confirmed receipts are a TRUSTED input contract; no engine success verification
or post-state proof is invented here. Failed/overdue/lost-destination reconciliation
and serialization are future work. Unknown outcomes must not be retried in the
game automatically merely because the registry retains their pending description.

Intermediate: ed5d46c49cd4fc413985e2ed84589c503999963c, branch
knight-ai-lot-c3d5b-wip1-20261109 (35 pure checks, before final three/sanitizers).
Final validation: 38 pure lifecycle oracles and ASan/UBSan with detect_leaks=0
(no leak-validation claim), no compiler warnings. Includes same-day stale tickets,
foreign identities, full/partial completion, failed vs uncertain execution, no
automatic retry after uncertainty, actual post-action copied state, exact pending
ID preservation, dependencies, invalid refresh, overdue actions, day rewind and
new effective TG costs. No production/old algorithm changed; prior C3D5A/C3D4
checks are preserved rather than repeated.

Pure fixture
emulates an actual success using Project, passes the resulting copied state as
fresh capture and checks no duplicate charge. Not a real game callback or campaign.
No native game rebuild is needed: no production translation unit includes this
new header. Existing validated native/game/balance results are not rerun.

Next: bounded production observer owner and actual refresh/receipt adapters,
reconciliation, then persistence/execution. New user go-ahead required.
