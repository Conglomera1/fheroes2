"""Independent numeric oracles for exact, bound local Peasant transfers."""
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
parser.add_argument("--profiles", nargs="*")
parser.add_argument("--modes", nargs="*")
args = parser.parse_args()
binary, output = args.binary.resolve(), args.output_dir.resolve()
output.mkdir(parents=True, exist_ok=True)
environment = dict(os.environ)
if args.library_dir:
    environment["LD_LIBRARY_PATH"] = ":".join([str(p.resolve()) for p in args.library_dir] + [environment.get("LD_LIBRARY_PATH", "")])
profiles = {
    "default": "# Default Farm\n",
    "tg": "# Default settings, physical Training Grounds\n",
    "custom": "conscription.gold_per_peasant = 137\nconscription.odd_day_limit = 1\nconscription.even_day_limit = 4\nconscription.peasant_success_ratio = 0.85\nconscription.archers = 12\nconscription.pikemen = 4\nconscription.swordsmen = 1\ntaxpayer.gold_per_peasant_per_day = 1\n",
    "compatibility": "taxpayer.require_farms = 0\n",
}
main_modes = (
    "exact", "new_slot", "full_merge", "source_split", "destination_split", "all_peasants", "empty_garrison", "last_unit", "leave_one", "split_last",
    "insufficient", "no_source", "full", "overflow", "overflow_alternate", "zero", "no_binding", "wrong_binding", "mobile", "no_accept", "unsupported",
    "stale", "forged_visitor", "forged_count", "changed_visitor", "changed_mission", "changed_source", "budget", "coverage", "two", "reverse",
    "false_clean", "true_empty", "false_effect", "wrong_quantity", "wrong_creature", "wrong_funds", "rollback_false", "rollback_true", "record_fault",
    "throw_after", "reentrant", "pure", "progress", "no_funds",
)
special = {
    "tg": ("exact", "all_peasants", "leave_one", "source_split", "no_funds", "progress"),
    "custom": ("exact", "source_split", "reverse", "no_funds", "progress"),
    "compatibility": ("exact", "new_slot", "reverse", "no_funds"),
}


def oracle(profile, mode, deposit):
    # Known fixture populations, independent of the adapter's slot/plan algorithm.
    g, h = 20, 10
    if mode in {"new_slot", "full"}:
        if deposit: g = 0
        else: h = 0
    if mode == "no_source":
        if deposit: h = 0
        else: g = 0
    if mode == "full_merge":
        if deposit: g = 3
        else: h = 3
    if mode == "source_split" and not deposit: g = 10
    if mode == "destination_split" and deposit: g = 10
    if mode in {"overflow", "overflow_alternate"}:
        huge = 2**32 - 1 + (3 if mode == "overflow_alternate" else 0)
        if deposit: g = huge
        else: h = huge
    if mode == "empty_garrison" and deposit: g = 0
    gb, hb = g, h
    qty = 7 if mode == "source_split" else 9 if mode == "leave_one" and deposit else 19 if mode == "leave_one" else 5
    if mode == "all_peasants" or (mode == "empty_garrison" and not deposit): qty = h if deposit else g
    status, calls, pending, blocked = 2, 1, 0, 0
    moved = qty
    no_program = {"zero", "no_binding", "wrong_binding", "mobile", "no_accept", "unsupported", "pure"}
    held = {"stale", "forged_visitor", "forged_count", "changed_visitor", "changed_mission", "changed_source", "budget", "coverage"}
    refused = {"insufficient", "no_source", "full", "overflow", "overflow_alternate"}
    if deposit and mode in {"last_unit", "split_last"}: refused.add(mode)
    if mode in no_program:
        status, calls, pending, moved = 0, 0, 0, 0
    elif mode in held:
        status, calls, pending, moved = 0, 0, 1, 0
    elif mode in refused:
        status, calls, pending, moved = 3, 0, 1, 0
    elif mode in {"two", "reverse"}:
        calls, moved = 2, 5 if mode == "two" else 0
    elif mode == "false_clean":
        status, calls = 3, 2
    elif mode == "rollback_false":
        status, pending, moved = 3, 1, 0
    elif mode == "true_empty" or mode == "false_effect" or mode.startswith("wrong_") or mode in {"rollback_true", "record_fault", "throw_after"}:
        status, pending, blocked = 4, 1, 1
        if mode in {"true_empty", "rollback_true"}: moved = 0
        if mode == "wrong_quantity": moved = 6
    g += moved if deposit else -moved
    h += -moved if deposit else moved
    if mode == "changed_source":
        if deposit: h += 1
        else: g += 1
    taxed = profile in {"default", "compatibility"} and mode != "progress" and mode not in {"overflow", "overflow_alternate"}
    # In compatibility mode a replaced hero is still in the roster; that mutation
    # is external to the adapter. The default matrix exercises replacement.
    extra = 10 if profile == "compatibility" and mode == "changed_visitor" else 0
    return dict(status=status, calls=calls, pending=pending, blocked=blocked,
                garrison_before=gb, garrison_after=g, visitor_before=hb, visitor_after=h,
                gold_spent=int(mode == "wrong_funds"), funds_same=int(mode != "wrong_funds"), stocks_same=1, buildings_same=1,
                progress_before=(4 if profile == "custom" else 3) if mode == "progress" else 0, progress_same=1,
                tax_before=(gb + hb) * 2 if taxed else 0, tax_after=(g + h + extra) * 2 if taxed else 0,
                synthetic_oracles=26 if mode == "pure" else 0)


records = []
for profile in args.profiles or profiles:
    if profile not in profiles: raise SystemExit("Unknown profile: " + profile)
    for mode in args.modes or (main_modes if profile == "default" else special[profile]):
        if mode not in main_modes: raise SystemExit("Unknown mode: " + mode)
        for direction in ("garrison", "visitor"):
            pair = []
            expected = oracle(profile, mode, direction == "garrison")
            for level in (0, 2):
                with tempfile.TemporaryDirectory(prefix="km-transfer-adapter-") as d:
                    root = Path(d); (root / "fheroes2").mkdir()
                    ini = profiles[profile]
                    if mode in {"overflow", "overflow_alternate"}:
                        # Isolate troop-count limits from unrelated capped-income
                        # behavior in this synthetic UINT32_MAX fixture.
                        ini += "taxpayer.gold_per_peasant_per_day = 0\n"
                    (root / "fheroes2/knight_mod.ini").write_text(ini)
                    cfg = root / "settings.cfg"; cfg.write_text(f"taxpayer test diagnostics = {level}\n")
                    p = subprocess.run([str(binary), str(cfg), mode, direction, profile], env=dict(environment, XDG_CONFIG_HOME=d),
                                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=40)
                    (output / f"{profile}-{mode}-{direction}-d{level}.log").write_text(f"RETURN_CODE={p.returncode}\n" + p.stdout)
                    assert p.returncode == 0, (profile, mode, direction, level, p.stdout)
                    result = next(line for line in p.stdout.splitlines() if line.startswith("RESULT "))
                    raw = dict(x.split("=", 1) for x in result.split()[1:])
                    observed = {k: v if k in {"profile", "mode", "direction"} else int(v) for k, v in raw.items()}
                    for key, value in expected.items(): assert observed[key] == value, (profile, mode, direction, key, observed[key], value, observed)
                    interval = p.stdout.split("TRANSFER_BEGIN\n", 1)[1].split("TRANSFER_END\n", 1)[0]
                    rows = [json.loads(line.split("[KNIGHT ECONOMY] ", 1)[1]) for line in interval.splitlines() if "[KNIGHT ECONOMY] " in line]
                    assert not any(r["event"] in {"BUILD_ACTION", "TROOP_PURCHASE"} for r in rows), rows
                    if not level: assert not rows
                    pair.append(observed); records.append(dict(observed, diagnostics=level, exit_code=p.returncode))
                    (output / "observations.json").write_text(json.dumps(records, indent=2) + "\n")
            assert pair[0] == pair[1], (profile, mode, direction, pair)
            print(f"PASS {profile}/{mode}/{direction}: status={pair[0]['status']} calls={pair[0]['calls']} peasants={pair[0]['garrison_before']}/{pair[0]['visitor_before']}->{pair[0]['garrison_after']}/{pair[0]['visitor_after']}; diagnostics neutral", flush=True)
print(f"PASS {len(records)} isolated transfer processes, {len(records)//2} diagnostic pairs")
