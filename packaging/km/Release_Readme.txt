KNIGHT MOD v17 - Windows x86

Extract into your existing fheroes2 installation, preserving folders.
Run fheroes2KM.exe. Your Heroes II game data and existing 32-bit SDL runtime
are required. This is a mod update, not a replacement for the original data.
Your knight_mod.ini is not overwritten; existing balance settings are kept.
SOURCE_COMMIT.txt identifies the exact source used for this executable.

NORMAL PLAY
Observer features and Knight telemetry are OFF by default. Previous values of
"taxpayer test reveal map" and "taxpayer test diagnostics" from an observer
installation do not enable these features in this release executable.

OPTIONAL DIAGNOSTICS
Close the game, then edit fheroes2.cfg in your fheroes2 configuration directory.
To enable full observer inspection and the normal actual-state journal, set:
knight observer = on
knight diagnostics = 1

Observer presentation reveals the map only for you; it does not clear AI fog
or grant AI extra knowledge. Enemy army/town inspection remains read-only.
Level 2 enables additional planner experiments and produces a much larger log.
To return to normal play, close the game and set:
knight observer = off
knight diagnostics = 0

V17 CHANGES
A cancelled loaded supply mission keeps its physical cargo identified until
a friendly castle can receive it. Returned Peasants are deposited before
military recruitment and protected during the current kingdom-turn army
rearrangements, including urgent reinforcement and full garrisons. A hero
with only Peasants retains the last unit needed to remain valid. Dismissal,
owner/world/load changes clear stale cargo. Military priorities still apply.

If the destination TG becomes a Farm, a pending pickup is cancelled. Loaded
cargo is redirected to a nearby safe productive town; finishing at the former
TG remains possible when it is the closest useful deposit. Without a nearby
safe destination, physical cargo stays identified for the next friendly visit.

Existing construction priorities, Farm/TG costs, J1 naval opening and navigation
after J1 are retained. This version does not claim globally optimal AI play.
