"""Check actual-engine forecasts and diagnostic neutrality in fresh processes."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from check_economy_log import compare

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
                    assert all(e["schema"] == 2 for e in events)
                    assert len({e["run"] for e in events}) == 1
                    sessions = [e for e in events if e["event"] == "SESSION_BEGIN"]
                    assert len(sessions) == 1
                    assert sessions[0]["success_scaled"] == round(float(ratio) * 1000000)
                    assert sessions[0]["require_farms"] == bool(farms)
                    assert (sessions[0]["odd_limit"], sessions[0]["even_limit"]) == (2, 3)
                    openings = [e for e in events if e["event"] == "OPENING_BALANCE"]
                    assert len(openings) == 1 and len(openings[0]["funds"]) == 7
                    for cause in ("daily_income", "calendar_event", "conscription", "building_purchase", "troop_purchase", "market_trade"):
                        assert any(e["cause"] == cause and e["event"] in ("FUNDS_CREDIT", "FUNDS_DEBIT") for e in events), cause
                    assert sum(e["event"] == "BUILD_ACTION" for e in events) == 1
                    assert sum(e["event"] == "TROOP_PURCHASE" for e in events) == 1
                    assert sum(e["event"] == "MARKET_TRADE" for e in events) == 1
                    assert all(len(e["closing_materials"]) == 6 for e in events if e["event"] == "FORECAST")
                    projected = [e for e in events if e["event"] == "FORECAST_DAY"]
                    assert len(projected) == 6 * 14
                    for day in projected:
                        assert day["opening_gold"] + day["income"] + day["event_gold"] - day["fees"] == day["closing_gold"]
                        assert [a + b + c for a, b, c in zip(day["opening_materials"], day["income_materials"], day["event_materials"])] == day["closing_materials"]
                        assert day["garrison_used"] + day["hut_used"] == sum(day["recruits"]) + day["failed"]
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
                    comparisons = compare(lines)
                    assert len(comparisons) == 5
                    assert sum(item["status"] == "match" for item in comparisons) == 4
                    difference = next(item for item in comparisons if item["status"] != "match")
                    assert difference["day"] == 3 and difference["delta"]["gold"] == -100
                    assert difference["observed_transaction_causes"] == ["calendar_event"]
                    assert [difference["delta"][r] for r in ("wood", "ore", "mercury", "sulfur", "crystal", "gems")] == [7, 8, 9, 10, 11, 12]
        assert states[0] == states[1], "Diagnostics changed the game state"
        print(f"PASS: farms={farms}, success={ratio}: engine matches forecast; diagnostics neutral")
print("All 12 fresh-process runs passed: seven resources, explicit event, causes, schema and neutrality")
