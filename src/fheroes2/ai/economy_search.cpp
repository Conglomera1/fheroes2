#include "economy_search.h"
#include "economy_trace.h"

AI::EconomySearch::Result AI::EconomySearch::observe( R::Session & evaluator, const G::Result & proposals, M::Budget & budget, Work & work, const int player )
{
    auto result = search( evaluator, proposals, budget, work );
    const auto optional = []( const auto & value ) { std::ostringstream out; out << std::setprecision( 17 ); if ( value ) out << *value; else out << "null"; return out.str(); };
    const auto planId = [&]( const std::optional<size_t> index ) -> std::optional<uint64_t> { if ( index ) return result.plans[*index].candidate.id; return {}; };
    const auto strings = []( const std::set<std::string> & values ) { std::ostringstream out; out << '['; bool first = true; for ( const auto & value : values ) { if ( !first ) out << ','; first = false; out << Economy::jsonQuote( value ); } out << ']'; return out.str(); };
    const char * statuses[] = { "ready", "partial", "no_verified_plan", "invalid" };
    ECONOMY_TRACE( 2, "SEARCH_EVALUATION", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"status\":" << Economy::jsonQuote( statuses[static_cast<size_t>( result.status )] )
                   << ",\"best\":" << optional( planId( result.best ) ) << ",\"ordinary\":" << optional( planId( result.ordinary ) )
                   << ",\"active\":" << optional( planId( result.active ) ) << ",\"reference\":" << optional( planId( result.noInvestment ) )
                   << ",\"attempts\":" << result.counts.attempts << ",\"evaluations\":" << result.counts.evaluations << ",\"cache_hits\":" << result.counts.cacheHits
                   << ",\"merge_rejected\":" << result.counts.rejectedMerges << ",\"unranked\":" << result.counts.unranked << ",\"beam_dropped\":" << result.counts.droppedBeam
                   << ",\"substitutions\":" << result.counts.substitutions << ",\"input_omitted\":" << result.counts.omittedInputs << ",\"passes\":" << result.passes
                   << ",\"stop\":" << Economy::jsonQuote( result.stop ) << ",\"gaps\":" << strings( result.gaps )
                   << ",\"projections_left\":" << budget.projections << ",\"days_left\":" << budget.mechanics.days << ",\"attempts_left\":" << budget.mechanics.attempts
                   << ",\"forces_left\":" << budget.forces << ",\"merges_left\":" << work.merges << ",\"longer_horizon_needed\":" << ( result.longerHorizonNeeded ? "true" : "false" ) << ",\"actionable\":false" );
    const auto parent = Economy::traceState().sequence;
    for ( size_t i = 0; i < result.sources.size(); ++i ) {
        const auto & source = result.sources[i];
        ECONOMY_TRACE( 2, "SEARCH_SOURCE", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent << ",\"index\":" << i
                       << ",\"source\":" << source.id << ",\"families\":" << source.families << ",\"label\":" << Economy::jsonQuote( source.label )
                       << ",\"active\":" << ( source.active ? "true" : "false" ) );
    }
    for ( const auto & plan : result.plans ) {
        ECONOMY_TRACE( 2, "SEARCH_PLAN", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent << ",\"id\":" << plan.candidate.id
                       << ",\"verified\":" << ( plan.score.verified ? "true" : "false" ) << ",\"ordinary\":" << ( plan.ordinary ? "true" : "false" )
                       << ",\"active\":" << ( plan.active ? "true" : "false" ) << ",\"families\":" << plan.candidate.families
                       << ",\"payments_protected\":" << ( plan.score.reserve.protectedTrajectory ? "true" : "false" )
                       << ",\"reserve_reason\":" << Economy::jsonQuote( plan.score.reserve.reason )
                       << ",\"additional_need_known\":" << ( plan.score.reserve.additionalNeedKnown ? "true" : "false" )
                       << ",\"robust_utility\":" << optional( plan.score.robust ) << ",\"central_utility\":" << optional( plan.score.central )
                       << ",\"immediate_gold\":" << plan.score.immediateGold << ",\"variants\":" << plan.score.variants << ",\"gaps\":" << strings( plan.score.gaps )
                       << ",\"D\":" << [&]() { if ( !plan.score.defenseComplete ) return std::string( "null" ); std::ostringstream out; out << '['; for ( size_t i = 0; i < plan.score.defenses.size(); ++i ) { if ( i ) out << ','; out << ( plan.score.defenses[i] ? "true" : "false" ); } out << ']'; return out.str(); }()
                       << ",\"members\":[" << [&]() { std::ostringstream out; for ( size_t i = 0; i < plan.members.size(); ++i ) { if ( i ) out << ','; out << plan.members[i]; } return out.str(); }() << ']'
                       << ",\"actions\":[" << [&]() { std::ostringstream out; for ( size_t i = 0; i < plan.candidate.actions.size(); ++i ) { if ( i ) out << ','; const auto & a = plan.candidate.actions[i]; out << "{\"day\":" << a.day << ",\"town\":" << a.town << ",\"type\":" << static_cast<int>( a.kind ) << ",\"building\":" << a.building << ",\"pool\":" << a.pool << ",\"count\":" << a.count << ",\"required\":" << ( a.required ? "true" : "false" ) << '}'; } return out.str(); }() << ']' );
        for ( const auto & witness : plan.score.witnesses )
            ECONOMY_TRACE( 2, "SEARCH_VARIANT", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent << ",\"id\":" << plan.candidate.id
                           << ",\"scenario\":" << witness.scenario << ",\"funded\":" << ( witness.funded ? "true" : "false" )
                           << ",\"failed_day\":" << witness.failedDay << ",\"failed_action\":" << witness.failedAction << ",\"reason\":" << Economy::jsonQuote( witness.reason )
                           << ",\"cancelled\":" << witness.cancelled << ",\"cancelled_required\":" << witness.required
                           << ",\"minimum_funds\":" << ( witness.minimum ? Economy::arrayJson( *witness.minimum ) : "null" ) );
    }
    return result;
}
