"""Independent cost/effect oracles for the generated isolated Farm/TG fixture."""
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
    "default": "# Defaults\n",
    "custom": "training_grounds.gold = 4321\ntraining_grounds.wood = 7\ntraining_grounds.ore = 13\nfarm_reconversion.gold = 1234\nfarm_reconversion.wood = 3\nfarm_reconversion.ore = 5\ntraining_grounds.require_captains_quarters = 0\ntaxpayer.gold_per_peasant_per_day = 3\n",
    "disabled": "conscription.enabled = 0\n",
    "extra_requirement": "training_grounds.require_moat = 1\n",
    "free": "training_grounds.gold = 0\ntraining_grounds.wood = 0\ntraining_grounds.ore = 0\nfarm_reconversion.gold = 0\n",
    "compatibility": "taxpayer.require_farms = 0\n",
}
main_modes = (
    "to_tg", "to_farm", "first_farm", "visitor_to_tg", "visitor_to_farm", "progress_to_farm",
    "poor_gold_tg", "poor_wood_tg", "poor_ore_tg", "poor_gold_farm", "poor_wood_farm", "poor_ore_farm",
    "already", "not_today", "missing_farm", "missing_captain", "missing_moat", "disabled_farm", "disabled_tg", "disabled_source_farm",
    "unsupported", "shape", "no_accept", "stale", "forged", "changed_flag", "changed_army", "budget", "coverage", "two_builds",
    "false_clean", "true_empty", "false_effect", "wrong_cost", "wrong_building", "wrong_stock", "wrong_flag", "wrong_army", "record_fault", "throw_after", "reentrant", "pure",
)
special = {
    "disabled": ("to_tg", "to_farm", "first_farm"),
    "extra_requirement": ("missing_moat", "to_farm", "first_farm"),
    "free": ("to_tg", "to_farm", "first_farm", "poor_gold_tg", "poor_wood_tg", "poor_ore_tg"),
    "compatibility": ("to_tg", "to_farm", "visitor_to_tg", "first_farm"),
}
reverse = {"to_farm", "visitor_to_farm", "progress_to_farm", "poor_gold_farm", "poor_wood_farm", "poor_ore_farm", "disabled_farm", "first_farm"}


def oracle(profile, mode):
    custom, free = profile == "custom", profile == "free"
    farm_target = mode in reverse
    tg_before = (farm_target and mode != "first_farm") or mode == "already"
    farm_before = not tg_before and mode not in ("first_farm", "missing_farm")
    listed = (1000, 0, 0) if mode == "first_farm" else ((1234, 3, 5) if custom else (0 if free else 1500, 0, 0)) if farm_target else ((4321, 7, 13) if custom else (0, 0, 0) if free else (2500, 10, 10))
    if mode == "unsupported":
        listed = (2000, 5, 5)  # Knight Mage Guild, never admitted by this policy.
    peasants = 17 if mode == "progress_to_farm" else 23 if mode.startswith("visitor_") else 20
    rate = 3 if custom else 2
    compatible = profile == "compatibility"
    before_tax = peasants * rate if farm_before or compatible else 0
    status, calls, pending, blocked = 2, 1, 0, 0
    spent = listed
    built = True
    clean_modes = {"unsupported", "shape", "no_accept", "pure"}
    held_modes = {"stale", "forged", "changed_flag", "changed_army", "budget", "coverage", "two_builds"}
    failed = mode in {"already", "not_today", "missing_farm", "disabled_farm", "disabled_tg", "disabled_source_farm"}
    failed |= mode == "missing_captain" and not custom
    failed |= mode == "missing_moat" and profile == "extra_requirement"
    failed |= profile == "disabled" and not farm_target
    poor_resource = next((i for i, word in enumerate(("gold", "wood", "ore")) if mode.startswith("poor_" + word)), None)
    failed |= poor_resource is not None and listed[poor_resource] > 0
    if mode in clean_modes:
        status, calls, pending, spent, built = 0, 0, 0, (0, 0, 0), False
    elif mode in held_modes:
        status, calls, pending, spent, built = 0, 0, 2 if mode == "two_builds" else 1, (0, 0, 0), False
    elif failed:
        status, calls, pending, spent, built = 3, 0, 1, (0, 0, 0), False
    elif mode == "false_clean":
        status, calls = 3, 2  # First refusal; explicit verified retry succeeds later.
    elif mode == "true_empty" or mode == "false_effect" or mode.startswith("wrong_") or mode in {"record_fault", "throw_after"}:
        status, pending, blocked = 4, 1, 1
        if mode == "true_empty":
            spent, built = (0, 0, 0), False
        if mode == "wrong_cost":
            spent = (listed[0] - 1, listed[1], listed[2])
        if mode == "wrong_building":
            spent, built = (1250, 0, 5), False  # Actual injected Statue purchase.
        if mode == "wrong_stock":
            spent = (listed[0] + 150, listed[1], listed[2])
    farm_after = farm_target if built else farm_before
    tg_after = not farm_target if built else tg_before
    after_peasants = peasants + (mode in {"changed_army", "wrong_army"})
    after_tax = after_peasants * rate if farm_after or compatible else 0
    return dict(status=status, calls=calls, pending=pending, blocked=blocked,
                gold_spent=spent[0], wood_spent=spent[1], ore_spent=spent[2],
                farm_before=int(farm_before), farm_after=int(farm_after), tg_before=int(tg_before), tg_after=int(tg_after),
                can_build_after=int(not built and mode not in {"not_today", "changed_flag", "wrong_building"} or mode == "wrong_flag"),
                tax_before=before_tax, tax_after=after_tax, income_delta=after_tax - before_tax + (250 if mode == "wrong_building" else 0),
                hut_growth_before=22 if farm_before else 14, hut_growth_after=22 if farm_after else 14,
                stock_same=int(mode != "wrong_stock"), progress_before=3 if mode == "progress_to_farm" else 0, progress_same=1,
                list_gold=listed[0], list_wood=listed[1], list_ore=listed[2], synthetic_oracles=27 if mode == "pure" else 0)


records = []
for profile in args.profiles or profiles:
    if profile not in profiles:
        raise SystemExit("Unknown profile: " + profile)
    for mode in args.modes or (main_modes if profile in ("default", "custom") else special[profile]):
        if mode not in main_modes:
            raise SystemExit("Unknown mode: " + mode)
        pair = []
        expected = oracle(profile, mode)
        for level in (0, 2):
            with tempfile.TemporaryDirectory(prefix="km-build-adapter-") as d:
                root = Path(d)
                (root / "fheroes2").mkdir()
                (root / "fheroes2/knight_mod.ini").write_text(profiles[profile])
                cfg = root / "settings.cfg"
                cfg.write_text(f"taxpayer test diagnostics = {level}\n")
                p = subprocess.run([str(binary), str(cfg), mode, profile], env=dict(environment, XDG_CONFIG_HOME=d),
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=40)
                (output / f"{profile}-{mode}-d{level}.log").write_text(f"RETURN_CODE={p.returncode}\n" + p.stdout)
                assert p.returncode == 0, (profile, mode, level, p.stdout)
                result = next(line for line in p.stdout.splitlines() if line.startswith("RESULT "))
                raw = dict(x.split("=", 1) for x in result.split()[1:])
                observed = {k: v if k in ("mode", "profile") else int(v) for k, v in raw.items()}
                for key, value in expected.items():
                    assert observed[key] == value, (profile, mode, key, observed[key], value, observed)
                interval = p.stdout.split("BUILD_BEGIN\n", 1)[1].split("BUILD_END\n", 1)[0]
                traces = [json.loads(line.split("[KNIGHT ECONOMY] ", 1)[1]) for line in interval.splitlines() if "[KNIGHT ECONOMY] " in line]
                builds = [r for r in traces if r["event"] == "BUILD_ACTION"]
                expected_builds = int(expected["calls"] > 0 and mode != "true_empty")
                assert len(builds) == (expected_builds if level else 0), (profile, mode, builds)
                if builds:
                    assert builds[0]["kind"] == "actual"
                    assert tuple(builds[0]["cost"][r] for r in ("gold", "wood", "ore")) == ((1250, 0, 5) if mode == "wrong_building" else (observed["list_gold"], observed["list_wood"], observed["list_ore"]))
                assert len([r for r in traces if r["event"] == "TROOP_PURCHASE"]) == int(bool(level) and mode == "wrong_stock")
                if not level:
                    assert not traces
                pair.append(observed)
                records.append(dict(observed, diagnostics=level, exit_code=p.returncode))
                (output / "observations.json").write_text(json.dumps(records, indent=2) + "\n")
        assert pair[0] == pair[1], (profile, mode, pair)
        print(f"PASS {profile}/{mode}: status={pair[0]['status']} calls={pair[0]['calls']} cost={pair[0]['gold_spent']}/{pair[0]['wood_spent']}/{pair[0]['ore_spent']} tax={pair[0]['tax_before']}->{pair[0]['tax_after']}; diagnostics neutral", flush=True)
print(f"PASS {len(records)} isolated building processes, {len(records)//2} diagnostic pairs")
