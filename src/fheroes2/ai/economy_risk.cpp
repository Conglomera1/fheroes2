#include "economy_risk.h"
#include "economy_trace.h"

AI::EconomyRisk::Result AI::EconomyRisk::evaluateObserved( Session & session, const G::Candidate & candidate, const int player, const M::Budget & budget )
{
    auto result = session.evaluate( candidate );
    const char * statuses[] = { "scored", "exposed", "incomplete", "limit", "invalid" };
    const auto number = []( const auto & value ) { std::ostringstream out; out << std::setprecision( 17 ); if ( value ) out << *value; else out << "null"; return out.str(); };
    std::ostringstream gaps; gaps << '['; bool first = true;
    for ( const auto & gap : result.gaps ) { if ( !first ) gaps << ','; first = false; gaps << Economy::jsonQuote( gap ); }
    gaps << ']';
    ECONOMY_TRACE( 2, "RISK_EVALUATION", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"candidate\":" << candidate.id
                   << ",\"status\":" << Economy::jsonQuote( statuses[static_cast<size_t>( result.status )] )
                   << ",\"robust_utility\":" << number( result.robustUtility ) << ",\"observed_minimum\":" << number( result.observedMinimum )
                   << ",\"worst_utility_scenario\":" << number( result.worstUtility ) << ",\"worst_funding_scenario\":" << number( result.worstFunding )
                   << ",\"tightest_resource_scenarios\":[" << [&]() { std::ostringstream out; for ( size_t i = 0; i < 7; ++i ) { if ( i ) out << ','; out << number( result.tightestResource[i] ); } return out.str(); }() << ']'
                   << ",\"omitted\":" << result.omitted << ",\"gaps\":" << gaps.str() << ",\"projection_budget_left\":" << budget.projections
                   << ",\"force_budget_left\":" << budget.forces );
    const auto parent = Economy::traceState().sequence;
    for ( const auto & variant : result.variants ) {
        const auto & value = variant.evaluation;
        ECONOMY_TRACE( 2, "RISK_VARIANT", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"candidate\":" << candidate.id
                       << ",\"evaluation_seq\":" << parent << ",\"scenario\":" << variant.scenario << ",\"label\":" << Economy::jsonQuote( variant.label )
                       << ",\"defense_complete\":" << ( value.defense.complete() ? "true" : "false" ) << ",\"D\":[" << [&]() { std::ostringstream out; for ( size_t i = 0; i < value.defense.cities.size(); ++i ) { if ( i ) out << ','; const auto v = value.defense.cities[i].viable; out << ( v ? ( *v ? "true" : "false" ) : "null" ); } return out.str(); }() << ']'
                       << ",\"funded\":" << ( value.financial.covered() ? "true" : "false" ) << ",\"utility\":" << number( value.utility )
                       << ",\"minimum_funds\":" << ( variant.minimumFunds ? Economy::arrayJson( *variant.minimumFunds ) : "null" )
                       << ",\"first_payment_gap\":" << Economy::arrayJson( value.financial.budget.extraNeeded )
                       << ",\"cancelled_required\":[" << [&]() { std::ostringstream out; for ( size_t i = 0; i < value.financial.cancelledRequiredActions.size(); ++i ) { if ( i ) out << ','; out << value.financial.cancelledRequiredActions[i]; } return out.str(); }() << ']'
                       << ",\"failed_day\":" << value.financial.failedDay << ",\"failed_action\":" << value.financial.failedAction
                       << ",\"reason\":" << Economy::jsonQuote( value.financial.reason ) << ",\"cancelled\":[" << [&]() {
                            std::ostringstream out; for ( size_t i = 0; i < value.financial.cancelledActions.size(); ++i ) { if ( i ) out << ','; out << value.financial.cancelledActions[i]; } return out.str(); }() << ']' );
        for ( const auto & day : value.financial.days ) {
            const auto strength = std::find_if( value.days.begin(), value.days.end(), [&]( const auto & row ) { return row.day == day.morning.day; } );
            const auto known = [&]( double M::Row::* field ) -> std::optional<double> { if ( strength == value.days.end() ) return {}; return ( *strength ).*field; };
            ECONOMY_TRACE( 2, "RISK_DAY", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"candidate\":" << candidate.id
                           << ",\"evaluation_seq\":" << parent << ",\"scenario\":" << variant.scenario << ",\"projected_day\":" << day.morning.day
                           << ",\"funds\":" << Economy::arrayJson( day.afterActions ) << ",\"tax\":" << day.morning.tax << ",\"fees\":" << day.morning.fees
                           << ",\"baseline_force\":" << number( known( &M::Row::baseline ) ) << ",\"proposed_force\":" << number( known( &M::Row::proposed ) )
                           << ",\"difference\":" << number( known( &M::Row::difference ) ) );
        }
        for ( const auto & loss : value.financial.appliedLosses )
            ECONOMY_TRACE( 2, "RISK_TOWN_LOSS", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"candidate\":" << candidate.id
                           << ",\"evaluation_seq\":" << parent << ",\"scenario\":" << variant.scenario << ",\"town\":" << loss.town
                           << ",\"first_unavailable_day\":" << loss.firstUnavailableDay );
    }
    return result;
}
