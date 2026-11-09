# C3D5A: explicit pending-local-program contract

Parent C3D4: 9320eafcc58c3c307ca628ce0ceffe108525351c.
Short isolated block. Header-only, no production caller, no world execution or
save/INI/default changes. This is not yet a persistent/live commitment registry.

Each program supplies a stable project ID, complete remaining action schedule,
source town and explicit beneficiary. Each action has a stable ID, project, town,
date, operation and exact quantity/building. Accept only local town Build/Recruit
for now. Recruit uses Project's normal garrison-first placement; no exact visitor
delivery is promised. Remote deliveries, visitor beneficiaries and transfers are
unsupported, even for a confirmed stationary visitor, until placement/transport
are explicitly modeled. No role, current path or priority grants such authority.

A caller-issued Stamp (player/day/snapshot revision) binds a book to ONE frozen
context including effective INI. It must match the current stamp and mission day.
This is NOT automatic live freshness detection or a cryptographic capability.
The future caller MUST replace revision after relevant world/mission/INI changes.
There is no live caller in this block. Required future pre-execution refresh remains.

Import atomically into a COPY; preserve all original funds/armies/INI and unrelated
retained actions. Existing project schedules must match EXACTLY: no partial grant,
extension, changed operation/count/required flag/date/identity. Deduplicate exact
retained steps once. Reject conflicting IDs; retain dates after the horizon and
mark them unresolved. Preserve old same-day order; do not silently reschedule.

Dependencies are explicit, bounded and acyclic; no automatic inference from hints.
Stable topological order for newly inserted programs. Reject chronological or old
same-day order conflicts rather than rewrite old commitments. Prerequisites with
dependants remain protected from isolated recourse. Undeclared dependencies are
still the trusted future caller's responsibility, not proven absent by this type.

Abandonment defaults to forbidden. Draft Recourse::Revisable permissions require
explicit allowAbandon + caller assertion of independent abandonment, no executed
prefix and no represented dependants. If any old retained project lacks a complete
book entry, grant NO new release permissions (unknown dependent commitments).
Previously executed effects remain solely in the copied initial state; no refund
or rebuilt infrastructure. A protected/partially executed project remains retained.

Ready means syntactically imported, NOT legal/funded/selected/executable. Use the
existing exact Project and common Risk/Recourse session with the SAME budgets to
verify INI-dependent prices, stock, prerequisites, slots and each required risk.
No private projection allowance, virtual money or cached fixed unit price.
Returned permissions are hypothetical inputs to Recourse, not live cancellations.

Bounds: 64 programs/dependencies per program, 1024 pending/merged retained actions,
256 towns/mission heroes. No context or permission is returned after a failure.

Intermediate validation: new pure suite in progress. Sanitized checks and final
integration checks with the existing pure Project/Recourse still due at this point.
No native rebuild needed: this header is not included by production translation
units. Existing C3D4 native/real-map results are preserved, not rerun.

Next block: a real registry producer/lifecycle and its refresh, not treating
legacy BudgetEntry/priority hints as funded promises. User go-ahead required.
