"""Compare next-morning forecasts to actual balances without treating drift as a bug.

Usage: python3 tests/check_economy_log.py fheroes2.log
Only schema 2 is supported. Report observed transactions as context, never as
proof that all world changes have been recorded or that an error is explained.
"""
import json
import sys

RESOURCES = ("gold", "wood", "ore", "mercury", "sulfur", "crystal", "gems")


def compare(lines):
    predictions, transactions, comparisons = {}, {}, []
    for line in lines:
        if "[KNIGHT ECONOMY] " not in line:
            continue
        event = json.loads(line.split("[KNIGHT ECONOMY] ", 1)[1])
        if event["schema"] != 2 or "player" not in event:
            continue
        key = event["run"], event["segment"], event["player"]
        if event["event"] in ("FUNDS_CREDIT", "FUNDS_DEBIT"):
            transactions.setdefault(key, []).append(event)
        if event["event"] == "FORECAST_DAY" and event["projected_day"] == event["forecast_origin_day"] + 1:
            predictions[key] = event
            transactions[key] = []
        if event["event"] != "DAILY_CLOSE":
            continue
        predicted = predictions.pop(key, None)
        if predicted is None or predicted["projected_day"] != event["day"]:
            continue
        expected = dict(zip(RESOURCES, [predicted["closing_gold"], *predicted["closing_materials"]]))
        delta = {name: event["funds"][name] - expected[name] for name in RESOURCES}
        context = sorted({item["cause"] for item in transactions.get(key, [])
                          if item["cause"] not in ("daily_income", "conscription")})
        comparisons.append({"run": event["run"], "segment": event["segment"], "player": event["player"],
                            "forecast_origin_day": predicted["forecast_origin_day"], "day": event["day"],
                            "status": "match" if not any(delta.values()) else "difference_requires_context",
                            "delta": delta, "observed_transaction_causes": context})
    return comparisons


if __name__ == "__main__":
    with open(sys.argv[1], encoding="utf-8", errors="replace") as stream:
        for comparison in compare(stream):
            print(json.dumps(comparison))
