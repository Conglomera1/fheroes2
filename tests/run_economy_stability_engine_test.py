"""Read-only stability, real INI, shared work ceiling and selected day-1 replay."""
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
    for mode in ("generated", "irrelevant", "productive"):
        summaries = []
        for diagnostics in (0, 2):
            with tempfile.TemporaryDirectory(prefix="km-stability-") as directory:
                root = Path(directory)
                (root / "fheroes2").mkdir()
                (root / "fheroes2" / "knight_mod.ini").write_text(ini)
                config = root / "settings.cfg"
                config.write_text(f"taxpayer test diagnostics = {diagnostics}\n")
                result = subprocess.run([executable, str(config), mode], env=dict(os.environ, XDG_CONFIG_HOME=directory),
                                        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=40)
                assert result.returncode == 0, (name, mode, result.stdout)
                summaries.append(next(line for line in result.stdout.splitlines() if line.startswith("RESULT")))
                observed = result.stdout.split("OBS_BEGIN\n", 1)[1].split("OBS_END\n", 1)[0]
                records = [json.loads(line.split("[KNIGHT ECONOMY] ", 1)[1]) for line in observed.splitlines() if "[KNIGHT ECONOMY] " in line]
                assert "[TAXPAYER" not in observed
                if diagnostics == 0:
                    assert not records
                else:
                    assert records and all(row["kind"] == "evaluation" for row in records)
                    summary = next(row for row in records if row["event"] == "STABILITY_EVALUATION")
                    assert summary["certified"] and not summary["actionable"] and summary["reserved"] == 64
                    assert summary["proof_evaluations"] + summary["search_evaluations"] + summary["projections_left"] + 1 == 256
                    plans = [row for row in records if row["event"] == "STABILITY_PLAN"]
                    assert next(row for row in plans if row["index"] == summary["selected_index"])["verified"]
                    proofs = [row for row in records if row["event"] == "STABILITY_PROOF"]
                    if mode == "irrelevant":
                        assert proofs and not proofs[0]["accepted"] and proofs[0]["without_verified"]
                    if mode == "productive":
                        assert proofs and proofs[0]["accepted"] and proofs[0]["without_verified"]
                        repairs = [row for row in records if row["event"] == "STABILITY_REPAIR"]
                        assert repairs and repairs[0]["requested"] == 11 and repairs[0]["kept"] == 8
                    assert all(row.get("evaluation_seq", summary["seq"]) == summary["seq"] for row in records)
        assert summaries[0] == summaries[1], (name, mode, summaries)
        print(f"PASS {name}/{mode}: {summaries[0]}; diagnostics neutral", flush=True)
print("All 12 stability engine processes passed")
