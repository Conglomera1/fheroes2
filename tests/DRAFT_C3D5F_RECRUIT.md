# C3D5F DRAFT — recruitment receipt probe

Historical draft instructions below. The draft was subsequently compiled and
extended locally; completed observations were recovered and saved. Current status:
see `RESULTS_C3D5F_FINAL.md`. The original draft source is retained in commit
39d2cd4b497f22789f94a5db82cba8ca501b71c6; its replacement is `recruit_receipt_probe.cpp`.

Base: 9e84baeb37a814d5487b126ad2b76231ec99fd3f (validated C3D5D).
Source draft only: NOT compiled, NOT executed, NOT a validated checkpoint.
No production code or global recruitment behavior is changed.

After restoring the exact checkout/dependencies, compile draft_recruit_receipt_engine_test.cpp
like the existing native engine fixtures: all game objects except fheroes2.o, engine/Smacker
libraries, identical defines/SDL flags. Use an isolated temporary configuration directory.
Assertions must stay enabled; setup follows the existing pipeline fixture and still needs validation.

Run each mode in a fresh process with arguments: <settings.cfg> <mode>.

- raw_exact: request the actual current Archer stock; require exact purchase, stock removal and payment.
- raw_overstock: request one more than current stock directly through RecruitMonster.
  Record requested count, stock removed, units added and cost. Exit 2 means an observed inconsistency;
  preserve it as a red characterization case, never report it as a passing test.
  A clean rejection with no effects is also consistent.
- guarded_overstock: a TEST-LOCAL stock check skips the API call; require no changes.
  This is a guard demonstration, not a production executor or permission to use a registry ticket.

Seed stock by normal BuyBuilding, measure money AFTER setup, keep a free garrison slot,
and exclude a visitor to isolate this issue. Validate fixture setup/compilation first.
Do not modify RecruitMonster just to make this probe pass. Determine whether caller invariants
already cover current game paths, then guard the future exact-program executor appropriately.

Required next work: actual compilation/run, retain all outputs including nonzero status,
then synchronous receipt design and rollback boundaries from C3D5E.
No balance campaigns or unrelated unchanged tests need to be rerun.
