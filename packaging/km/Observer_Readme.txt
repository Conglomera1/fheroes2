KNIGHT MOD: OBSERVER DEBUG BUILD v7 (Windows x86)

Extract this ZIP into your existing fheroes2 installation, preserving folders.
Run fheroes2KM-debug.exe. Your existing Heroes II data and 32-bit SDL runtime
from that installation are required. This build includes the live Farm/TG AI.

V7 TEST
Start AoW and Utopeia again from day 1 with the same factions and existing INI.
Keep the log through elimination, or aim for day 168-175 if play continues.
Revision 7 is recorded in OBSERVER_KINGDOM. SOURCE_COMMIT.txt identifies the code.
This version coordinates routine spending, mobilisation and Farm recovery,
adds economic roles to secondary Knight towns and requires useful naval missions.

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
- boat purchases and naval missions, unit upgrades and hero waiting reasons;
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
