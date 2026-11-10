"""Reuse the previous headless UI recipe with CURRENT production AI objects.

Only rendering, audio playback and event-loop sleeping are disabled in temporary
copies. No old balance AI override, RNG override or economic policy is linked.
Owned Heroes II data and a map must be supplied through the normal data layout.
"""
import argparse
from pathlib import Path
import shlex
import subprocess

p = argparse.ArgumentParser()
p.add_argument("--output", type=Path, required=True)
p.add_argument("--sdl-flags", required=True)
p.add_argument("--sdl-libs", required=True)
p.add_argument("--ldflags", default="")
a = p.parse_args(); root = Path(__file__).resolve().parents[1]; output = a.output.resolve()
work = output.parent / (output.name + "-objects"); work.mkdir(parents=True, exist_ok=True)
includes = ["-I" + str(x) for x in (root / "src/fheroes2").iterdir() if x.is_dir()] + ["-I" + str(root / "src/engine"), "-I" + str(root / "src/thirdparty/libsmacker")]
flags = ["g++", "-std=c++17", "-O0", "-g", "-DWITH_DEBUG", "-DWITH_IMAGE", "-DFHEROES2_TAXPAYER_DIAGNOSTICS", *includes, *shlex.split(a.sdl_flags)]
patches = {
    "screen": ("src/engine/screen.cpp", "void Display::_renderFrame( const Rect & roi ) const\n    {", "void Display::_renderFrame( const Rect & roi ) const\n    {\n        static_cast<void>( roi ); return;"),
    "localevent": ("src/engine/localevent.cpp", "if ( eventProcessingTimer.getMs() < globalLoopSleepTime )", "if ( false )"),
    "audio_manager": ("src/fheroes2/audio/audio_manager.cpp", "void PlayMusicAsync( const int trackId, const Music::PlaybackMode playbackMode )\n    {", "void PlayMusicAsync( const int trackId, const Music::PlaybackMode playbackMode )\n    {\n        return;"),
}
objects = []
for name, (source, old, new) in patches.items():
    text = (root / source).read_text(); assert text.count(old) == 1
    temporary = work / (name + ".cpp"); temporary.write_text(text.replace(old, new))
    obj = work / (name + ".o"); subprocess.run([*flags, "-c", str(temporary), "-o", str(obj)], check=True); objects.append(obj)
runner = work / "runner.o"
subprocess.run([*flags, "-c", str(root / "tests/live_knight_map_smoke.cpp"), "-o", str(runner)], check=True)
game = sorted(x for x in (root / "src/dist/fheroes2").glob("*.o") if x.name not in {"fheroes2.o", "audio_manager.o"})
subprocess.run(["g++", "-rdynamic", "-o", str(output), str(runner), *map(str, objects), *map(str, game),
                str(root / "src/dist/engine/libengine.a"), str(root / "src/dist/thirdparty/libsmacker/libsmacker.a"), "-pthread",
                *shlex.split(a.sdl_libs), "-lz", *shlex.split(a.ldflags)], check=True)
print("Built full-planner map smoke runner; UI-only substitutions")
