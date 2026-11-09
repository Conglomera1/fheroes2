#include "economy_commitments.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
namespace C = AI::EconomyCommitments;
namespace G = AI::EconomyCandidates;
namespace X = AI::EconomyMissions;
namespace Q = AI::EconomyRecourse;
namespace M = AI::EconomyMilitary;
G::Context context()
{
    G::Context c; c.base = fixture( false, 1 ); c.base.throughDay = 15;
    c.base.start.economy.nonTaxIncome = c.base.start.economy.taxRate = 0;
    c.base.start.economy.gold = 150; c.base.start.economy.towns[0].troops.pools[0] = 50;
    return c;
}
C::Program program( uint64_t id = 10, uint64_t actionId = 7 )
{
    C::Program p; p.id = id; p.town = 42;
    auto a = G::recruit( 42, 1, 2, 2 ); a.id = actionId; a.project = id; a.required = true; p.pending = { a }; return p;
}
Q::Result prepare( const C::Result & imported, M::Budget & budget )
{
    assert( imported.context ); const auto & c = *imported.context;
    return Q::prepare( c, imported.permissions, { { 1, 42, M::Slot::TownArmy, 1, 15, true, true } },
        []( const auto & t, auto ) { return M::Value{ true, 100.0 * P::count( t.garrison, Monster::ARCHER ), {} }; }, budget,
        { {}, true }, []( const auto &, const auto & ) { return AI::EconomyDefense::Value{ true, 10000, {} }; }, { {}, true, {} } );
}
int main()
{
    size_t n = 0; const auto c = context(); const C::Stamp stamp{ 1, 1, 17 };
    const auto missions = X::classify( c.base.start, {}, {}, {} );
    auto p = program(); C::Book book{ stamp, { p } };
    auto run = [&]( const C::Book & b ) { return C::import( c, missions, stamp, b ); };
    auto out = run( book );
    assert( out.status == C::Status::Ready && out.context && out.context->base.actions.size() == 1 && out.permissions.empty()
            && out.protectedProjects.count( 10 ) && !out.fundingVerified && !out.actionable ); ++n;
    assert( out.context->base.start.economy.gold == 150 && c.base.actions.empty() && C::same( out.context->base.actions[0], p.pending[0] ) ); ++n;
    M::Budget denied; auto prepared = prepare( out, denied );
    assert( prepared.status == Q::Status::Blocked && !prepared.session && prepared.released.empty() && denied.projections == 255 ); ++n;
    p.allowAbandon = p.independent = true; book.programs = { p }; out = run( book );
    assert( out.permissions.size() == 1 && out.permissions[0].project == 10 && C::same( out.permissions[0].actions[0], p.pending[0] ) ); ++n;
    M::Budget allowed; prepared = prepare( out, allowed );
    assert( prepared.status == Q::Status::Ready && prepared.session && prepared.released.size() == 1 && allowed.projections == 253
            && prepared.session->context()->base.start.economy.gold == 150 && out.context->base.actions.size() == 1 && !prepared.actionable ); ++n;
    auto stale = book; ++stale.stamp.revision; auto invalid = run( stale );
    assert( !invalid.context && invalid.permissions.empty() && invalid.status == C::Status::Invalid ); ++n;
    stale = book; ++stale.stamp.player; assert( !run( stale ).context ); ++n;
    stale = book; ++stale.stamp.day; assert( !run( stale ).context ); ++n;
    auto oldMissions = missions; oldMissions.day = 2; assert( !C::import( c, oldMissions, stamp, book ).context ); ++n;
    auto hint = book; hint.programs[0].source = C::Source::LegacyHint;
    assert( run( hint ).reason == "legacy_hint_is_not_commitment_authority" ); ++n;
    auto retained = c; retained.base.actions = p.pending;
    out = C::import( retained, missions, stamp, book ); assert( out.context && out.context->base.actions.size() == 1 && out.permissions.size() == 1 ); ++n;
    auto changed = book; changed.programs[0].pending[0].count = 3;
    assert( !C::import( retained, missions, stamp, changed ).context ); ++n;
    auto partial = retained; auto extra = p.pending[0]; extra.id = 8; extra.day = 3; partial.base.actions.push_back( extra );
    assert( C::import( partial, missions, stamp, book ).reason == "partial_or_extended_existing_program" ); ++n;
    auto extended = book; extended.programs[0].pending.push_back( extra );
    assert( C::import( retained, missions, stamp, extended ).reason == "partial_or_extended_existing_program" ); ++n;
    auto duplicate = book; duplicate.programs.push_back( p ); assert( !run( duplicate ).context ); ++n;
    duplicate = book; duplicate.programs[0].pending.push_back( p.pending[0] ); assert( !run( duplicate ).context ); ++n;
    duplicate = book; duplicate.programs.push_back( program( 20, 7 ) ); invalid = run( duplicate );
    assert( !invalid.context && invalid.permissions.empty() && invalid.reason == "duplicate_new_action_identity" ); ++n;
    auto past = book; past.programs[0].pending[0].day = 0; assert( !run( past ).context ); ++n;
    auto unordered = extended; unordered.programs[0].pending[1].day = 1; assert( !run( unordered ).context ); ++n;
    auto wrongTown = book; wrongTown.programs[0].pending[0].town = 70; assert( !run( wrongTown ).context ); ++n;
    auto badShape = book; badShape.programs[0].pending[0].count = 0; assert( !run( badShape ).context ); ++n;
    badShape = book; badShape.programs[0].pending[0].pool = 6; assert( !run( badShape ).context ); ++n;
    badShape = book; badShape.programs[0].pending[0].id = uint64_t{1} << 63; assert( !run( badShape ).context ); ++n;
    auto remote = book; remote.programs[0].destination = C::Destination::RemoteHero; remote.programs[0].hero = 5;
    assert( run( remote ).status == C::Status::Unsupported && !run( remote ).context ); ++n;
    auto guest = book; guest.programs[0].destination = C::Destination::StationaryVisitor; guest.programs[0].hero = 5;
    assert( run( guest ).reason == "beneficiary_local_assignment_unverified" ); ++n;
    auto visitorContext = c; visitorContext.base.start.towns[0].hero = 5;
    X::Hero hero; hero.id = 5; hero.tile = hero.town = 42; hero.role = X::Role::Fighter; hero.patrol = true;
    const auto visitorMissions = X::classify( visitorContext.base.start, {hero}, {}, {} );
    assert( C::import( visitorContext, visitorMissions, stamp, guest ).reason == "exact_visitor_delivery_not_implemented" ); ++n;
    auto transfer = book; transfer.programs[0].pending[0].kind = P::Kind::ToVisitor;
    assert( run( transfer ).status == C::Status::Unsupported ); ++n;
    auto prefix = book; prefix.programs[0].executedPrefix = true; out = run( prefix );
    assert( out.context && out.permissions.empty() && out.protectedProjects.count(10) && out.context->base.start.economy.gold == 150 ); ++n;
    auto dependent = program( 20, 8 ); dependent.allowAbandon = dependent.independent = true; dependent.dependsOn = {10}; dependent.pending[0].day = 3;
    C::Book dependencies{ stamp, {dependent, p} }; out = run( dependencies );
    assert( out.context && out.permissions.size() == 1 && out.permissions[0].project == 20 && out.protectedProjects.count(10)
            && out.context->base.actions[0].project == 10 ); ++n;
    auto cycle = dependencies; cycle.programs[1].dependsOn = {20}; assert( run( cycle ).reason == "cyclic_commitment_dependencies" ); ++n;
    auto missing = dependencies; missing.programs[0].dependsOn = {30}; assert( !run( missing ).context ); ++n;
    auto late = dependencies; late.programs[0].pending[0].day = 1;
    assert( run( late ).reason == "dependency_schedule_requires_rescheduling" ); ++n;
    // A represented same-day prerequisite is accepted only if its already retained order is correct.
    auto sameDay = dependencies; sameDay.programs[0].pending[0].day = 2;
    auto reversed = c; reversed.base.actions = { sameDay.programs[0].pending[0], p.pending[0] };
    assert( C::import( reversed, missions, stamp, sameDay ).status == C::Status::Unsupported ); ++n;
    auto uncovered = c; auto other = program( 30, 9 ).pending[0]; uncovered.base.actions = {other};
    out = C::import( uncovered, missions, stamp, book );
    assert( out.context && out.context->base.actions.size() == 2 && C::same( out.context->base.actions[0], other )
            && out.permissions.empty() && out.protectedProjects.count(10) && out.protectedProjects.count(30) ); ++n;
    auto far = book; far.programs[0].pending[0].day = 30; out = run( far );
    assert( out.context && out.beyondHorizon.count(10) && out.context->base.actions[0].day == 30 && !out.fundingVerified ); ++n;
    auto unaffordable = run( book ); P::WorkBudget work; auto financial = P::project( unaffordable.context->base, work );
    assert( financial.status == P::Status::Unfunded && financial.failedAction == 7 ); ++n;
    // Import never caches prices. The exact projection uses the supplied effective config.
    auto tg = program(); tg.pending = { action( 7, 1, P::Kind::Build, 42, BUILD_TRAINING_GROUNDS ) }; tg.pending[0].project = 10;
    C::Book buildBook{ stamp, {tg} }; auto funded = c; funded.base.start.economy.gold = 3000;
    funded.base.config.trainingGold = 2500; auto imported = C::import( funded, missions, stamp, buildBook ); P::WorkBudget b1;
    auto first = P::project( imported.context->base, b1 ); assert( first.covered() && first.days.front().afterActions[0] == 500 );
    funded.base.config.trainingGold = 3100; auto newStamp = stamp; ++newStamp.revision; auto freshBook = buildBook; freshBook.stamp = newStamp;
    imported = C::import( funded, missions, newStamp, freshBook ); P::WorkBudget b2;
    assert( P::project( imported.context->base, b2 ).status == P::Status::Unfunded ); ++n;
    assert( !C::import( funded, missions, newStamp, buildBook ).context ); ++n;
    auto many = book; many.programs.resize(65,p); assert( run( many ).status == C::Status::Limit ); ++n;
    auto unchanged = run( C::Book{stamp,{}} ); assert( unchanged.context && unchanged.context->base.actions.empty() && unchanged.permissions.empty() ); ++n;
    auto notIndependent = book; notIndependent.programs[0].independent = false; out = run( notIndependent );
    assert( out.context && out.permissions.empty() && out.protectedProjects.count(10) ); ++n;
    auto notAuthorized = book; notAuthorized.programs[0].allowAbandon = false; out = run( notAuthorized );
    assert( out.context && out.permissions.empty() && out.protectedProjects.count(10) ); ++n;
    // An optional continuation cannot alias a newly retained action under another project.
    auto conflict = c; conflict.active = p.pending; conflict.active[0].project = 99;
    invalid = C::import( conflict, missions, stamp, book );
    assert( !invalid.context && invalid.permissions.empty() && invalid.reason == "conflicting_active_commitment_identity" ); ++n;
    conflict.active = p.pending; out = C::import( conflict, missions, stamp, book );
    assert( out.context && out.context->active.size() == 1 && C::same( out.context->active[0], p.pending[0] ) ); ++n;
    std::cout << "PASS " << n << " dated-commitment oracles (identity, snapshot, beneficiaries, dependencies, dynamic cost and explicit recourse)\n";
}
