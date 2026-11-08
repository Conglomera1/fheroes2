#include "economy_inputs.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
namespace I = AI::EconomyInputs;
namespace M = AI::EconomyMilitary;
namespace G = AI::EconomyCandidates;
G::Context context()
{ G::Context c; c.base = fixture( false, 1 ); c.base.throughDay = 15; return c; }
I::Enemy enemy( int tile = 9 )
{ I::Enemy e; e.observation.enemyTile = tile; e.observation.enemyHero = 2; e.observation.enemyStrength = 100; e.observation.known = true; e.mobile = true; e.movePoints = 1000; return e; }
int main()
{
    size_t n = 0; const auto c = context(); I::Work w;
    auto f = I::assemble( c, { enemy() }, { 42 }, true, []( const auto &, auto ) { return 1000U; }, w );
    assert( f.valid && f.defenses.coverageComplete && f.defenses.threats[0].lastSafeDay == 1 && w.paths == 127 ); ++n;
    assert( f.assignments.size() == 1 && f.assignments[0].slot == M::Slot::TownArmy && f.assignments[0].town == 42 && !f.actionable ); ++n;
    assert( f.risks.scenarios.size() == 1 && f.risks.scenarios[0].losses[0].firstUnavailableDay == 2 ); ++n;
    assert( c.base.start.economy.gold == f.context.base.start.economy.gold ); ++n;
    I::Work w2; f = I::assemble( c, { enemy() }, { 42 }, true, []( const auto &, auto ) { return 1001U; }, w2 );
    assert( f.defenses.threats[0].lastSafeDay == 2 && f.defenses.threats[0].enemyStrength == 100 ); ++n;
    I::Work none; f = I::assemble( c, {}, { 42 }, true, {}, none );
    assert( f.valid && f.defenses.coverageComplete && f.defenses.threats.empty() && none.paths == 128 ); ++n;
    I::Work missing; f = I::assemble( c, { enemy() }, { 42 }, true, {}, missing );
    assert( f.valid && !f.defenses.coverageComplete && !f.defenses.threats[0].lastSafeDay ); ++n;
    I::Work unknown; f = I::assemble( c, { enemy() }, { 42 }, true, []( const auto &, auto ) { return 0U; }, unknown );
    assert( !f.defenses.coverageComplete && !f.defenses.threats[0].reachable && f.risks.scenarios.empty() ); ++n;
    auto stationary = enemy(); stationary.mobile = false; I::Work fixed;
    f = I::assemble( c, { stationary }, { 42 }, true, {}, fixed );
    assert( !f.defenses.coverageComplete && fixed.paths == 128 && f.gaps.count( "enemy_recruitment_threat_unverified" ) ); ++n;
    auto slow = enemy(); slow.movePoints = 0; I::Work zero;
    f = I::assemble( c, { slow }, { 42 }, true, {}, zero ); assert( !f.defenses.coverageComplete && zero.paths == 128 ); ++n;
    auto hidden = enemy(); hidden.observation.known = false; I::Work fog;
    f = I::assemble( c, { hidden }, { 42 }, true, []( const auto &, auto ) { assert( false ); return 1U; }, fog );
    assert( !f.defenses.coverageComplete && fog.paths == 128 && f.defenses.threats.empty() ); ++n;
    I::Work incomplete; f = I::assemble( c, {}, { 42 }, false, {}, incomplete ); assert( !f.defenses.coverageComplete && !f.risks.coverageComplete ); ++n;
    I::Work exhausted; exhausted.paths = 0; f = I::assemble( c, { enemy() }, { 42 }, true, []( const auto &, auto ) { assert( false ); return 1U; }, exhausted );
    assert( f.valid && !f.defenses.coverageComplete && exhausted.paths == 0 ); ++n;
    I::Work distant; f = I::assemble( c, { enemy() }, { 42 }, true, []( const auto &, auto ) { return 16000U; }, distant );
    assert( f.defenses.coverageComplete && f.defenses.threats.empty() && f.outsideHorizon == 1 ); ++n;
    I::Work boundary; f = I::assemble( c, { enemy() }, { 42 }, true, []( const auto &, auto ) { return 15000U; }, boundary );
    assert( f.defenses.threats[0].lastSafeDay == 15 && f.risks.scenarios.empty() ); ++n;
    auto two = c; auto t = two.base.start.towns[0]; t.tile = 70; two.base.start.towns.push_back( t );
    auto e = two.base.start.economy.towns[0]; e.tile = 70; two.base.start.economy.towns.push_back( e );
    I::Work ranked; f = I::assemble( two, { enemy( 10 ), enemy( 9 ) }, { 70, 42 }, true, []( const auto &, auto ) { return 1U; }, ranked );
    assert( f.valid && f.defenses.threats.size() == 4 && f.assignments.size() == 2 && ranked.paths == 124 ); ++n;
    assert( f.defenses.threats[0].enemyTile == 9 && f.defenses.threats[0].priority == 1 && f.defenses.threats[1].priority == 0 ); ++n;
    I::Work duplicate; f = I::assemble( c, { enemy(), enemy() }, { 42 }, true, {}, duplicate ); assert( !f.valid && duplicate.paths == 128 ); ++n;
    I::Work dupTown; f = I::assemble( c, {}, { 42, 42 }, true, {}, dupTown ); assert( !f.valid ); ++n;
    I::Work otherTown; f = I::assemble( c, {}, { 70 }, true, {}, otherTown ); assert( !f.valid ); ++n;
    auto invalid = c; invalid.base.throughDay = 14; I::Work badH;
    f = I::assemble( invalid, {}, { 42 }, true, {}, badH ); assert( !f.valid ); ++n;
    auto extreme = enemy(); extreme.observation.enemyStrength = std::numeric_limits<double>::infinity(); I::Work inf;
    f = I::assemble( c, { extreme }, { 42 }, true, {}, inf ); assert( f.valid && !f.defenses.coverageComplete && inf.paths == 128 ); ++n;
    std::vector<I::Enemy> many; for ( int i = 0; i < 130; ++i ) many.push_back( enemy( 100 + i ) ); I::Work shared;
    f = I::assemble( c, many, { 42 }, true, []( const auto &, auto ) { return 1U; }, shared );
    assert( f.queried == 128 && shared.paths == 0 && !f.defenses.coverageComplete && f.defenses.threats.size() == 130 ); ++n;
    auto again = I::assemble( c, { enemy() }, { 42 }, true, {}, shared ); assert( again.queried == 0 && !again.defenses.coverageComplete ); ++n;
    auto nearMax = c; nearMax.base.start.economy.day = UINT32_MAX - 14; nearMax.base.throughDay = UINT32_MAX;
    I::Work overflow; f = I::assemble( nearMax, { enemy() }, { 42 }, true, []( const auto &, auto ) { return UINT32_MAX; }, overflow );
    assert( f.valid && f.outsideHorizon == 1 && f.defenses.threats.empty() ); ++n;
    auto visitor = c; visitor.base.start.towns[0].hero = 5; I::Work uncommitted;
    f = I::assemble( visitor, {}, { 42 }, true, {}, uncommitted );
    assert( f.valid && !f.assignments[0].verified && !f.defenses.coverageComplete && f.gaps.count( "visitor_assignment_not_committed" ) ); ++n;
    std::cout << "PASS " << n << " planner-input oracles (knowledge, deadlines, risk, local use and shared paths)\n";
}
