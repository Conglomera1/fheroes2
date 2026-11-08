#include "economy_stability.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
namespace T = AI::EconomyStability;
namespace S = AI::EconomySearch;
namespace R = AI::EconomyRisk;
namespace M = AI::EconomyMilitary;
namespace D = AI::EconomyDefense;
namespace G = AI::EconomyCandidates;
G::Context context()
{
    G::Context c; c.base = fixture( false, 1 ); c.base.throughDay = 15;
    c.base.start.economy.nonTaxIncome = c.base.start.economy.taxRate = 0;
    c.base.start.towns[0].incomeAttributionKnown = true; c.base.start.economy.towns[0].troops.pools[0] = 50;
    return c;
}
G::Candidate conversion( const uint64_t id = 1 )
{ G::Candidate p; p.id = id; p.actions = { action( 0, 1, P::Kind::Build, 42, BUILD_TRAINING_GROUNDS ) }; return p; }
G::Result proposals( std::vector<G::Candidate> projects ) { G::Result r; r.valid = true; r.projects = std::move( projects ); return r; }
T::Result run( const G::Context & c, const G::Result & p, M::Budget & budget, S::Work & work,
               M::Scorer scorer, const D::Registry & threats = { {}, true }, D::Scorer guard = {}, const R::Registry & risk = { {}, true, {} } )
{
    std::vector<M::Assignment> uses;
    for ( const auto & town : c.base.start.towns ) uses.push_back( { uint64_t( uses.size() + 1 ), town.tile, M::Slot::TownArmy, 1, 15, true, true } );
    R::Session evaluator( c, uses, scorer, budget, threats, guard, risk ); return T::select( evaluator, p, budget, work );
}
M::Value role( const P::Town & town, M::Slot ) { return { true, town.mask & BUILD_TRAINING_GROUNDS ? 100.0 : 0.0, {} }; }
M::Value zero( const P::Town &, M::Slot ) { return { true, 0, {} }; }
int main()
{
    size_t n = 0;
    S::Score old, next; old.verified = next.verified = true; old.defenseComplete = next.defenseComplete = true;
    old.scope = next.scope = std::make_shared<D::ComparisonScope>(); old.robust = old.central = 100;
    next.robust = next.central = 105; assert( !T::compare( next, old ).accepted ); ++n;
    next.robust = next.central = 120; assert( T::compare( next, old ).accepted ); ++n;
    old.robust = old.central = 90; next.robust = next.central = 100;
    assert( !T::compare( next, old ).accepted && T::compare( next, old ).gain == 10 && T::compare( next, old ).margin == 10 ); ++n;
    next.robust = next.central = 100.0001; assert( T::compare( next, old ).accepted ); ++n;
    old.robust = old.central = -100; next.robust = next.central = -80; assert( T::compare( next, old ).accepted ); ++n;
    next.robust = next.central = -95; assert( !T::compare( next, old ).accepted ); ++n;
    old.robust = old.central = 0; next.robust = next.central = 0.1; assert( !T::compare( next, old ).accepted ); ++n;
    next.robust = next.central = 0.11; assert( T::compare( next, old ).accepted ); ++n;
    old.robust = old.central = 100; next.robust = 120; next.central = 99.99999; assert( T::compare( next, old ).accepted ); ++n;
    next.central = 99; assert( !T::compare( next, old ).accepted ); ++n;
    old.defenses = { false, true }; next.defenses = { true, false }; assert( T::compare( next, old ).reason == "urgent_defense_made_viable" ); ++n;
    old.defenses = { true, false }; next.defenses = { false, true }; assert( !T::compare( next, old ).accepted ); ++n;
    old.defenses.clear(); next.defenses.clear(); next.central = 120;
    auto different = next; different.scope = std::make_shared<D::ComparisonScope>(); assert( !T::compare( different, old ).accepted ); ++n;
    different = next; different.verified = false; assert( !T::compare( different, old ).accepted ); ++n;
    different = next; different.robust = std::numeric_limits<double>::infinity(); assert( !T::compare( different, old ).accepted ); ++n;
    auto c = context(); auto tg = conversion();
    assert( T::changes( c, tg ) == std::vector<size_t>{ 0 } ); ++n;
    auto farm = tg; farm.actions[0].building = BUILD_WEL2;
    assert( T::changes( c, farm ).empty() ); ++n;
    auto initialTG = c; initialTG.base.start.towns[0].mask &= ~BUILD_WEL2; initialTG.base.start.towns[0].mask |= BUILD_TRAINING_GROUNDS;
    assert( T::changes( initialTG, farm ).size() == 1 ); ++n;
    auto chain = tg; auto back = farm.actions[0]; back.day = 2; chain.actions.push_back( back ); assert( T::changes( c, chain ).size() == 2 ); ++n;
    // Even when the base role conversion is committed, it is not counted twice.
    auto committed = c; committed.base.actions = tg.actions; assert( T::changes( committed, G::Candidate{} ).empty() ); ++n;
    M::Budget budget; S::Work work; auto result = run( c, proposals( { tg } ), budget, work, role );
    assert( result.selected && result.certified && !result.actionable && result.proofs.size() == 1 && result.proofs[0].decision.accepted ); ++n;
    assert( result.proofs[0].cached && result.evaluations == 0 && budget.projections == 253 ); ++n;
    // Useless conversion rejected without a numerical fluctuation becoming an emergency.
    M::Budget uselessBudget; S::Work uselessWork; result = run( c, proposals( { tg } ), uselessBudget, uselessWork, zero );
    assert( result.selected && T::changes( c, result.search.plans[*result.selected].candidate ).empty() ); ++n;
    // A strong operation in another town cannot hide an irrelevant TG in town 42.
    auto second = c.base.start.towns[0]; second.tile = 70; second.race = Race::WRLK;
    auto economic = c.base.start.economy.towns[0]; economic.tile = 70;
    c.base.start.towns.push_back( second ); c.base.start.economy.towns.push_back( economic ); P::refresh( c.base.start );
    auto bundle = tg; bundle.actions.push_back( G::recruit( 70, 1, 5, 2 ) );
    M::Budget bundledBudget; S::Work bundledWork;
    const auto archers = []( const P::Town & town, M::Slot ) { return M::Value{ true, 100.0 * P::count( town.garrison, Monster::ARCHER ), {} }; };
    result = run( c, proposals( { bundle } ), bundledBudget, bundledWork, archers );
    assert( !result.proofs.empty() && !result.proofs[0].decision.accepted && result.proofs[0].without.verified ); ++n;
    assert( result.selected && result.search.plans[*result.selected].candidate.actions.empty() ); ++n;
    // Two conversions: the second is pointless; the first may survive as a separate candidate.
    auto both = tg; auto other = tg.actions[0]; other.town = 70; both.actions.push_back( other );
    c.base.start.towns[1].race = Race::KNGT;
    const auto oneRole = []( const P::Town & town, M::Slot ) { return M::Value{ true, town.tile == 42 && ( town.mask & BUILD_TRAINING_GROUNDS ) ? 100.0 : 0.0, {} }; };
    M::Budget pairBudget; S::Work pairWork; result = run( c, proposals( { both } ), pairBudget, pairWork, oneRole );
    assert( result.proofs.size() == 2 && result.proofs[0].decision.accepted && !result.proofs[1].decision.accepted ); ++n;
    assert( result.selected && result.search.plans[*result.selected].candidate.actions.empty() ); ++n;
    // A missing legal counterfactual is not treated as proof of benefit: keep safe fallback.
    c = context(); auto illegalWithout = tg; illegalWithout.actions.push_back( back );
    M::Budget illegalBudget; S::Work illegalWork;
    result = run( c, proposals( { illegalWithout } ), illegalBudget, illegalWork,
        []( const P::Town & t, M::Slot ) { return M::Value{ true, t.mask & BUILD_WEL2 ? 100.0 : 0.0, {} }; } );
    assert( result.selected && result.search.plans[*result.selected].candidate.actions.empty() ); ++n;
    // Useful TG: removing it reduces the recruitment pool. Rebuild a LEGAL ordinary
    // comparison by capping that optional purchase, not by discarding all its troops.
    c = context(); c.base.start.economy.towns[0].troops.pools[0] = 0;
    auto productive = tg; productive.actions.push_back( G::recruit( 42, 1, 25, 15 ) );
    M::Budget productiveBudget; S::Work productiveWork;
    result = run( c, proposals( { productive } ), productiveBudget, productiveWork, archers );
    assert( result.selected && result.proofs.size() == 1 && result.proofs[0].decision.accepted ); ++n;
    assert( result.proofs[0].without.verified && result.proofs[0].repairs.size() == 1
            && result.proofs[0].repairs[0].requested == 25 && result.proofs[0].repairs[0].kept == 16 && result.evaluations == 2 ); ++n;
    assert( productiveBudget.projections == 251 && result.search.plans[*result.selected].candidate.actions[1].count == 25 ); ++n;
    // Required military amounts cannot be silently reduced to fabricate a comparator.
    productive.actions[1].required = true; M::Budget requiredBudget; S::Work requiredWork;
    result = run( c, proposals( { productive } ), requiredBudget, requiredWork, archers );
    assert( result.selected && result.search.plans[*result.selected].candidate.actions.empty() && !result.proofs[0].decision.accepted ); ++n;
    // Reserve stops the search without restoring the real counter; the proof must use it.
    M::Budget reserveBudget; reserveBudget.projections = 4; S::Work reserveWork;
    result = run( c, proposals( { productive } ), reserveBudget, reserveWork, archers );
    assert( result.reserved == 1 && reserveBudget.projections == 0 && result.selected
            && result.search.plans[*result.selected].candidate.actions.empty() ); ++n;
    // A TG-to-Farm conversion may finance real military purchases instead. Keep
    // the valid ordinary counterfactual affordable, including a zero-sized purchase.
    c = context(); c.base = fixture( true, 1 ); c.base.throughDay = 15;
    c.base.start.economy.nonTaxIncome = 0; c.base.start.economy.gold = 1500;
    c.base.start.towns[0].incomeAttributionKnown = true;
    c.base.start.economy.towns[0].troops.pools[0] = 50;
    auto fiscal = farm; fiscal.actions.push_back( G::recruit( 42, 1, 10, 15 ) );
    M::Budget fiscalBudget; S::Work fiscalWork; result = run( c, proposals( { fiscal } ), fiscalBudget, fiscalWork, archers );
    assert( result.selected && result.proofs.size() == 1 && result.proofs[0].decision.accepted && result.proofs[0].without.verified ); ++n;
    assert( result.proofs[0].repairs.size() == 1 && result.proofs[0].repairs[0].kept == 0 ); ++n;
    // The same repair is rechecked centrally AND against a justified loss elsewhere.
    c = context(); c.base.start.economy.towns[0].troops.pools[0] = 0;
    second = c.base.start.towns[0]; second.tile = 70;
    economic = c.base.start.economy.towns[0]; economic.tile = 70;
    c.base.start.towns.push_back( second ); c.base.start.economy.towns.push_back( economic ); P::refresh( c.base.start );
    D::Threat distantThreat; distantThreat.id = 1; distantThreat.town = 70; distantThreat.enemyTile = 9;
    distantThreat.known = distantThreat.reachable = true; distantThreat.lastSafeDay = 2; distantThreat.enemyStrength = 1;
    R::Registry distantRisk{ { { 1, R::Focus::Destination, { { 70, 3 } }, { 1 }, false, "other town lost" } }, true, {} };
    productive.actions[1].required = false;
    M::Budget variantBudget; S::Work variantWork;
    result = run( c, proposals( { productive } ), variantBudget, variantWork, archers, { { distantThreat }, true },
        []( const auto &, const auto & ) { return D::Value{ true, 10000, {} }; }, distantRisk );
    assert( result.selected && result.proofs.size() == 1 && result.proofs[0].decision.accepted
            && result.proofs[0].without.witnesses.size() == 2 && variantBudget.projections == 246 ); ++n;
    // All currencies constrain a counterfactual purchase, including a rare-resource cap.
    auto stockCheck = fixture( false, 1 ); stockCheck.actions = { G::recruit( 42, 1, 3, 1 ) };
    stockCheck.actions[0].id = stockCheck.actions[0].project = 1;
    stockCheck.start.economy.towns[0].troops.pools[0] = 5;
    stockCheck.start.towns[0].dwellings[1].basic.price[6] = 2;
    stockCheck.start.economy.materials[5] = 3; P::WorkBudget stockBudget;
    auto failure = P::project( stockCheck, stockBudget );
    assert( failure.reason == "payment_unavailable" && failure.failedRecruitAffordable == 1 && !failure.failedRecruitStock ); ++n;
    // Active commitment becomes unaffordable after a parameter change; abandon, never bypass legality.
    c = context(); c.active = tg.actions; c.base.start.economy.gold = 300; c.base.config.trainingGold = 1000;
    G::Candidate ordinary; ordinary.id = 2; ordinary.actions = { G::recruit( 42, 1, 1, 2 ) };
    M::Budget invalidBudget; S::Work invalidWork; result = run( c, proposals( { ordinary } ), invalidBudget, invalidWork, archers );
    assert( result.current && T::demonstrablyImpossible( result.search.plans[*result.current] ) && result.selected
            && result.search.plans[*result.selected].candidate.actions.size() == 1 ); ++n;
    S::Plan unknown; unknown.score.risk = R::Status::Incomplete; assert( !T::demonstrablyImpossible( unknown ) ); ++n;
    // Unknown deadline forbids a certificate, including an ordinary result.
    c = context(); D::Threat threat; threat.id = 1; threat.town = 42; threat.known = threat.reachable = true; threat.enemyStrength = 1;
    M::Budget unknownBudget; S::Work unknownWork; result = run( c, proposals( { tg } ), unknownBudget, unknownWork, role, { { threat }, true } );
    assert( !result.selected && !result.certified ); ++n;
    // A conversion can rescue a verified urgent defense without satisfying the score margin.
    threat.lastSafeDay = 2; threat.enemyStrength = 95; threat.enemyTile = 9;
    M::Budget urgentBudget; S::Work urgentWork;
    result = run( c, proposals( { tg } ), urgentBudget, urgentWork, zero, { { threat }, true },
        []( const P::Town & t, const D::Threat & ) { return D::Value{ true, t.mask & BUILD_TRAINING_GROUNDS ? 200.0 : 0.0, {} }; } );
    assert( result.selected && result.proofs.size() == 1 && result.proofs[0].decision.reason == "urgent_defense_made_viable" ); ++n;
    // Risk exposure cannot be waived by emergency or by a good central utility.
    R::Registry risk{ { { 1, R::Focus::Destination, { { 42, 3 } }, { 1 }, false, "lost town" } }, true, {} };
    auto exposed = tg; exposed.actions[0].day = 4; exposed.actions[0].required = true;
    M::Budget riskBudget; S::Work riskWork;
    result = run( c, proposals( { exposed } ), riskBudget, riskWork, role, { { threat }, true },
        []( const auto &, const auto & ) { return D::Value{ true, 200, {} }; }, risk );
    assert( !result.selected || result.search.plans[*result.selected].candidate.actions.empty() ); ++n;
    // Shared limits never create proof credit. Protected ordinary actions remain available.
    M::Budget shortBudget; shortBudget.projections = 2; S::Work shortWork;
    result = run( context(), proposals( { tg } ), shortBudget, shortWork, role );
    assert( result.selected && result.search.plans[*result.selected].candidate.actions.empty() && shortBudget.projections == 0 ); ++n;
    M::Budget dayBudget; dayBudget.mechanics.days = 0; S::Work dayWork;
    result = run( context(), proposals( { tg } ), dayBudget, dayWork, role ); assert( !result.selected ); ++n;
    M::Budget mergeBudget; S::Work mergeWork; mergeWork.merges = 0;
    result = run( context(), proposals( { tg } ), mergeBudget, mergeWork, role );
    assert( result.selected && result.search.plans[*result.selected].candidate.actions.empty() ); ++n;
    // With no merge credit after search, even cached marginal proofs cannot be invented.
    c = context(); c.active = tg.actions; M::Budget proofBudget; S::Work proofWork; proofWork.merges = 1;
    result = run( c, proposals( {} ), proofBudget, proofWork, role );
    assert( result.selected && result.search.plans[*result.selected].candidate.actions.empty() && !result.rejected.empty() ); ++n;
    auto distant = proposals( { tg } ); distant.longerHorizonNeeded = true;
    M::Budget horizonBudget; S::Work horizonWork; result = run( context(), distant, horizonBudget, horizonWork, role );
    assert( result.selected && result.search.plans[*result.selected].candidate.actions.empty() ); ++n;
    // Repeat deterministically; diagnostics and prior evaluations must not add budget.
    M::Budget repeatBudget; S::Work repeatWork; const auto repeated = run( context(), proposals( { tg } ), repeatBudget, repeatWork, role );
    assert( repeated.proofs.size() == 1 && repeated.proofs[0].decision.accepted && repeatBudget.projections == 253 ); ++n;
    std::cout << "PASS " << n << " stability oracles (margin, individual conversions, urgent defense, limits and safe fallback)\n";
}
