# C3D5D: bound local Peasant transfers in observation programs

Parent C3D5C: 44bb0efa8ec118b8219bc24c655def94e9eb66df.
Extend the existing contract with Program.visitor, a LOCAL actor binding, separate
from the beneficiary. Town remains the beneficiary; remote/exact-visitor delivery
destinations remain unsupported. Accept only ToGarrison/ToVisitor Peasant counts,
no arbitrary creature/pool/building transfer. Actual legality, source population,
slots, last hero troop and subsequent economy still use unchanged Project logic.

Binding requires the same town visitor in captured physical and mission state,
with fixed local PATROL radius zero. Missing, changed, mobile or unobserved hero
blocks import; matching stationary-ID set alone is insufficient. Keep the binding
on partially executed programs. ALL visitor-bound programs remain protected even
if caller asks for abandonment: the old recourse permission shape cannot carry
that actor binding. No new execution or abandonment authority is introduced.

Registry reporting tickets include the bound visitor ID; mismatched receipts fail.
Refresh against a changed visitor blocks the registry and invalidates old tickets.
The snapshot observer binds each local transfer to that exact actor and preserves
the entire source schedule/order, without dropping any transfer. Future planned
remote transports do not become supported through this path.

Intermediate checkpoint: 8f0e877da5ce39a628994ab23faa8a7d62550f6d,
knight-ai-lot-c3d5d-wip1-20261109 (21 initial pure checks before final validation).

Final: 21 new pure checks and the same 21 ASan/UBSan (detect_leaks=0; no leak
validation claimed). Relevant regressions: commitments 44, registry 38, bridge 23.
Native game compilation/link with no warnings. All 16 native Pipeline processes
pass with default/custom INI and diagnostics off/on. Both known and fixed-patrol
plans now stage completely; uncertain paths/mobile visitor remain held. The
patrol fixture explicitly checks every transfer is bound to its actual visitor,
all source links survive, no abandonment is authorized, real funds/armies unchanged
and shared projection counts remain 64 (searched) or 256 (held).

Native patrol fixture must now stage its WHOLE selected local-transfer plan, with
the original shared budget and unchanged real funds/armies. Control routes still
do not constitute a map campaign or real KingdomTurn/execution-callback test.
No gameplay/default/INI/save/resource change or Windows package; still observation.
