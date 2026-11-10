"""Link the isolated adapter fixture with an existing matching native debug build.

Does not rebuild production or fetch dependencies. SDL flags/libs are supplied
as in the matching game build. Output belongs outside the source tree.
"""
import argparse
from pathlib import Path
import shlex
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--sdl-flags", required=True)
parser.add_argument("--sdl-libs", required=True)
parser.add_argument("--ldflags", default="")
parser.add_argument("--sanitize", action="store_true", help="Instrument fixture/adapter only; game objects remain uninstrumented")
parser.add_argument("--source", default="tests/exact_recruit_adapter_engine_test.cpp", help="Repository-relative isolated native fixture")
parser.add_argument("--exclude-object", action="append", default=[], help="Game object included directly by the fixture, repeatable")
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
output = args.output.resolve()
output.parent.mkdir(parents=True, exist_ok=True)
objects = sorted(p for p in (root / "src/dist/fheroes2").glob("*.o") if p.name not in {"fheroes2.o", *args.exclude_object})
if len(objects) < 200:
    raise SystemExit("Matching native game objects are missing; restore the documented debug build first")
includes = ["-I" + str(p) for p in (root / "src/fheroes2").iterdir() if p.is_dir()]
includes += ["-I" + str(root / "src/engine"), "-I" + str(root / "src/thirdparty/libsmacker")]
sanitizers = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if args.sanitize else []
source = (root / args.source).resolve()
if not source.is_relative_to(root / "tests") or not source.is_file():
    raise SystemExit("Fixture source must be an existing file under tests/")
fixture_object = output.with_suffix(".o")
subprocess.run(["g++", "-c", str(source), "-o", str(fixture_object), "-std=c++17", "-O0", "-g", "-fsigned-char", "-pthread",
                "-Wall", "-Wextra", "-Wshadow", "-Wfloat-conversion", "-pedantic", "-UNDEBUG", "-DWITH_DEBUG", "-DWITH_IMAGE",
                "-DFHEROES2_TAXPAYER_DIAGNOSTICS", *sanitizers, *includes, *shlex.split(args.sdl_flags)], check=True)
subprocess.run(["g++", "-o", str(output), str(fixture_object), *map(str, objects),
                str(root / "src/dist/engine/libengine.a"), str(root / "src/dist/thirdparty/libsmacker/libsmacker.a"),
                "-pthread", *sanitizers, *shlex.split(args.sdl_libs), "-lz", *shlex.split(args.ldflags)], check=True)
print("Built isolated adapter fixture" + (" (fixture/adapter ASan+UBSan only)" if args.sanitize else ""))
