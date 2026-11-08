#include "economy_candidates.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>

namespace G = AI::EconomyCandidates;

G::Context context( const size_t towns = 1, const uint32_t day = 1 )
{
    G::Context result;
    result.base = fixture( false, day ); result.base.throughDay = day + 13;
    auto & original = result.base.start.towns[0];
    for ( size_t pool = 0; pool < 6; ++pool ) {
        original.dwellings[pool].basic.quickStrength = ( pool + 1 ) * 10;
        original.dwellings[pool].improved.quickStrength = ( pool + 1 ) * 12;
    }
    auto & state = result.base.start;
    const auto model = state.towns[0]; const auto economic = state.economy.towns[0];
    state.towns.clear(); state.economy.towns.clear();
    for ( size_t i = 0; i < towns; ++i ) {
        state.towns.push_back( model ); state.towns.back().tile = static_cast<int32_t>( 42 + i );
        state.economy.towns.push_back( economic ); state.economy.towns.back().tile = static_cast<int32_t>( 42 + i );
        for ( size_t pool = 1; pool < 4; ++pool ) state.economy.towns.back().troops.pools[pool - 1] = 20;
    }
    return result;
}

G::Result generate( const G::Context & c ) { G::Budget budget; return G::generate( c, budget ); }

int main()
{
    size_t checks = 0;
    auto c = context();
    auto result = generate( c );
    assert( result.valid && !result.projects.empty() && result.projects.size() <= 12 && result.references.size() == 1 ); ++checks;
    const auto repeated = generate( c );
    assert( repeated.projects.size() == result.projects.size() );
    for ( size_t i = 0; i < result.projects.size(); ++i ) assert( G::signature( result.projects[i] ) == G::signature( repeated.projects[i] ) );
    assert( c.base.start.economy.gold == 10000 && c.base.start.economy.towns[0].troops.garrison == 100 ); ++checks;
    assert( result.gaps.count( "financial_and_military_evaluation_pending" ) && result.gaps.count( "army_upgrades_unsupported" ) ); ++checks;
    size_t projected = 0;
    P::WorkBudget work;
    for ( const auto & candidate : result.projects ) {
        P::Input input; assert( G::materialize( c, candidate, input ) );
        if ( P::project( input, work ).covered() ) ++projected;
    }
    assert( projected ); ++checks;

    // Construction chains must keep all prerequisites, even beyond six actions.
    c = context(); auto & town = c.base.start.towns[0];
    town.mask &= ~( BUILD_WEL2 | BUILD_CAPTAIN | DWELLING_MONSTER4 );
    P::Building captain; captain.id = BUILD_CAPTAIN; captain.price[0] = 500; town.catalog.emplace( captain.id, captain );
    for ( uint32_t guild = BUILD_MAGEGUILD1; guild <= BUILD_MAGEGUILD5; guild <<= 1 ) {
        P::Building b; b.id = guild; b.price[0] = 100; b.prerequisites = guild == BUILD_MAGEGUILD1 ? 0 : guild >> 1;
        town.catalog.emplace( guild, b );
    }
    c.base.config.requiredBuildings |= BUILD_MAGEGUILD5;
    P::refresh( c.base.start );
    result = generate( c );
    bool longChain = false, financed = false;
    for ( const auto & candidate : result.projects ) if ( candidate.families & G::bit( G::Family::Training ) ) {
        if ( candidate.actions.size() > 7 ) {
            longChain = true;
            P::Input input; assert( G::materialize( c, candidate, input ) ); P::WorkBudget budget;
            financed = financed || P::project( input, budget ).covered();
            uint32_t prior = 0;
            for ( const auto & a : candidate.actions ) if ( a.kind == P::Kind::Build ) { assert( a.day > prior ); prior = a.day; }
        }
    }
    assert( longChain && financed ); ++checks;
    town.disabled |= BUILD_WEL2;
    result = generate( c );
    assert( std::none_of( result.projects.begin(), result.projects.end(), []( const auto & p ) { return p.families & G::bit( G::Family::Training ); } ) ); ++checks;

    // Urgent quantity requests bypass ordinary local pruning and are ordered by deadline.
    c = context( 70 );
    for ( int32_t tile = 111; tile >= 42; --tile ) c.requests.push_back( { tile, 1, 1, 2, 0, true } );
    result = generate( c );
    assert( result.projects.size() == 64 && result.urgentDropped == 6 && result.gaps.count( "urgent_selection_limit" ) );
    for ( size_t i = 0; i < 64; ++i ) assert( result.projects[i].urgent && result.projects[i].town == static_cast<int32_t>( 42 + i ) );
    ++checks;
    c.requests.push_back( c.requests[0] );
    // This duplicate lies outside the retained 64, and is still an omitted unique request once.
    result = generate( c );
    assert( result.projects.size() == 64 && result.urgentDropped == 6 ); ++checks;

    // Equal cities alternate within each family; day changes rotate the starting town.
    c = context( 20 ); result = generate( c );
    assert( result.projects.size() == 64 && result.gaps.count( "common_selection_reduction" ) ); ++checks;
    std::set<int32_t> firstRecruit;
    int32_t first = -1;
    for ( const auto & p : result.projects ) if ( p.families & G::bit( G::Family::Recruit ) ) {
        if ( first < 0 ) first = p.town;
        firstRecruit.insert( p.town ); if ( firstRecruit.size() == 4 ) break;
    }
    assert( firstRecruit.size() == 4 ); ++checks;
    c.base.start.economy.day++; c.base.throughDay++;
    result = generate( c );
    const auto next = std::find_if( result.projects.begin(), result.projects.end(), []( const auto & p ) { return p.families & G::bit( G::Family::Recruit ); } );
    assert( next != result.projects.end() && next->town != first ); ++checks;
    std::map<int32_t, size_t> localCounts;
    for ( const auto & p : result.projects ) if ( !p.urgent ) ++localCounts[p.town];
    for ( const auto & item : localCounts ) assert( item.second <= 12 );
    ++checks;

    // Shared generation limits retain references and visibly stop optional exploration.
    G::Budget tiny; tiny.proposals = 2;
    result = G::generate( c, tiny );
    assert( result.valid && !tiny.proposals && result.gaps.count( "proposal_limit" ) && !result.references.empty() ); ++checks;
    const auto exhausted = G::generate( c, tiny );
    assert( exhausted.projects.empty() && exhausted.references.size() == 1 ); ++checks;

    // Retained commitments are not paid twice in the active reference.
    c = context();
    auto committed = action( 50, 1, P::Kind::Build, 42, BUILD_WELL );
    c.base.actions = { committed }; c.active = { committed, action( 51, 2, P::Kind::Recruit, 42, 1 ) };
    result = generate( c );
    P::Input input;
    assert( result.valid && result.references.size() == 2 && G::materialize( c, result.references[1], input ) && input.actions.size() == 2 ); ++checks;
    c.active[0].day = 2;
    assert( !generate( c ).valid ); ++checks;

    // Preserve the current cycle and derive first possible success from the loss remainder.
    c = context();
    c.base.config.successRate = 100000;
    auto & state = c.base.start.economy.towns[0].troops.state;
    Conscription::advance( state, 44, c.base.config.quotas ); state.lossRemainder = 900000;
    result = generate( c );
    bool timing = false;
    for ( const auto & p : result.projects ) if ( p.families & G::bit( G::Family::Training ) ) {
        const auto & buy = p.actions.back();
        if ( buy.day == 5 ) { assert( buy.pool == 2 ); timing = true; }
    }
    assert( timing && state.total == 44 && state.lossRemainder == 900000 ); ++checks;
    c.base.config.successRate = 0;
    result = generate( c );
    assert( std::none_of( result.projects.begin(), result.projects.end(), []( const auto & p ) { return p.families & G::bit( G::Family::Training ); } ) ); ++checks;

    // Rare resources constrain the nominal maximum, even with abundant gold.
    c = context();
    c.base.start.towns[0].mask |= DWELLING_MONSTER6;
    c.base.start.towns[0].dwellings[5].basic.quickStrength = 1000000;
    c.base.start.towns[0].dwellings[5].basic.price[6] = 1;
    c.base.start.towns[0].extraPools[1] = 20; c.base.start.economy.materials[5] = 2;
    result = generate( c );
    bool limited = false;
    for ( const auto & p : result.projects ) if ( p.families & G::bit( G::Family::Recruit ) )
        for ( const auto & a : p.actions ) if ( a.pool == 5 && a.day == 1 ) { assert( a.count <= 2 ); limited = limited || a.count == 2; }
    assert( limited ); ++checks;

    c = context(); c.base.start.towns[0].hero = 7;
    c.base.start.towns[0].visitor = { P::Stack{ Monster::PEASANT, 10 } }; c.base.start.economy.towns[0].visitingPeasants = 10;
    result = generate( c );
    bool partial = false;
    for ( const auto & p : result.projects ) for ( const auto & a : p.actions ) if ( a.kind == P::Kind::ToGarrison ) { assert( a.count == 9 ); partial = true; }
    assert( partial ); ++checks;

    c = context( 2 );
    c.base.start.towns[0].mask &= ~BUILD_WEL2;
    c.base.start.towns[1].race = Race::WRLK; c.base.start.towns[1].mask |= DWELLING_MONSTER6;
    c.base.start.towns[1].dwellings[5].basic.id = Monster::GREEN_DRAGON;
    c.base.start.towns[1].dwellings[5].basic.quickStrength = 1000000;
    c.base.start.towns[1].extraPools[1] = 10;
    result = generate( c );
    bool crossFaction = false;
    for ( const auto & p : result.projects ) if ( p.town == 42 && ( p.families & G::bit( G::Family::Fiscal ) ) ) {
        if ( p.actions.back().town == 43 ) {
            P::Input fiscalPlan; assert( G::materialize( c, p, fiscalPlan ) ); P::WorkBudget fiscalWork;
            crossFaction = crossFaction || P::project( fiscalPlan, fiscalWork ).covered();
        }
    }
    assert( crossFaction ); ++checks;

    c = context(); c.base.start.towns[0].mask &= ~BUILD_WEL2; c.base.start.towns[0].mask |= BUILD_TRAINING_GROUNDS;
    result = generate( c );
    assert( std::any_of( result.projects.begin(), result.projects.end(), []( const auto & p ) {
        return p.milestone && p.actions.back().day == 2 && p.actions.back().pool == 1;
    } ) ); ++checks;
    c = context(); c.base.throughDay = c.base.start.economy.day;
    result = generate( c );
    assert( result.longerHorizonNeeded && result.gaps.count( "effect_or_action_beyond_horizon" ) ); ++checks;

    c = context(); c.base.start.towns[0].mask &= ~BUILD_WEL2;
    c.base.start.towns[0].catalog.at( BUILD_WEL2 ).prerequisites = BUILD_WEL2;
    assert( generate( c ).gaps.count( "cyclic_or_invalid_prerequisite" ) ); ++checks;
    c = context(); c.base.start.economy.towns[0].troops.state.next = 3;
    assert( !generate( c ).valid ); ++checks;
    for ( const size_t towns : { size_t{ 0 }, size_t{ 1 }, size_t{ 5 }, size_t{ 17 } } ) for ( const uint32_t day : { 1U, 2U, 7U, 8U } ) {
        c = context( towns, day ); result = generate( c );
        assert( result.valid && result.projects.size() <= 64 );
        std::map<int32_t, size_t> counts;
        std::set<std::string> signatures;
        for ( const auto & p : result.projects ) { assert( signatures.insert( G::signature( p ) ).second ); ++counts[p.town]; }
        for ( const auto & item : counts ) assert( item.second <= 12 );
        ++checks;
    }
    std::cout << "PASS " << checks << " bounded-generation cases; long funded chain and candidate replay included\n";
}
