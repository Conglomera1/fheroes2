# Knight construction priorities and next-morning funding

This replaces the v5 initial-Farm shortcut. It does not replace Farm/TG role
comparisons or the ordinary priority lists for secondary buildings.

The primary sequence is Statue, Thatched Hut, Archery Range, Well, Blacksmith,
Tavern, Armory, Cathedral/Jousting Arena (native preference for Cathedral when
both can be built), then the initial Farm. Well/Tavern are dwelling prerequisites.
Farm is never treated as an automatic reconversion of existing Training Grounds.
Actual building costs and native prerequisites/disabled buildings are used.

Today's directly affordable primary options are enumerated first. A scoped ledger
then protects next-morning primary purchases, in the existing castle processing
order. Funds are shared across all cities: a reservation has one price, not an
independent copy of the treasury for each city. The one-day forecast includes
normal resource income, taxes and conditional TG fees, not speculative captures,
hero loot, unseen map events or marketplace trades. Insufficient forecasts fail
conservatively. Calendar changes not represented by the economic snapshot are
not promised as future funds.

A present purchase is accepted when the resulting next-morning funds still cover
the retained priorities. Paying one's own reserved building releases that promise.
Lower-cost options can be considered if an earlier current choice would delay a
reserved dwelling; their relative legacy ordering remains intact. Pure secondary
building competition (including guilds versus dwelling upgrades) is not replaced
with a global most-expensive-first rule or a special crystal reserve.

Native construction legality is always rechecked. The dated economic projector
checks supported building effects and all seven resources. Castle/shipyard map
side effects are handled by the real purchase; their verified cash costs are
forecast separately, including the castle's incremental 750-gold income.
Urgent military spending can invalidate funding: weaker reservations are dropped
with a trace, rather than creating money or carrying an impossible promise.
The ledger exists only during the actual castle development pass; no saved orders
or save-format changes are introduced. All observations use current owner state.

## Verification

- `knight_construction_test.cpp`: 8 isolated funding scenarios, including both
  approved 900-gold examples, missing wood, shared funds and crystal competition.
- `knight_construction_engine_test.cpp`: the actual production construction code,
  linked against current game objects. Ten profiles cover Statue, Hut, Archery,
  Well, Farm with no population, exactly 1000 gold, waiting versus a cheaper
  compatible moat, two-city funding, sawmill income and castle income.
- `run_construction_engine.py`: 20 native processes (diagnostics 0 and 2), checking
  identical decisions and funds. JSON results are in results/knight_construction.
- `live_knight_role_test.cpp` and `run_live_knight_role.py`: 20 native Farm/TG
  adapter processes. The adapter is tested separately from initial construction.
  These are generated-state tests, not replays of AoW or Utopeia.

Build the funding unit with C++17. For engine integration, compile the native
production debug/diagnostic objects first; then use `build_construction_engine.py`
with output object, output binary, SDL2 headers and SDL2_mixer library paths.
It excludes the ordinary main and ai_planner_castle objects because the runner
includes the actual castle-development translation unit. Optional final argument
selects an alternative test source, such as tests/live_knight_role_test.cpp.

The local headless integration used SDL2 headers 2.32.4, the installed SDL2 runtime,
and a temporary SDL2_mixer 2.8.1 build with WAV support only. Gameplay production
sources were unchanged by that environment setup. The distributed Windows build
uses its ordinary full dependency package. No manual Windows 7 runtime test or
full-map replay is implied by these fixtures.
