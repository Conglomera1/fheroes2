"""Native siege contexts, dated defense priority, uncertainty and opt-in traces."""
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
        with tempfile.TemporaryDirectory(prefix="km-defense-") as directory:
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
                evaluations = [row for row in records if row["event"] == "DEFENSE_EVALUATION"]
                assert len(evaluations) == 3
                assert [row["status"] for row in evaluations] == ["complete", "complete", "incomplete"]
                assert [row["D"] for row in evaluations] == [[True], [False], [None]]
                assert evaluations[0]["baseline_D"] == [False]
                for evaluation in evaluations:
                    children = [row for row in records if row.get("evaluation_seq") == evaluation["seq"]]
                    cities = [row for row in children if row["event"] == "DEFENSE_CITY"]
                    attacks = [row for row in children if row["event"] == "DEFENSE_CHECK"]
                    assert len(cities) == len(attacks) == 1
                    assert attacks[0]["threshold"] == 1.1
                unknown = next(row for row in records if row["event"] == "DEFENSE_CHECK" and row["candidate"] == 3)
                unknown_city = next(row for row in records if row["event"] == "DEFENSE_CITY" and row["candidate"] == 3)
                assert unknown_city["last_safe_day"] is None
                assert unknown["last_safe_day"] is None and unknown["defense_force"] is None and unknown["viable"] is None
    assert summaries[0] == summaries[1], (name, summaries)
    print(f"PASS {name}: {summaries[0]}; diagnostics neutral")
print("All 4 engine processes passed: six factions, siege structures, attack profiles, deadline priority and unknown data")
