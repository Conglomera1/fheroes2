# Live Knight Farm / Training Grounds pilot

This block activates one immediate production decision in CastleDevelopment.
On weekly reviews, a safe Knight town with an existing Farm or Training Grounds
compares its current role against conversion over fourteen days. It reuses the
existing economy projection and native local army valuation. It does not activate
the diagnostic planner's generic candidate search or enqueue future orders.

The comparison includes normal local infantry purchases in both alternatives,
growth, income, training, seven resource prices, stocks and army capacity. Its
2000-gold reserve, weekly review and 10% military margin are pilot heuristics,
not a proof of optimal play. Existing military development and defense remain
responsible for actual recruitment, hero movement and threatened towns.

## Final verification

- Full native game compiled successfully.
- Ten targeted cases, each with diagnostics 0 and 2: twenty successful processes.
- Actual CastleDevelopment converts Farm to TG with sufficient funds, restores
  Farm when training is unproductive or too expensive, and applies custom costs.
- Reserve, missing prerequisite, weekday, mobile visitor, unsafe town and zero
  training yield prevent the new conversion. Forecasting leaves live state intact.
- Diagnostics settings produce identical choices and resulting payments.
- Windows source manifest now lists the existing economy cpp dependencies and
  the new module. XML and all fourteen AI translation units pass native release
  syntax checks without diagnostics or debug defines. This is not a Windows build.
- Karlosge.mp2 completed fourteen real KingdomTurn days in 11.15 seconds, exit 0.
  Seeded Green Knight town 5168 built TG on day 1, trained seventeen units through
  day 8, then paid for Farm reconversion on day 8. Forecasts favored the TG first
  and Farm later; production BUILD_ACTION receipts confirm both purchases.
- This one seeded, revealed-map run establishes integration, not balanced or
  stable behavior across maps. Weekly role switching remains a pilot heuristic.
- The 2000-gold reserve applies to this role decision and hypothetical buying;
  the ordinary AI can spend below it afterwards (observed in the map run).

Evidence: role_observations.json, map_observations.json, verification.json and
execution_logs.json.gz (JSON containing twenty targeted logs, native build log
and complete map log). Original map/game assets are not distributed.

Intermediate remote checkpoint: b5e591e60a14f69c867103f74ae30420dd019701.
Final source is based on C3D5J, 55cf801eab416a09185025f1191643c60cc7cc99.

Next deliverable: build the existing Windows KM x86 package from this pilot
branch, then try a normal game before extending policy or running broad studies.

Build and run entry points:

- tests/build_exact_recruit_adapter.py with --source tests/live_knight_role_test.cpp
  and --exclude-object ai_planner_castle.o; tests/run_live_knight_role.py.
- tests/build_live_map_smoke.py compiles temporary CURRENT UI sources with
  rendering, audio playback and event-loop sleeps disabled. It links current AI
  objects without the previous balance AI or RNG override. Supply owned game
  assets and an existing map through the normal game data layout. The KM resource
  archive is copied from files/data/resurrection.h2d under resurrectionKM.h2d,
  exactly as in .github/workflows/km-alpha.yml.
- The map runner seeds one Knight town and treasury and reveals the map. It is
  a controlled integration smoke, not an unchanged campaign or balance study.

No proprietary map or original game asset is included in these results.
