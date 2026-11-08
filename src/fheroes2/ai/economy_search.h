// Bounded combined plans, observation only. Selection here is NOT authorization to execute.
#pragma once
#include "economy_risk.h"

namespace AI::EconomySearch
{
    namespace P = EconomyProject;
    namespace G = EconomyCandidates;
    namespace M = EconomyMilitary;
    namespace D = EconomyDefense;
    namespace R = EconomyRisk;
    struct Work { uint64_t merges = 16384; }; // Shared across searches in the same player turn.
    struct Merge { bool valid = false; G::Candidate candidate; std::string reason, key; };
    inline Merge combine( const G::Context & context, const std::vector<const G::Candidate *> & components, const uint64_t id )
    {
        Merge result; result.candidate.id = id;
        struct Node { P::Action action; bool retained; };
        std::vector<Node> nodes;
        std::map<uint64_t, size_t> identities;
        std::map<std::tuple<uint32_t, int32_t, uint32_t>, size_t> buildings;
        std::set<std::pair<size_t, size_t>> edges;
        std::map<uint32_t, size_t> lastBase;
        auto add = [&]( const P::Action & a, const bool retained ) -> std::optional<size_t> {
            if ( a.id && identities.count( a.id ) ) {
                const auto i = identities.at( a.id );
                if ( !G::sameAction( nodes[i].action, a ) ) return {};
                return i;
            }
            const auto key = std::make_tuple( a.day, a.town, a.building );
            if ( a.kind == P::Kind::Build && buildings.count( key ) ) {
                const auto i = buildings.at( key );
                if ( nodes[i].retained && a.required && !nodes[i].action.required ) return {};
                nodes[i].action.required = nodes[i].action.required || a.required;
                if ( a.id ) identities[a.id] = i;
                return i;
            }
            if ( nodes.size() >= 1024 ) return {};
            const auto i = nodes.size(); nodes.push_back( { a, retained } );
            if ( a.id ) identities[a.id] = i;
            if ( a.kind == P::Kind::Build ) buildings[key] = i;
            return i;
        };
        auto sequence = [&]( const std::vector<P::Action> & actions, const bool retained ) {
            std::map<uint32_t, size_t> last;
            for ( const auto & action : actions ) {
                const auto node = add( action, retained ); if ( !node ) return false;
                const auto previous = last.find( action.day );
                if ( previous != last.end() && previous->second != *node ) edges.emplace( previous->second, *node );
                last[action.day] = *node;
            }
            if ( retained ) lastBase = std::move( last );
            return true;
        };
        if ( !sequence( context.base.actions, true ) ) { result.reason = "invalid_retained_identity"; return result; }
        for ( const auto * component : components ) {
            if ( !component || !sequence( component->actions, false ) ) { result.reason = "shared_identity_or_action_limit"; return result; }
            result.candidate.families |= component->families;
        }
        std::map<std::pair<uint32_t, int32_t>, uint32_t> construction;
        for ( size_t i = 0; i < nodes.size(); ++i ) {
            const auto & node = nodes[i];
            if ( !node.retained && lastBase.count( node.action.day ) ) edges.emplace( lastBase.at( node.action.day ), i );
            if ( node.action.kind == P::Kind::Build ) {
                const auto inserted = construction.emplace( std::make_pair( node.action.day, node.action.town ), node.action.building );
                if ( !inserted.second && inserted.first->second != node.action.building ) { result.reason = "construction_day_conflict"; return result; }
            }
        }
        std::vector<size_t> degree( nodes.size() ); std::vector<std::vector<size_t>> after( nodes.size() );
        for ( const auto & edge : edges ) { ++degree[edge.second]; after[edge.first].push_back( edge.second ); }
        std::set<size_t> ready;
        for ( size_t i = 0; i < nodes.size(); ++i ) if ( !degree[i] ) ready.insert( i );
        size_t visited = 0;
        while ( !ready.empty() ) {
            const auto i = *ready.begin(); ready.erase( ready.begin() ); ++visited;
            if ( !nodes[i].retained ) result.candidate.actions.push_back( nodes[i].action );
            for ( const auto next : after[i] ) if ( !--degree[next] ) ready.insert( next );
        }
        if ( visited != nodes.size() ) { result.reason = "shared_order_cycle"; return result; }
        P::Input input;
        if ( !G::materialize( context, result.candidate, input ) ) { result.reason = "invalid_materialized_identity"; return result; }
        // Schedule equivalence includes explicit identities; zero-ID steps are assigned identically by materialize.
        std::ostringstream key;
        for ( const auto & a : result.candidate.actions ) key << a.day << ':' << a.town << ':' << static_cast<int>( a.kind ) << ':' << a.building
            << ':' << a.pool << ':' << a.count << ':' << a.required << ':' << a.id << ':' << ( a.id ? a.project : 0 ) << ';';
        result.key = key.str(); result.valid = true; return result;
    }
    struct Witness { uint64_t scenario; bool funded; uint32_t failedDay; uint64_t failedAction; std::string reason; size_t cancelled, required; std::optional<P::Resources> minimum; };
    struct Score
    {
        bool verified = false, centralFunded = false, defenseComplete = false;
        std::shared_ptr<const D::ComparisonScope> scope;
        R::Status risk = R::Status::Incomplete;
        std::optional<double> robust, central;
        std::vector<bool> defenses;
        int64_t immediateGold = 0;
        std::optional<int64_t> todayGold;
        std::set<std::string> gaps;
        size_t variants = 0;
        std::vector<Witness> witnesses;
    };
    struct Plan
    {
        G::Candidate candidate;
        std::vector<size_t> members;
        Score score;
        bool active = false, ordinary = true;
    };
    struct Counts { uint64_t attempts = 0, cacheHits = 0, rejectedMerges = 0, evaluations = 0, unranked = 0, droppedBeam = 0, substitutions = 0, omittedInputs = 0; };
    enum class Status { Ready, Partial, NoVerified, Invalid };
    struct Source { uint64_t id; uint32_t families; std::string label; bool active; };
    struct Result
    {
        Status status = Status::Invalid;
        std::vector<Source> sources;
        std::vector<Plan> plans; // Compact: NO copied C2 end-state/catalogs or full risk trajectories.
        std::optional<size_t> best, ordinary, active, noInvestment;
        std::vector<size_t> beam;
        Counts counts;
        size_t passes = 0;
        std::string stop;
        std::set<std::string> gaps;
        std::vector<int32_t> unresolvedTowns;
        bool longerHorizonNeeded = false;
        bool actionable = false; // Stability/counterfactual checks and planner integration are later blocks.
    };
    inline int utilityOrder( const double a, const double b )
    {
        const double epsilon = 1e-6 * std::max( { 1.0, std::abs( a ), std::abs( b ) } );
        return a > b + epsilon ? 1 : b > a + epsilon ? -1 : 0;
    }
    inline bool prefer( const Plan & a, const Plan & b )
    {
        if ( !a.score.verified || !b.score.verified || !a.score.scope || a.score.scope != b.score.scope || a.score.defenses.size() != b.score.defenses.size() ) return false;
        for ( size_t i = 0; i < a.score.defenses.size(); ++i ) if ( a.score.defenses[i] != b.score.defenses[i] ) return a.score.defenses[i];
        const auto robust = utilityOrder( *a.score.robust, *b.score.robust ); if ( robust ) return robust > 0;
        const auto central = utilityOrder( *a.score.central, *b.score.central ); if ( central ) return central > 0;
        if ( a.active != b.active ) return a.active;
        if ( a.score.immediateGold != b.score.immediateGold ) return a.score.immediateGold < b.score.immediateGold;
        return a.candidate.id < b.candidate.id;
    }
    inline Result search( R::Session & evaluator, const G::Result & proposals, M::Budget & budget, Work & work )
    {
        Result result;
        const auto * context = evaluator.context();
        if ( !context ) { result.gaps.insert( "risk_context_unavailable" ); return result; }
        budget.projections = std::min<size_t>( budget.projections, 256 );
        work.merges = std::min<uint64_t>( work.merges, 16384 );
        result.gaps = proposals.gaps;
        result.longerHorizonNeeded = proposals.longerHorizonNeeded;
        if ( result.longerHorizonNeeded ) result.gaps.insert( context->base.throughDay - context->base.start.economy.day < 28 ? "common_horizon_review_required" : "distant_effects_not_ranked" );
        for ( const auto * pending : { "financial_and_military_evaluation_pending", "combinations_pending", "best_ordinary_reference_not_ranked" } ) result.gaps.erase( pending );
        result.gaps.insert( "bounded_search_not_exhaustive" ); result.gaps.insert( "role_stability_and_execution_pending" );
        std::vector<G::Candidate> sources;
        size_t entries = context->base.actions.size() + context->active.size();
        if ( entries > 4096 || context->active.size() > 1024 ) { result.gaps.insert( "search_reference_input_limit" ); return result; }
        std::set<uint64_t> ids;
        uint64_t serial = 1;
        if ( !proposals.valid ) result.gaps.insert( "invalid_proposals_references_only" );
        else for ( size_t sourceIndex = 0; sourceIndex < std::min<size_t>( proposals.projects.size(), 64 ); ++sourceIndex ) {
            const auto & source = proposals.projects[sourceIndex];
            if ( !source.id || source.id >= ( uint64_t{ 1 } << 63 ) - 1024 || !ids.insert( source.id ).second ) { result.gaps.insert( "invalid_source_identity" ); ++result.counts.omittedInputs; continue; }
            serial = std::max( serial, source.id + 1 );
            if ( sources.size() >= 64 || source.actions.size() > 1024 || source.actions.size() > 4096 - entries ) {
                result.gaps.insert( "search_proposal_input_limit" ); ++result.counts.omittedInputs; continue;
            }
            entries += source.actions.size(); sources.push_back( source );
        }
        if ( proposals.projects.size() > 64 ) { result.counts.omittedInputs += proposals.projects.size() - 64; result.gaps.insert( "search_proposal_input_limit" ); }
        std::optional<size_t> activeSource;
        if ( !context->active.empty() ) { activeSource = sources.size(); G::Candidate active; active.actions = context->active; sources.push_back( std::move( active ) ); }
        for ( size_t i = 0; i < sources.size(); ++i ) result.sources.push_back( { sources[i].id, sources[i].families, activeSource && i == *activeSource ? "active_continuation" : sources[i].label, activeSource && i == *activeSource } );
        std::map<std::string, size_t> schedules;
        std::shared_ptr<const D::ComparisonScope> frame;
        std::string defenseKey;
        std::vector<D::City> defenseCities;
        std::optional<int64_t> referenceGold;
        bool stopped = false;
        auto evaluate = [&]( std::vector<size_t> members, const bool protectedReference = false ) -> std::optional<size_t> {
            if ( stopped ) return {};
            if ( !work.merges && !protectedReference ) { stopped = true; result.stop = "shared_merge_limit"; return {}; }
            if ( work.merges ) --work.merges;
            ++result.counts.attempts;
            std::sort( members.begin(), members.end() ); members.erase( std::unique( members.begin(), members.end() ), members.end() );
            std::vector<const G::Candidate *> components;
            for ( const auto index : members ) components.push_back( &sources.at( index ) );
            auto merged = combine( *context, components, serial++ );
            if ( !merged.valid ) { ++result.counts.rejectedMerges; result.gaps.insert( merged.reason ); return {}; }
            const bool active = activeSource && std::binary_search( members.begin(), members.end(), *activeSource );
            const auto cached = schedules.find( merged.key );
            if ( cached != schedules.end() ) { ++result.counts.cacheHits; if ( active && !result.plans[cached->second].active ) result.plans[cached->second].members = members; result.plans[cached->second].active = result.plans[cached->second].active || active; return cached->second; }
            if ( result.plans.size() >= 256 ) { stopped = true; result.stop = "compact_plan_limit"; return {}; }
            if ( budget.projections < evaluator.requiredProjections() ) { stopped = true; result.stop = "shared_projection_preflight_limit"; return {}; }
            auto evaluation = evaluator.evaluate( merged.candidate ); ++result.counts.evaluations;
            Plan plan; plan.candidate = std::move( merged.candidate ); plan.members = std::move( members ); plan.active = active;
            plan.ordinary = std::none_of( plan.candidate.actions.begin(), plan.candidate.actions.end(), []( const auto & a ) { return a.kind == P::Kind::Build && a.building == BUILD_TRAINING_GROUNDS; } );
            auto & score = plan.score; score.risk = evaluation.status; score.robust = evaluation.robustUtility; score.gaps = evaluation.gaps; score.variants = evaluation.variants.size();
            for ( const auto & variant : evaluation.variants ) {
                const auto & financial = variant.evaluation.financial;
                score.witnesses.push_back( { variant.scenario, financial.covered(), financial.failedDay, financial.failedAction,
                    financial.reason, financial.cancelledActions.size(), financial.cancelledRequiredActions.size(), variant.minimumFunds } );
            }
            if ( evaluation.status == R::Status::Exposed ) score.gaps.insert( "risk_payment_or_required_action_exposure" );
            if ( !evaluation.variants.empty() ) {
                const auto & central = evaluation.variants[0].evaluation;
                score.centralFunded = central.financial.covered(); score.central = central.utility;
                if ( !central.financial.days.empty() ) score.todayGold = central.financial.days[0].afterActions[0];
                const auto & defense = central.defense; score.scope = defense.scope;
                if ( !frame && defense.complete() ) { frame = defense.scope; defenseKey = defense.key; defenseCities = defense.cities; }
                bool complete = defense.complete() && frame && defense.scope == frame && defense.key == defenseKey;
                for ( const auto & city : defense.cities ) { if ( !city.viable ) complete = false; score.defenses.push_back( city.viable.value_or( false ) ); }
                score.defenseComplete = complete;
                if ( !complete ) { score.gaps.insert( "defense_comparison_unavailable" ); score.gaps.insert( defense.gaps.begin(), defense.gaps.end() ); }
                score.verified = evaluation.status == R::Status::Scored && score.centralFunded && complete && score.robust && score.central
                    && std::isfinite( *score.robust ) && std::isfinite( *score.central ) && score.todayGold;
            }
            if ( result.plans.empty() ) referenceGold = score.todayGold;
            if ( referenceGold && score.todayGold ) score.immediateGold = *referenceGold - *score.todayGold;
            if ( score.immediateGold < 0 ) { score.verified = false; score.gaps.insert( "inconsistent_immediate_cost" ); }
            if ( !score.verified ) ++result.counts.unranked;
            const auto index = result.plans.size(); schedules.emplace( std::move( merged.key ), index ); result.plans.push_back( std::move( plan ) );
            if ( evaluation.status == R::Status::Limit ) { stopped = true; result.stop = "shared_projection_day_attempt_or_force_limit"; }
            return index;
        };
        auto refresh = [&]() {
            result.best.reset(); result.ordinary.reset(); result.beam.clear();
            // Tolerance comparisons are not a strict weak ordering: use a stable scan, never std::sort with that predicate.
            std::vector<size_t> remaining;
            for ( size_t i = 0; i < result.plans.size(); ++i ) if ( result.plans[i].score.verified ) {
                remaining.push_back( i );
                if ( !result.best || prefer( result.plans[i], result.plans[*result.best] ) ) result.best = i;
                if ( result.plans[i].ordinary && ( !result.ordinary || prefer( result.plans[i], result.plans[*result.ordinary] ) ) ) result.ordinary = i;
            }
            const auto retain = [&]( const std::optional<size_t> index ) {
                if ( index && result.plans[*index].score.verified && std::find( result.beam.begin(), result.beam.end(), *index ) == result.beam.end() ) result.beam.push_back( *index );
            };
            retain( result.best ); retain( result.ordinary ); retain( result.active );
            while ( result.beam.size() < 8 && !remaining.empty() ) {
                std::optional<size_t> best;
                for ( const auto i : remaining ) if ( std::find( result.beam.begin(), result.beam.end(), i ) == result.beam.end()
                    && ( !best || prefer( result.plans[i], result.plans[*best] ) ) ) best = i;
                if ( !best ) break;
                result.beam.push_back( *best );
                remaining.erase( std::find( remaining.begin(), remaining.end(), *best ) );
            }
        };
        result.noInvestment = evaluate( {}, true );
        if ( activeSource ) result.active = evaluate( { *activeSource }, true );
        for ( size_t i = 0; i < sources.size() && !stopped; ++i ) if ( !activeSource || i != *activeSource ) evaluate( { i } );
        refresh();
        for ( size_t pass = 0; pass < 6 && !stopped && !result.beam.empty(); ++pass ) {
            const auto beam = result.beam; ++result.passes;
            for ( const auto index : beam ) {
                const auto members = result.plans[index].members;
                for ( size_t source = 0; source < sources.size() && !stopped; ++source ) {
                    if ( std::binary_search( members.begin(), members.end(), source ) ) continue;
                    auto extended = members; extended.push_back( source ); evaluate( std::move( extended ) );
                    for ( size_t old = 0; old < members.size() && !stopped; ++old ) {
                        auto replaced = members; replaced.erase( replaced.begin() + old ); replaced.push_back( source );
                        ++result.counts.substitutions; evaluate( std::move( replaced ) );
                    }
                }
            }
            refresh(); if ( beam == result.beam ) { result.stop = "beam_stable"; break; }
        }
        if ( result.stop.empty() ) result.stop = result.passes == 6 ? "six_pass_limit" : "no_ranked_extensions";
        refresh();
        result.counts.droppedBeam = std::count_if( result.plans.begin(), result.plans.end(), []( const auto & plan ) { return plan.score.verified; } ) - result.beam.size();
        for ( size_t i = 0; i < defenseCities.size(); ++i ) {
            bool solved = false;
            for ( const auto & plan : result.plans ) if ( plan.score.verified && i < plan.score.defenses.size() ) solved = solved || plan.score.defenses[i];
            if ( !solved ) result.unresolvedTowns.push_back( defenseCities[i].town );
        }
        std::sort( result.unresolvedTowns.begin(), result.unresolvedTowns.end() ); result.unresolvedTowns.erase( std::unique( result.unresolvedTowns.begin(), result.unresolvedTowns.end() ), result.unresolvedTowns.end() );
        if ( !result.unresolvedTowns.empty() ) result.gaps.insert( "unresolved_defenses_need_recourse_review" );
        if ( stopped || result.passes == 6 || result.counts.omittedInputs ) result.gaps.insert( result.stop );
        if ( !result.best ) result.gaps.insert( "no_complete_plan_in_examined_set" );
        result.status = !result.best ? Status::NoVerified : stopped || result.counts.omittedInputs || !proposals.valid || result.longerHorizonNeeded ? Status::Partial : Status::Ready;
        return result;
    }
    Result observe( R::Session & evaluator, const G::Result & proposals, M::Budget & budget, Work & work, int player );
}
