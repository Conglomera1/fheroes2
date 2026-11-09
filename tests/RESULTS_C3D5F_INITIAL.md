# C3D5F intermediate native observation — 10 November 2026

Source: 39d2cd4b497f22789f94a5db82cba8ca501b71c6, production code unchanged
from validated C3D5D. Native game/fixture compiled and linked, no compiler warnings.
Original draft fixture ran in three fresh isolated processes with diagnostics 0.

| Mode | Exit | Requested | Initial stock | Units added | Stock removed | Gold paid | Consistent |
|---|---:|---:|---:|---:|---:|---:|---|
| raw_exact | 0 | 8 | 8 | 8 | 8 | 1200 | yes |
| raw_overstock | **2** | 9 | 8 | **9** | **8** | 1350 | **no** |
| guarded_overstock | 0 | 9 | 8 | 0 | 0 | 0 | yes, API skipped |

The stock discrepancy is now reproduced through the real Castle::RecruitMonster
API. The raw inconsistency is retained as a RED observation, not a passing API
regression. This fixture intentionally bypasses normal caller recruitment limits;
it does not establish that normal AI/game paths issue this request. Existing
getRecruitLimit and copied Project checks must not be assumed bypassed in games.

The guard remains test-local. No global API correction, executor, live receipt,
gameplay/default/INI/save change or player package is introduced.

Next within this block: check empty stock, insufficient funds, full destinations,
visitor fallback and diagnostics neutrality, then report before changing the API.

Raw initial output retained verbatim:

```text
MODE raw_exact EXIT 0
RESULT mode=raw_exact invoked=1 accepted=1 requested=8 stock_before=8 stock_after=0 units_added=8 stock_removed=8 gold_spent=1200 receipt_consistent=1
MODE raw_overstock EXIT 2
RESULT mode=raw_overstock invoked=1 accepted=1 requested=9 stock_before=8 stock_after=0 units_added=9 stock_removed=8 gold_spent=1350 receipt_consistent=0
MODE guarded_overstock EXIT 0
RESULT mode=guarded_overstock invoked=0 accepted=0 requested=9 stock_before=8 stock_after=8 units_added=0 stock_removed=0 gold_spent=0 receipt_consistent=1
```
