#include "economy_stability.h"
#include "economy_trace.h"

AI::EconomyStability::Result AI::EconomyStability::observe( R::Session & evaluator, const G::Result & proposals, M::Budget & budget,
                                                         S::Work & work, const int player )
{
    auto result = select( evaluator, proposals, budget, work );
    const auto optional = []( const auto & value ) { std::ostringstream out; out << std::setprecision( 17 ); if ( value ) out << *value; else out << "null"; return out.str(); };
    ECONOMY_TRACE( 2, "STABILITY_EVALUATION", ",\"kind\":\"evaluation\",\"player\":" << player
                   << ",\"selected_index\":" << optional( result.selected ) << ",\"fallback_index\":" << optional( result.fallback )
                   << ",\"current_index\":" << optional( result.current ) << ",\"certified\":" << ( result.certified ? "true" : "false" )
                   << ",\"actionable\":false,\"reserved\":" << result.reserved << ",\"proof_evaluations\":" << result.evaluations
                   << ",\"proof_cache_hits\":" << result.cacheHits << ",\"search_evaluations\":" << result.search.counts.evaluations
                   << ",\"search_stop\":" << Economy::jsonQuote( result.search.stop ) << ",\"projections_left\":" << budget.projections
                   << ",\"days_left\":" << budget.mechanics.days << ",\"attempts_left\":" << budget.mechanics.attempts
                   << ",\"forces_left\":" << budget.forces << ",\"merges_left\":" << work.merges );
    const auto parent = Economy::traceState().sequence;
    for ( size_t i = 0; i < result.search.plans.size(); ++i ) {
        const auto & plan = result.search.plans[i];
        ECONOMY_TRACE( 2, "STABILITY_PLAN", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                       << ",\"index\":" << i << ",\"id\":" << plan.candidate.id << ",\"verified\":" << ( plan.score.verified ? "true" : "false" )
                       << ",\"payments_protected\":" << ( plan.score.reserve.protectedTrajectory ? "true" : "false" )
                       << ",\"reserve_reason\":" << Economy::jsonQuote( plan.score.reserve.reason )
                       << ",\"active\":" << ( plan.active ? "true" : "false" ) << ",\"robust\":" << optional( plan.score.robust )
                       << ",\"central\":" << optional( plan.score.central )
                       << ",\"D\":" << [&]() { if ( !plan.score.defenseComplete ) return std::string( "null" ); std::ostringstream out; out << '[';
                            for ( size_t j = 0; j < plan.score.defenses.size(); ++j ) { if ( j ) out << ','; out << ( plan.score.defenses[j] ? "true" : "false" ); }
                            out << ']'; return out.str(); }()
                       << ",\"actions\":[" << [&]() { std::ostringstream out; for ( size_t j = 0; j < plan.candidate.actions.size(); ++j ) {
                            if ( j ) out << ',';
                            const auto & a = plan.candidate.actions[j];
                            out << "{\"day\":" << a.day << ",\"town\":" << a.town << ",\"kind\":" << static_cast<int>( a.kind )
                                << ",\"building\":" << a.building << ",\"pool\":" << a.pool << ",\"count\":" << a.count << '}';
                        } return out.str(); }() << ']' );
        for ( const auto & gap : plan.score.gaps )
            ECONOMY_TRACE( 2, "STABILITY_GAP", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                           << ",\"plan_index\":" << i << ",\"gap\":" << Economy::jsonQuote( gap ) );
        for ( const auto & witness : plan.score.witnesses )
            ECONOMY_TRACE( 2, "STABILITY_PLAN_VARIANT", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                           << ",\"plan_index\":" << i << ",\"scenario\":" << witness.scenario << ",\"funded\":" << ( witness.funded ? "true" : "false" )
                           << ",\"failed_day\":" << witness.failedDay << ",\"failed_action\":" << witness.failedAction
                           << ",\"reason\":" << Economy::jsonQuote( witness.reason ) << ",\"required_cancelled\":" << witness.required
                           << ",\"minimum_funds\":" << ( witness.minimum ? Economy::arrayJson( *witness.minimum ) : "null" ) );
    }
    for ( const auto & proof : result.proofs ) {
        ECONOMY_TRACE( 2, "STABILITY_PROOF", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                       << ",\"plan_index\":" << proof.plan << ",\"action_index\":" << proof.action << ",\"town\":" << proof.town
                       << ",\"day\":" << proof.day << ",\"building\":" << proof.building << ",\"accepted\":" << ( proof.decision.accepted ? "true" : "false" )
                       << ",\"reason\":" << Economy::jsonQuote( proof.decision.reason ) << ",\"gain\":" << proof.decision.gain << ",\"margin\":" << proof.decision.margin
                       << ",\"cached\":" << ( proof.cached ? "true" : "false" ) << ",\"without_verified\":" << ( proof.without.verified ? "true" : "false" )
                       << ",\"without_robust\":" << optional( proof.without.robust ) << ",\"without_central\":" << optional( proof.without.central ) );
        for ( const auto & repair : proof.repairs )
            ECONOMY_TRACE( 2, "STABILITY_REPAIR", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                           << ",\"plan_index\":" << proof.plan << ",\"action_index\":" << proof.action << ",\"town\":" << repair.town
                           << ",\"day\":" << repair.day << ",\"pool\":" << repair.pool << ",\"requested\":" << repair.requested << ",\"kept\":" << repair.kept );
        for ( const auto & witness : proof.without.witnesses )
            ECONOMY_TRACE( 2, "STABILITY_VARIANT", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                           << ",\"plan_index\":" << proof.plan << ",\"action_index\":" << proof.action << ",\"scenario\":" << witness.scenario
                           << ",\"funded\":" << ( witness.funded ? "true" : "false" ) << ",\"failed_day\":" << witness.failedDay
                           << ",\"failed_action\":" << witness.failedAction << ",\"reason\":" << Economy::jsonQuote( witness.reason )
                           << ",\"required_cancelled\":" << witness.required
                           << ",\"minimum_funds\":" << ( witness.minimum ? Economy::arrayJson( *witness.minimum ) : "null" ) );
    }
    for ( const auto & rejected : result.rejected )
        ECONOMY_TRACE( 2, "STABILITY_REJECTED", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                       << ",\"plan_index\":" << rejected.plan << ",\"reason\":" << Economy::jsonQuote( rejected.reason ) );
    for ( const auto & gap : result.gaps )
        ECONOMY_TRACE( 2, "STABILITY_GAP", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                       << ",\"plan_index\":null,\"gap\":" << Economy::jsonQuote( gap ) );
    return result;
}
