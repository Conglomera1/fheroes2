"""Actual engine mission capture and refresh. No map campaign or target search."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

exe = str(Path(sys.argv[1]).resolve())
profiles = {"default": "# Default\n", "custom": "taxpayer.gold_per_peasant_per_day = 1\nconscription.gold_per_peasant = 137\nconscription.odd_day_limit = 1\nconscription.even_day_limit = 4\nconscription.peasant_success_ratio = 0.85\n"}
for name, ini in profiles.items():
    summaries = []
    for level in (0, 2):
        with tempfile.TemporaryDirectory(prefix="km-missions-") as d:
            root = Path(d); (root / "fheroes2").mkdir(); (root / "fheroes2/knight_mod.ini").write_text(ini)
            cfg = root / "settings.cfg"; cfg.write_text(f"taxpayer test diagnostics = {level}\n")
            p = subprocess.run([exe, str(cfg)], env=dict(os.environ, XDG_CONFIG_HOME=d), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=40)
            assert p.returncode == 0, p.stdout
            summaries.append(next(l for l in p.stdout.splitlines() if l.startswith("RESULT")))
            observed = p.stdout.split("OBS_BEGIN\n", 1)[1].split("OBS_END\n", 1)[0]
            rows = [json.loads(l.split("[KNIGHT ECONOMY] ", 1)[1]) for l in observed.splitlines() if "[KNIGHT ECONOMY] " in l]
            if level == 0:
                assert not rows
            else:
                assert rows and all(r["kind"] == "evaluation" for r in rows)
                snapshots = [r for r in rows if r["event"] == "PLANNER_MISSIONS"]
                assert len(snapshots) == 3
                assert all(r["heroes"] == 6 and r["targets"] == 3 and r["resource_hints"] == 7 and not r["future_commitments_complete"]
                           and not r["military_outlets_exhausted"] and not r["actionable"] for r in snapshots)
                initial = [r for r in rows if r.get("evaluation_seq") == snapshots[0]["evaluation_seq"]]
                heroes = [r for r in initial if r["event"] == "PLANNER_HERO_INTENT"]
                assert len(heroes) == 6 and len({r["role"] for r in heroes}) == 5 and sum(r["use"] == "fixed_local" for r in heroes) == 1
                assert all(not r["delivery_verified"] for r in heroes)
                assert len([r for r in initial if r["event"] == "PLANNER_PRIORITY_LINK"]) == 2
                assert all(not r["reserved_funds"] for r in initial if r["event"] == "PLANNER_RESOURCE_HINT")
                assert not any(r["event"] == "PLANNER_INPUT_GAP" for r in initial)
                assert any(r["event"] == "PLANNER_INPUT_GAP" and r["gap"] == "visitor_assignment_not_committed" for r in rows)
    assert summaries[0] == summaries[1]
    print(f"PASS {name}: {summaries[0]}; diagnostics neutral", flush=True)
print("All 4 mission-evidence engine processes passed")
