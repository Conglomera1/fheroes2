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
    for mode in ("known", "unknown", "visitor", "patrol"):
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
                    summary = next(r for r in rows if r["event"] == "PIPELINE_EVALUATION")
                    assert not summary["actionable"] and summary["paths_left"] == 126
                    registry = next(r for r in rows if r["event"] == "REGISTRY_OBSERVATION")
                    assert registry["scope"] == "hypothetical_snapshot_only" and not registry["live_commitments"] and not registry["actionable"]
                    assert not registry["retained_between_turns"] and not registry["reference_replaced"] and registry["actual_receipts"] == 0
                    if mode in ("known", "patrol"):
                        assert registry["status"] == "staged" and registry["pending_actions"] > 0
                        assert len([r for r in rows if r["event"] == "REGISTRY_SOURCE_LINK"]) == registry["pending_actions"]
                        actions = [r for r in rows if r["event"] == "REGISTRY_PENDING_ACTION"]
                        assert len(actions) == registry["pending_actions"] and all(not a["abandonment_authorized"] for a in actions)
                        transfers = [a for a in actions if a["operation"] in (2, 3)]
                        assert all(a["visitor"] >= 0 for a in transfers)
                        if mode == "patrol":
                            assert transfers
                        assert summary["searched"] and summary["projections_left"] < 256
                        stable = next(r for r in rows if r["event"] == "STABILITY_EVALUATION")
                        assert stable["certified"] and not stable["actionable"]
                        if summary["widened"]:
                            assert summary["through_day"] == 29 and summary["generation_passes"] == 2
                            captures = [r for r in rows if r["event"] == "PLANNER_INPUTS"]
                            assert captures[-1]["through_day"] == 29 and captures[-1]["path_queries"] == 0
                    else:
                        assert registry["status"] == "held" and registry["pending_actions"] == 0
                        assert summary["status"] == "held" and not summary["searched"] and summary["projections_left"] == 256
                        assert not any(r["event"] == "STABILITY_EVALUATION" for r in rows)
        assert summaries[0] == summaries[1], summaries
        print(f"PASS {name}/{mode}: {summaries[0]}; diagnostics neutral", flush=True)
print("All 16 observation-pipeline engine processes passed")
