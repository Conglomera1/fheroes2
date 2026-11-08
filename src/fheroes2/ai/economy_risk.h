// Conditional risk variants, never probabilities or live actions.
#pragma once
#include "economy_military.h"

namespace AI::EconomyRisk
{
    namespace P = EconomyProject;
    namespace G = EconomyCandidates;
    namespace M = EconomyMilitary;
    namespace D = EconomyDefense;
    enum class Focus { Income, Destination };
    struct Scenario
    {
        uint64_t id = 0;
        Focus focus = Focus::Income;
        std::vector<P::TownLoss> losses; // Jointly justified losses; alternatives from one enemy stay in separate scenarios.
        std::vector<uint64_t> evidence; // IDs of known D threats, not invented future enemies.
        bool linkedLossesVerified = false;
        std::string label;
        uint32_t preselectionPriority = 0; // Supplied exposure rank, lower first; no universal gold-to-force weight.
    };
    struct Registry
    {
        std::vector<Scenario> scenarios;
        bool coverageComplete = false;
        std::set<std::string> gaps; // Includes unsupported blocked routes/other recurring sources; never silently erase them.
    };
    enum class Status { Scored, Exposed, Incomplete, Limit, Invalid };
    struct Variant { uint64_t scenario = 0; std::string label; M::Result evaluation; std::optional<P::Resources> minimumFunds = {}; };
    struct Result
    {
        Status status = Status::Incomplete;
        std::vector<Variant> variants; // Central first, then at most two justified variants.
        std::optional<double> robustUtility, observedMinimum;
        std::optional<uint64_t> worstUtility, worstFunding;
        std::array<std::optional<uint64_t>, 7> tightestResource;
        std::set<std::string> gaps;
        size_t omitted = 0;
    };

    class Session
    {
    public:
        Session( const G::Context & context, const std::vector<M::Assignment> & assignments, M::Scorer scorer, M::Budget & budget,
                 const D::Registry & defenses, D::Scorer defender, Registry registry )
        {
            if ( !context.base.losses.empty() ) { invalid_.insert( "central_state_already_perturbed" ); return; }
            if ( registry.scenarios.size() > 1024 ) { invalid_.insert( "risk_input_limit" ); return; }
            size_t entries = registry.scenarios.size();
            constexpr size_t inputEntryLimit = 4096;
            for ( const auto & scenario : registry.scenarios ) {
                if ( scenario.losses.size() > inputEntryLimit - entries ) { invalid_.insert( "risk_total_input_limit" ); return; }
                entries += scenario.losses.size();
                if ( scenario.evidence.size() > inputEntryLimit - entries ) { invalid_.insert( "risk_total_input_limit" ); return; }
                entries += scenario.evidence.size();
            }
            if ( defenses.threats.size() > 1024 || context.base.start.towns.size() > 256 ) { invalid_.insert( "risk_context_limit" ); return; }
            std::map<uint64_t, const D::Threat *> knownThreats;
            for ( const auto & threat : defenses.threats ) knownThreats.emplace( threat.id, &threat );
            std::set<int32_t> knownTowns;
            for ( const auto & town : context.base.start.towns ) knownTowns.insert( town.tile );
            gaps_ = std::move( registry.gaps );
            if ( !registry.coverageComplete ) gaps_.insert( "risk_coverage_not_certified" );
            if ( !defenses.coverageComplete ) gaps_.insert( "known_threat_coverage_not_certified" );
            std::set<uint64_t> ids;
            std::set<std::string> signatures;
            std::vector<Scenario> unique;
            for ( auto scenario : registry.scenarios ) {
                if ( !scenario.id || !ids.insert( scenario.id ).second || scenario.losses.empty() || scenario.losses.size() > 256
                     || scenario.evidence.size() > 1024 || ( scenario.focus != Focus::Income && scenario.focus != Focus::Destination ) ) {
                    invalid_.insert( "invalid_risk_scenario" ); continue;
                }
                bool verified = scenario.losses.size() == 1 || scenario.linkedLossesVerified;
                std::set<std::pair<int32_t, uint64_t>> supportedLosses;
                for ( const auto id : scenario.evidence ) {
                    const auto found = knownThreats.find( id );
                    if ( found == knownThreats.end() ) { verified = false; continue; }
                    const auto & threat = *found->second;
                    if ( !threat.known || !threat.reachable || !threat.lastSafeDay || !std::isfinite( threat.enemyStrength ) || threat.enemyStrength <= 0 ) {
                        verified = false; continue;
                    }
                    supportedLosses.emplace( threat.town, uint64_t{ *threat.lastSafeDay } + 1 );
                }
                std::set<int32_t> towns;
                std::sort( scenario.losses.begin(), scenario.losses.end(), []( const auto & a, const auto & b ) { return a.town < b.town; } );
                std::ostringstream signature;
                for ( const auto & loss : scenario.losses ) {
                    if ( !towns.insert( loss.town ).second || loss.firstUnavailableDay <= context.base.start.economy.day
                         || loss.firstUnavailableDay > context.base.throughDay
                         || !knownTowns.count( loss.town ) ) verified = false;
                    verified = verified && supportedLosses.count( std::make_pair( loss.town, uint64_t{ loss.firstUnavailableDay } ) );
                    signature << loss.town << ':' << loss.firstUnavailableDay << ';';
                }
                if ( !verified ) { gaps_.insert( "risk_loss_or_linkage_unverified" ); ++omitted_; continue; }
                if ( !signatures.insert( signature.str() ).second ) continue;
                unique.push_back( std::move( scenario ) );
            }
            // Preserve an income variant and a destination variant first, not the first two towns in interface order.
            std::sort( unique.begin(), unique.end(), []( const auto & a, const auto & b ) { return std::tie( a.preselectionPriority, a.id ) < std::tie( b.preselectionPriority, b.id ); } );
            std::vector<size_t> selected;
            for ( const auto focus : { Focus::Income, Focus::Destination } ) {
                const auto found = std::find_if( unique.begin(), unique.end(), [&]( const auto & s ) { return s.focus == focus; } );
                if ( found != unique.end() ) selected.push_back( static_cast<size_t>( found - unique.begin() ) );
            }
            for ( size_t i = 0; selected.size() < 2 && i < unique.size(); ++i )
                if ( std::find( selected.begin(), selected.end(), i ) == selected.end() ) selected.push_back( i );
            if ( selected.size() < unique.size() ) { omitted_ += unique.size() - selected.size(); gaps_.insert( "risk_variant_selection_limit" ); }
            if ( !invalid_.empty() ) return;
            sessions_.push_back( std::make_unique<M::Session>( context, assignments, scorer, budget, defenses, defender ) );
            scenarios_.push_back( Scenario{} );
            for ( const auto index : selected ) {
                auto altered = context;
                altered.base.losses = unique[index].losses;
                sessions_.push_back( std::make_unique<M::Session>( std::move( altered ), assignments, scorer, budget, defenses, defender ) );
                scenarios_.push_back( std::move( unique[index] ) );
            }
        }
        const G::Context * context() const { return sessions_.empty() ? nullptr : &sessions_[0]->context(); }
        size_t requiredProjections() const { size_t n = 0; for ( const auto & session : sessions_ ) n += session->requiredProjections(); return n; }

        Result evaluate( const G::Candidate & candidate )
        {
            Result result; result.gaps = gaps_; result.omitted = omitted_;
            if ( !invalid_.empty() ) { result.status = Status::Invalid; result.gaps.insert( invalid_.begin(), invalid_.end() ); return result; }
            std::optional<size_t> worstFundingIndex;
            P::Resources floors; floors.fill( std::numeric_limits<int64_t>::max() );
            bool exposed = false, incomplete = !result.gaps.empty(), limited = false;
            for ( size_t i = 0; i < sessions_.size(); ++i ) {
                Variant variant{ scenarios_[i].id, i ? scenarios_[i].label : "central", sessions_[i]->evaluate( candidate ) };
                const auto & evaluation = variant.evaluation;
                if ( !evaluation.financial.cancelledRequiredActions.empty() ) { exposed = true; result.gaps.insert( "required_action_cancelled_by_loss" ); }
                if ( evaluation.financial.covered() ) {
                    P::Resources minimum; minimum.fill( std::numeric_limits<int64_t>::max() );
                    for ( const auto & day : evaluation.financial.days ) for ( size_t r = 0; r < 7; ++r ) minimum[r] = std::min( minimum[r], day.afterActions[r] );
                    variant.minimumFunds = minimum;
                    for ( size_t r = 0; r < 7; ++r ) if ( minimum[r] < floors[r] ) { floors[r] = minimum[r]; result.tightestResource[r] = variant.scenario; }
                }
                if ( evaluation.utility && ( !result.observedMinimum || *evaluation.utility < *result.observedMinimum ) ) {
                    result.observedMinimum = evaluation.utility; result.worstUtility = variant.scenario;
                }
                if ( evaluation.status == M::Status::Unfunded ) {
                    if ( !result.worstFunding || evaluation.financial.failedDay < result.variants[*worstFundingIndex].evaluation.financial.failedDay ) {
                        result.worstFunding = variant.scenario; worstFundingIndex = result.variants.size();
                    }
                    exposed = true;
                }
                else if ( !evaluation.financial.covered() || !evaluation.utility ) {
                    incomplete = true; result.gaps.insert( evaluation.gaps.begin(), evaluation.gaps.end() );
                    if ( !evaluation.financial.reason.empty() ) result.gaps.insert( evaluation.financial.reason );
                }
                limited = limited || evaluation.status == M::Status::Limit;
                result.variants.push_back( std::move( variant ) );
            }
            if ( !result.worstFunding ) result.worstFunding = result.tightestResource[0];
            if ( limited ) { result.status = Status::Limit; result.gaps.insert( "shared_risk_evaluation_limit" ); }
            else if ( incomplete ) result.status = Status::Incomplete;
            else if ( exposed ) result.status = Status::Exposed;
            else { result.status = Status::Scored; result.robustUtility = result.observedMinimum; }
            return result;
        }
    private:
        std::vector<Scenario> scenarios_;
        std::vector<std::unique_ptr<M::Session>> sessions_;
        std::set<std::string> gaps_, invalid_;
        size_t omitted_ = 0;
    };
    Result evaluateObserved( Session & session, const G::Candidate & candidate, int player, const M::Budget & budget );
}
