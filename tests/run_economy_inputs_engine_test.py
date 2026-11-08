"""Engine snapshots; controlled route evidence, not a real map-path campaign."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
exe = str(Path(sys.argv[1]).resolve())
profiles = {"default": "# Default\n", "custom": "taxpayer.gold_per_peasant_per_day = 1\nconscription.gold_per_peasant = 137\nconscription.odd_day_limit = 1\nconscription.even_day_limit = 4\nconscription.peasant_success_ratio = 0.85\n"}
for name, ini in profiles.items():
    for mode in ("known", "unknown", "visitor"):
        summaries = []
        for level in (0, 2):
            with tempfile.TemporaryDirectory(prefix="km-inputs-") as d:
                root = Path(d); (root / "fheroes2").mkdir(); (root / "fheroes2/knight_mod.ini").write_text(ini)
                cfg = root / "settings.cfg"; cfg.write_text(f"taxpayer test diagnostics = {level}\n")
                p = subprocess.run([exe, str(cfg), mode], env=dict(os.environ, XDG_CONFIG_HOME=d), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=40)
                assert p.returncode == 0, p.stdout
                summaries.append(next(l for l in p.stdout.splitlines() if l.startswith("RESULT")))
                observed = p.stdout.split("OBS_BEGIN\n", 1)[1].split("OBS_END\n", 1)[0]
                rows = [json.loads(l.split("[KNIGHT ECONOMY] ", 1)[1]) for l in observed.splitlines() if "[KNIGHT ECONOMY] " in l]
                if level == 0:
                    assert not rows
                else:
                    assert rows and all(r["kind"] == "evaluation" for r in rows)
                    summary = next(r for r in rows if r["event"] == "PLANNER_INPUTS")
                    assert summary["valid"] and not summary["actionable"] and summary["path_queries"] == 2 and summary["paths_left"] == 126
                    threats = [r for r in rows if r["event"] == "PLANNER_THREAT"]
                    assert len(threats) == 2 and summary["coverage_complete"] == (mode == "known")
                    assert all((r["last_safe_day"] == 2) if mode != "unknown" else (r["last_safe_day"] is None) for r in threats)
        assert summaries[0] == summaries[1], summaries
        print(f"PASS {name}/{mode}: {summaries[0]}; diagnostics neutral", flush=True)
print("All 12 input snapshot engine processes passed")
