"""Actual candidate generation, bounded combined ranking and real day-1 replay."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

executable = str(Path(sys.argv[1]).resolve())
profiles = {
    "default": "# Defaults\n",
    "custom": "taxpayer.gold_per_peasant_per_day = 1\nconscription.gold_per_peasant = 137\n"
              "conscription.odd_day_limit = 1\nconscription.even_day_limit = 4\nconscription.peasant_success_ratio = 0.85\n"
              "conscription.archers = 3\nconscription.pikemen = 2\nconscription.swordsmen = 1\n",
}
for name, ini in profiles.items():
    summaries = []
    for diagnostics in (0, 2):
        with tempfile.TemporaryDirectory(prefix="km-search-") as directory:
            root = Path(directory)
            (root / "fheroes2").mkdir()
            (root / "fheroes2" / "knight_mod.ini").write_text(ini)
            config = root / "settings.cfg"
            config.write_text(f"taxpayer test diagnostics = {diagnostics}\n")
            result = subprocess.run([executable, str(config)], env=dict(os.environ, XDG_CONFIG_HOME=directory),
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=40)
            assert result.returncode == 0, result.stdout
            summaries.append(next(line for line in result.stdout.splitlines() if line.startswith("RESULT")))
            observed = result.stdout.split("OBS_BEGIN\n", 1)[1].split("OBS_END\n", 1)[0]
            records = [json.loads(line.split("[KNIGHT ECONOMY] ", 1)[1]) for line in observed.splitlines() if "[KNIGHT ECONOMY] " in line]
            assert "[TAXPAYER" not in observed
            if diagnostics == 0:
                assert not records
            else:
                assert records and all(row["kind"] == "evaluation" for row in records)
                evaluations = [row for row in records if row["event"] == "SEARCH_EVALUATION"]
                assert len(evaluations) == 1
                summary = evaluations[0]
                assert summary["best"] is not None and not summary["actionable"] and summary["passes"] <= 6
                plans = [row for row in records if row["event"] == "SEARCH_PLAN"]
                assert len(plans) == summary["evaluations"] and len(plans) <= 256
                winner = next(row for row in plans if row["id"] == summary["best"])
                assert winner["verified"] and winner["payments_protected"] and winner["additional_need_known"] and winner["robust_utility"] > 0 and winner["D"] == []
                variants = [row for row in records if row["event"] == "SEARCH_VARIANT" and row["id"] == summary["best"]]
                assert variants and all(row["funded"] and len(row["minimum_funds"]) == 7 for row in variants)
                assert any(row["event"] == "SEARCH_SOURCE" for row in records)
                assert all(row.get("evaluation_seq", summary["seq"]) == summary["seq"] for row in records)
    assert summaries[0] == summaries[1], (name, summaries)
    print(f"PASS {name}: {summaries[0]}; diagnostics neutral")
print("All 4 search engine processes passed: actual proposals, complete ranked fallback, bounded search and real day-1 replay")
