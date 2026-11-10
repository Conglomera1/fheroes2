# C3D5F — recovered native recruitment observations

10 November 2026. The completed local fixture, runner, 18 raw logs and JSON
observations were recovered after the interruption. They extend the three initial
observations saved at e9e47d5149b2398bc1e36c0f5205bdef99878b6f. Production remains
identical to validated C3D5D (9e84baeb37a814d5487b126ad2b76231ec99fd3f).

The original draft was replaced locally by `recruit_receipt_probe.cpp` before
these runs. The historical draft remains accessible at 39d2cd4b497f22789f94a5db82cba8ca501b71c6.
`run_recruit_receipt_probe.py` records observations; it is not an API fix or a
green regression suite. Raw inconsistency retains process exit code **2**.

Each row below was executed in two fresh processes, with diagnostics **0 and 2**.
Amounts and destinations match across each pair. Gold is measured after setup.

| Scenario | Invoked / accepted | Request / stock | Added / removed | Gold paid | Destination | Exit, both levels |
|---|---|---|---|---:|---|---:|
| raw_exact | yes / yes | 8 / 8 | 8 / 8 | 1200 | garrison | 0 |
| raw_overstock | yes / yes | 9 / 8 | **9 / 8** | 1350 | garrison | **2** |
| guarded_overstock | no / no | 9 / 8 | 0 / 0 | 0 | none | 0 |
| raw_empty | yes / yes | 1 / 0 | **1 / 0** | 150 | garrison | **2** |
| guarded_empty | no / no | 1 / 0 | 0 / 0 | 0 | none | 0 |
| raw_zero | yes / no | 0 / 8 | 0 / 0 | 0 | none | 0 |
| poor | yes / no | 8 / 8 | 0 / 0 | 0 | none | 0 |
| full | yes / no | 8 / 8 | 0 / 0 | 0 | no visitor, garrison full | 0 |
| visitor | yes / yes | 8 / 8 | 8 / 8 | 1200 | visitor, garrison full | 0 |

There are **14 consistent observations and 4 RED observations**, representing
**two distinct inconsistent scenarios**, each at both diagnostic levels.
These calls deliberately bypass ordinary caller limits. No claim is made that
normal game/AI paths issue them. The stock guard exists only in the fixture.

## Evidence and verification at recovery

`results/c3d5f/` contains the 18 original logs, `observations.json`, `summary.txt`
and `recovery_verification.json`. Recovery compared every RESULT field and raw
return code against the JSON, checked the nine diagnostic pairs, and checked
purchase traces within PROBE_BEGIN/PROBE_END. Diagnostics 0 emitted no economic
rows inside that interval; level 2 emitted a purchase row for each accepted call,
whose request and stock-removal fields matched the measured result.

The recovered native build log and fixture compile log contain no compiler
warning/error lines. Their hashes and the hashes of the recovered sources and
binaries are retained in `recovery_verification.json`; binaries are not delivered
as player packages. Those hashes identify recovered artifacts and are not a new
reproducible-build or source-to-binary attestation.

**No new compilation or engine run was performed during recovery.** No historical
balance campaign or unchanged regression suite was repeated. No sanitizer or
strategic validation is added by these observations.

The earlier Library reprise described the pre-build draft. This recovered record
supersedes its statement that C3D5F had not been compiled/executed; the original
reprise remains a historical checkpoint.

## Limits and next preparation

The probe uses Archers. It compares all seven balances, unrelated creature counts
and the current visitor, but does not establish every building, upgrade, stack
overflow, visitor binding, transfer or rollback invariant. In particular `full`
has no visitor: **both armies full** is a separate untested case. No normal AI
action is attributed to a registry program from these traces.

Define the future executor's preconditions and final synchronous confirmation
from these results and C3D5E. Keep execution authority separate from a hypothetical
registry ticket, and preserve the uncertain-result stop. Do not patch the global
recruitment API to turn these RED observations green.
