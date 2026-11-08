#include "economy_candidates.h"
#include "economy_trace.h"
#include "kingdom.h"

namespace
{
    std::string actionsJson( const std::vector<AI::EconomyProject::Action> & actions )
    {
        std::ostringstream out;
        out << '[';
        bool first = true;
        for ( const auto & a : actions ) {
            if ( !first ) out << ',';
            first = false;
            out << "{\"day\":" << a.day << ",\"town\":" << a.town << ",\"type\":" << static_cast<int>( a.kind )
                << ",\"building\":" << a.building << ",\"pool\":" << a.pool << ",\"count\":" << a.count
                << ",\"required\":" << ( a.required ? "true" : "false" ) << '}';
        }
        out << ']'; return out.str();
    }
}

AI::EconomyCandidates::Observation AI::EconomyCandidates::observe( const Kingdom & kingdom, const uint32_t throughDay, Budget & budget )
{
    Observation observation;
    observation.context.base = EconomyProject::capture( kingdom, throughDay );
    observation.proposals = generate( observation.context, budget );
    if ( Settings::Get().taxpayerTestDiagnostics() >= 2 && fheroes2::taxpayerSimulationDepth == 0 ) {
        const auto & result = observation.proposals;
        std::ostringstream gaps;
        gaps << '[';
        bool first = true;
        for ( const auto & gap : result.gaps ) { if ( !first ) gaps << ','; first = false; gaps << Economy::jsonQuote( gap ); }
        gaps << ']';
        ECONOMY_TRACE( 2, "CANDIDATE_COVERAGE", ",\"kind\":\"proposal\",\"player\":" << static_cast<int>( kingdom.GetColor() )
                       << ",\"valid_input\":" << ( result.valid ? "true" : "false" ) << ",\"references\":" << result.references.size()
                       << ",\"selected\":" << result.projects.size() << ",\"towns_visited\":" << result.townsVisited
                       << ",\"urgent_dropped\":" << result.urgentDropped << ",\"proposal_budget_left\":" << budget.proposals
                       << ",\"longer_horizon_needed\":" << ( result.longerHorizonNeeded ? "true" : "false" ) << ",\"gaps\":" << gaps.str() );
        const uint64_t batch = Economy::traceState().sequence;
        const char * names[] = { "recruit", "army_upgrade", "production", "fiscal", "training", "supply_wait" };
        static_assert( sizeof( names ) / sizeof( names[0] ) == familyCount );
        for ( size_t i = 0; i < familyCount; ++i ) {
            const auto & counts = result.families[i];
            ECONOMY_TRACE( 2, "CANDIDATE_FAMILY", ",\"kind\":\"proposal\",\"player\":" << static_cast<int>( kingdom.GetColor() )
                           << ",\"batch_seq\":" << batch << ",\"family\":" << Economy::jsonQuote( names[i] ) << ",\"generated\":" << counts.generated
                           << ",\"duplicates\":" << counts.duplicates << ",\"local_dropped\":" << counts.localDropped
                           << ",\"common_dropped\":" << counts.commonDropped << ",\"kept\":" << counts.kept );
        }
        for ( const auto & reason : result.blocked )
            ECONOMY_TRACE( 2, "CANDIDATE_BLOCKED", ",\"kind\":\"proposal\",\"player\":" << static_cast<int>( kingdom.GetColor() )
                           << ",\"batch_seq\":" << batch << ",\"reason\":" << Economy::jsonQuote( reason.first ) << ",\"count\":" << reason.second );
        const auto trace = [&]( const Candidate & c, const bool reference ) {
            ECONOMY_TRACE( 2, "CANDIDATE_PROPOSAL", ",\"kind\":\"proposal\",\"player\":" << static_cast<int>( kingdom.GetColor() )
                           << ",\"batch_seq\":" << batch << ",\"id\":" << c.id << ",\"town\":" << c.town << ",\"families\":" << c.families
                           << ",\"reference\":" << ( reference ? "true" : "false" ) << ",\"label\":" << Economy::jsonQuote( c.label )
                           << ",\"urgent\":" << ( c.urgent ? "true" : "false" ) << ",\"milestone\":" << ( c.milestone ? "true" : "false" )
                           << ",\"deadline\":" << c.deadline << ",\"priority\":" << c.priority
                           << ",\"quick_gain\":" << c.quickGain << ",\"tax_opportunity\":" << c.taxOpportunity << ",\"nominal_gold\":" << c.nominalGold
                           << ",\"actions\":" << actionsJson( c.actions ) );
        };
        for ( const auto & c : result.references ) trace( c, true );
        for ( const auto & c : result.projects ) trace( c, false );
    }
    return observation;
}
