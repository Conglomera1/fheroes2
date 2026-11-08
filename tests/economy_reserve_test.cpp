#include "economy_reserve.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
namespace V = AI::EconomyReserve;
namespace B = AI::EconomyBudget;
namespace R = AI::EconomyRisk;
namespace M = AI::EconomyMilitary;
namespace G = AI::EconomyCandidates;
B::Step payment( uint64_t id, uint32_t day, int64_t gold ) { B::Step s; s.id = s.project = id; s.day = day; s.cost[0] = gold; return s; }
V::EnvelopeInput fixed()
{ V::EnvelopeInput x; x.ledger.today = 1; x.ledger.throughDay = 3; x.fixedFlowsVerified = x.scheduleVerified = true; x.peasantCost = 20; return x; }
R::Result evaluate( const G::Context & c, const G::Candidate & candidate, M::Budget & budget )
{
    R::Session session( c, { { 1, 42, M::Slot::TownArmy, 1, 15, true, true } },
        []( const auto &, auto ) { return M::Value{ true, 0, {} }; }, budget, { {}, true }, {}, { {}, true, {} } );
    return session.evaluate( candidate );
}
int main()
{
    size_t n = 0;
    auto x = fixed(); x.ledger.funds[0] = 1000; x.ledger.steps = { payment( 1, 1, 600 ), payment( 2, 3, 500 ) };
    auto e = V::envelope( x ); assert( e.complete && !e.conservative && e.sufficientAdditional[0] == 100 && e.firstShortfallDay == 3 ); ++n;
    assert( x.ledger.funds[0] == 1000 && x.ledger.mornings.empty() ); ++n;
    x.ledger.steps[1].day = 1; B::Morning m; m.id = 10; m.day = 2; m.income[0] = 1000; x.ledger.mornings = { m };
    e = V::envelope( x ); assert( e.complete && e.sufficientAdditional[0] == 100 && e.firstShortfallDay == 1 ); ++n;
    x = fixed(); x.ledger.steps = { payment( 1, 3, 0 ) }; x.ledger.steps[0].cost = { 1, 2, 3, 4, 5, 6, 7 };
    e = V::envelope( x ); assert( e.complete && e.sufficientAdditional == x.ledger.steps[0].cost ); ++n;
    x.fixedFlowsVerified = false; assert( !V::envelope( x ).complete ); ++n;
    x.fixedFlowsVerified = true; x.scheduleVerified = false; assert( !V::envelope( x ).complete ); ++n;
    x = fixed(); x.training = { { 42, 2, 3 } }; e = V::envelope( x );
    assert( e.complete && e.conservative && e.sufficientAdditional[0] == 600 ); ++n;
    x.config.successRate = 0; x.config.quotas = { 1, 0, 0 }; assert( V::envelope( x ).sufficientAdditional[0] == 600 ); ++n;
    x.config.oddDayLimit = 1; x.config.evenDayLimit = 4; x.config.goldPerPeasant = 137;
    assert( V::envelope( x ).sufficientAdditional[0] == 785 ); ++n;
    x.config.enabled = false; assert( V::envelope( x ).sufficientAdditional[0] == 0 ); ++n;
    x = fixed(); x.ledger.today = 7; x.ledger.throughDay = 9; x.training = { { 42, 8, 9 } };
    assert( V::envelope( x ).sufficientAdditional[0] == 600 ); ++n; // calendar days 1,2 => 2,3
    x.training.push_back( { 42, 9, 9 } ); assert( !V::envelope( x ).complete ); ++n;
    x = fixed(); x.training = { { 42, 2, 3 } }; x.ledger.mornings = { m }; x.ledger.mornings[0].automaticBound[0] = 1;
    assert( !V::envelope( x ).complete ); ++n;
    x = fixed(); x.ledger.steps = { payment( uint64_t{ 1 } << 63, 1, 1 ) }; x.training = { { 42, 2, 3 } };
    assert( V::envelope( x ).complete ); ++n; // generated IDs cannot alias real obligations
    x = fixed(); x.ledger.steps = { payment( 1, 4, 1 ) }; assert( !V::envelope( x ).complete ); ++n;
    x = fixed(); x.config.oddDayLimit = UINT32_MAX; x.training = { { 42, 2, 3 } }; assert( !V::envelope( x ).complete ); ++n;
    x = fixed(); x.ledger.funds[0] = B::resourceMaximum;
    x.ledger.steps = { payment( 1, 1, B::resourceMaximum ), payment( 2, 1, 1 ) };
    assert( !V::envelope( x ).complete ); ++n;
    x = fixed(); x.ledger.steps.resize( B::entryLimit + 1 ); assert( !V::envelope( x ).complete ); ++n;
    x = fixed(); x.ledger.steps = { payment( 1, 1, 5 ), payment( 2, 2, B::resourceMaximum ), payment( 3, 2, 5 ) };
    m = {}; m.id = 10; m.day = 2; m.income[0] = B::resourceMaximum; x.ledger.mornings = { m };
    assert( V::envelope( x ).reason == "capped_flow_envelope_unresolved" ); ++n;
    // No naked score or unverified result certifies that the required payments are protected.
    R::Result empty; assert( !V::assess( empty ).protectedTrajectory ); ++n;
    empty.status = R::Status::Scored; assert( !V::assess( empty ).protectedTrajectory ); ++n;
    G::Context c; c.base = fixture( false, 1 ); c.base.throughDay = 15;
    c.base.actions = { action( 7, 3, P::Kind::Recruit, 42, 1 ) }; c.base.actions[0].pool = 1; c.base.actions[0].required = true;
    c.base.start.economy.towns[0].troops.pools[0] = 10;
    G::Candidate duplicate; duplicate.id = 9; duplicate.actions = c.base.actions;
    auto claims = V::commitments( c, duplicate ); assert( claims.valid && claims.retained.size() == 1 && claims.beyondHorizon.empty() ); ++n;
    duplicate.actions[0].count = 2; assert( !V::commitments( c, duplicate ).valid ); ++n;
    G::Candidate later; later.id = 8; auto future = c.base.actions[0]; future.id = future.project = 8; future.day = 20; later.actions = { future };
    claims = V::commitments( c, later ); assert( claims.valid && claims.retained.size() == 2 && claims.nextMilestone == 20 ); ++n;
    M::Budget goodBudget; G::Candidate noNew; noNew.id = 1;
    auto actual = evaluate( c, noNew, goodBudget ); auto a = V::assess( actual );
    assert( a.protectedTrajectory && a.additionalNeedKnown && a.additionalNeed == P::Resources{} && a.variants.size() == 1 ); ++n;
    assert( goodBudget.projections == 254 ); ++n;
    // Spending now cannot reuse gold retained for a future required purchase.
    c.base.start.economy.nonTaxIncome = c.base.start.economy.taxRate = 0; c.base.start.economy.gold = 150;
    G::Candidate extra; extra.id = 2; extra.actions = { G::recruit( 42, 1, 1, 1 ) };
    M::Budget shared; auto exposed = evaluate( c, extra, shared ); a = V::assess( exposed );
    assert( !a.protectedTrajectory && !a.additionalNeedKnown && a.variants[0].failedAction == 7 && a.variants[0].failedDay == 3 ); ++n;
    assert( c.base.start.economy.gold == 150 && shared.projections == 254 ); ++n;
    x = fixed(); x.ledger.funds[0] = 150; x.ledger.steps = { payment( 1, 1, 150 ), payment( 7, 3, 150 ) };
    auto refinement = V::refine( exposed, { { 0, x } } );
    assert( refinement.envelopeKnown && refinement.sufficientAdditional[0] == 150 && !refinement.actual.protectedTrajectory && !refinement.actionable ); ++n;
    assert( shared.projections == 254 ); ++n; // accounting refinement gives NO extra projection credit
    auto unknown = exposed; unknown.status = R::Status::Incomplete;
    assert( !V::refine( unknown, { { 0, x } } ).envelopeKnown ); ++n;
    assert( !V::refine( exposed, {} ).envelopeKnown ); ++n;
    auto limit = exposed; limit.status = R::Status::Limit;
    assert( !V::refine( limit, { { 0, x } } ).envelopeKnown ); ++n;
    auto other = exposed; other.variants.push_back( other.variants[0] ); other.variants[1].scenario = 4;
    assert( !V::refine( other, { { 0, x } } ).envelopeKnown ); ++n;
    auto second = x; second.ledger.steps[1].cost[1] = 8;
    refinement = V::refine( other, { { 0, x }, { 4, second } } );
    assert( refinement.envelopeKnown && refinement.sufficientAdditional[0] == 150 && refinement.sufficientAdditional[1] == 8 ); ++n;
    second.ledger.funds[0] = 0;
    assert( !V::refine( other, { { 0, x }, { 4, second } } ).envelopeKnown ); ++n;
    auto tooMany = exposed; tooMany.variants.resize( 4 );
    assert( !V::assess( tooMany ).protectedTrajectory && !V::refine( tooMany, {} ).envelopeKnown ); ++n;
    // An exact affordable trajectory wins over an excessive bound. Success ratio is irrelevant to fees.
    c = {}; c.base = fixture( true, 1 ); c.base.throughDay = 15;
    c.base.start.economy.nonTaxIncome = c.base.start.economy.taxRate = 0;
    c.base.start.towns[0].garrison[0].count = c.base.start.economy.towns[0].troops.garrison = 1;
    c.base.start.economy.towns[0].troops.hut = 0;
    c.base.start.towns[0].catalog.at( BUILD_STATUE ).price[0] = 99;
    c.base.actions = { action( 7, 2, P::Kind::Build, 42, BUILD_STATUE ) }; c.base.actions[0].required = true;
    G::Candidate delayed; delayed.id = 2; delayed.actions = c.base.actions; c.base.actions.clear();
    c.base.start.economy.gold = 99; M::Budget scarce;
    actual = evaluate( c, delayed, scarce ); assert( V::assess( actual ).protectedTrajectory ); ++n;
    x = fixed(); x.ledger.throughDay = 15; x.ledger.funds[0] = 99; x.ledger.steps = { payment( 7, 2, 99 ) }; x.training = { { 42, 2, 15 } };
    assert( V::envelope( x ).sufficientAdditional[0] > 0 ); ++n;
    refinement = V::refine( actual, { { 0, x } } );
    assert( refinement.envelopeKnown && refinement.actual.protectedTrajectory && refinement.sufficientAdditional == P::Resources{} ); ++n;
    // One extra coin triggers a formation, breaking the same later payment: never binary-search gold.
    c.base.start.economy.gold = 100; M::Budget triggered; auto changed = evaluate( c, delayed, triggered );
    assert( !V::assess( changed ).protectedTrajectory && changed.variants[0].evaluation.financial.failedDay == 2 ); ++n;
    assert( scarce.projections == triggered.projections && triggered.projections == 254 ); ++n;
    // A horizon/coverage limit does not turn an observed minimum into spendable cash.
    M::Budget futureBudget; actual = evaluate( c, later, futureBudget ); assert( !V::assess( actual ).additionalNeedKnown ); ++n;
    M::Budget exhausted; exhausted.projections = 0; actual = evaluate( c, noNew, exhausted );
    assert( !V::assess( actual ).protectedTrajectory && exhausted.projections == 0 ); ++n;
    std::cout << "PASS " << n << " reserve/commitment oracles (real funds, conditional envelope, calendar, identities and non-monotone fees)\n";
}
