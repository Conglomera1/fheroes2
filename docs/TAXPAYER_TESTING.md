## Early Farm investment

On non-defensive development turns, a Knight town without a Farm now examines
that Farm before ordinary development if its current garrison and stationed
hero would recover the actual Farm cost within seven daily tax credits.
The configured tax rate is used. At 2 gold/day and 1000 gold construction cost,
72 Peasants qualify; 90 repay the investment in six credits.

The early attempt uses the actual price (funds multiplier 1), retains normal
construction prerequisites, daily limits, difficulty restrictions and the
existing marketplace purchasing behavior. Taxation off or Farms not required
leaves the original order unchanged. Smaller populations retain the previous
Farm position and multiplier 2. No funds are reserved across towns or turns.

`early Farm investment` traces record population, potential daily tax, actual
cost, payback in daily credits (0 means no income), eligibility, difficulty,
pre-attempt building status and purchase result. A failed early attempt falls
through to the existing development logic; it does not block other buildings.

# Integrated Taxpayer build

The main mod includes the tested AI, fiscal protection and shared Farm description.
Windows integration packages build Release-SDL2 for x86 and x64 with
`TaxpayerDiagnostics=true`: detailed telemetry remains enabled (level 2 by default),
but privileged map reveal and exact observer inspection are unavailable, even if
an existing configuration enables them. Normal information rules apply.
CMake equivalent: `-DENABLE_TAXPAYER_DIAGNOSTICS=ON`.

`knight_mod.ini` is generated in the existing fheroes2 configuration directory.
The simplified template is used for new files; existing settings are preserved.
Restart after editing the two taxation settings. Diagnostic settings remain in
`fheroes2.cfg`, separate from the taxation file.

Windows archives include runtime libraries, compiled translations and engine assets.
The accompanying installers are for the same build; no renamed beta executables
or language catalogs are introduced. Original Heroes II data is not included.

The upstream multi-platform publishing workflow is restricted to the upstream
repository; this fork uses the integration workflow to upload build artifacts
without publishing releases or deploying a website.

# Taxpayer observer / diagnostics test build

This branch retains Farm priority 2 and adds fiscal protection to the observer/diagnostics baseline.
Privileged inspection is available only when compiled with
`FHEROES2_TAXPAYER_TEST`. Ordinary builds ignore the observer switch and
suppress Taxpayer traces unless diagnostics are explicitly enabled. Visual Studio: `/property:TaxpayerTestBuild=true`.
CMake: `-DENABLE_TAXPAYER_TEST=ON` (default OFF).

In fheroes2.cfg:

```ini
taxpayer test reveal map = on
taxpayer test diagnostics = 2
```

Reveal controls rendering, radar and right-click inspection only. Actual
map fog, AI knowledge, left-click actions and pathfinding are untouched.
Right-click heroes or towns to see exact, unabbreviated stack counts;
a stationed hero popup includes the town garrison as a separate section.
Disable reveal with `off` to restore the normal intelligence rules.

Diagnostics: 0 off, 1 daily balances, tax credits, battles, captures and
hero dismissals; 2 additionally logs recruitment refusals, building status
and resources, transfers, waits, selected targets, and fiscal candidates.
Hut fiscal bonus and Farm candidate decisions are deduplicated once per
hero/tile/day. A rejected fiscal bonus does not necessarily mean the AI
will reject the hut: normal military valuation still applies. Candidate
distance is the existing cost/approximation, not a newly measured detour.

`[TAXPAYER DIAG]` daily snapshots separate stationed hero Peasants from
garrisons and include all owned Peasants (also those away from Farms).
`daily credit` is actual next-day income application, not a projection.
Battle records describe army counts before and after battle resolution;
hero dismissal/reset and town cleanup are separate records. Transfers
record both source and destination counts, including normal military
transfers when protection is suspended. Reconcile snapshots with these
events instead of summing repeated `retained` observations.

Keep the complete log from day 1, build/commit, map, difficulty, starting
factions, and knight_mod/tax settings. A comparison of different games
does not isolate the effect of one economic rule.

## Fiscal protection revision

Taxable Peasants remain in Farm garrisons during routine and defensive
reinforcement. Deposits remain enabled in unsafe regions: the normal
battle preparation already lets a garrison defend its own town. This does
not add an evacuation route or order heroes to stay for taxes.

When replacement recruitment would sacrifice taxpayers, compare seven
days of income with the existing recruitment opportunity-cost rule.
Regional danger alone never bypasses that comparison. For known DEFEND
tasks, evaluate army copies before and after the actual stack replacement,
including normal castle bonuses and five-slot defense arrangement.
A still-doomed defense preserves the population unless this is the last
town, a known defense threat exists, and no active friendly hero remains
elsewhere. A pivotal replacement needs an estimated defense/enemy ratio
of at least 1.10 and a relative improvement of at least 10 percent. These
are experimental estimates, not combat outcome guarantees.

Decision log enum: 0 economic comparison, 1 preserve, 2 pivotal replacement,
3 last stand. Copy-only army reorganizations are marked `simulated 1`;
exclude them when counting real transfers.
