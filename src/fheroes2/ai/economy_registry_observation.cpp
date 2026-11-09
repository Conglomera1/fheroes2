#include "economy_registry_observation.h"
#include "economy_trace.h"

void AI::EconomyRegistryObservation::log( const Result & result, const int player )
{
    ECONOMY_TRACE( 2, "REGISTRY_OBSERVATION", ",\"kind\":\"evaluation\",\"player\":" << player
        << ",\"scope\":\"hypothetical_snapshot_only\",\"status\":" << Economy::jsonQuote( name( result.status ) )
        << ",\"reason\":" << Economy::jsonQuote( result.reason ) << ",\"token\":" << result.token << ",\"selected_plan\":" << result.selectedPlan
        << ",\"programs\":" << result.programs << ",\"pending_actions\":" << ( result.registry ? result.registry->pending() : 0 )
        << ",\"revision\":" << ( result.registry ? result.registry->revision() : 0 )
        << ",\"live_commitments\":false,\"retained_between_turns\":false,\"actual_receipts\":0,\"reference_replaced\":false,\"actionable\":false" );
    const auto parent = Economy::traceState().sequence;
    for ( const auto & link : result.links )
        ECONOMY_TRACE( 2, "REGISTRY_SOURCE_LINK", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
            << ",\"registry_action\":" << link.action << ",\"source_action\":" << link.sourceAction
            << ",\"source_project\":" << link.sourceProject << ",\"source_index\":" << link.sourceIndex );
    if ( result.registry ) {
        const auto imported = result.registry->view();
        if ( imported.context ) for ( const auto & action : imported.context->base.actions )
            ECONOMY_TRACE( 2, "REGISTRY_PENDING_ACTION", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                << ",\"id\":" << action.id << ",\"project\":" << action.project << ",\"day\":" << action.day << ",\"town\":" << action.town
                << ",\"operation\":" << static_cast<int>( action.kind ) << ",\"building\":" << action.building << ",\"pool\":" << action.pool
                << ",\"count\":" << action.count << ",\"required\":" << ( action.required ? "true" : "false" ) << ",\"abandonment_authorized\":false" );
    }
}
