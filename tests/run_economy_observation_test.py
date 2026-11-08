"""Check actual-engine forecasts and diagnostic neutrality in fresh processes."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

executable = str(Path(sys.argv[1]).resolve())
for farms in (0, 1):
    for ratio in ("1.0", "0.85", "0"):
        states = []
        for diagnostics in (0, 2):
            with tempfile.TemporaryDirectory(prefix="km-economy-") as directory:
                folder = Path(directory) / "fheroes2"
                folder.mkdir()
                (folder / "knight_mod.ini").write_text(
                    f"taxpayer.require_farms = {farms}\n"
                    f"conscription.peasant_success_ratio = {ratio}\n", encoding="utf-8"
                )
                settings = Path(directory) / "fheroes2.cfg"
                settings.write_text(f"taxpayer test diagnostics = {diagnostics}\n", encoding="utf-8")
                result = subprocess.run([executable, str(settings)], env=dict(os.environ, XDG_CONFIG_HOME=directory),
                                        text=True, capture_output=True, timeout=20)
                if result.returncode:
                    raise RuntimeError(f"Engine test failed: {farms}/{ratio}/{diagnostics}\n{result.stdout}\n{result.stderr}")
                lines = (result.stdout + result.stderr).splitlines()
                state = [line for line in lines if line.startswith("RESULT ")]
                assert len(state) == 1, state
                states.append(state[0])
                events = [json.loads(line.split("[KNIGHT ECONOMY] ", 1)[1]) for line in lines if "[KNIGHT ECONOMY] " in line]
                if diagnostics == 0:
                    assert not events
                else:
                    assert events, "Diagnostic build required"
                    assert [e["seq"] for e in events] == list(range(1, len(events) + 1))
                    for event in events:
                        if event["event"] not in ("FUNDS_CREDIT", "FUNDS_DEBIT"):
                            continue
                        credit = event["event"] == "FUNDS_CREDIT"
                        delta = event["delta"] if credit else event["requested"]
                        for resource, before in event["before"].items():
                            expected = max(0, before + (delta[resource] if credit else -delta[resource]))
                            assert event["after"][resource] == expected, (event, resource, expected)
                    actual = [e for e in events if e["event"] == "CONSCRIPTION_DAY"]
                    assert len(actual) == 12  # Two TG, six processed mornings.
                    for event in actual:
                        attempts = event["garrison_used"] + event["hut_used"]
                        assert attempts == sum(event["recruits"]) + event["failed"]
                        assert event["garrison_before"] - event["garrison_after"] == event["garrison_used"]
                        assert event["hut_before"] - event["hut_after"] == event["hut_used"]
                        assert event["fees"] == attempts * event["fee_per_attempt"] + event["hut_used"] * 20
                    assert len([e for e in events if e["event"] == "FORECAST"]) == 6
        assert states[0] == states[1], "Diagnostics changed the game state"
        print(f"PASS: farms={farms}, success={ratio}: engine matches forecast; diagnostics neutral")
print("All 12 fresh-process runs passed")
