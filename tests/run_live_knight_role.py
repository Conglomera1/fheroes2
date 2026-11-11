"""A small integration matrix through the real CastleDevelopment implementation."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("binary", type=Path)
parser.add_argument("--output-dir", type=Path, required=True)
parser.add_argument("--library-dir", type=Path, action="append", default=[])
args = parser.parse_args()
output = args.output_dir.resolve(); output.mkdir(parents=True, exist_ok=True)
env = dict(os.environ)
env["LD_LIBRARY_PATH"] = ":".join([str(p.resolve()) for p in args.library_dir] + [env.get("LD_LIBRARY_PATH", "")])
profiles = {m: "# Default\n" for m in ("rich", "poor", "danger", "prerequisite", "weekday", "mobile", "unproductive", "recover", "expensive", "custom_cost")}
profiles["unproductive"] += "conscription.peasant_success_ratio = 0\n"
profiles["recover"] += "conscription.odd_day_limit = 0\nconscription.even_day_limit = 0\n"
profiles["expensive"] += "conscription.gold_per_peasant = 1000\n"
profiles["custom_cost"] += "training_grounds.gold = 3456\ntraining_grounds.wood = 7\ntraining_grounds.ore = 9\n"
records = []
for mode, ini in profiles.items():
    pair = []
    for level in (0, 2):
        with tempfile.TemporaryDirectory(prefix="km-live-role-") as d:
            root = Path(d); (root / "fheroes2").mkdir(); (root / "fheroes2/knight_mod.ini").write_text(ini)
            cfg = root / "settings.cfg"; cfg.write_text(f"taxpayer test diagnostics = {level}\n")
            p = subprocess.run([str(args.binary.resolve()), str(cfg), mode], env=dict(env, XDG_CONFIG_HOME=d), text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            (output / f"{mode}-d{level}.log").write_text(f"RETURN_CODE={p.returncode}\n" + p.stdout)
            assert p.returncode == 0, (mode, level, p.stdout)
            choice = next(line for line in p.stdout.splitlines() if line.startswith("CHOICE "))
            result = next(line for line in p.stdout.splitlines() if line.startswith("RESULT "))
            raw = dict(field.split("=", 1) for field in result.split()[1:])
            result = {k: v if k == "mode" else int(v) for k, v in raw.items()}
            expected = 2500 if mode in {"rich", "mobile"} else 3456 if mode == "custom_cost" else 1500 if mode in {"recover", "expensive"} else 0
            assert result["paid"] == expected, (mode, result)
            inside = p.stdout.split("LIVE_BEGIN\n", 1)[1].split("LIVE_END\n", 1)[0]
            rows = [json.loads(line.split("[KNIGHT ECONOMY] ", 1)[1]) for line in inside.splitlines() if "[KNIGHT ECONOMY] " in line]
            if not level: assert not rows
            builds = [r for r in rows if r["event"] == "BUILD_ACTION"]
            assert len(builds) == int(bool(level) and expected > 0)
            assert not any(r["event"] == "TROOP_PURCHASE" for r in rows)
            pair.append((choice, result)); records.append(dict(result, diagnostics=level, choice=choice, exit_code=p.returncode))
    assert pair[0] == pair[1], pair
    print("PASS", pair[0][0], pair[0][1], flush=True)
(output / "observations.json").write_text(json.dumps(records, indent=2) + "\n")
print("PASS 20 development-path processes, diagnostics neutral")
