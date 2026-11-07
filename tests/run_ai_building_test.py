"""Run the native AI building integration executable with isolated real INIs.

Pass the path to an assertion-enabled executable built from ai_building_test.cpp
and game objects, excluding the game's main and ai_planner_castle.o. Each case
uses a fresh process because the production INI loader caches its settings.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

executable = Path(sys.argv[1]).resolve()
profiles = [(2713, 3, 5), (500, 2, 4), (0, 0, 0), (1500, 0, 0)]
for gold, wood, ore in profiles:
    with tempfile.TemporaryDirectory(prefix="km-ai-building-") as temporary:
        folder = Path(temporary) / "fheroes2"
        folder.mkdir()
        (folder / "knight_mod.ini").write_text(
            "taxpayer.gold_per_peasant_per_day = 2\n"
            "taxpayer.require_farms = 1\n"
            "training_grounds.gold = 3456\n"
            "training_grounds.wood = 7\n"
            "training_grounds.ore = 9\n"
            f"farm_reconversion.gold = {gold}\n"
            f"farm_reconversion.wood = {wood}\n"
            f"farm_reconversion.ore = {ore}\n",
            encoding="utf-8",
        )
        environment = dict(os.environ, XDG_CONFIG_HOME=temporary)
        result = subprocess.run([str(executable)], env=environment, text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError(f"Profile {gold}/{wood}/{ore} failed: {result.stdout}\n{result.stderr}")
        print(f"PASS: reconversion {gold} gold / {wood} wood / {ore} ore; training 3456/7/9")
print("All 4 isolated INI profiles passed")
