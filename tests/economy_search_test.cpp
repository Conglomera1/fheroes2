#include "economy_search.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
namespace S = AI::EconomySearch;
namespace R = AI::EconomyRisk;
namespace M = AI::EconomyMilitary;
namespace D = AI::EconomyDefense;
namespace G = AI::EconomyCandidates;
G::Context context()
{
    G::Context c; c.base = fixture( false, 1 ); c.base.throughDay = 15;
    c.base.start.economy.taxRate = 0; c.base.start.economy.nonTaxIncome = 0;
    c.base.start.economy.towns[0].troops.pools[0] = 100;
    auto target = c.base.start.towns[0]; target.tile = 70; target.race = Race::WRLK; target.garrison = {};
    target.dwellings[1].basic.id = Monster::GARGOYLE; target.incomeAttributionKnown = true;
    auto economic = c.base.start.economy.towns[0]; economic.tile = 70; economic.troops.garrison = 0;
    c.base.start.towns[0].incomeAttributionKnown = true;
    c.base.start.towns.push_back( target ); c.base.start.economy.towns.push_back( economic ); P::refresh( c.base.start ); return c;
}
std::vector<M::Assignment> uses() { return { { 1, 42, M::Slot::TownArmy, 1, 15, true, true }, { 2, 70, M::Slot::TownArmy, 1, 15, true, true } }; }
M::Value army( const P::Town & t, M::Slot ) { return { true, 100.0 * ( P::count( t.garrison, Monster::ARCHER ) + P::count( t.garrison, Monster::RANGER ) + P::count( t.garrison, Monster::GARGOYLE ) ), {} }; }
D::Value defense( const P::Town & t, const D::Threat & ) { return { true, army( t, M::Slot::TownArmy ).force, {} }; }
G::Candidate buy( const uint64_t id, const int town, const uint32_t day = 2, const uint32_t count = 1 ) { G::Candidate p; p.id = id; p.families = G::bit( G::Family::Recruit ); p.actions = { G::recruit( town, 1, count, day ) }; return p; }
G::Result proposals( std::vector<G::Candidate> p ) { G::Result r; r.valid = true; r.projects = std::move( p ); return r; }
S::Result run( const G::Context & c, const G::Result & p, M::Budget & budget, S::Work & work, const D::Registry & d = { {}, true }, const R::Registry & r = { {}, true, {} }, M::Scorer scorer = army, D::Scorer guard = defense )
{ R::Session evaluator( c, uses(), std::move( scorer ), budget, d, std::move( guard ), r ); return S::search( evaluator, p, budget, work ); }
int main()
{
    size_t checks = 0;
    auto c = context(); c.base.start.economy.gold = 225;
    M::Budget budget; S::Work work; const auto p = proposals( { buy( 1, 42 ), buy( 2, 70 ) } );
    auto result = run( c, p, budget, work );
    assert( result.best && result.plans[*result.best].score.verified && *result.plans[*result.best].score.robust == 100 ); ++checks;
    assert( result.plans[*result.best].candidate.actions.size() == 1 && result.noInvestment && result.ordinary && !result.actionable ); ++checks;
    assert( std::any_of( result.plans.begin(), result.plans.end(), []( const auto & n ) { return n.members.size() == 2 && !n.score.verified; } ) ); ++checks;
    assert( budget.projections < 256 && result.counts.cacheHits && result.beam.size() <= 8 && result.passes <= 6 ); ++checks;
    M::Budget repeatBudget; S::Work repeatWork; const auto repeated = run( c, p, repeatBudget, repeatWork );
    assert( G::signature( repeated.plans[*repeated.best].candidate ) == G::signature( result.plans[*result.best].candidate ) && budget.projections == repeatBudget.projections ); ++checks;
    // A Knight Farm finances a later Warlock recruitment which cannot be funded alone.
    c = context(); c.base.start.economy.gold = 1000; c.base.start.economy.taxRate = 2;
    c.base.start.towns[0].mask &= ~BUILD_WEL2; P::refresh( c.base.start );
    G::Candidate farm; farm.id = 1; farm.families = G::bit( G::Family::Fiscal ); auto build = action( 0, 1, P::Kind::Build, 42, BUILD_WEL2 ); farm.actions = { build };
    M::Budget fiscalBudget; S::Work fiscalWork; result = run( c, proposals( { farm, buy( 2, 70, 9, 10 ) } ), fiscalBudget, fiscalWork );
    assert( result.best && result.plans[*result.best].members.size() == 2 && *result.plans[*result.best].score.robust == 500 ); ++checks;
    assert( result.plans[*result.best].ordinary && result.ordinary == result.best ); ++checks;
    // A shared prerequisite is built/paid once, while distinct purchases are paid twice.
    c = context(); c.base.start.economy.gold = 1400;
    auto a = buy( 1, 42, 2 ), b = buy( 2, 42, 3 );
    build.building = DWELLING_UPGRADE2; a.actions.insert( a.actions.begin(), build ); b.actions.insert( b.actions.begin(), build );
    auto merged = S::combine( c, { &a, &b }, 20 );
    assert( merged.valid && merged.candidate.actions.size() == 3 ); ++checks;
    M::Budget sharedBudget; S::Work sharedWork; result = run( c, proposals( { a, b } ), sharedBudget, sharedWork );
    assert( result.best && result.plans[*result.best].candidate.actions.size() == 3 && *result.plans[*result.best].score.robust > 190 ); ++checks;
    b.actions[0].building = BUILD_WELL; assert( !S::combine( c, { &a, &b }, 21 ).valid ); ++checks;
    a = buy( 1, 42 ); b = a; b.id = 2;
    assert( S::combine( c, { &a, &b }, 22 ).candidate.actions.size() == 2 ); ++checks;
    a.actions[0].id = a.actions[0].project = 7; b.actions = a.actions;
    assert( S::combine( c, { &a, &b }, 23 ).candidate.actions.size() == 1 ); ++checks;
    b.actions[0].count = 2; assert( !S::combine( c, { &a, &b }, 24 ).valid ); ++checks;
    b.actions[0].count = 1; b.actions[0].id = b.actions[0].project = 8;
    assert( S::combine( c, { &a, &b }, 25 ).candidate.actions.size() == 2 ); ++checks;
    auto retained = c; retained.base.actions = a.actions;
    assert( S::combine( retained, { &a }, 26 ).candidate.actions.empty() ); ++checks;
    P::Action left = action( 0, 1, P::Kind::Build, 42, BUILD_STATUE ), right = action( 0, 1, P::Kind::Build, 70, BUILD_WELL );
    a.actions = { left, right }; b.actions = { right, left };
    assert( S::combine( c, { &a, &b }, 27 ).reason == "shared_order_cycle" ); ++checks;
    // Rare resources, not just gold, are shared.
    c = context(); c.base.start.economy.materials[0] = 1;
    c.base.start.towns[0].dwellings[1].basic.price[1] = c.base.start.towns[1].dwellings[1].basic.price[1] = 1;
    M::Budget rareBudget; S::Work rareWork; result = run( c, p, rareBudget, rareWork );
    assert( *result.plans[*result.best].score.robust == 100 ); ++checks;
    // An early viable defense wins over a stronger late army; the combined purchase cannot be paid.
    c = context(); c.base.start.economy.gold = 750;
    D::Threat threat; threat.id = 1; threat.town = 70; threat.enemyTile = 9; threat.enemyStrength = 50; threat.lastSafeDay = 2; threat.known = threat.reachable = true;
    D::Registry threats{ { threat }, true };
    M::Budget deadlineBudget; S::Work deadlineWork; result = run( c, proposals( { buy( 1, 70, 2 ), buy( 2, 70, 3, 5 ) } ), deadlineBudget, deadlineWork, threats );
    assert( result.best && result.plans[*result.best].score.defenses == std::vector<bool>{ true } && result.plans[*result.best].candidate.actions[0].count == 1 ); ++checks;
    assert( result.unresolvedTowns.empty() ); ++checks;
    threat.lastSafeDay.reset(); threats.threats = { threat };
    M::Budget unknownBudget; S::Work unknownWork; result = run( c, p, unknownBudget, unknownWork, threats );
    assert( !result.best && result.status == S::Status::NoVerified ); ++checks;
    // Risk is compared before central power, after the equal defense vectors.
    c = context(); c.base.start.economy.gold = 150; threat.lastSafeDay = 2; threat.enemyStrength = 1; threats.threats = { threat };
    R::Registry risk{ { { 1, R::Focus::Destination, { { 70, 3 } }, { 1 }, false, "lost destination" } }, true, {} };
    M::Budget riskBudget; S::Work riskWork;
    result = run( c, p, riskBudget, riskWork, threats, risk,
        []( const P::Town & t, M::Slot ) { return M::Value{ true, 100.0 * P::count( t.garrison, Monster::ARCHER ) + 200.0 * P::count( t.garrison, Monster::GARGOYLE ), {} }; },
        []( const auto &, const auto & ) { return D::Value{ true, 10000, {} }; } );
    assert( result.best && result.plans[*result.best].candidate.actions[0].town == 42 ); ++checks;
    // Active continuation breaks a numerical tie, not a real improvement.
    c = context(); c.base.start.economy.gold = 150; c.active = buy( 1, 42 ).actions;
    M::Budget activeBudget; S::Work activeWork; result = run( c, proposals( { buy( 2, 70 ) } ), activeBudget, activeWork );
    assert( result.active && result.best && result.plans[*result.best].active ); ++checks;
    S::Work exhausted; exhausted.merges = 0; M::Budget referenceBudget;
    result = run( c, p, referenceBudget, exhausted );
    assert( result.best && result.plans[*result.best].active && result.status == S::Status::Partial && referenceBudget.projections == 253 ); ++checks;
    M::Budget shortBudget; shortBudget.projections = 2; S::Work shortWork;
    result = run( context(), p, shortBudget, shortWork );
    assert( result.best == result.noInvestment && result.status == S::Status::Partial && result.counts.evaluations == 1 && shortBudget.projections == 0 ); ++checks;
    M::Budget dayBudget; dayBudget.mechanics.days = 0; S::Work dayWork;
    result = run( context(), p, dayBudget, dayWork ); assert( !result.best ); ++checks;
    auto invalidProposals = p; invalidProposals.valid = false; M::Budget fallbackBudget; S::Work fallbackWork;
    result = run( c, invalidProposals, fallbackBudget, fallbackWork ); assert( result.best && result.plans[*result.best].active && result.status == S::Status::Partial ); ++checks;
    // Enough distinct proposals exercise additions, substitutions and the eight-plan beam.
    std::vector<G::Candidate> many;
    for ( uint32_t i = 1; i <= 10; ++i ) many.push_back( buy( i, 42, i ) );
    M::Budget beamBudget; S::Work beamWork; result = run( context(), proposals( many ), beamBudget, beamWork );
    assert( result.best && result.beam.size() <= 8 && result.passes <= 6 && result.counts.substitutions > 0 && result.plans.size() <= 256 ); ++checks;
    // One prerequisite chain can exceed six construction actions.
    c = context(); G::Candidate chain = buy( 1, 42, 9 ); chain.actions.clear();
    const uint32_t buildings[] = { BUILD_TAVERN, BUILD_THIEVESGUILD, BUILD_MARKETPLACE, BUILD_MAGEGUILD1, BUILD_MAGEGUILD2, BUILD_MAGEGUILD3, BUILD_MAGEGUILD4, BUILD_MAGEGUILD5 };
    for ( size_t i = 0; i < 8; ++i ) { P::Building info; info.id = buildings[i]; info.price[0] = 1; c.base.start.towns[0].catalog[info.id] = info; chain.actions.push_back( action( 0, static_cast<uint32_t>( i + 1 ), P::Kind::Build, 42, info.id ) ); }
    chain.actions.push_back( G::recruit( 42, 1, 1, 9 ) ); M::Budget chainBudget; S::Work chainWork;
    result = run( c, proposals( { chain } ), chainBudget, chainWork ); assert( result.best && result.plans[*result.best].candidate.actions.size() == 9 ); ++checks;
    // The tolerance comparator is used by stable scans, not as a non-transitive std::sort predicate.
    S::Plan x, y; x.score.verified = y.score.verified = true; x.score.scope = y.score.scope = std::make_shared<D::ComparisonScope>();
    x.score.robust = 100; y.score.robust = 100.00005; x.score.central = y.score.central = 100; x.active = true;
    assert( S::prefer( x, y ) ); ++checks;
    y.score.scope = std::make_shared<D::ComparisonScope>(); assert( !S::prefer( x, y ) ); ++checks;
    // Horizon extension must be common and explicit; a late effect is not assigned zero value.
    auto distant = proposals( { buy( 1, 42, 9 ), buy( 2, 70, 20 ) } ); distant.longerHorizonNeeded = true;
    M::Budget horizonBudget; S::Work horizonWork; result = run( context(), distant, horizonBudget, horizonWork );
    assert( result.best && result.longerHorizonNeeded && result.status == S::Status::Partial && result.gaps.count( "common_horizon_review_required" )
            && *result.plans[*result.best].score.robust == 50 ); ++checks;
    // Common retained payments are not counted a second time in the immediate-cost tie break.
    c = context(); auto paid = buy( 1, 42, 1 ); paid.actions[0].id = paid.actions[0].project = 7; c.base.actions = paid.actions;
    M::Budget committedBudget; S::Work committedWork; result = run( c, proposals( { paid, buy( 2, 42, 1 ) } ), committedBudget, committedWork );
    assert( result.best && result.plans[*result.best].score.immediateGold == 150 && *result.plans[*result.best].score.robust == 100 ); ++checks;
    std::cout << "PASS " << checks << " combined-search oracles (synergy, real shared costs, identities, defense/risk ordering and bounded fallback)\n";
}
