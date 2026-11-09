#include "economy_registry.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
#include <type_traits>
namespace V = AI::EconomyRegistry;
namespace C = AI::EconomyCommitments;
namespace G = AI::EconomyCandidates;
namespace X = AI::EconomyMissions;
G::Context context()
{ G::Context c; c.base = fixture( false, 1 ); c.base.throughDay = 15; c.base.start.economy.nonTaxIncome = c.base.start.economy.taxRate = 0;
  c.base.start.economy.towns[0].troops.pools[0] = 50; return c; }
C::Program draft( const uint32_t day = 1 )
{ C::Program p; p.town = 42; p.allowAbandon = p.independent = true; p.pending = { G::recruit( 42, 1, 1, day ) }; return p; }
X::Snapshot missions( const G::Context & c ) { return X::classify( c.base.start, {}, {}, {} ); }
// Pure test harness only: emulate actual success with the exact existing Project routine,
// then provide that post-action state as fresh capture. No game callback is installed.
G::Context execute( const G::Context & initial, const P::Action & a )
{
    auto input = initial.base; input.throughDay = input.start.economy.day; input.actions = {a}; P::WorkBudget work;
    const auto result = P::project( input, work ); assert( result.covered() );
    auto fresh = initial; fresh.base.start = result.end; return fresh;
}
int main()
{
    static_assert( !std::is_copy_constructible<V::Registry>::value && !std::is_move_constructible<V::Registry>::value,
                   "Registry identity must not be cloned or accidentally reused" );
    size_t n = 0; auto c = context(); V::Registry registry( 1, 99 );
    assert( !registry.ready() && !registry.view().context && registry.stage( draft() ).status == V::Status::Held ); ++n;
    assert( registry.refresh( c, missions(c) ).status == V::Status::Ready && registry.revision() == 1 && registry.pending() == 0 ); ++n;
    const auto added = registry.stage( draft() ); auto view = registry.view();
    assert( added.status == V::Status::Ready && added.project == 1 && view.context && view.permissions.size() == 1 && registry.pending() == 1 && registry.revision() == 2 ); ++n;
    const auto first = view.context->base.actions[0]; const auto receipt = registry.ticket( first.id );
    assert( receipt && receipt->registry == 99 && first.id == 1 && first.project == 1 && !added.actionable ); ++n;
    auto forged = *receipt; forged.action.count = 2;
    assert( registry.record( forged, V::Outcome::Executed ).status == V::Status::Invalid && registry.ready() && registry.pending() == 1 ); ++n;
    forged = *receipt; forged.registry = 98;
    assert( registry.record( forged, V::Outcome::Executed ).status == V::Status::Invalid && registry.pending() == 1 ); ++n;
    forged = *receipt; ++forged.stamp.revision;
    assert( registry.record( forged, V::Outcome::Executed ).status == V::Status::Invalid ); ++n;
    const auto fresh = execute( c, first );
    assert( registry.record( *receipt, V::Outcome::Executed ).status == V::Status::Held && !registry.ready() && registry.pending() == 0 && !registry.view().context ); ++n;
    assert( c.base.start.economy.gold == 10000 && fresh.base.start.economy.gold == 9850 ); ++n;
    assert( registry.record( *receipt, V::Outcome::Executed ).status == V::Status::Invalid && registry.pending() == 0 ); ++n;
    assert( registry.refresh( fresh, missions(fresh) ).status == V::Status::Ready && registry.view().context->base.start.economy.gold == 9850
            && registry.view().context->base.actions.empty() ); ++n;
    P::WorkBudget emptyBudget; const auto noRepeat = P::project(registry.view().context->base,emptyBudget);
    assert( noRepeat.covered() && noRepeat.end.economy.gold == 9850 && fresh.base.start.economy.towns[0].troops.pools[0] == 49 && noRepeat.purchases.empty()
            && P::count(noRepeat.end.towns[0].garrison,Monster::ARCHER) == 1 ); ++n;
    assert( registry.stage( draft() ).project == 2 && registry.view().context->base.actions[0].id == 2 ); ++n;
    const auto retry = *registry.ticket(2); assert( registry.record( retry, V::Outcome::Failed ).status == V::Status::Held && registry.pending() == 1 && !registry.ready() ); ++n;
    assert( registry.refresh( fresh, missions(fresh) ).status == V::Status::Ready && registry.pending() == 1 && registry.view().context->base.start.economy.gold == 9850 ); ++n;
    assert( registry.record( retry, V::Outcome::Executed ).status == V::Status::Invalid && registry.pending() == 1 ); ++n;
    const auto beforeRefresh = *registry.ticket(2); assert( registry.refresh( fresh, missions(fresh) ).status == V::Status::Ready );
    assert( registry.record( beforeRefresh, V::Outcome::Executed ).status == V::Status::Invalid ); ++n;
    auto invalid = fresh; invalid.base.start.towns.clear();
    assert( registry.refresh( invalid, missions(invalid) ).status == V::Status::Held && !registry.ready() && registry.pending() == 1 && !registry.ticket(2) ); ++n;
    assert( registry.refresh( fresh, missions(fresh) ).status == V::Status::Ready ); ++n;
    auto tomorrow = fresh; tomorrow.base.start.economy.day = 2; tomorrow.base.throughDay = 16;
    assert( registry.refresh( tomorrow, missions(tomorrow) ).status == V::Status::Held && registry.pending() == 1 ); ++n; // No silently skipped overdue action.
    assert( registry.refresh( fresh, missions(fresh) ).status == V::Status::Ready );
    const auto uncertain = *registry.ticket(2);
    assert( registry.record( uncertain, V::Outcome::Uncertain ).status == V::Status::Held && registry.pending() == 1 ); ++n;
    assert( registry.refresh( fresh, missions(fresh) ).reason == "uncertain_execution_requires_reconciliation"
            && !registry.ready() && !registry.ticket(2) && !registry.view().context && registry.pending() == 1 ); ++n;
    // Partial execution removes only the confirmed first step; the remaining program is protected.
    V::Registry chain(1,100); assert( chain.refresh(c,missions(c)).status == V::Status::Ready );
    auto two = draft(); two.pending.push_back( G::recruit(42,1,1,2) ); assert( chain.stage(two).status == V::Status::Ready );
    auto cv = chain.view(); const auto step = cv.context->base.actions[0]; assert( !chain.ticket(2) );
    assert( chain.record(*chain.ticket(step.id),V::Outcome::Executed).status == V::Status::Held );
    auto post = execute(c,step); assert( chain.refresh(post,missions(post)).status == V::Status::Ready );
    cv = chain.view(); assert( cv.context->base.actions.size() == 1 && cv.context->base.actions[0].id == 2 && cv.permissions.empty() && cv.protectedProjects.count(1) ); ++n;
    post.base.start.economy.day = 2; post.base.throughDay = 16;
    assert( chain.refresh(post,missions(post)).status == V::Status::Ready && chain.ticket(2) ); ++n;
    assert( chain.record(*chain.ticket(2),V::Outcome::Executed).status == V::Status::Held && chain.pending() == 0 ); ++n;
    // Dependency completion needs a receipt; only then does it release the dependent ticket.
    V::Registry deps(1,101); deps.refresh(c,missions(c)); auto parent = deps.stage(draft());
    auto dependent = draft(); dependent.dependsOn = {parent.project}; assert( deps.stage(dependent).status == V::Status::Ready );
    assert( deps.ticket(1) && !deps.ticket(2) && deps.view().protectedProjects.count(1) ); ++n;
    const auto parentAction = deps.ticket(1)->action; deps.record(*deps.ticket(1),V::Outcome::Executed);
    auto afterParent = execute(c,parentAction); assert( deps.refresh(afterParent,missions(afterParent)).status == V::Status::Ready && deps.ticket(2) ); ++n;
    // New draft failure leaves the valid old book and existing tickets untouched.
    const auto validTicket = *deps.ticket(2); const auto revision = deps.revision(); auto bad = draft(); bad.pending[0].id = 55;
    assert( deps.stage(bad).status == V::Status::Invalid && deps.revision() == revision && deps.ticket(2)->stamp.revision == validTicket.stamp.revision ); ++n;
    auto hint = draft(); hint.source = C::Source::LegacyHint;
    assert( deps.stage(hint).status == V::Status::Invalid && deps.revision() == revision ); ++n;
    auto far = draft(30); assert( deps.stage(far).status == V::Status::Ready && !deps.ticket(3) && deps.view().beyondHorizon.count(3) ); ++n;
    assert( deps.record(validTicket,V::Outcome::Executed).status == V::Status::Invalid && deps.pending() == 2 ); ++n;
    auto external = c; external.base.actions = {first};
    assert( deps.refresh(external,missions(external)).status == V::Status::Invalid && !deps.view().context ); ++n;
    // A new game/load must use a different registry token even if player/day/IDs/revision coincide.
    V::Registry other(1,102); other.refresh(c,missions(c)); other.stage(draft());
    assert( other.record(*receipt,V::Outcome::Executed).status == V::Status::Invalid && other.pending() == 1 ); ++n;
    V::Registry noOwner(0,0); assert( noOwner.refresh(c,missions(c)).status == V::Status::Invalid ); ++n;
    // A fresh effective INI is used after refresh; registry stores operations, not old fixed prices.
    V::Registry prices(1,103); prices.refresh(c,missions(c)); auto building = draft();
    P::Action tg; tg.kind=P::Kind::Build; tg.town=42; tg.day=1; tg.building=BUILD_TRAINING_GROUNDS; building.pending={tg}; prices.stage(building);
    auto expensive = c; expensive.base.config.trainingGold = 11000;
    assert( prices.refresh(expensive,missions(expensive)).status == V::Status::Ready );
    P::WorkBudget work; const auto replay=P::project(prices.view().context->base,work);
    assert( replay.status == P::Status::Unfunded && expensive.base.start.economy.gold == 10000 ); ++n;
    // Failure never allocates published IDs; the next accepted program is stable.
    auto idBefore = other.view().context->base.actions[0].project; assert( other.stage(bad).status == V::Status::Invalid );
    assert( other.stage(draft()).project == idBefore+1 ); ++n;
    V::Registry rewind(1,104); auto nextDay = c; nextDay.base.start.economy.day = 2; nextDay.base.throughDay = 16;
    assert( rewind.refresh(nextDay,missions(nextDay)).status == V::Status::Ready
            && rewind.refresh(c,missions(c)).reason == "registry_day_rewind_requires_new_instance" && !rewind.ready() ); ++n;
    const auto beforeInvalid = *other.ticket(1); const auto oldRevision = other.revision();
    assert( other.record(beforeInvalid,static_cast<V::Outcome>(99)).status == V::Status::Invalid
            && other.ready() && other.revision() == oldRevision && other.pending() == 2 ); ++n;
    std::cout << "PASS " << n << " registry-lifecycle oracles (stable IDs, receipts, refresh, dependencies, no double payment and effective INI)\n";
}
