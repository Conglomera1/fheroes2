#include "economy_pipeline.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
namespace O = AI::EconomyPipeline;
namespace I = AI::EconomyInputs;
namespace M = AI::EconomyMilitary;
namespace D = AI::EconomyDefense;
namespace G = AI::EconomyCandidates;
G::Context context( uint32_t through = 15 )
{ G::Context c; c.base = fixture( false, 1 ); c.base.throughDay = through; c.base.start.towns[0].incomeAttributionKnown = true; c.base.start.economy.towns[0].troops.pools[0] = 50; return c; }
M::Value force( const P::Town & town, M::Slot )
{ return { true, 100.0 * ( P::count( town.garrison, Monster::ARCHER ) + P::count( town.garrison, Monster::RANGER ) ), {} }; }
D::Value defend( const P::Town &, const D::Threat & ) { return { true, 10000, {} }; }
I::Enemy enemy()
{ I::Enemy e; e.observation.enemyTile = 9; e.observation.enemyHero = 2; e.observation.enemyStrength = 100; e.observation.known = true; e.mobile = true; e.movePoints = 1000; return e; }
int main()
{
    size_t n = 0; O::Work work;
    auto frame = I::assemble( context(), {}, { 42 }, true, {}, work.paths );
    auto result = O::evaluate( frame, work, force, defend );
    assert( result.searched && result.selection.certified && result.selection.selected && !result.actionable && !result.selection.actionable ); ++n;
    assert( result.preparation.session && result.preparation.released.empty() && work.evaluation.projections < 256 ); ++n;
    assert( frame.context.base.start.economy.gold == 10000 && result.frame.context.base.start.economy.gold == 10000 ); ++n;
    auto uncommitted = context(); uncommitted.base.start.towns[0].hero = 5;
    O::Work visitorWork; frame = I::assemble( uncommitted, {}, { 42 }, true, {}, visitorWork.paths );
    result = O::evaluate( frame, visitorWork, force, defend );
    assert( !result.searched && result.status == O::Status::Held && visitorWork.evaluation.projections == 256 && visitorWork.generation.proposals == 16384 ); ++n;
    assert( result.gaps.count( "visitor_assignment_not_committed" ) ); ++n;
    O::Work pinned; frame = I::assemble( uncommitted, {}, { 42 }, true, {}, pinned.paths, { 5 } );
    assert( frame.valid && frame.assignments[0].verified && frame.gaps.empty() ); ++n;
    result = O::evaluate( frame, pinned, force, defend ); assert( result.searched && result.selection.certified ); ++n;
    O::Work foreign; frame = I::assemble( uncommitted, {}, { 42 }, true, {}, foreign.paths, { 6 } );
    result = O::evaluate( frame, foreign, force, defend ); assert( result.status == O::Status::Invalid && !result.searched && foreign.evaluation.projections == 256 ); ++n;
    O::Work incomplete; frame = I::assemble( context(), { enemy() }, { 42 }, true, []( const auto &, auto ) { return 0U; }, incomplete.paths );
    result = O::evaluate( frame, incomplete, force, defend );
    assert( result.status == O::Status::Held && !result.searched && incomplete.evaluation.projections == 256 ); ++n;
    // Enlarge only the horizon, preserving exact state/INI and cached path evidence.
    O::Work extension; frame = I::assemble( context(), { enemy() }, { 42 }, true, []( const auto &, auto ) { return 16000U; }, extension.paths );
    assert( frame.outsideHorizon == 1 && frame.routes.size() == 1 && extension.paths.paths == 127 ); ++n;
    const auto wide = I::widen( frame, extension.paths );
    assert( wide.valid && wide.context.base.throughDay == 29 && wide.defenses.threats[0].lastSafeDay == 16 && wide.queried == 0 && extension.paths.paths == 127 ); ++n;
    assert( wide.context.base.start.economy.gold == frame.context.base.start.economy.gold && wide.context.base.config.goldPerPeasant == frame.context.base.config.goldPerPeasant ); ++n;
    result = O::evaluate( frame, extension, force, defend );
    assert( result.widened && result.frame.context.base.throughDay == 29 && result.generationPasses == 2 && extension.paths.paths == 127 ); ++n;
    // This last-town loss is unsupported by risk: no score is fabricated just to proceed.
    assert( result.status == O::Status::Held && !result.searched && !result.selection.certified ); ++n;
    I::Work invalidWide; auto noWiden = I::widen( wide, invalidWide ); assert( !noWiden.valid && invalidWide.paths == 128 ); ++n;
    O::Work noRoute; frame = I::assemble( context(), { enemy() }, { 42 }, true, []( const auto &, auto ) { return 0U; }, noRoute.paths );
    auto stillUnknown = I::widen( frame, noRoute.paths );
    assert( !stillUnknown.defenses.coverageComplete && stillUnknown.queried == 0 && noRoute.paths.paths == 127 ); ++n;
    // Exhausted current quota does not erase a route already proven in this frozen frame.
    extension.paths.paths = 0; auto retained = I::widen( I::assemble( context(), {}, { 42 }, true, {}, invalidWide ), extension.paths );
    assert( retained.valid && retained.defenses.coverageComplete && extension.paths.paths == 0 ); ++n;
    auto unpaid = context(); unpaid.base.start.economy.gold = 0; unpaid.base.start.economy.nonTaxIncome = 0; unpaid.base.start.economy.taxRate = 0;
    unpaid.base.actions = { action( 7, 1, P::Kind::Recruit, 42, 1 ) }; unpaid.base.actions[0].pool = 1;
    O::Work payments; frame = I::assemble( unpaid, {}, { 42 }, true, {}, payments.paths );
    result = O::evaluate( frame, payments, force, defend );
    assert( result.status == O::Status::Held && !result.searched && result.preparation.released.empty() && result.preparation.reason == "failed_project_not_revisable" ); ++n;
    O::Work exhausted; exhausted.evaluation.projections = 0; frame = I::assemble( context(), {}, { 42 }, true, {}, exhausted.paths );
    result = O::evaluate( frame, exhausted, force, defend ); assert( !result.searched && result.status == O::Status::Held && exhausted.evaluation.projections == 0 ); ++n;
    O::Work generation; generation.generation.proposals = 0; frame = I::assemble( context(), {}, { 42 }, true, {}, generation.paths );
    result = O::evaluate( frame, generation, force, defend );
    assert( result.proposals.projects.empty() && generation.generation.proposals == 0 ); ++n;
    O::Work repeat; frame = I::assemble( context(), {}, { 42 }, true, {}, repeat.paths );
    const auto repeated = O::evaluate( frame, repeat, force, defend );
    assert( repeated.searched && repeated.selection.certified && repeat.evaluation.projections == work.evaluation.projections && repeat.search.merges == work.search.merges ); ++n;
    std::cout << "PASS " << n << " observation-pipeline oracles (gating, shared budget, horizon, cached evidence and stationary use)\n";
}
