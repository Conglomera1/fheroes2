#include "economy_military.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>

namespace M = AI::EconomyMilitary;
namespace G = AI::EconomyCandidates;

G::Context context()
{
    G::Context c; c.base = fixture( false, 1 ); c.base.throughDay = 15;
    c.base.start.economy.towns[0].troops.pools[0] = 20;
    return c;
}
G::Candidate candidate( const uint32_t day )
{
    G::Candidate c; c.id = 1;
    auto a = action( 0, day, P::Kind::Recruit, 42, 1 ); a.project = 0; a.pool = 1; c.actions = { a };
    return c;
}
M::Value archerForce( const P::Town & town, M::Slot )
{ return { true, static_cast<double>( P::count( town.garrison, Monster::ARCHER ) ) * 100, {} }; }
std::vector<M::Assignment> assignments() { return { { 1, 42, M::Slot::TownArmy, 1, 15, true, true } }; }

int main()
{
    size_t checks = 0;
    auto c = context(); M::Budget budget;
    M::Session session( c, assignments(), archerForce, budget );
    auto result = session.evaluate( candidate( 2 ) );
    assert( result.status == M::Status::Scored && result.utility && *result.utility == 100 && result.financial.covered() ); ++checks;
    result = session.evaluate( candidate( 9 ) );
    assert( result.utility && *result.utility == 50 ); ++checks;
    result = session.evaluate( candidate( 1 ) );
    assert( result.utility && *result.utility == 100 && result.days.size() == 15 ); ++checks;
    assert( budget.projections == 252 && budget.mechanics.days == 24576 - 4 * 14 ); ++checks;

    auto delayed = assignments(); delayed[0].firstDay = 9;
    M::Budget delayedBudget; M::Session delayedSession( c, delayed, archerForce, delayedBudget );
    assert( *delayedSession.evaluate( candidate( 2 ) ).utility == 50 && *delayedSession.evaluate( candidate( 9 ) ).utility == 50 ); ++checks;
    auto unused = assignments(); unused[0].useful = false;
    M::Budget unusedBudget; M::Session unusedSession( c, unused, archerForce, unusedBudget );
    assert( *unusedSession.evaluate( candidate( 2 ) ).utility == 0 && unusedBudget.forces == 131072 ); ++checks;
    M::Budget unknownBudget; M::Session unknown( c, {}, archerForce, unknownBudget );
    result = unknown.evaluate( candidate( 2 ) );
    assert( result.financial.covered() && !result.utility && result.gaps.count( "town_usefulness_not_registered" ) ); ++checks;
    auto unverified = assignments(); unverified[0].verified = false;
    M::Budget unverifiedBudget; M::Session uncertain( c, unverified, archerForce, unverifiedBudget );
    assert( !uncertain.evaluate( candidate( 2 ) ).utility ); ++checks;
    auto overlap = assignments(); overlap.push_back( { 2, 42, M::Slot::TownArmy, 8, 15, true, true } );
    M::Budget overlapBudget; M::Session doubled( c, overlap, archerForce, overlapBudget );
    assert( doubled.evaluate( candidate( 2 ) ).status == M::Status::Invalid && overlapBudget.projections == 256 ); ++checks;

    G::Candidate taxOnly; taxOnly.id = 2;
    auto peasant = action( 0, 1, P::Kind::Recruit, 42, 1 ); peasant.project = 0; taxOnly.actions = { peasant };
    assert( *session.evaluate( taxOnly ).utility == 0 ); ++checks;
    G::Candidate training; training.id = 3;
    auto build = action( 0, 1, P::Kind::Build, 42, BUILD_TRAINING_GROUNDS ); build.project = 0; training.actions = { build };
    assert( *session.evaluate( training ).utility == 0 ); ++checks;
    M::Budget peasantBudget;
    M::Session weakTroops( c, assignments(), []( const P::Town & town, M::Slot ) { return M::Value{ true, static_cast<double>( P::count( town.garrison ) ), {} }; }, peasantBudget );
    assert( *weakTroops.evaluate( training ).utility < 0 ); ++checks;

    auto impossible = candidate( 1 ); impossible.actions[0].count = 1000000;
    assert( !session.evaluate( impossible ).utility ); ++checks;
    c = context(); c.base.start.economy.gold = 0; c.base.start.economy.nonTaxIncome = 0;
    M::Budget poorBudget; M::Session poor( c, assignments(), archerForce, poorBudget );
    result = poor.evaluate( candidate( 1 ) );
    assert( result.status == M::Status::Unfunded && !result.utility ); ++checks;
    assert( !session.evaluate( candidate( 16 ) ).utility ); ++checks;

    c = context(); M::Budget one; one.projections = 1;
    M::Session limited( c, assignments(), archerForce, one );
    assert( limited.evaluate( candidate( 2 ) ).status == M::Status::Limit ); ++checks;
    M::Budget noForces; noForces.forces = 0;
    M::Session forceLimited( c, assignments(), archerForce, noForces );
    assert( forceLimited.evaluate( candidate( 2 ) ).status == M::Status::Limit ); ++checks;
    M::Budget invalidForceBudget;
    M::Session invalidForce( c, assignments(), []( const P::Town &, M::Slot ) { return M::Value{ true, std::numeric_limits<double>::quiet_NaN(), {} }; }, invalidForceBudget );
    assert( !invalidForce.evaluate( candidate( 2 ) ).utility ); ++checks;

    // A common 28-day horizon uses exactly 28 future samples, not a second delay discount.
    auto longContext = context(); longContext.base.throughDay = 29;
    auto longAssignments = assignments(); longAssignments[0].lastDay = 29;
    M::Budget longBudget; M::Session longSession( longContext, longAssignments, archerForce, longBudget );
    assert( *longSession.evaluate( candidate( 16 ) ).utility == 50 ); ++checks;
    // Retained actions are present in M0 and cannot earn a second benefit when named again.
    auto committed = context();
    auto retained = candidate( 2 ); retained.actions[0].id = retained.actions[0].project = 7;
    committed.base.actions = retained.actions;
    M::Budget committedBudget; M::Session retainedSession( committed, assignments(), archerForce, committedBudget );
    assert( *retainedSession.evaluate( retained ).utility == 0 ); ++checks;
    auto shortContext = context(); shortContext.base.throughDay = 8;
    M::Budget shortBudget; M::Session shortSession( shortContext, assignments(), archerForce, shortBudget );
    assert( shortSession.evaluate( candidate( 2 ) ).status == M::Status::Invalid && shortBudget.projections == 256 ); ++checks;

    M::Budget frozenBudget; M::Session frozen( c, assignments(), archerForce, frozenBudget );
    c.base.start.economy.gold = 0;
    assert( frozen.evaluate( candidate( 1 ) ).financial.covered() ); ++checks;
    std::cout << "PASS " << checks << " military utility oracles (timing, no double credit, funding, assignments and shared limits)\n";
}
