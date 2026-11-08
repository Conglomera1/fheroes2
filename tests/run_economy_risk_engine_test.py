"""Conditional town loss versus actual ownership change, income, tax and TG fees."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

executable = str(Path(sys.argv[1]).resolve())
profiles = {
    "global_tax_failed_training": "taxpayer.require_farms = 0\nconscription.peasant_success_ratio = 0\n",
    "default": "# Defaults\n",
    "custom": "taxpayer.gold_per_peasant_per_day = 1\nconscription.gold_per_peasant = 137\n"
              "conscription.odd_day_limit = 1\nconscription.even_day_limit = 4\nconscription.peasant_success_ratio = 0.85\n"
              "conscription.archers = 3\nconscription.pikemen = 2\nconscription.swordsmen = 1\n",
}
for name, ini in profiles.items():
    summaries = []
    for diagnostics in (0, 2):
        with tempfile.TemporaryDirectory(prefix="km-risk-") as directory:
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
                evaluations = [row for row in records if row["event"] == "RISK_EVALUATION"]
                assert len(evaluations) == 1 and evaluations[0]["status"] == "scored" and not evaluations[0]["gaps"]
                variants = [row for row in records if row["event"] == "RISK_VARIANT"]
                assert len(variants) == 2 and all(row["funded"] and len(row["minimum_funds"]) == 7 and not row["cancelled_required"] for row in variants)
                assert len(evaluations[0]["tightest_resource_scenarios"]) == 7
                assert evaluations[0]["robust_utility"] <= variants[0]["utility"]
                days = [row for row in records if row["event"] == "RISK_DAY"]
                assert len(days) == 30 and all(len(row["funds"]) == 7 for row in days)
                assert all(row["baseline_force"] is not None and row["proposed_force"] is not None for row in days)
                losses = [row for row in records if row["event"] == "RISK_TOWN_LOSS"]
                assert len(losses) == 1 and losses[0]["first_unavailable_day"] == 3
                assert all(row.get("evaluation_seq", evaluations[0]["seq"]) == evaluations[0]["seq"] for row in records)
    assert summaries[0] == summaries[1], (name, summaries)
    print(f"PASS {name}: {summaries[0]}; diagnostics neutral")
print("All 6 risk engine processes passed: real capture replay, dynamic INI fees, losses and neutral observation")
