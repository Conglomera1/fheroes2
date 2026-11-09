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

Intermediate verification pending final sanitizers/native/regression checks.
Native patrol fixture must now stage its WHOLE selected local-transfer plan, with
the original shared budget and unchanged real funds/armies. Control routes still
do not constitute a map campaign or real KingdomTurn/execution-callback test.
No gameplay/default/INI/save/resource change or Windows package; still observation.
