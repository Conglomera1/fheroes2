# C3D5H — isolated exact-recruit adapter

Parent: C3D5G `2ced18db0b0eea056baaaf9f3733c00545fca3b2`.
Test-only implementation under `tests/`; no production include or AI connection.

Intermediate checkpoint: `950ed92b6584cb8a0dc7e0a0877f482064eecf9d`, branch
`knight-ai-lot-c3d5h-wip1-20261110`: 80 native processes passed; sanitizer runs
were pending at that checkpoint.

Final validation: compiled/linked against unchanged C3D5D native debug objects,
no final native compiler/linker warnings. **80 native processes / 40 diagnostic
pairs passed**, plus the same **80 processes with ASan+UBSan on the fixture and
adapter only**, `detect_leaks=0`, UBSan halt-on-error. Native and instrumented
RESULT records are identical. No full-engine sanitizer or leak-validation claim.
The sanitized link emitted one dependency warning: system libtiff's libjpeg.so.62
may conflict with libjpeg.so.8. It is preserved in `sanitized-build.txt`; image
decoding is not exercised. The 80 instrumented controls completed without a
reported sanitizer error. No game sources or dependencies were modified to hide
that warning.

The generated fixture explicitly accepts ONE complete current-day local Recruit
program (up to 16 steps), never an observer registry or a legacy hint. This narrow
scope deliberately excludes multi-program ordering, future dates, transports,
buildings, persistence and live KingdomTurn integration. One town, no enemies,
and at most its fixed local visitor are fixture premises, not strategic coverage
in an actual map. No program extension/reacceptance or automatic retry is allowed.

`Owner::prepare` captures actual world/INI/mission state, refreshes before issuing
a ticket and checks the head's stock, actual upgraded creature/price, all seven
balances, arithmetic ranges and garrison-first placement. It verifies the entire
remaining schedule with the existing common Recourse/Risk/Project implementation,
real local scorers and the caller's SAME work counters. Empty complete threat
coverage is explicit only for this generated fixture; incomplete coverage holds.

`Owner::run` requires that exact preparation, the current ticket and unchanged
physical/config/visitor mission evidence. It dispatches one real recruitment,
then compares all seven balances, all six stocks, buildings/disabled flags,
construction availability, actor/day/config and BOTH armies by slot. A verified
success records Executed and removes one head. A proven clean refusal records
Failed and retains the head. Contradictory or uncertain effects block both retry
and new preparation. A rejected/throwing post-action record also blocks the owner.
There is no reconciliation bypass or simulated payment applied to the world.

The callback parameter and `invalidateForTest` are explicitly test fault seams.
`true_empty`, `false_effect`, incorrect costs/quantity/destination, slot reorder,
post-effect record invalidation and exception cases are **injected tests**, not
additional engine bugs. `false_clean` first injects a clean refusal, then checks
an explicitly invoked retry after refresh/reverification. The RESULT `status`
describes the first dispatch result; `calls` counts callback dispatches, not
necessarily underlying engine API calls. Both final effects are independently
checked by the Python numeric oracles and purchase-trace counts.

The `pure` mode contains 38 distinct synthetic arithmetic/classification oracles,
including affordability/overflow and cost mismatch for each of seven resources.
It is run twice, not counted as 76 independent oracles. `identity` additionally
rejects 12 mutations of ticket identity before executing the untouched ticket.
Native fixture assertions stay enabled (`-UNDEBUG`).

## Reproduce

First restore the matching native diagnostic debug game objects and SDL2/
engine/Smacker dependencies from the prior recipe. Source under `src/` remains
the same as C3D5D; do not rerun old balance campaigns to restore this fixture.

Use `build_exact_recruit_adapter.py --output PATH --sdl-flags=FLAGS
--sdl-libs=LIBS --ldflags=FLAGS` with the matching native build flags. It compiles
only the fixture, then links all existing game objects except `fheroes2.o`.
`--sanitize` instruments the fixture/adapter with ASan+UBSan; the existing engine
objects are still uninstrumented. Do not report it as a sanitized full engine.

Run `run_exact_recruit_adapter.py BINARY --output-dir DIRECTORY`, with repeated
`--library-dir PATH` for runtime dependency directories as needed. Each mode runs
in a fresh process with isolated default INI and diagnostics 0/2. The runner
requires identical RESULT fields and independently specified numeric effects,
plus actual purchase-trace counts. `--modes` supports a justified targeted rerun.

See `results/c3d5h/` for outputs. This implements the FIRST isolated recruitment
block proposed in C3D5G, not its complete future executor/strategic test matrix.

## Final boundaries and next block

All 160 raw process logs, independent numeric observations, build summaries and
source/log hashes are retained. The two C3D5F raw stock discrepancies remain
unchanged; the adapter prevents those requests before reaching the API. No old
balance campaigns or unchanged historical suites were rerun. Production source,
INI/defaults, translations, saves and resources remain unchanged. No release,
Windows executable or player package is produced.

Still unsupported here: multiple accepted programs/towns, future-day execution,
real threat/mission coverage, live ownership/acceptance, persistence and recovery
of uncertain actions. Complete configuration is compared across the operation,
but the native matrix uses isolated default INI only. Seven-resource arithmetic
is synthetic; this block does not recruit every faction's resource-priced unit.
Record rejection after effects is injected and tested; the recording-exception
catch is defensive code, not an injected allocation-failure reproduction.

Proposed next bounded block: an isolated supported building adapter, starting
with Farm/TG effective prices and conversions under default/custom INI, with
building-specific final effects. Keep live AI integration and exact transfers
separate. Stop after this checkpoint and the user-facing bilan; **Sol / High**
recommended for that next block, after another explicit go-ahead.
