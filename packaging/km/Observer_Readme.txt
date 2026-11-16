KNIGHT MOD: OBSERVER DEBUG BUILD v15 (Windows x86)

Extract this ZIP into your existing fheroes2 installation, preserving folders.
Run fheroes2KM-debug.exe. Your existing Heroes II data and 32-bit SDL runtime
from that installation are required. This build includes the live Farm/TG AI.

V15 TEST
Start AoW and Utopeia again from day 1 with the same factions and existing INI.
Keep the log through elimination, or aim for day 168-175 if play continues.
Revision 15 is recorded in OBSERVER_KINGDOM. SOURCE_COMMIT.txt identifies the code.
Farm/TG forecasts no longer repeat past construction/trade materials every day.
Other historical gold spending is a discretionary allowance capped before
consuming the operating/Farm recovery reserve. Forecast validity is explicit.
Boats are considered before hero movement, separately from today's building.
Available native routes (including Summon Boat) are preferred to new purchases.
New purchases require a hero able to reach the coast this turn; heroes waiting
in defense are excluded. One explorer is funded per connected known water area.
This is an exploration policy, not a general multi-hero naval transport planner.

V15 CHANGES
Day-one meeting tiles may be native coast tiles. A funded coordinated opening
requires a legal second land step before purchases are committed. After army
sharing, the land hero pays this step to clear the castle exit before the sailor
can depart. Refusal reasons and NAVAL_OPENING_CLEARANCE expose this sequencing.
A loaded population carrier can stage beside a safe friendly destination with
a visiting hero for the rest of one day, keeping cargo and destination. If the
visitor remains next day or safety/access fails, normal fallback still applies.
Reachable critical military tasks retain priority over waiting.
A seven-day temporary TG cycle must improve military force AND leave at least
as much forecast gold as keeping the Farm, including both conversions and
recruitment. Long-term mobilisation and emergency recovery keep their rules.
Costs, INI overrides and navigation policy after day one are unchanged.

PREVIOUS V14 CHANGES
Day-one openings can prepare the two heroes before ordinary hero movement.
For a safe starting port, with one owned town/hero, a built shipyard and a known
sea exploration objective: build the Statue if needed, reserve the actual hero
and boat costs, recruit useful affordable Knight troops, move the first hero
one legal empty adjacent land step, and hire the preferred navigator. The
navigator takes five singles. Available home-faction T2s are preferred; missing
T2s may be replaced with existing T2s from the other armies on day one. Shooters
are placed on the flanks, with the third at the center when there are three.
The surplus goes immediately to the adjacent mainland hero. Farm Peasants go
to taxation unless retaining them makes a reachable visible combat acceptable
under the native AI's army evaluation. This military retention lasts only J1.
A coordinated opening can finish below the ordinary 2000-gold reserve when all
mandatory purchases are funded. This exception does not change later budgets.
Missing crew/army capacity cancels the special preparation without buying an
unused boat for that opening. Existing later navigation policy is unchanged.
Population deliveries can now exchange one complete garrison stack with the
courier to receive Peasants in a full town, preserving every creature. Capacity
is checked before loading and again at delivery. The buffer still covers
fourteen days, counting pending pickups and actual cargo without duplication.
Safe Farms can supply TG even while labelled primary. Hunters/scouts may use
the same bounded 1000-point pickup / 2000-point delivery / 3000 total routes as
couriers; couriers are preferred, then scouts, then hunters. Safe waiting heroes
can be assigned. Fighters/champions, sea trips and known emergencies retain
their exclusions. No new hero is hired for these convoys.
Supply reviews now expose detailed refusal counters, including role, distance,
occupation, safety and army capacity. New opening records: NAVAL_OPENING_J1,
NAVAL_OPENING_PARTY, NAVAL_OPENING_PEASANTS. TOWN_GROWTH records actual before/
after dwelling stocks and Plague; kingdom snapshots include week type.
Factory TG costs remain 2500 gold/10 wood/10 ore; Farm recovery 1500 gold.
INI overrides continue to apply; the package contains no replacement INI.

PREVIOUS V13 CHANGES
Active TG towns request their missing fourteen-day Peasant buffer before running
out, using garrison, Hut stocks and scheduled Hut/Well growth. Assigned cargo
and pending pickups count toward demand, so another hero does not load the
same requested buffer. Nearby existing couriers/scouts/hunters can collect
from a safe rear Farm: pickup within 1000 land movement, delivery within 2000,
3000 total for couriers and 1000 total for scouts/hunters. No hero is purchased
for these deliveries. Military emergencies can cancel them. Captured, unsafe,
occupied or expired pickup routes release the reservation. Delivery cargo
retains the safe fallback introduced in v12.
Opening sea crews now use three T1 and two T2 creatures of the navigator's
faction, one per stack, replacing the v11/v12 all-T1 crews:
Knight: Archer, Peasant, Peasant, Peasant, Archer
Barbarian: Orc, Goblin, Goblin, Goblin, Orc
Sorceress: Sprite, Dwarf, Sprite, Dwarf, Sprite
Warlock: Centaur, Gargoyle, Centaur, Gargoyle, Centaur
Wizard: Halfling, Boar, Halfling, Boar, Halfling
Necromancer: Skeleton, Zombie, Skeleton, Zombie, Skeleton
An upgraded T2 may replace its base form when necessary. No troop is purchased
for the crew. If creatures or shore slots are insufficient, this special
formation is not applied and ordinary departure policy remains in force.
New records: POPULATION_SUPPLY_PLAN, POPULATION_SUPPLY_REVIEW and
NAVAL_OPENING_CREW_WAIT. POPULATION_ROUTE includes pickup and quantity.
Costs unchanged: TG 2500 gold/10 wood/10 ore, Farm recovery 1500 gold.
The INI continues to override these defaults. No INI is included in the ZIP.

PREVIOUS V12 CHANGES
Population deliveries receive a dedicated order after normal army preparation.
Native reinforcement/meetings preserve assigned cargo, except explicit military
emergencies. Blocked, expired or captured destinations trigger a nearby safe
Farm/TG fallback when one is reachable. Arrival deposits end the order.
Cash recovery considers next-day known income and conscription against the
operating envelope. Farm recovery records a seven-day minimum recovery period
also after an earlier mobilisation plan expired. Adventure purchases share the
same budget; survival/known defense exceptions remain explicit.
Defensive Knight construction also honors the primary building sequence.
For an opening with a feasible second hero and a built shipyard, the first hero
stays on land. The second recruit prefers Sorceress, then the port faction,
then native recruit value among the actual candidates. Sorceress keeps five
Sprites in single-creature stacks when available. Surplus stays ashore.
New events: POPULATION_ROUTE, MOBILIZATION_CASH_REVIEW, NAVAL_OPENING_RECRUIT.
Roles now expose primary and endangered separately from economic.

PREVIOUS V11 CHANGES
Ordinary Knight heroes no longer deposit residents before a Farm or TG exists.
The first exploration voyage assigned from an owned shipyard during days 1-7
uses up to five existing level-1 creatures, one per stack. Higher creatures
and surplus remain in the port garrison for the mainland hero to collect.
If no level-1 creature is available, one weakest existing creature is retained.
No troop is purchased for this rule. Unsafe ports and transfers that cannot
fit the shore army are excluded. A newly unreachable route rolls back transfer.
Native battle preparation preserves these single-creature stacks throughout
the first voyage. The exception ends on first disembarkation; it is not applied
to ordinary land heroes, later transport voyages or human players.
New transfer operation: opening_sea_explorer, including crew type/count and
both armies before/after. A successful escape is not guaranteed.
Opening assignment memory resets on reload, like other naval objectives.

PREVIOUS V10 CHANGES
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
