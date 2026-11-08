// Central, dated local-army utility. No autonomous actions, routes or battle outcomes.
#pragma once
#include "economy_candidates.h"
#include "economy_defense.h"
#include <optional>

class Kingdom;

namespace AI::EconomyMilitary
{
    namespace P = EconomyProject;
    namespace G = EconomyCandidates;
    enum class Slot { TownArmy, Visitor };
    struct Assignment
    {
        uint64_t id = 0;
        int32_t town = -1;
        Slot slot = Slot::TownArmy;
        uint32_t firstDay = 0, lastDay = 0;
        bool verified = false, useful = true;
    };
    struct Value { bool complete = false; double force = 0; std::string reason; };
    using Scorer = std::function<Value( const P::Town &, Slot )>;
    Value localArmyValue( const Kingdom & kingdom, const P::Town & town, Slot slot );
    struct Budget
    {
        size_t projections = 256;
        uint64_t forces = 131072;
        P::WorkBudget mechanics;
    };
    struct Row { uint32_t day = 0; double baseline = 0, proposed = 0, difference = 0; };
    enum class Status { Scored, Unfunded, Incomplete, Limit, Invalid };
    struct Result
    {
        Status status = Status::Incomplete;
        std::optional<double> utility;
        P::Result financial;
        std::set<std::string> gaps;
        std::vector<Row> days;
        EconomyDefense::Evaluation defense, baselineDefense;
    };

    class Session
    {
    public:
        Session( G::Context context, std::vector<Assignment> assignments, Scorer scorer, Budget & budget,
                 std::optional<EconomyDefense::Registry> defenses = {}, EconomyDefense::Scorer defenseScorer = {} )
            : context_( std::move( context ) ), assignments_( std::move( assignments ) ), scorer_( std::move( scorer ) ), budget_( budget ),
              defenses_( std::move( defenses ) ), defenseScorer_( std::move( defenseScorer ) )
        {
            const auto today = context_.base.start.economy.day;
            const auto end = context_.base.throughDay;
            if ( !scorer_ || !today || end <= today || ( uint64_t{ end } - today != 14 && uint64_t{ end } - today != 28 ) || assignments_.size() > 1024 ) {
                invalid_.insert( "invalid_military_window_or_scorer" ); return;
            }
            if ( defenses_ ) {
                const auto validation = EconomyDefense::prepare( *defenses_, context_.base, static_cast<bool>( defenseScorer_ ), defenseScope_ );
                if ( validation.status == EconomyDefense::Status::Invalid || validation.status == EconomyDefense::Status::Limit ) {
                    invalid_.insert( validation.gaps.begin(), validation.gaps.end() ); return;
                }
            }
            std::map<int32_t, int> towns;
            for ( const auto & t : context_.base.start.towns ) towns.emplace( t.tile, t.hero );
            std::set<uint64_t> ids;
            std::set<int32_t> registered;
            for ( size_t i = 0; i < assignments_.size(); ++i ) {
                const auto & a = assignments_[i];
                if ( !a.id || !ids.insert( a.id ).second || !towns.count( a.town ) || a.firstDay > a.lastDay
                     || ( a.slot == Slot::Visitor && towns.at( a.town ) < 0 )
                     || ( a.slot != Slot::Visitor && a.slot != Slot::TownArmy ) ) invalid_.insert( "invalid_assignment" );
                registered.insert( a.town );
                if ( !a.verified ) unknown_.insert( "unverified_assignment" );
                // TownArmy includes its visitor: overlapping local uses would count the hero twice.
                for ( size_t j = 0; j < i; ++j ) if ( a.town == assignments_[j].town && a.firstDay <= assignments_[j].lastDay
                    && assignments_[j].firstDay <= a.lastDay ) invalid_.insert( "overlapping_army_assignment" );
            }
            for ( const auto & t : towns ) if ( !registered.count( t.first ) ) unknown_.insert( "town_usefulness_not_registered" );
        }

        Result evaluate( const G::Candidate & candidate )
        {
            Result result;
            if ( !invalid_.empty() ) { result.status = Status::Invalid; result.gaps = invalid_; return result; }
            if ( !baseline_ ) baseline_ = trace( context_.base );
            result.baselineDefense = baseline_->defense;
            if ( !baseline_->financial.covered() ) {
                result.status = baseline_->limited ? Status::Limit : Status::Incomplete;
                result.gaps = baseline_->gaps; result.gaps.insert( "baseline_unavailable" );
                if ( !baseline_->financial.reason.empty() ) result.gaps.insert( baseline_->financial.reason );
                return result;
            }
            P::Input input;
            if ( !G::materialize( context_, candidate, input ) ) { result.status = Status::Invalid; result.gaps.insert( "invalid_materialization" ); return result; }
            auto proposed = trace( input );
            result.financial = std::move( proposed.financial );
            result.defense = std::move( proposed.defense );
            result.gaps = baseline_->gaps; result.gaps.insert( proposed.gaps.begin(), proposed.gaps.end() );
            if ( proposed.limited || baseline_->limited ) { result.status = Status::Limit; return result; }
            if ( !result.financial.covered() ) {
                result.status = result.financial.status == P::Status::Unfunded ? Status::Unfunded : Status::Incomplete;
                result.gaps.insert( result.financial.reason ); return result;
            }
            if ( !result.gaps.empty() || proposed.values.size() != baseline_->values.size() ) { result.status = Status::Incomplete; return result; }
            double sum = 0;
            for ( size_t i = 0; i < proposed.values.size(); ++i ) {
                const auto & a = baseline_->values[i]; const auto & b = proposed.values[i];
                const double difference = b.second - a.second;
                if ( a.first != b.first || !std::isfinite( difference ) ) { result.gaps.insert( "invalid_daily_force" ); return result; }
                result.days.push_back( { a.first, a.second, b.second, difference } );
                // The current morning is already processed. U averages tomorrow through H, once.
                if ( a.first > context_.base.start.economy.day ) sum += difference;
            }
            const double utility = sum / ( context_.base.throughDay - context_.base.start.economy.day );
            if ( !std::isfinite( utility ) ) { result.gaps.insert( "utility_out_of_range" ); return result; }
            result.utility = utility; result.status = Status::Scored;
            return result;
        }

    private:
        struct Trace
        {
            P::Result financial;
            std::vector<std::pair<uint32_t, double>> values;
            std::set<std::string> gaps;
            bool limited = false;
            EconomyDefense::Evaluation defense;
        };
        Trace trace( const P::Input & input )
        {
            Trace trace;
            trace.gaps = unknown_;
            if ( defenses_ ) trace.defense = EconomyDefense::prepare( *defenses_, input, static_cast<bool>( defenseScorer_ ), defenseScope_ );
            if ( !budget_.projections ) { trace.limited = true; trace.gaps.insert( "shared_projection_count_limit" ); return trace; }
            --budget_.projections;
            trace.financial = P::project( input, budget_.mechanics, [&]( const P::State & state ) {
                double total = 0;
                for ( const auto & a : assignments_ ) {
                    if ( !a.verified || !a.useful || state.economy.day < a.firstDay || state.economy.day > a.lastDay ) continue;
                    if ( !budget_.forces ) { trace.limited = true; trace.gaps.insert( "shared_force_limit" ); continue; }
                    --budget_.forces;
                    const auto town = std::find_if( state.towns.begin(), state.towns.end(), [&]( const P::Town & t ) { return t.tile == a.town; } );
                    if ( town == state.towns.end() ) { trace.gaps.insert( "assigned_town_missing" ); continue; }
                    if ( !town->available ) continue; // The captured position/visitor no longer contributes to our army.
                    const auto value = scorer_( *town, a.slot );
                    if ( !value.complete || !std::isfinite( value.force ) || value.force < 0 ) {
                        trace.gaps.insert( value.reason.empty() ? "invalid_force_context" : value.reason ); continue;
                    }
                    total += value.force;
                }
                if ( !std::isfinite( total ) ) trace.gaps.insert( "force_sum_out_of_range" );
                trace.values.emplace_back( state.economy.day, total );
                EconomyDefense::record( trace.defense, state, budget_.forces, defenseScorer_ );
            } );
            if ( trace.financial.status == P::Status::Limit ) { trace.limited = true; trace.gaps.insert( trace.financial.reason ); }
            EconomyDefense::finish( trace.defense, trace.financial.covered() );
            if ( trace.defense.status == EconomyDefense::Status::Limit ) { trace.limited = true; trace.gaps.insert( "shared_defense_force_limit" ); }
            return trace;
        }
        G::Context context_; // Freeze the common initial state and horizon for all comparisons.
        std::vector<Assignment> assignments_;
        Scorer scorer_;
        Budget & budget_;
        std::set<std::string> invalid_, unknown_;
        std::optional<Trace> baseline_;
        std::optional<EconomyDefense::Registry> defenses_;
        EconomyDefense::Scorer defenseScorer_;
        std::shared_ptr<const EconomyDefense::ComparisonScope> defenseScope_ = std::make_shared<EconomyDefense::ComparisonScope>();
    };

    inline EconomyDefense::Order compareDefense( const Result & a, const Result & b )
    {
        if ( !a.financial.covered() || !b.financial.covered() ) return EconomyDefense::Order::Incomparable;
        return EconomyDefense::comparePriority( a.defense, b.defense );
    }

    // Observation only: no selection or execution in the running AI.
    Result evaluateObserved( Session & session, const G::Candidate & candidate, int player, const Budget & budget );
}
