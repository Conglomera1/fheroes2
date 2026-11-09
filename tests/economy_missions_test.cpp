#include "economy_inputs.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
namespace X = AI::EconomyMissions;
namespace I = AI::EconomyInputs;
namespace G = AI::EconomyCandidates;
int main()
{
    auto input = fixture( false, 1 ); input.throughDay = 15;
    auto state = input.start; state.towns[0].hero = 5;
    X::Hero visitor; visitor.id = 5; visitor.tile = visitor.town = 42; visitor.role = X::Role::Fighter;
    size_t n = 0;
    auto fixed = visitor; fixed.patrol = true; fixed.patrolRadius = 0;
    auto s = X::classify( state, { fixed }, {}, {} );
    assert( s.valid && s.gaps.empty() && s.stationaryVisitors == std::set<int32_t>{5} && s.heroes[0].use == X::Use::FixedLocal && !s.actionable ); ++n;
    s = X::classify( state, { visitor }, {}, {} ); assert( s.valid && s.stationaryVisitors.empty() && s.gaps.count( "visitor_assignment_not_committed" ) ); ++n;
    auto sleeping = visitor; sleeping.sleeping = true; sleeping.movement = 0;
    s = X::classify( state, { sleeping }, {}, {} ); assert( s.valid && s.gaps.count( "visitor_assignment_not_committed" ) ); ++n;
    auto courier = visitor; courier.role = X::Role::Courier; courier.pathTarget = 99;
    s = X::classify( state, { courier }, {}, {} ); assert( s.valid && s.gaps.count( "visitor_assignment_not_committed" ) && s.limitations.count( "existing_path_is_not_delivery_commitment" ) ); ++n;
    auto patrol = fixed; patrol.patrolRadius = 2;
    s = X::classify( state, { patrol }, {}, {} ); assert( s.valid && s.stationaryVisitors.empty() && s.heroes[0].use == X::Use::MobilePatrol ); ++n;
    auto empty = state; empty.towns[0].hero = -1;
    auto field = fixed; field.town = -1; field.tile = 90;
    s = X::classify( empty, { field }, {}, {} ); assert( s.valid && s.gaps.empty() && s.stationaryVisitors.empty() && s.heroes[0].use == X::Use::MobilePatrol ); ++n;
    for ( const auto role : { X::Role::Scout, X::Role::Courier, X::Role::Hunter, X::Role::Fighter, X::Role::Champion } ) {
        field.role = role; field.patrol = false;
        s = X::classify( empty, { field }, {}, {} ); assert( s.valid && s.gaps.empty() && s.heroes[0].role == role && s.heroes[0].use == X::Use::MobileRole );
    } ++n;
    s = X::classify( state, { fixed }, { { 42, X::Intent::Defend, { 99 } }, { 99, X::Intent::Attack, { 42 } }, { 72, X::Intent::Reinforce, {} } },
                     { { 6, 3, false, true }, { 0, 2300, true, false } } );
    assert( s.valid && s.gaps.empty() && s.targets.size() == 3 && s.targets[0].tile == 42 && s.needs[0].resource == 0 && s.needs[0].missing == 2300
            && s.limitations.count( "resource_hints_are_not_reserved_funds" ) && s.limitations.count( "priority_targets_not_allocated_to_heroes" ) ); ++n;
    assert( !X::classify( state, { fixed, fixed }, {}, {} ).valid ); ++n;
    auto wrong = fixed; wrong.town = -1;
    assert( !X::classify( state, { wrong }, {}, {} ).valid ); ++n;
    wrong = fixed; wrong.tile = 50;
    assert( !X::classify( state, { wrong }, {}, {} ).valid ); ++n;
    assert( !X::classify( state, {}, {}, {} ).valid ); ++n;
    wrong = fixed; wrong.role = X::Role::Unknown;
    assert( !X::classify( state, { wrong }, {}, {} ).valid ); ++n;
    assert( !X::classify( state, { fixed }, { { 42, X::Intent::Unknown, {} } }, {} ).valid ); ++n;
    assert( !X::classify( state, { fixed }, { { 42, X::Intent::Attack, {} }, { 42, X::Intent::Defend, {} } }, {} ).valid ); ++n;
    assert( !X::classify( state, { fixed }, { { 42, X::Intent::Attack, { -1 } } }, {} ).valid ); ++n;
    assert( !X::classify( state, { fixed }, {}, { { 0, -1, true, false } } ).valid ); ++n;
    assert( !X::classify( state, { fixed }, {}, { { 7, 1, true, false } } ).valid ); ++n;
    assert( !X::classify( state, { fixed }, {}, { { 0, 1, true, false }, { 0, 2, false, false } } ).valid ); ++n;
    auto zero = state; zero.economy.day = 0;
    assert( !X::classify( zero, { fixed }, {}, {} ).valid ); ++n;
    assert( !X::classify( state, std::vector<X::Hero>( 257, fixed ), {}, {} ).valid ); ++n;
    std::vector<X::Target> links; X::Target target{ 42, X::Intent::Attack, {} };
    for ( int i = 0; i < 2049; ++i ) target.related.insert( i );
    links.push_back( target );
    assert( !X::classify( state, { fixed }, links, {} ).valid ); ++n;
    // A target to defend a town does not allocate its mobile visitor to permanent defense.
    s = X::classify( state, { visitor }, { { 42, X::Intent::Defend, { 99 } } }, {} );
    assert( s.valid && s.stationaryVisitors.empty() && s.gaps.count( "visitor_assignment_not_committed" ) ); ++n;
    // Widening retains the exact classification and leaves paths, money and retained actions untouched.
    G::Context context; context.base = input; context.base.start = state;
    I::Work work; auto frame = I::assemble( context, {}, { 42 }, true, {}, work, {5} );
    frame.missions = X::classify( state, { fixed }, { { 42, X::Intent::Reinforce, {} } }, { { 0, 2300, true, false } } );
    const auto wide = I::widen( frame, work );
    assert( wide.valid && wide.missions && wide.missions->day == 1 && wide.missions->needs[0].missing == 2300
            && wide.missions->targets[0].intent == X::Intent::Reinforce && wide.missions->heroes[0].use == X::Use::FixedLocal && work.paths == 128
            && wide.context.base.actions.empty() && wide.context.base.start.economy.gold == state.economy.gold ); ++n;
    auto second = field; second.id = 2; second.tile = 100;
    s = X::classify( empty, { field, second }, {}, {} ); assert( s.valid && s.heroes[0].id == 2 ); ++n;
    std::cout << "PASS " << n << " mission-evidence oracles (roles, patrol, intent, budget hints, validation and frozen widening)\n";
}
