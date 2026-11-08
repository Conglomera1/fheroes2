#include "economy_risk.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <cmath>
#include <iostream>
namespace R = AI::EconomyRisk;
namespace M = AI::EconomyMilitary;
namespace D = AI::EconomyDefense;
namespace G = AI::EconomyCandidates;

G::Context context( const bool training = false )
{
    G::Context c; c.base = fixture( training, 1 ); c.base.throughDay = 15;
    auto destination = c.base.start.towns[0]; destination.tile = 70; destination.garrison = {}; destination.mask &= ~BUILD_TRAINING_GROUNDS;
    destination.mask |= BUILD_WEL2;
    c.base.start.towns.push_back( destination );
    auto economic = c.base.start.economy.towns[0]; economic.tile = 70; economic.troops.garrison = 0; economic.troops.pools[0] = 100;
    c.base.start.economy.towns.push_back( economic );
    c.base.start.towns[0].incomeAttributionKnown = c.base.start.towns[1].incomeAttributionKnown = true;
    c.base.start.towns[0].recurringIncome[0] = 1000;
    P::refresh( c.base.start ); return c;
}
D::Registry defenses()
{
    D::Threat a; a.id = 1; a.town = 42; a.enemyTile = 8; a.enemyStrength = 100; a.known = a.reachable = true; a.lastSafeDay = 2;
    auto b = a; b.id = 2; b.town = 70; b.lastSafeDay = 4;
    return { { a, b }, true };
}
R::Registry risks()
{ return { { { 1, R::Focus::Income, { { 42, 3 } }, { 1 }, false, "source lost" },
             { 2, R::Focus::Destination, { { 70, 5 } }, { 2 }, false, "destination lost" } }, true, {} }; }
std::vector<M::Assignment> assignments() { return { { 1, 42, M::Slot::TownArmy, 1, 29, true, true }, { 2, 70, M::Slot::TownArmy, 1, 29, true, true } }; }
M::Value force( const P::Town & town, M::Slot ) { return { true, static_cast<double>( P::count( town.garrison, Monster::ARCHER ) ) * 100, {} }; }
D::Value guard( const P::Town & town, const D::Threat & ) { return { true, static_cast<double>( P::population( town.garrison ) ), {} }; }
G::Candidate buy( const uint32_t day = 2, const uint32_t count = 1 ) { G::Candidate c; c.id = 1; c.actions = { G::recruit( 70, 1, count, day ) }; return c; }
int main()
{
    size_t checks = 0; auto c = context(); M::Budget budget;
    R::Session session( c, assignments(), force, budget, defenses(), guard, risks() );
    auto result = session.evaluate( buy() );
    assert( result.status == R::Status::Scored && result.variants.size() == 3 && result.robustUtility ); ++checks;
    assert( std::abs( *result.robustUtility - 300.0 / 14 ) < 1e-9 && *result.variants[0].evaluation.utility == 100 ); ++checks;
    assert( result.worstUtility == 2 && result.tightestResource[0] && result.worstFunding ); ++checks;
    assert( budget.projections == 250 && budget.mechanics.days == 24576 - 84 ); ++checks;
    result = session.evaluate( buy( 6 ) );
    assert( result.status == R::Status::Scored && *result.robustUtility == 0 && result.variants[2].evaluation.financial.cancelledActions.size() == 1 ); ++checks;
    assert( budget.projections == 247 ); ++checks; // Three cached baselines, not six new projections.
    auto required = buy( 6 ); required.actions[0].required = true;
    result = session.evaluate( required );
    assert( result.status == R::Status::Exposed && !result.robustUtility && result.variants[2].evaluation.financial.cancelledRequiredActions.size() == 1 ); ++checks;
    // Same shock in M0 and proposal: the destroyed initial population is not charged again as a project loss.
    G::Candidate noInvestment; noInvestment.id = 2;
    result = session.evaluate( noInvestment ); assert( result.status == R::Status::Scored && *result.robustUtility == 0 ); ++checks;
    // Losing a source can leave a purchase elsewhere unfunded, without deleting the foreign purchase.
    c.base.start.economy.gold = 0; M::Budget poorBudget;
    R::Session poor( c, assignments(), force, poorBudget, defenses(), guard, risks() );
    result = poor.evaluate( buy( 4, 20 ) );
    assert( result.variants[0].evaluation.financial.covered() && result.status == R::Status::Exposed && !result.robustUtility ); ++checks;
    assert( result.worstFunding == 1 && result.variants[1].evaluation.financial.failedDay == 4 ); ++checks;
    // Equal impacts deduplicate even when described once as income and once as destination risk.
    auto duplicate = risks(); duplicate.scenarios[1].losses = duplicate.scenarios[0].losses; duplicate.scenarios[1].evidence = { 1 };
    M::Budget duplicateBudget; R::Session dedup( context(), assignments(), force, duplicateBudget, defenses(), guard, duplicate );
    assert( dedup.evaluate( buy() ).variants.size() == 2 ); ++checks;
    // No identified disturbance: central is the only scenario, with certified coverage.
    M::Budget calmBudget; R::Session calm( context(), assignments(), force, calmBudget, D::Registry{ {}, true }, guard, R::Registry{ {}, true, {} } );
    result = calm.evaluate( buy() ); assert( result.status == R::Status::Scored && result.variants.size() == 1 && result.robustUtility == result.variants[0].evaluation.utility ); ++checks;
    for ( int variant = 0; variant < 6; ++variant ) {
        auto uncertain = risks();
        if ( variant == 0 ) uncertain.coverageComplete = false;
        if ( variant == 1 ) uncertain.scenarios[0].evidence = { 99 };
        if ( variant == 2 ) uncertain.scenarios[0].losses[0].firstUnavailableDay = 4;
        if ( variant == 3 ) uncertain.gaps.insert( "blocked_route_without_verified_alternative" );
        if ( variant == 4 ) uncertain.scenarios[0].losses.push_back( { 70, 5 } );
        if ( variant == 5 ) uncertain.scenarios.push_back( { 3, R::Focus::Income, { { 42, 3 }, { 70, 5 } }, { 1, 2 }, true, "joint" } );
        M::Budget b; R::Session unknown( context(), assignments(), force, b, defenses(), guard, uncertain );
        const auto r = unknown.evaluate( buy() ); assert( r.status == R::Status::Incomplete && !r.robustUtility ); ++checks;
    }
    // Unknown income attribution is incomplete, not free continued castle income.
    c = context(); c.base.start.towns[0].incomeAttributionKnown = false;
    M::Budget unknownIncomeBudget; R::Session unknownIncome( c, assignments(), force, unknownIncomeBudget, defenses(), guard, risks() );
    result = unknownIncome.evaluate( buy() ); assert( result.status == R::Status::Incomplete && !result.robustUtility ); ++checks;
    // Automatic fees stop at loss, calendar/composition state stays intact, lost troops cannot be purchased.
    auto tg = context( true ).base; tg.losses = { { 42, 3 } }; tg.start.economy.towns[0].troops.state.total = 44;
    P::WorkBudget work; const auto projected = P::project( tg, work );
    assert( projected.covered() && !projected.end.towns[0].available && !projected.end.economy.towns[0].training ); ++checks;
    assert( projected.days[1].morning.fees > 0 && projected.days[2].morning.fees == 0 && projected.end.economy.towns[0].troops.state.total >= 44 ); ++checks;
    // Lost tax populations disappear even when taxation does not require a Farm.
    auto global = context().base; global.start.economy.requireFarms = false; global.losses = { { 42, 3 } };
    P::WorkBudget globalWork; const auto taxes = P::project( global, globalWork ); assert( taxes.covered() && taxes.days[1].morning.tax == 200 && taxes.days[2].morning.tax == 0 ); ++checks;
    // Same assets cannot be lost twice; no current morning is processed retroactively.
    for ( int invalid = 0; invalid < 3; ++invalid ) {
        auto input = context().base; input.losses = { { 42, 3 } };
        if ( invalid == 0 ) input.losses.push_back( { 42, 4 } );
        if ( invalid == 1 ) input.losses[0].firstUnavailableDay = 1;
        if ( invalid == 2 ) input.losses[0].town = 999;
        P::WorkBudget b; assert( !P::project( input, b ).covered() ); ++checks;
    }
    auto all = context().base; all.losses = { { 42, 3 }, { 70, 3 } };
    P::WorkBudget allWork; assert( P::project( all, allWork ).reason == "kingdom_continuation_after_last_town_unmodelled" ); ++checks;
    auto future = context().base; future.losses = { { 42, 3 } }; auto cancelled = action( 7, 20, P::Kind::Recruit, 42, 1 ); cancelled.pool = 1; future.actions = { cancelled };
    P::WorkBudget futureWork; const auto cancelledFuture = P::project( future, futureWork ); assert( cancelledFuture.covered() && cancelledFuture.cancelledActions == std::vector<uint64_t>{ 7 } ); ++checks;
    M::Budget tiny; tiny.projections = 3; R::Session limited( context(), assignments(), force, tiny, defenses(), guard, risks() );
    assert( limited.evaluate( buy() ).status == R::Status::Limit ); ++checks;
    // Loss-related fee savings cannot make U_robust exceed the central utility: central is always included.
    auto trainingContext = context( true ); M::Budget trainingBudget;
    R::Session training( trainingContext, assignments(), force, trainingBudget, defenses(), guard, risks() );
    result = training.evaluate( buy() ); assert( result.robustUtility && *result.robustUtility <= *result.variants[0].evaluation.utility ); ++checks;
    auto established = context(); established.base.start.towns[0].garrison[1] = { Monster::ARCHER, 10 };
    established.base.start.towns[1].garrison[0] = { Monster::ARCHER, 2 };
    M::Budget establishedBudget; R::Session unchanged( established, assignments(), force, establishedBudget, defenses(), guard, risks() );
    const auto unaffected = unchanged.evaluate( noInvestment );
    assert( unaffected.robustUtility && *unaffected.robustUtility == 0 ); ++checks;
    auto invalidRisk = risks(); invalidRisk.scenarios[1].id = invalidRisk.scenarios[0].id;
    M::Budget invalidBudget; R::Session invalid( context(), assignments(), force, invalidBudget, defenses(), guard, invalidRisk );
    assert( invalid.evaluate( buy() ).status == R::Status::Invalid && invalidBudget.projections == 256 ); ++checks;
    auto jointContext = context();
    auto reserveTown = jointContext.base.start.towns[1]; reserveTown.tile = 99;
    auto reserveEconomic = jointContext.base.start.economy.towns[1]; reserveEconomic.tile = 99;
    jointContext.base.start.towns.push_back( reserveTown ); jointContext.base.start.economy.towns.push_back( reserveEconomic );
    auto jointUses = assignments(); jointUses.push_back( { 3, 99, M::Slot::TownArmy, 1, 15, true, true } );
    R::Registry jointRegistry{ { { 3, R::Focus::Income, { { 42, 3 }, { 70, 5 } }, { 1, 2 }, true, "linked losses" } }, true, {} };
    M::Budget jointBudget; R::Session joint( jointContext, jointUses, force, jointBudget, defenses(), guard, jointRegistry );
    const auto linked = joint.evaluate( buy() );
    assert( linked.status == R::Status::Scored && linked.variants.size() == 2 && linked.variants[1].evaluation.financial.appliedLosses.size() == 2 ); ++checks;
    auto oversized = risks(); oversized.scenarios.clear();
    for ( uint64_t id = 1; id <= 4; ++id ) { auto repeated = risks().scenarios[0]; repeated.id = id; repeated.evidence.assign( 1024, 1 ); oversized.scenarios.push_back( repeated ); }
    M::Budget oversizedBudget; R::Session boundedInputs( context(), assignments(), force, oversizedBudget, defenses(), guard, oversized );
    assert( boundedInputs.evaluate( buy() ).status == R::Status::Invalid && oversizedBudget.projections == 256 ); ++checks;
    std::cout << "PASS " << checks << " risk oracles (dated loss, cancellation, real funding, scenario coverage and shared limits)\n";
}
