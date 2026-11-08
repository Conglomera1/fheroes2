#include "economy_recourse.h"
#include "economy_stability.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
namespace Q = AI::EconomyRecourse;
namespace G = AI::EconomyCandidates;
namespace M = AI::EconomyMilitary;
namespace D = AI::EconomyDefense;
namespace R = AI::EconomyRisk;
G::Context context()
{
    G::Context c; c.base = fixture( false, 1 ); c.base.throughDay = 15;
    c.base.start.economy.nonTaxIncome = c.base.start.economy.taxRate = 0;
    c.base.start.economy.gold = 150; c.base.start.economy.towns[0].troops.pools[0] = 50;
    c.base.start.towns[0].incomeAttributionKnown = true;
    auto a = G::recruit( 42, 1, 2, 2 ); a.id = 7; a.project = 10; a.required = true; c.base.actions = { a }; return c;
}
Q::Result prepare( const G::Context & c, const std::vector<Q::Revisable> & revisions, M::Budget & budget,
                   const D::Registry & defenses = { {}, true }, const R::Registry & risks = { {}, true, {} } )
{
    std::vector<M::Assignment> uses;
    for ( const auto & town : c.base.start.towns ) uses.push_back( { uint64_t( uses.size() + 1 ), town.tile, M::Slot::TownArmy, 1, 15, true, true } );
    return Q::prepare( c, revisions, uses, []( const P::Town & t, auto ) { return M::Value{ true, 100.0 * P::count( t.garrison, Monster::ARCHER ), {} }; }, budget,
        defenses, []( const auto &, const auto & ) { return D::Value{ true, 10000, {} }; }, risks );
}
int main()
{
    size_t n = 0; auto c = context(); M::Budget budget;
    auto out = prepare( c, {}, budget );
    assert( out.status == Q::Status::Blocked && !out.session && out.failures.size() == 1 ); ++n;
    assert( out.failures[0].action == 7 && out.failures[0].day == 2 && out.failures[0].reason == "payment_unavailable" ); ++n;
    assert( budget.projections == 255 ); ++n; // One failed baseline, no speculative score.
    M::Budget releaseBudget; const Q::Revisable revision{ 10, c.base.actions };
    out = prepare( c, { revision }, releaseBudget );
    assert( out.status == Q::Status::Ready && out.session && out.released.size() == 1 && !out.actionable ); ++n;
    assert( out.regenerate && out.released[0].project == 10 && out.released[0].actions == std::vector<uint64_t>{ 7 } ); ++n;
    assert( out.attempts == 2 && releaseBudget.projections == 253 && out.funding.protectedTrajectory ); ++n;
    assert( out.session->context()->base.start.economy.gold == 150 && c.base.actions.size() == 1 && out.session->context()->base.actions.empty() ); ++n;
    // Use the prepared SAME session and shared counters for actual candidate ranking.
    G::Candidate buy; buy.id = 1; buy.actions = { G::recruit( 42, 1, 1, 2 ) };
    auto evaluation = out.session->evaluate( buy );
    assert( evaluation.status == R::Status::Scored && *evaluation.robustUtility == 100 && releaseBudget.projections == 252 ); ++n;
    auto stale = revision; stale.actions[0].count = 1; M::Budget staleBudget;
    out = prepare( c, { stale }, staleBudget ); assert( out.status == Q::Status::Invalid && staleBudget.projections == 256 ); ++n;
    auto partial = c; auto extra = c.base.actions[0]; extra.id = 8; extra.day = 3; partial.base.actions.push_back( extra ); M::Budget partialBudget;
    out = prepare( partial, { revision }, partialBudget ); assert( out.reason == "partial_project_permission" ); ++n;
    M::Budget duplicateBudget; out = prepare( c, { revision, revision }, duplicateBudget ); assert( out.status == Q::Status::Invalid ); ++n;
    auto old = c; old.base.actions[0].day = 0; M::Budget oldBudget;
    out = prepare( old, {}, oldBudget ); assert( out.reason == "invalid_retained_actions" && oldBudget.projections == 256 ); ++n;
    auto good = c; good.base.actions[0].count = 1; M::Budget goodBudget;
    out = prepare( good, { { 10, good.base.actions } }, goodBudget );
    assert( out.status == Q::Status::Ready && out.released.empty() && !out.regenerate && goodBudget.projections == 254 ); ++n;
    // Revision keeps other obligations and never restores the old optional continuation.
    auto active = c; auto fixed = action( 8, 1, P::Kind::Build, 42, BUILD_STATUE ); fixed.project = 20;
    active.base.start.towns[0].catalog[BUILD_STATUE].price[0] = 10; active.base.actions.insert( active.base.actions.begin(), fixed );
    active.active = { c.base.actions[0] }; M::Budget activeBudget;
    out = prepare( active, { revision }, activeBudget );
    assert( out.session && out.session->context()->active.empty() && out.session->context()->base.actions.size() == 1 ); ++n;
    assert( out.session->context()->base.actions[0].id == 8 && out.session->context()->base.start.economy.gold == 150 ); ++n;
    // A retained protected response cannot be erased just because an earlier optional expense caused its failure.
    auto protectedPlan = c; protectedPlan.base.actions[0].count = 1; auto spend = G::recruit( 42, 1, 1, 1 ); spend.id = 8; spend.project = 20;
    protectedPlan.base.actions.insert( protectedPlan.base.actions.begin(), spend ); M::Budget protectedBudget;
    out = prepare( protectedPlan, { { 20, { spend } } }, protectedBudget );
    assert( out.status == Q::Status::Blocked && !out.session && out.released.empty() ); ++n; // Conservative: no subset optimization claimed.
    // Multiple failed soft projects require explicit permission for each, in the same budget.
    auto two = c; auto b = c.base.actions[0]; b.id = 8; b.project = 20; b.day = 3; two.base.actions.push_back( b ); M::Budget twoBudget;
    out = prepare( two, { revision, { 20, { b } } }, twoBudget );
    assert( out.session && out.released.size() == 2 && out.attempts == 3 && twoBudget.projections == 252 ); ++n;
    auto disabled = c; auto build = action( 7, 1, P::Kind::Build, 42, BUILD_TRAINING_GROUNDS ); build.project = 10; disabled.base.actions = { build };
    disabled.base.config.enabled = false; M::Budget disabledBudget;
    out = prepare( disabled, { { 10, { build } } }, disabledBudget );
    assert( out.session && out.released[0].evidence[0].reason == "building_unavailable" ); ++n;
    // Unknown catalog/initial data and horizon failures do not authorize abandonment.
    auto unknown = disabled; unknown.base.start.towns[0].catalog.erase( BUILD_TRAINING_GROUNDS ); M::Budget unknownBudget;
    out = prepare( unknown, { { 10, { build } } }, unknownBudget );
    assert( out.status == Q::Status::Incomplete && out.released.empty() ); ++n;
    auto distant = c; distant.base.actions[0].day = 20; M::Budget distantBudget;
    out = prepare( distant, { { 10, distant.base.actions } }, distantBudget );
    assert( out.status == Q::Status::Incomplete && out.released.empty() ); ++n;
    M::Budget shortBudget; shortBudget.projections = 2; out = prepare( c, { revision }, shortBudget );
    assert( out.status == Q::Status::Limit && !out.session && out.released.size() == 1 && shortBudget.projections == 1 ); ++n;
    M::Budget exhausted; exhausted.projections = 0; out = prepare( c, { revision }, exhausted );
    assert( out.status == Q::Status::Limit && out.released.empty() && out.attempts == 0 ); ++n;
    M::Budget dayBudget; dayBudget.mechanics.days = 0; out = prepare( c, { revision }, dayBudget );
    assert( out.status == Q::Status::Limit && out.released.empty() ); ++n;
    // Two-town known-loss variant: an authorized future purchase loses its destination.
    auto loss = context(); loss.base.start.economy.gold = 10000;
    auto town = loss.base.start.towns[0]; town.tile = 70; auto economic = loss.base.start.economy.towns[0]; economic.tile = 70;
    loss.base.start.towns.push_back( town ); loss.base.start.economy.towns.push_back( economic );
    loss.base.actions[0].town = 70; loss.base.actions[0].day = 4;
    D::Threat threat; threat.id = 1; threat.town = 70; threat.enemyTile = 9; threat.enemyStrength = 1; threat.known = threat.reachable = true; threat.lastSafeDay = 2;
    R::Registry risks{ { { 1, R::Focus::Destination, { { 70, 3 } }, { 1 }, false, "destination lost" } }, true, {} };
    M::Budget lossBudget; out = prepare( loss, { { 10, loss.base.actions } }, lossBudget, { { threat }, true }, risks );
    assert( out.session && out.released.size() == 1 && out.released[0].evidence[0].reason == "required_destination_lost" ); ++n;
    assert( out.released[0].evidence[0].scenario == 1 && lossBudget.projections == 248 ); ++n;
    M::Budget unapproved; out = prepare( loss, {}, unapproved, { { threat }, true }, risks );
    assert( out.status == Q::Status::Blocked && out.released.empty() ); ++n;
    auto noCoverage = risks; noCoverage.coverageComplete = false; M::Budget coverageBudget;
    out = prepare( loss, { { 10, loss.base.actions } }, coverageBudget, { { threat }, true }, noCoverage );
    assert( !out.session && out.status == Q::Status::Incomplete ); ++n;
    // Required payments carried in the observed world are never refunded during revisions.
    assert( loss.base.start.economy.gold == 10000 && loss.base.actions[0].id == 7 ); ++n;
    std::cout << "PASS " << n << " explicit-recourse oracles; failed reference, strict permissions, shared budget, retained commitments and risk\n";
}
