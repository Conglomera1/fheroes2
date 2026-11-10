# C3D5I — isolated Farm/Training Grounds construction adapter

Parent H: `9f588bb260ba23b2214ea767001f4087d262f98e`.
Test-only extension; production source and normal AI are unchanged.

Intermediate state: **200 native building processes / 100 diagnostic pairs pass**,
covering six isolated INI profiles; **80 recruitment regression processes** equal
H's records exactly. Native fixture compilation/linking has no warnings. Sanitizer
validation is pending at this intermediate checkpoint.

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

See `results/c3d5i/` for the saved validation outputs. Stop at the saved final
checkpoint and bilan. Exact transfers, persistence, general buildings, future
execution and the live AI connection are later blocks.
