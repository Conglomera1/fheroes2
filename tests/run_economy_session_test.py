"""Exercise real save/load and trace segmentation, with/without diagnostics."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

executable = str(Path(sys.argv[1]).resolve())
states = []
for diagnostics in (0, 2):
    with tempfile.TemporaryDirectory(prefix="km-session-") as directory:
        root = Path(directory)
        (root / "fheroes2").mkdir()
        (root / "fheroes2" / "knight_mod.ini").write_text("# Default configuration\n")
        settings = root / "settings.cfg"
        settings.write_text(f"taxpayer test diagnostics = {diagnostics}\n")
        save = root / "session.SAV"
        result = subprocess.run([executable, str(settings), str(save)],
                                env=dict(os.environ, XDG_CONFIG_HOME=directory),
                                capture_output=True, text=True, timeout=20)
        assert result.returncode == 0, result.stdout + result.stderr
        lines = (result.stdout + result.stderr).splitlines()
        states.append(next(line for line in lines if line.startswith("RESULT ")))
        events = [json.loads(line.split("[KNIGHT ECONOMY] ", 1)[1]) for line in lines if "[KNIGHT ECONOMY] " in line]
        if diagnostics == 0:
            assert not events
            continue
        sessions = [e for e in events if e["event"] == "SESSION_BEGIN"]
        assert [e["reason"] for e in sessions] == ["world_initialized", "save_loaded"]
        assert sessions[1]["segment"] > sessions[0]["segment"]
        assert sessions[0]["run"] == sessions[1]["run"]
        assert sessions[1]["source_path_bytes"] == str(save)
        assert [e["seq"] for e in events] == list(range(1, len(events) + 1))
        assert sum(e["event"] == "SAVE_WRITTEN" for e in events) == 1
        opening = [e for e in events if e["event"] == "OPENING_BALANCE"]
        assert len(opening) == 2 and opening[1]["funds"]["gold"] == opening[0]["funds"]["gold"] + 42
assert states[0] == states[1]
print("PASS: real save/load, session IDs, opening balances, context nesting/escaping, diagnostic neutrality (2 runs)")
