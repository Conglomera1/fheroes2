KNIGHT MOD: OBSERVER DEBUG BUILD v10 (Windows x86)

Extract this ZIP into your existing fheroes2 installation, preserving folders.
Run fheroes2KM-debug.exe. Your existing Heroes II data and 32-bit SDL runtime
from that installation are required. This build includes the live Farm/TG AI.

V10 TEST
Start AoW and Utopeia again from day 1 with the same factions and existing INI.
Keep the log through elimination, or aim for day 168-175 if play continues.
Revision 10 is recorded in OBSERVER_KINGDOM. SOURCE_COMMIT.txt identifies the code.
Farm/TG forecasts no longer repeat past construction/trade materials every day.
Other historical gold spending is a discretionary allowance capped before
consuming the operating/Farm recovery reserve. Forecast validity is explicit.
Boats are considered before hero movement, separately from today's building.
Available native routes (including Summon Boat) are preferred to new purchases.
New purchases require a hero able to reach the coast this turn; heroes waiting
in defense are excluded. One explorer is funded per connected known water area.
This is an exploration policy, not a general multi-hero naval transport planner.

V10 CHANGES
City roles are reviewed weekly, rather than replaced on every planning pass.
Funded TG commitments survive a new front or an economic reclassification;
real cash recovery remains possible. After the commitment, Farm/TG options
are compared again rather than automatically replacing TG in a rear town.
Mobilisation is examined before optional recruitment, while priority local
construction keeps its daily building slot. Other cities preserve the coming
conscription AND infantry recruitment envelope; the producing town can use
its own infantry allocation.
Population reserves bridge fourteen days of conscription using known Hut
stocks and normal Hut/Well growth, without a prospective Farm growth bonus.
Nearby couriers/scouts can move surplus to safe fiscal Farms or replenish
an active TG. These are bounded land routes, not a new long-range transport AI.
Before departure towards the sea from a Knight town, residents are reconciled
even if the Farm is not built. A hero always keeps a valid army (one Peasant
if that is its only troop). No population transfer is forced in known danger.
New records: MOBILIZATION_EARLY_REVIEW, KNIGHT_POPULATION and population
OBSERVER_ARMY_TRANSFER events; KNIGHT_TOWN_ROLE includes reason, role_until
and known_front_cost (native path cost, not straight-line distance).
Role history, naval objectives and mobilisation commitments reset on reload;
existing TG are recognised when roles are rebuilt. Use continuous runs for
stability tests, starting from day 1.
Factory TG costs remain 2500 gold, 10 wood, 10 ore; Farm recovery 1500 gold.
The ZIP does not replace knight_mod.ini. If your previous test used reduced
costs, restore the desired values in that file before launching the game.

PREVIOUS V9 CHANGES
Naval missions continue from one sea objective to the next. Visible rewards
are preferred to empty frontiers; continuation routes must remain on water.
After a useful disembarkation is selected, a new naval order must not cancel
the still-pending land objective on the following day.
TG entry requires a meaningful military gain and funds for one week's known
conscription and recruitment costs, Farm recovery and reserved development.
The accepted forecast establishes a bounded mobilisation plan (14 days, or
7 for a selected temporary cycle). Cash/strategic recovery remains possible;
a failed plan is not immediately restarted on a temporary cash rebound.
Routine spending protects upcoming conscription fees. A marketplace exchange
and its following construction are reviewed together BEFORE either payment.
Naval land objectives and mobilisation plans are transient AI memory; they
reset on reload. Financial/gain checks still apply after loading a save.

CONTROLS
- Map and radar show the whole world, including objects and enemy heroes.
- Right-click a hero or town for exact army counts.
- Hold Ctrl and left-click any town to open its actual town screen READ ONLY.
  Inspect troops, buildings, dwelling stocks and owner income. Exit normally.
  Purchases, recruitment, army changes, hero management and construction keys
  are blocked in this observation window.
- Ordinary clicks keep their usual gameplay behavior.
- Enable AI movement animation in game settings to watch enemy moves.

OBSERVER IS FOR THE PERSON AT THE SCREEN
No gameplay fog is cleared, even for your own faction. The visual override
does not grant AI players map knowledge, inspection powers, extra funds or
control of your faction. Human travel still follows ordinary discovered-map
rules. Existing game difficulty bonuses remain the game's normal rules.

TELEMETRY
Actual-event logging is forced on in this dedicated executable. Existing
settings cannot accidentally turn it off. Expensive historical level-2
planner experiments are not activated. The trace includes:
- session, map seed, turn/date, build label, difficulty and mod settings;
- money/resources, income, expenses, trades, buildings, troop purchases;
- towns before/after AI and human turns: owner, buildings, garrison, visitor,
  six dwelling stocks/costs/growth, Farm/TG and training-cycle state;
- heroes before/after turns: exact armies, position, stats, level/experience,
  movement, mana, morale/luck, skills, artifacts, AI role and route;
- AI selected targets/scores, movement orders, tile steps and completed actions;
- battle armies before/after, results and experience;
- actual gameplay fog count/hash per faction for checking isolation;
- Farm/TG forecasts, decision reasons and actual building purchases;
- shared budgets, emergency exceptions and economic/military town roles;
- full exchange/building costs, mobilisation funding and plan boundaries;
- boat purchases, naval reviews/missions, Summon Boat source/destination/mana;
- successive sea stages and the objectives motivating disembarkation;
- unit upgrades and hero waiting reasons;
- garrisons selected for defense and garrisons before/after capture.

This is not a deterministic replay or a trace of every rejected tactical idea.
The log may become large on long games. It is LOCAL; nothing is sent online.
On Windows it is normally %APPDATA%\fheroes2\fheroes2.log.
After testing, close the game and send the log, your save and knight_mod.ini
for analysis. Copy the log before starting another long test session.

To remove this debug package, delete fheroes2KM-debug.exe and this readme.
Keep shared KM data if another Knight Mod executable still uses it.
Original game data, executable and saves are not removed by this package.

Designed by Conglomera. Based on fheroes2 1.1.17.
