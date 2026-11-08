#include "economy_recourse.h"
#include "economy_trace.h"
AI::EconomyRecourse::Result AI::EconomyRecourse::observe( const G::Context & context, const std::vector<Revisable> & permissions,
    const std::vector<M::Assignment> & assignments, const M::Scorer & scorer, M::Budget & budget, const D::Registry & defenses,
    const D::Scorer & defender, const R::Registry & risks, const int player )
{
    auto out = prepare( context, permissions, assignments, scorer, budget, defenses, defender, risks );
    const char * names[] = { "ready", "blocked", "incomplete", "limit", "invalid" };
    ECONOMY_TRACE( 2, "REFERENCE_PREPARATION", ",\"kind\":\"evaluation\",\"player\":" << player
        << ",\"status\":" << Economy::jsonQuote( names[static_cast<size_t>( out.status )] ) << ",\"reason\":" << Economy::jsonQuote( out.reason )
        << ",\"attempts\":" << out.attempts << ",\"released_projects\":" << out.released.size() << ",\"regenerate\":" << ( out.regenerate ? "true" : "false" )
        << ",\"payments_protected\":" << ( out.funding.protectedTrajectory ? "true" : "false" ) << ",\"actionable\":false"
        << ",\"projections_left\":" << budget.projections << ",\"days_left\":" << budget.mechanics.days
        << ",\"attempts_left\":" << budget.mechanics.attempts << ",\"forces_left\":" << budget.forces );
    const auto parent = Economy::traceState().sequence;
    for ( const auto & release : out.released ) for ( const auto id : release.actions )
        ECONOMY_TRACE( 2, "REFERENCE_RELEASE", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
            << ",\"project\":" << release.project << ",\"action\":" << id );
    for ( const auto & failure : out.failures )
        ECONOMY_TRACE( 2, "REFERENCE_FAILURE", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
            << ",\"scenario\":" << failure.scenario << ",\"action\":" << failure.action << ",\"day\":" << failure.day
            << ",\"reason\":" << Economy::jsonQuote( failure.reason ) );
    return out;
}
