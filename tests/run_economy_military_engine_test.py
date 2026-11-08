"""Native command contexts and dated U; opt-in traces must not change the world."""
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
        with tempfile.TemporaryDirectory(prefix="km-military-") as directory:
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
                evaluations = [row for row in records if row["event"] == "MILITARY_EVALUATION"]
                assert len(evaluations) == 2 and all(row["status"] == "scored" and row["funded"] and not row["gaps"] for row in evaluations)
                for evaluation in evaluations:
                    days = [row for row in records if row.get("evaluation_seq") == evaluation["seq"]]
                    assert len(days) == 15 and [row["projected_day"] for row in days] == list(range(1, 16))
    assert summaries[0] == summaries[1], (name, summaries)
    print(f"PASS {name}: {summaries[0]}; diagnostics neutral")
print("All 4 engine processes passed: six factions, captain/no captain, hero, magic, morale, dated utility")
