# Taxpayer observer / diagnostics test build

This branch retains the gameplay rules of fceefdb (Farm priority 2).
Privileged inspection is available only when compiled with
`FHEROES2_TAXPAYER_TEST`. Ordinary builds ignore the observer switch and
suppress Taxpayer traces. Visual Studio: `/property:TaxpayerTestBuild=true`.
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
