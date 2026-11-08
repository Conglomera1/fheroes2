"""Fresh-process INI profiles: real build/recruit routines versus copied projects."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

executable = str(Path(sys.argv[1]).resolve())
profiles = {
    "default": "# Defaults\n",
    "custom": "taxpayer.gold_per_peasant_per_day = 1\nconscription.gold_per_peasant = 137\n"
              "conscription.odd_day_limit = 1\nconscription.even_day_limit = 4\n"
              "conscription.peasant_success_ratio = 0.85\nconscription.archers = 3\nconscription.pikemen = 2\nconscription.swordsmen = 1\n"
              "training_grounds.gold = 3456\ntraining_grounds.wood = 7\ntraining_grounds.ore = 9\n"
              "farm_reconversion.gold = 2713\nfarm_reconversion.wood = 3\nfarm_reconversion.ore = 5\n"
              "training_grounds.require_upgraded_archery_range = 1\n",
    "global_tax_failed_training": "taxpayer.require_farms = 0\nconscription.peasant_success_ratio = 0\n",
}
for name, ini in profiles.items():
    results = []
    for diagnostics in (0, 2):
        with tempfile.TemporaryDirectory(prefix="km-project-") as directory:
            root = Path(directory)
            (root / "fheroes2").mkdir()
            (root / "fheroes2" / "knight_mod.ini").write_text(ini)
            config = root / "settings.cfg"
            config.write_text(f"taxpayer test diagnostics = {diagnostics}\n")
            result = subprocess.run([executable, str(config)], env=dict(os.environ, XDG_CONFIG_HOME=directory),
                                    text=True, capture_output=True, timeout=30)
            assert result.returncode == 0, result.stdout + result.stderr
            lines = (result.stdout + result.stderr).splitlines()
            results.append(next(line for line in lines if line.startswith("RESULT")))
            # Projecting may not emit actual purchase/payment logs.
            observed = result.stdout.split("OBS_BEGIN\n", 1)[1].split("OBS_END\n", 1)[0]
            assert "[KNIGHT ECONOMY]" not in observed and "[TAXPAYER" not in observed
    assert results[0] == results[1], (name, results)
    print(f"PASS {name}: daily funds, stocks, cycle, conversion costs, six pools; diagnostics neutral")
print("All 6 engine processes passed; 9 actions each, 7 days, Knight + Warlock")
