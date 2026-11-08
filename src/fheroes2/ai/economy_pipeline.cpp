#include "economy_pipeline.h"
#include "economy_trace.h"
#include "kingdom.h"

AI::EconomyPipeline::Result AI::EconomyPipeline::observe( I::Frame frame, const Kingdom & kingdom, Work & work )
{
    const int player = static_cast<int>( kingdom.GetColor() );
    auto out = evaluate( std::move( frame ), work,
        [&]( const auto & town, auto slot ) { return M::localArmyValue( kingdom, town, slot ); },
        [&]( const auto & town, const auto & threat ) { return D::localDefenseValue( kingdom, town, threat ); },
        [&]( auto & session, const auto & proposals, auto & budget, auto & search ) { return T::observe( session, proposals, budget, search, player ); } );
    if ( out.widened ) I::log( out.frame, player, work.paths );
    const char * names[] = { "observed", "partial", "held", "invalid" };
    ECONOMY_TRACE( 2, "PIPELINE_EVALUATION", ",\"kind\":\"evaluation\",\"player\":" << player
        << ",\"status\":" << Economy::jsonQuote( names[static_cast<size_t>( out.status )] ) << ",\"reason\":" << Economy::jsonQuote( out.reason )
        << ",\"actionable\":false,\"searched\":" << ( out.searched ? "true" : "false" ) << ",\"widened\":" << ( out.widened ? "true" : "false" )
        << ",\"through_day\":" << out.frame.context.base.throughDay << ",\"generation_passes\":" << out.generationPasses
        << ",\"proposals\":" << out.proposals.projects.size() << ",\"preparation_attempts\":" << out.preparation.attempts
        << ",\"projections_left\":" << work.evaluation.projections << ",\"days_left\":" << work.evaluation.mechanics.days
        << ",\"training_attempts_left\":" << work.evaluation.mechanics.attempts << ",\"forces_left\":" << work.evaluation.forces
        << ",\"paths_left\":" << work.paths.paths << ",\"generation_left\":" << work.generation.proposals << ",\"merges_left\":" << work.search.merges );
    const auto parent = Economy::traceState().sequence;
    for ( const auto & gap : out.gaps )
        ECONOMY_TRACE( 2, "PIPELINE_GAP", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent << ",\"gap\":" << Economy::jsonQuote( gap ) );
    return out;
}
