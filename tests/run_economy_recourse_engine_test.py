"""Real engine observation: authorized recourse, shared budget and ordinary replay."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
exe = str(Path(sys.argv[1]).resolve())
profiles = {"default": "# Default\n", "custom": "conscription.gold_per_peasant = 137\nconscription.odd_day_limit = 1\nconscription.even_day_limit = 4\nconscription.peasant_success_ratio = 0.85\n"}
for name, ini in profiles.items():
    for mode in ("deny", "allow"):
        summaries = []
        for level in (0, 2):
            with tempfile.TemporaryDirectory(prefix="km-reference-") as d:
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
                    summary = next(r for r in rows if r["event"] == "REFERENCE_PREPARATION")
                    assert not summary["actionable"]
                    failures = [r for r in rows if r["event"] == "REFERENCE_FAILURE"]
                    assert failures and failures[0]["action"] == 777 and failures[0]["reason"] == "recruit_stock_unavailable"
                    releases = [r for r in rows if r["event"] == "REFERENCE_RELEASE"]
                    if mode == "deny":
                        assert summary["status"] == "blocked" and not releases and summary["projections_left"] == 255
                    else:
                        assert summary["status"] == "ready" and summary["attempts"] == 2 and summary["payments_protected"]
                        assert len(releases) == 1 and releases[0]["action"] == 777 and releases[0]["project"] == 888
                        stable = next(r for r in rows if r["event"] == "STABILITY_EVALUATION")
                        assert stable["certified"] and not stable["actionable"] and stable["projections_left"] < summary["projections_left"]
        assert summaries[0] == summaries[1], summaries
        print(f"PASS {name}/{mode}: {summaries[0]}; diagnostics neutral", flush=True)
print("All 8 recourse engine processes passed")
