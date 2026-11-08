#include "economy_military.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>

namespace D = AI::EconomyDefense;
namespace M = AI::EconomyMilitary;
namespace G = AI::EconomyCandidates;

G::Context context()
{
    G::Context c; c.base = fixture( false, 1 ); c.base.throughDay = 15;
    c.base.start.economy.towns[0].troops.pools[0] = 20;
    return c;
}
D::Threat threat( const uint64_t id = 1, const int town = 42, const uint32_t day = 2, const double enemy = 100 )
{ D::Threat t; t.id = id; t.town = town; t.enemyTile = 5; t.priority = 0; t.lastSafeDay = day; t.enemyStrength = enemy; t.known = t.reachable = true; return t; }
std::vector<M::Assignment> assignments() { return { { 1, 42, M::Slot::TownArmy, 1, 15, true, true } }; }
M::Value force( const P::Town & t, M::Slot ) { return { true, 100 + 10.0 * P::count( t.garrison, Monster::ARCHER ), {} }; }
D::Value defense( const P::Town & t, const D::Threat & ) { const auto value = force( t, M::Slot::TownArmy ); return { value.complete, value.force, {} }; }
G::Candidate recruit( const uint32_t day, const uint32_t count = 1 )
{ G::Candidate c; c.id = 1; c.actions = { G::recruit( 42, 1, count, day ) }; return c; }

int main()
{
    size_t checks = 0;
    auto c = context(); D::Registry registry{ { threat() }, true };
    M::Budget budget; M::Session session( c, assignments(), force, budget, registry, defense );
    const auto early = session.evaluate( recruit( 2 ) );
    const auto late = session.evaluate( recruit( 3, 5 ) );
    assert( early.defense.complete() && early.defense.cities[0].viable == true ); ++checks;
    assert( early.baselineDefense.cities[0].viable == false && early.baselineDefense.checks[0].ratio.value() == 1 ); ++checks;
    assert( late.defense.cities[0].viable == false && late.utility.value() > early.utility.value() ); ++checks;
    assert( M::compareDefense( early, late ) == D::Order::Better && M::compareDefense( late, early ) == D::Order::Worse ); ++checks;
    assert( budget.projections == 253 && budget.mechanics.days == 24576 - 42 ); ++checks;
    assert( budget.forces == 131072 - 45 - 3 ); ++checks; // Three day trajectories + exactly one defense per projection, cached baseline.
    auto zero = session.evaluate( recruit( 3 ) );
    auto coverage = D::coverage( { &late.defense, &zero.defense } );
    assert( coverage.compared == 2 && coverage.unresolved == std::vector<int32_t>{ 42 } ); ++checks;
    coverage = D::coverage( { &early.defense, &late.defense } );
    assert( coverage.unresolved.empty() ); ++checks;
    // High ratios below threshold do not win a viable-defense bit.
    auto subthreshold = registry; subthreshold.threats[0].enemyStrength = 101;
    M::Budget subBudget; M::Session sub( c, assignments(), force, subBudget, subthreshold, defense );
    const auto improvement = sub.evaluate( recruit( 2 ) );
    const auto noHelp = sub.evaluate( recruit( 3 ) );
    assert( D::comparePriority( improvement.defense, noHelp.defense ) == D::Order::Equal ); ++checks;
    auto impossible = recruit( 1, 1000000 );
    assert( M::compareDefense( session.evaluate( impossible ), early ) == D::Order::Incomparable ); ++checks;
    // Unknown deadline/path/coverage, hidden observation and missing scorer remain unknown.
    for ( int variant = 0; variant < 6; ++variant ) {
        auto unknown = registry;
        if ( variant == 0 ) unknown.threats[0].lastSafeDay.reset();
        if ( variant == 1 ) unknown.threats[0].reachable = false;
        if ( variant == 2 ) unknown.threats[0].known = false;
        if ( variant == 3 ) unknown.coverageComplete = false;
        if ( variant == 4 ) unknown.threats[0].lastSafeDay = 16;
        M::Budget b; M::Session pending( c, assignments(), force, b, unknown, variant == 5 ? D::Scorer{} : defense );
        const auto result = pending.evaluate( recruit( 2 ) );
        assert( !result.defense.complete() && M::compareDefense( result, early ) == D::Order::Incomparable ); ++checks;
    }
    M::Budget noRegistryBudget; M::Session noRegistry( c, assignments(), force, noRegistryBudget );
    assert( noRegistry.evaluate( recruit( 2 ) ).defense.status == D::Status::NotRequested ); ++checks;
    D::Registry empty{ {}, true };
    M::Budget emptyBudget; M::Session quiet( c, assignments(), force, emptyBudget, empty );
    assert( quiet.evaluate( recruit( 2 ) ).defense.complete() ); ++checks;
    // Two threats at distinct dates: require viability against each, never sum enemy armies.
    auto two = registry; two.threats.push_back( threat( 2, 42, 4, 140 ) );
    M::Budget twoBudget; M::Session threatened( c, assignments(), force, twoBudget, two, defense );
    const auto multi = threatened.evaluate( recruit( 2, 6 ) );
    assert( multi.defense.checks.size() == 2 && multi.defense.cities.size() == 2 && multi.defense.cities[0].viable == true ); ++checks;
    const auto inadequate = threatened.evaluate( recruit( 2 ) );
    assert( inadequate.defense.checks[0].viable == true && inadequate.defense.checks[1].viable == false && inadequate.defense.cities[0].viable == true && inadequate.defense.cities[1].viable == false ); ++checks;
    // Order by last safe phase, then existing planner rank, then stable town ID.
    auto orderedContext = c; orderedContext.base.start.towns.push_back( orderedContext.base.start.towns[0] );
    orderedContext.base.start.towns[1].tile = 70;
    auto orderRegistry = registry; auto second = threat( 2, 70, 1 ); second.priority = 9; orderRegistry.threats.push_back( second );
    auto ordered = D::prepare( orderRegistry, orderedContext.base, true );
    assert( ordered.cities[0].town == 70 ); ++checks;
    orderRegistry.threats[1].lastSafeDay = 2; orderRegistry.threats[0].priority = 10;
    ordered = D::prepare( orderRegistry, orderedContext.base, true ); assert( ordered.cities[0].town == 70 ); ++checks;
    orderRegistry.threats[1].priority = 10;
    ordered = D::prepare( orderRegistry, orderedContext.base, true ); assert( ordered.cities[0].town == 42 ); ++checks;
    // No reordering to favor a candidate: first unresolved city dominates all later benefits.
    D::Evaluation a = ordered, b = ordered;
    a.status = b.status = D::Status::Complete;
    a.cities[0].viable = true; a.cities[1].viable = false;
    b.cities[0].viable = false; b.cities[1].viable = true;
    assert( D::comparePriority( a, b ) == D::Order::Better ); ++checks;
    b.key += "different observation"; assert( D::comparePriority( a, b ) == D::Order::Incomparable ); ++checks;
    coverage = D::coverage( { &a, &b, nullptr } ); assert( coverage.compared == 1 && coverage.omitted == 2 ); ++checks;
    // Corrupt registries fail before spending projection budget.
    for ( int variant = 0; variant < 5; ++variant ) {
        auto invalid = registry;
        if ( variant == 0 ) invalid.threats[0].enemyStrength = 0;
        if ( variant == 1 ) invalid.threats[0].enemyStrength = std::numeric_limits<double>::quiet_NaN();
        if ( variant == 2 ) invalid.threats[0].town = 900;
        if ( variant == 3 ) invalid.threats.push_back( invalid.threats[0] );
        if ( variant == 4 ) invalid.threats.resize( 1025, invalid.threats[0] );
        M::Budget bgt; M::Session bad( c, assignments(), force, bgt, invalid, defense );
        assert( bad.evaluate( recruit( 2 ) ).status == M::Status::Invalid && bgt.projections == 256 ); ++checks;
    }
    M::Budget exhausted; exhausted.forces = 16; // Baseline 15 daily forces + deadline exhausts the whole shared budget.
    M::Session bounded( c, assignments(), force, exhausted, registry, defense );
    assert( bounded.evaluate( recruit( 2 ) ).status == M::Status::Limit ); ++checks;
    M::Budget nanBudget; M::Session nan( c, assignments(), force, nanBudget, registry,
        []( const auto &, const auto & ) { return D::Value{ true, std::numeric_limits<double>::quiet_NaN(), {} }; } );
    assert( !nan.evaluate( recruit( 2 ) ).defense.complete() ); ++checks;
    // The current day can be the last safe action phase.
    auto today = registry; today.threats[0].lastSafeDay = 1;
    M::Budget todayBudget; M::Session now( c, assignments(), force, todayBudget, today, defense );
    assert( now.evaluate( recruit( 1 ) ).defense.cities[0].viable == true && now.evaluate( recruit( 2 ) ).defense.cities[0].viable == false ); ++checks;
    c.base.start.economy.gold = 0;
    M::Budget poorBudget; M::Session poor( c, assignments(), force, poorBudget, today, defense );
    const auto unfunded = poor.evaluate( recruit( 1 ) );
    assert( !unfunded.defense.complete() && M::compareDefense( unfunded, early ) == D::Order::Incomparable ); ++checks;
    auto longContext = context(); longContext.base.throughDay = 29;
    auto distant = registry; distant.threats[0].lastSafeDay = 20;
    auto longUses = assignments(); longUses[0].lastDay = 29;
    M::Budget longBudget; M::Session longSession( longContext, longUses, force, longBudget, distant, defense );
    assert( longSession.evaluate( recruit( 19 ) ).defense.cities[0].viable == true ); ++checks;
    auto staleContext = context(); staleContext.base.start.economy.day = 7; staleContext.base.throughDay = 21;
    auto staleRegistry = registry; staleRegistry.threats[0].lastSafeDay = 6;
    M::Budget staleBudget; M::Session staleSession( staleContext, assignments(), force, staleBudget, staleRegistry, defense );
    assert( !staleSession.evaluate( recruit( 8 ) ).defense.complete() ); ++checks;
    auto inconsistent = registry; inconsistent.threats.push_back( threat( 2 ) ); inconsistent.threats[1].priority = 99;
    M::Budget inconsistentBudget; M::Session badRank( context(), assignments(), force, inconsistentBudget, inconsistent, defense );
    assert( badRank.evaluate( recruit( 2 ) ).status == M::Status::Invalid ); ++checks;
    auto noUtilityUses = assignments(); noUtilityUses[0].useful = false;
    M::Budget defenseOnlyBudget; defenseOnlyBudget.forces = 1;
    M::Session defenseLimited( context(), noUtilityUses, force, defenseOnlyBudget, registry, defense );
    assert( defenseLimited.evaluate( recruit( 2 ) ).status == M::Status::Limit ); ++checks;
    auto tiny = registry; tiny.threats[0].enemyStrength = 1e-100;
    M::Budget overflowBudget; M::Session overflow( context(), assignments(), force, overflowBudget, tiny,
        []( const auto &, const auto & ) { return D::Value{ true, 1e300, {} }; } );
    assert( !overflow.evaluate( recruit( 2 ) ).defense.complete() ); ++checks;
    M::Budget separateBudget; M::Session separate( context(), assignments(), force, separateBudget, registry, defense );
    const auto separateResult = separate.evaluate( recruit( 2 ) );
    assert( separateResult.defense.complete() && M::compareDefense( separateResult, early ) == D::Order::Incomparable ); ++checks;
    // A later threat against the same city must not jump ahead of another city's earlier deadline.
    auto interleavedRegistry = orderRegistry;
    interleavedRegistry.threats[1].lastSafeDay = 3;
    auto futureSameCity = threat( 3, 42, 4 ); futureSameCity.priority = 10;
    interleavedRegistry.threats.push_back( futureSameCity );
    auto interleaved = D::prepare( interleavedRegistry, orderedContext.base, true );
    assert( interleaved.cities.size() == 3 && interleaved.cities[0].town == 42 && interleaved.cities[1].town == 70
            && interleaved.cities[2].town == 42 && interleaved.cities[2].deadline == 4 ); ++checks;
    auto protectMiddle = interleaved, protectLater = interleaved;
    protectMiddle.status = protectLater.status = D::Status::Complete;
    protectMiddle.cities[0].viable = protectLater.cities[0].viable = true;
    protectMiddle.cities[1].viable = true; protectMiddle.cities[2].viable = false;
    protectLater.cities[1].viable = false; protectLater.cities[2].viable = true;
    assert( D::comparePriority( protectMiddle, protectLater ) == D::Order::Better ); ++checks;
    auto simultaneous = two; simultaneous.threats[1].lastSafeDay = 2;
    M::Budget simultaneousBudget; M::Session samePhase( context(), assignments(), force, simultaneousBudget, simultaneous, defense );
    const auto individualAttacks = samePhase.evaluate( recruit( 2, 6 ) );
    assert( individualAttacks.defense.cities.size() == 1 && individualAttacks.defense.cities[0].viable == true ); ++checks;
    assert( samePhase.evaluate( recruit( 2 ) ).defense.cities[0].viable == false ); ++checks;
    // Automatic conscription can weaken a defense: unpurchased trainees stay out of the army.
    auto trainingRegistry = registry; trainingRegistry.threats[0].enemyStrength = 90.6;
    const auto peasants = []( const P::Town & t, M::Slot ) { return M::Value{ true, static_cast<double>( P::count( t.garrison ) ), {} }; };
    const auto peasantDefense = []( const P::Town & t, const D::Threat & ) { return D::Value{ true, static_cast<double>( P::count( t.garrison ) ), {} }; };
    M::Budget trainingBudget; M::Session conversion( context(), assignments(), peasants, trainingBudget, trainingRegistry, peasantDefense );
    G::Candidate training; training.id = 9; auto trainingAction = action( 0, 1, P::Kind::Build, 42, BUILD_TRAINING_GROUNDS );
    training.actions = { trainingAction };
    const auto converted = conversion.evaluate( training );
    assert( converted.baselineDefense.cities[0].viable == true && converted.defense.cities[0].viable == false ); ++checks;
    std::cout << "PASS " << checks << " defense oracles (deadlines, priority, uncertainty, funding, coverage and shared work)\n";
}
