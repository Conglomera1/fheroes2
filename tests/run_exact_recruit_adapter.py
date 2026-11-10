"""Native isolated adapter controls; fault callbacks are test injections, not game bugs."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("binary", type=Path)
parser.add_argument("--output-dir", type=Path, required=True)
parser.add_argument("--modes", nargs="*", help="Limit a justified rerun to affected modes")
parser.add_argument("--library-dir", type=Path, action="append", default=[], help="Runtime SDL/dependency directory, repeatable")
args = parser.parse_args()
binary, output = args.binary.resolve(), args.output_dir.resolve()
environment = dict(os.environ)
if args.library_dir:
    environment["LD_LIBRARY_PATH"] = ":".join([str(p.resolve()) for p in args.library_dir] + [environment.get("LD_LIBRARY_PATH", "")])
output.mkdir(parents=True, exist_ok=True)
# Independent numeric oracles: status, dispatches, pending, blocked, gold, stock,
# garrison increment, visitor increment, purchase traces. All 7 resources and
# complete slots/stocks/config are additionally checked inside the native fixture.
expected = {
    "exact": (2, 1, 0, 0, 1200, 8, 8, 0, 1),
    "merge": (2, 1, 0, 0, 1200, 8, 8, 0, 1),
    "visitor": (2, 1, 0, 0, 1200, 8, 0, 8, 1),
    "visitor_merge": (2, 1, 0, 0, 1200, 8, 0, 8, 1),
    "both_full": (3, 0, 1, 0, 0, 0, 0, 0, 0),
    "full": (3, 0, 1, 0, 0, 0, 0, 0, 0),
    "overstock": (3, 0, 1, 0, 0, 0, 0, 0, 0),
    "empty": (3, 0, 1, 0, 0, 0, 0, 0, 0),
    "poor": (3, 0, 1, 0, 0, 0, 0, 0, 0),
    "zero": (0, 0, 0, 0, 0, 0, 0, 0, 0),
    "pool": (0, 0, 0, 0, 0, 0, 0, 0, 0),
    "unbuilt": (3, 0, 1, 0, 0, 0, 0, 0, 0),
    "upgrade": (2, 1, 0, 0, 1600, 8, 8, 0, 1),
    "overflow": (3, 0, 1, 0, 0, 0, 0, 0, 0),
    "visitor_overflow": (3, 0, 1, 0, 0, 0, 0, 0, 0),
    "no_accept": (0, 0, 0, 0, 0, 0, 0, 0, 0),
    "legacy": (0, 0, 0, 0, 0, 0, 0, 0, 0),
    "foreign": (0, 0, 1, 0, 0, 0, 0, 0, 0),
    "stale": (0, 0, 1, 0, 0, 0, 0, 0, 0),
    "forged": (0, 0, 1, 0, 0, 0, 0, 0, 0),
    "order": (0, 0, 2, 0, 0, 0, 0, 0, 0),
    "changed_funds": (0, 0, 1, 0, -1, 0, 0, 0, 0),
    "changed_visitor": (0, 0, 1, 0, 0, 0, 0, 0, 0),
    "changed_mission": (0, 0, 1, 0, 0, 0, 0, 0, 0),
    "changed_role": (0, 0, 1, 0, 0, 0, 0, 0, 0),
    "identity": (2, 1, 0, 0, 1200, 8, 8, 0, 1),
    "budget": (0, 0, 1, 0, 0, 0, 0, 0, 0),
    "coverage": (0, 0, 1, 0, 0, 0, 0, 0, 0),
    "two": (2, 2, 0, 0, 750, 5, 5, 0, 2),
    "false_clean": (3, 2, 0, 0, 1200, 8, 8, 0, 1),
    "true_empty": (4, 1, 1, 1, 0, 0, 0, 0, 0),
    "false_effect": (4, 1, 1, 1, 1200, 8, 8, 0, 1),
    "wrong_cost": (4, 1, 1, 1, 1199, 8, 8, 0, 1),
    "wrong_stock": (4, 1, 1, 1, 150, 1, 1, 0, 1),
    "wrong_destination": (4, 1, 1, 1, 1200, 8, 0, 8, 1),
    "slot_reorder": (4, 1, 1, 1, 1200, 8, 8, 0, 1),
    "record_fault": (4, 1, 1, 1, 1200, 8, 8, 0, 1),
    "throw_after": (4, 1, 1, 1, 1200, 8, 8, 0, 1),
    "reentrant": (2, 1, 0, 0, 1200, 8, 8, 0, 1),
    "pure": (0, 0, 0, 0, 0, 0, 0, 0, 0),
}
fields = ("status", "calls", "pending", "blocked", "gold_spent", "stock_removed", "garrison_added", "visitor_added")
records = []
for mode in args.modes or expected:
    if mode not in expected:
        raise SystemExit(f"Unknown mode: {mode}")
    pair = []
    for level in (0, 2):
        with tempfile.TemporaryDirectory(prefix="km-exact-adapter-") as d:
            root = Path(d)
            (root / "fheroes2").mkdir()
            (root / "fheroes2/knight_mod.ini").write_text("# Isolated defaults\n")
            cfg = root / "settings.cfg"
            cfg.write_text(f"taxpayer test diagnostics = {level}\n")
            p = subprocess.run([str(binary), str(cfg), mode], env=dict(environment, XDG_CONFIG_HOME=d),
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=40)
            (output / f"{mode}-d{level}.log").write_text(f"RETURN_CODE={p.returncode}\n" + p.stdout)
            assert p.returncode == 0, (mode, level, p.stdout)
            result = next(line for line in p.stdout.splitlines() if line.startswith("RESULT "))
            raw = dict(x.split("=", 1) for x in result.split()[1:])
            observed = {k: (v if k == "mode" else int(v)) for k, v in raw.items()}
            assert tuple(observed[k] for k in fields) == expected[mode][:-1], (mode, observed, expected[mode])
            assert observed["synthetic_oracles"] == (38 if mode == "pure" else 0)
            interval = p.stdout.split("ADAPTER_BEGIN\n", 1)[1].split("ADAPTER_END\n", 1)[0]
            traces = [json.loads(line.split("[KNIGHT ECONOMY] ", 1)[1]) for line in interval.splitlines() if "[KNIGHT ECONOMY] " in line]
            purchases = [row for row in traces if row["event"] == "TROOP_PURCHASE"]
            assert len(purchases) == (expected[mode][-1] if level else 0), (mode, purchases)
            assert all(row["kind"] == "actual" for row in purchases)
            if not level:
                assert not traces
            pair.append(observed)
            records.append(dict(observed, diagnostics=level, exit_code=p.returncode))
            (output / "observations.json").write_text(json.dumps(records, indent=2) + "\n")
    assert pair[0] == pair[1], (mode, pair)
    print(f"PASS {mode}: {pair[0]}; diagnostics neutral", flush=True)
print(f"PASS {len(records)} isolated native processes, {len(records)//2} diagnostic pairs; synthetic faults labelled in fixture")
