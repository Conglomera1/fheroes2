# Knight observer debug: delivered x86 package

The dedicated FHEROES2_KNIGHT_OBSERVER executable forces full visual reveal and
actual-state telemetry even if an older settings file disables diagnostics.
Ordinary builds keep their usual behavior. Observer builds report diagnostic
level 1 so the historical level-2 planner experiments are not activated.

Right-click inspects exact army counts. Ctrl-left-click opens the real town
screen with readOnly=true. Its separate event path blocks purchases, recruiting,
army redistribution, hero management, navigation keys and nested purchase
dialogs. Troop inspection, dwelling stock/building information and owner income
remain available. AI movement animation is shown when its speed setting is nonzero.

Only presentation is overridden: no gameplay fog, ownership, control, alliances,
funds or AI knowledge is changed. Normal scouting, difficulty bonuses and spells
keep working. World::ClearFog performs normal scouting around owned objects;
it is not a full-map reveal. Earlier pilot test descriptions are corrected.

Verification.json records completed native compilation, release syntax checks
with and without observer mode, actual town UI/keyboard/Ctrl-click isolation,
fourteen map days and successful Windows CI package checks. The UI test compared
all 598506 bytes of the serialized world before and after observation. All 7594
structured events from the fourteen-day test parsed successfully.

Windows source commit: eb94d12ada78a05a5b06c7633a65c75e3d1e3995.
Build: https://github.com/Conglomera1/fheroes2/actions/runs/42741240872
ZIP SHA256: d72f481b86821cb88e315a0a92d51443d3bb8090db1e32baaa61edd918e136f0.
The final checkpoint adds tested fixture corrections and verification records;
production code matches the compiled Windows source commit.

The stale dependency checksum was refreshed against both the official release
asset digest and official upstream installer; integrity checking stays enabled.
The initial CI attempt stopped at dependency verification before compilation.

The package requires an existing 32-bit fheroes2 installation and owned game
data. See packaging/km/Observer_Readme.txt. Local telemetry is not uploaded.
It is not a deterministic replay or a trace of every rejected tactical idea.
Raw local execution logs were lost in the workspace restoration during the
interruption and are not included. Observed verification results are preserved
here; durable Windows CI logs are linked above. No game assets/maps are committed.
