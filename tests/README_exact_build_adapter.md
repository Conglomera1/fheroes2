# C3D5I — isolated Farm/Training Grounds construction adapter

Parent H: `9f588bb260ba23b2214ea767001f4087d262f98e`.
Test-only extension; production source and normal AI are unchanged.

Intermediate checkpoint: `a89457841f26f07707d09e204f9ff1b4776e9665`, branch
`knight-ai-lot-c3d5i-wip1-20261110`: 200 native building processes and 80 recruitment
regressions passed; sanitizer runs were pending at that checkpoint.

Final: **200 native building processes / 100 diagnostic pairs pass** across six
INI profiles, plus the same **200 ASan+UBSan fixture/adapter processes**, with
identical RESULT records. **80 freshly compiled recruitment regressions** equal
H's records exactly after the shared-owner change. Native compilation/linking has
no warnings. The sanitized link preserves one known JPEG dependency warning;
see the exact instrumentation limits below.

## Contract and shared lifecycle

The previous test-only owner is parameterized by operation policy. `RecruitPolicy`
retains the old recruitment API/contract and `Owner` alias; `BuildFixture::Policy`
supports ONLY Knight Farm (`BUILD_WEL2`) and Training Grounds. Acceptance,
freshness, common Recourse/Risk/Project budgets, exact tickets, one-step recording,
clean failure and uncertain-effect blocking share the same lifecycle. The new
policy does not authorize a hypothetical observer or a legacy hint to execute.

The fixture is one generated town with no enemies, at most its fixed local
visitor, and a single explicit current-day program. It is not live strategic
coverage. Two builds on one town's current day fail common schedule verification
before the first purchase; the adapter does not consume an intermediate action
or silently postpone the second construction.

Before dispatch, require the real `AllowBuyBuilding`, the matching projected
catalog, effective INI prerequisites, the exact current `getBuildingCost` and all
seven funds. Farm's first construction uses its ordinary catalog price. Farm
reconversion from TG uses `farm_reconversion.*`; TG uses `training_grounds.*`.
Unknown catalogs, shapes, races and other buildings remain unsupported.

After the one real `BuyBuilding`, require the exact seven-resource payment,
mutually exclusive Farm/TG mask and consumed construction-day flag, with all
other buildings/disabled flags, six stocks, both armies, actor/day/config and
visitor mission facts conserved. Additional evidence captures actual income,
tax eligibility, future hut/pool growth and full training-cycle progress.
Conversion changes future eligibility/growth; it must not train or credit today's
income immediately, reset a partial training cycle, or replenish recruitment.

The policy independently predicts the flags/growth/tax delta for this bounded
fixture and compares the actual Kingdom snapshot and income. Compatibility mode
(`taxpayer.require_farms=0`) preserves taxation across conversion. Income
attribution must be supported by Project; arbitrary handicaps or unsupported
income effects are not admitted.

| Operation | Default gold / wood / ore | Custom gold / wood / ore |
|---|---|---|
| Farm to TG | 2500 / 10 / 10 | 4321 / 7 / 13 |
| TG back to Farm | 1500 / 0 / 0 | 1234 / 3 / 5 |
| First Farm | 1000 / 0 / 0 | 1000 / 0 / 0 |

## Native matrix

Default/custom profiles each cover 42 scenarios with diagnostics 0/2. Additional
disabled, extra-prerequisite, zero-price and compatibility profiles cover 16
profile/scenario combinations, also at both diagnostic levels: 200 processes in
all. Default/custom tax rates are 2/3, and custom captain prerequisite is disabled.
Runner expectations are independent numeric cost/flag/tax/growth/progress oracles,
with actual BUILD_ACTION and TROOP_PURCHASE trace counts checked separately.

Cases include both conversions, first Farm, fixed visitors, preserving a real
nonzero training cycle, insufficient gold/wood/ore, existing/banned buildings,
source-Farm ban, spent construction day, missing Farm/captain/INI-required moat,
unsupported building/shape, no acceptance, stale/changed tickets and physical
state, exhausted budgets/incomplete threat coverage and double same-day builds.

The 27 distinct pure receipt oracles per pure-mode process include seven funds,
six stocks, construction flags, income/tax, growth, progress and unsupported
catalog/building checks. Repeated runs are not independent new oracle designs.

False success, false return after a real build, wrong cost/building/stock/flag/
army, post-effect invalidation and thrown callback are explicitly INJECTED
faults, not newly reproduced game bugs. They must block without refund/retry.
The clean-false callback preserves the step, then an explicitly invoked verified
retry succeeds. RESULT status for that mode describes the first refusal;
dispatch count and final effects include the later explicit retry.

## Reproduction

Reuse the matching C3D5D native diagnostic-debug game objects and prior SDL/
engine/Smacker dependency recipe. The shared build script accepts
`--source tests/exact_build_adapter_engine_test.cpp` and the same output/SDL/link
flags as the recruitment fixture. Assertions remain enabled. `--sanitize`
instruments ONLY fixture/test-adapter code; linked engine objects remain normal.

Run `run_exact_build_adapter.py BINARY --output-dir DIRECTORY` with repeated
`--library-dir PATH` as needed. Fresh processes isolate INI caches per profile;
`--profiles` and `--modes` allow justified targeted runs. Run the existing
`run_exact_recruit_adapter.py` against a freshly linked recruitment fixture to
verify a shared-owner change. Old balance campaigns need not be rerun for this.

## Saved evidence and limits

`results/c3d5i/` contains normal/instrumented observations, recruitment regression
records, summaries, build outputs and verification/source hashes. The 480 complete
raw process logs are preserved without loss in three `*-raw.jsonl.gz` archives.
After gzip decompression, each JSON line has `path`, original `log` text and its
SHA-256. Every archived log was compared back to the original output. This keeps
the raw evidence without adding hundreds of separate log paths to the repository.

ASan+UBSan instruments the building fixture/policy and instantiated shared-owner
code; linked production game/engine objects remain uninstrumented. Recruitment
regressions in this block are native controls, not a new instrumented recruit run.
`ASAN_OPTIONS=detect_leaks=0`, `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
No full-engine sanitizer or leak-validation claim. The sanitized link warns that
system TIFF's libjpeg.so.62 may conflict with libjpeg.so.8; the warning is retained
in `sanitized-build.txt`. Image decoding is outside this fixture, and all 200
instrumented controls completed without a reported ASan/UBSan error.

Only Farm/TG are supported; dwelling stock initialization, spells/captain/map
effects and all other buildings remain held. The supported one-town generated
fixture certifies empty threat coverage, not safety on real maps. No live AI
acceptance, future-day/multi-town ownership, persistence or reconciliation of
uncertain actions is implemented. A native case preserves a real partial training
cycle, but this is not an end-to-end saved-game or next-turn executor test.

Production source, default INI, translations, saves, resources and player packages
are unchanged. Existing matching native objects were reused, no full production
rebuild. The recruitment suite was rerun because its owner changed; no unrelated
historical balance campaigns or unchanged suites were repeated.

Stop at the saved final checkpoint and bilan. Proposed next bounded block:
isolated exact Peasant transfers in both directions with a bound fixed visitor,
source/destination checks, the hero's last unit and final conservation checks.
**Sol / High** recommended, after the next explicit user go-ahead. Live integration,
persistence and general building execution remain separate work.
