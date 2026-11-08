# C2: projects on copied economic states

`AI::EconomyProject::capture(kingdom, throughDay)` captures the real kingdom in
interface town order, effective INI settings, six recruitment pools per town,
armies, construction availability, supported building costs/requirements and
creature forms/prices. It never buys anything. The live planner does not call
the new projector yet; candidate generation and scoring remain the next block.

Append identified, dated actions to the returned input and call
`project(input, sharedWorkBudget)`. Reuse the SAME work budget across candidate
evaluations in a player turn. Do not copy/reset it per purchase or variant.
Default bounds are 24,576 future days and 200,000 permitted training attempts;
there is also an input cap of 256 towns, 1,024 actions and 1,024 explicit events.
These are calculation bounds, not gameplay settings. A limit is not infeasibility.

The current morning is already processed. For future days the projector applies
normal weekly growth, income, explicit ordered calendar events and the exact B2
conditional conscription scheduler, then the dated player actions. Conversions
recompute next-morning taxes/growth; existing training cycle and failure remainder
are preserved. Purchases consume actual pools, pay normal creature prices and
join a garrison or, if necessary, its visiting hero without dismissing a stack.
Local Peasant transfers only use that same visiting hero and protect their last
creature. Visiting Peasants retain the current mod's existing taxation rules.

C1 audits projected net morning cash flows and dated action costs, including
construction slots. Exact pool/army evolution is checked by C2's copied state,
rather than duplicating it as invented stock arrivals in C1. Formation fees are
NOT charged twice: the C1 morning entry already contains their applied net effect.
The C2 day record separately retains B2 income, taxes, event effects and fees.
Only `Result::covered()` certifies the supported scenario. Never accept a plan
because its embedded budget alone is covered after a different legality failure.

On failure, `end` is the last completed day (or original input before any day
completed); it is not a complete plan. The failing action/day/reason are explicit.
Unverified actions beyond the horizon remain pending and prevent certification.
No hypothetical transaction is emitted as a real-game purchase log.

## Supported scope and exclusions

Supported: Farm/TG conversion and reconversion; supported ordinary buildings and
dwelling upgrades; all six recruitment tiers of all factions; local garrison /
visiting-hero Peasant exchange; normal weekly growth; explicit seven-resource events.
The build catalog excludes castle construction, shipyard and non-Knight shrine,
whose map/game-version effects need a separate adapter. Other unsupported actions
must return incomplete, never receive a zero-cost approximation.

No future battles/captures, remote population transport, market trades, creature
upgrades in existing armies, special weeks/plague or future visiting heroes are
simulated here. The map's hidden future events are not read. Absence of support
must remain visible when future candidate coverage/surplus is judged. Military
utility, path legality of remote missions and persistence are not C2.

## Verification

`economy_project_test.cpp`: compile C++17 with every `src/fheroes2/*` directory
and `src/engine` on the include path; no engine linking or original game assets
are required. Exercise the same executable under AddressSanitizer and UBSan;
under ptrace use `ASAN_OPTIONS=detect_leaks=0` (no leak claim).

`economy_project_engine_test.cpp`: compile with the game's debug/diagnostic flags
and link the game objects except `fheroes2.o`, plus engine/Smacker/SDL libraries.
Run `python3 tests/run_economy_project_engine_test.py PATH_TO_TEST_EXECUTABLE`.
It runs six fresh processes: three INI profiles, diagnostics off/on. Each follows
nine real actions over seven days in two Knight towns and one Warlock town,
compares daily resources and final pools/cycles, and checks observational neutrality.
The native engine fixture stays within week 1; weekly boundaries are mechanical
tests here, not a validation of special-week engine behaviour or long-game balance.
