// Dated, conditional defense checks. No paths, battles, purchases or autonomous decisions.
#pragma once
#include "economy_project.h"
#include "taxpayer_policy.h"
#include <cmath>
#include <iomanip>
#include <optional>
#include <memory>
#include <sstream>
#include <tuple>

class Kingdom;
namespace AI { struct EnemyArmy; }
namespace AI::EconomyDefense
{
    namespace P = EconomyProject;
    constexpr double viableRatio = taxpayerViableDefenseRatio;
    struct Threat
    {
        uint64_t id = 0;
        int32_t town = -1, enemyTile = -1, enemyHero = -1;
        uint32_t priority = 0; // Rank from the current planner: lower is more important.
        std::optional<uint32_t> lastSafeDay; // Last OWN action phase guaranteed before this attack; never a guessed arrival date.
        double enemyStrength = 0; // Observed full threat, without the planner's distance/panic discount.
        bool known = false, reachable = false, meleeWithoutBallistics = false;
    };
    // Caller must use existing visible planner data and provide phase-verified reachability/deadline.
    Threat snapshotThreat( const EnemyArmy & enemy, uint64_t id, int32_t town, uint32_t priority,
                           std::optional<uint32_t> lastSafeDay, bool known, bool reachable );
    struct Registry { std::vector<Threat> threats; bool coverageComplete = false; };
    struct Value { bool complete = false; double force = 0; std::string reason; };
    using Scorer = std::function<Value( const P::Town &, const Threat & )>;
    Value localDefenseValue( const Kingdom & kingdom, const P::Town & town, const Threat & threat );
    enum class Status { NotRequested, Pending, Complete, Incomplete, Limit, Invalid };
    struct Check
    {
        Threat threat;
        std::optional<double> force, ratio;
        std::optional<bool> viable;
        std::string reason;
    };
    struct City
    {
        int32_t town = -1;
        uint32_t deadline = 0, priority = 0;
        std::optional<bool> viable;
    };
    struct ComparisonScope {};
    struct Evaluation
    {
        Status status = Status::NotRequested;
        std::string key;
        std::shared_ptr<const ComparisonScope> scope;
        std::set<std::string> gaps;
        std::vector<Check> checks;
        std::vector<City> cities;
        bool complete() const { return status == Status::Complete; }
    };
    inline Evaluation prepare( const Registry & registry, const P::Input & input, const bool hasScorer,
                               std::shared_ptr<const ComparisonScope> scope = {} )
    {
        Evaluation result; result.status = Status::Pending;
        result.scope = scope ? std::move( scope ) : std::make_shared<ComparisonScope>();
        if ( registry.threats.size() > 1024 ) { result.status = Status::Limit; result.gaps.insert( "defense_input_limit" ); return result; }
        if ( !registry.coverageComplete ) result.gaps.insert( "known_defense_coverage_not_certified" );
        if ( !registry.threats.empty() && !hasScorer ) result.gaps.insert( "defense_scorer_missing" );
        auto threats = registry.threats;
        std::sort( threats.begin(), threats.end(), []( const auto & a, const auto & b ) { return a.id < b.id; } );
        std::set<uint64_t> ids;
        std::set<int32_t> towns;
        for ( const auto & town : input.start.towns ) towns.insert( town.tile );
        std::map<std::pair<uint32_t, int32_t>, City> cities;
        std::map<int32_t, uint32_t> priorities;
        std::ostringstream key; key << registry.coverageComplete << ';' << std::hexfloat;
        for ( const auto & threat : threats ) {
            if ( !threat.id || !ids.insert( threat.id ).second || !towns.count( threat.town ) || threat.enemyTile < 0
                 || !std::isfinite( threat.enemyStrength ) || threat.enemyStrength <= 0 ) {
                result.status = Status::Invalid; result.gaps.insert( "invalid_defense_threat" );
            }
            if ( !threat.known ) result.gaps.insert( "enemy_observation_unverified" );
            if ( !threat.reachable ) result.gaps.insert( "enemy_path_unverified" );
            if ( !threat.lastSafeDay ) result.gaps.insert( "defense_action_deadline_unknown" );
            else if ( *threat.lastSafeDay < input.start.economy.day ) result.gaps.insert( "stale_defense_deadline" );
            else if ( *threat.lastSafeDay > input.throughDay ) result.gaps.insert( "defense_deadline_outside_horizon" );
            const auto rank = priorities.emplace( threat.town, threat.priority );
            if ( !rank.second && rank.first->second != threat.priority ) { result.status = Status::Invalid; result.gaps.insert( "inconsistent_city_priority" ); }
            // Distinct deadlines stay distinct: a day-4 danger must not borrow this town's day-2 priority over another town attacked on day 3.
            cities.emplace( std::make_pair( threat.lastSafeDay.value_or( 0 ), threat.town ),
                            City{ threat.town, threat.lastSafeDay.value_or( 0 ), threat.priority, {} } );
            result.checks.push_back( { threat, {}, {}, {}, {} } );
            key << threat.id << ':' << threat.town << ':' << threat.enemyTile << ':' << threat.enemyHero << ':' << threat.priority
                << ':' << threat.lastSafeDay.value_or( 0 ) << ':' << threat.enemyStrength << ':' << threat.known
                << ':' << threat.reachable << ':' << threat.meleeWithoutBallistics << ';';
        }
        // Comparison requires the same frozen information AND planning window.
        key << input.start.economy.day << ':' << input.throughDay;
        result.key = key.str();
        for ( const auto & city : cities ) result.cities.push_back( city.second );
        std::sort( result.cities.begin(), result.cities.end(), []( const auto & a, const auto & b ) {
            return std::tie( a.deadline, a.priority, a.town ) < std::tie( b.deadline, b.priority, b.town );
        } );
        return result;
    }
    inline void record( Evaluation & result, const P::State & state, uint64_t & forceBudget, const Scorer & scorer )
    {
        if ( result.status == Status::Invalid || result.status == Status::Limit || !scorer ) return;
        for ( auto & check : result.checks ) {
            const auto & threat = check.threat;
            if ( !threat.known || !threat.reachable || !threat.lastSafeDay || *threat.lastSafeDay != state.economy.day ) continue;
            if ( !forceBudget ) { result.status = Status::Limit; result.gaps.insert( "shared_defense_force_limit" ); return; }
            --forceBudget;
            const auto town = std::find_if( state.towns.begin(), state.towns.end(), [&]( const auto & t ) { return t.tile == threat.town; } );
            if ( town == state.towns.end() ) { check.reason = "defended_town_missing"; result.gaps.insert( check.reason ); continue; }
            const auto value = scorer( *town, threat );
            if ( !value.complete || !std::isfinite( value.force ) || value.force < 0 ) {
                check.reason = value.reason.empty() ? "invalid_defense_context" : value.reason;
                result.gaps.insert( check.reason ); continue;
            }
            const double ratio = value.force / threat.enemyStrength;
            if ( !std::isfinite( ratio ) ) { check.reason = "defense_ratio_out_of_range"; result.gaps.insert( check.reason ); continue; }
            check.force = value.force; check.ratio = ratio; check.viable = ratio >= viableRatio;
        }
    }
    inline void finish( Evaluation & result, const bool funded )
    {
        if ( result.status == Status::NotRequested || result.status == Status::Invalid || result.status == Status::Limit ) return;
        if ( !funded ) result.gaps.insert( "defense_plan_not_fully_funded" );
        for ( auto & city : result.cities ) {
            bool all = true, unknown = false;
            for ( const auto & check : result.checks ) if ( check.threat.town == city.town && check.threat.lastSafeDay.value_or( 0 ) == city.deadline ) {
                if ( !check.viable ) unknown = true;
                else all = all && *check.viable;
            }
            if ( !unknown ) city.viable = all;
            else result.gaps.insert( "defense_check_unavailable" );
        }
        result.status = result.gaps.empty() ? Status::Complete : Status::Incomplete;
    }
    enum class Order { Better, Equal, Worse, Incomparable };
    inline Order comparePriority( const Evaluation & a, const Evaluation & b )
    {
        if ( !a.complete() || !b.complete() || a.key != b.key || !a.scope || a.scope != b.scope || a.cities.size() != b.cities.size() ) return Order::Incomparable;
        for ( size_t i = 0; i < a.cities.size(); ++i ) {
            if ( !a.cities[i].viable || !b.cities[i].viable ) return Order::Incomparable;
            if ( *a.cities[i].viable != *b.cities[i].viable ) return *a.cities[i].viable ? Order::Better : Order::Worse;
        }
        return Order::Equal;
    }
    struct Coverage { size_t compared = 0, omitted = 0; std::vector<int32_t> unresolved; };
    inline Coverage coverage( const std::vector<const Evaluation *> & evaluations )
    {
        Coverage result; const Evaluation * reference = nullptr;
        std::vector<bool> solved;
        for ( const auto * evaluation : evaluations ) {
            if ( !evaluation || !evaluation->complete() || ( reference && ( evaluation->key != reference->key || evaluation->scope != reference->scope ) ) ) { ++result.omitted; continue; }
            if ( !reference ) { reference = evaluation; solved.resize( reference->cities.size() ); }
            ++result.compared;
            for ( size_t i = 0; i < solved.size(); ++i ) solved[i] = solved[i] || evaluation->cities[i].viable.value_or( false );
        }
        std::set<int32_t> unresolved;
        if ( reference ) for ( size_t i = 0; i < solved.size(); ++i ) if ( !solved[i] ) unresolved.insert( reference->cities[i].town );
        result.unresolved.assign( unresolved.begin(), unresolved.end() );
        return result; // Unresolved among THESE evaluated alternatives, never proof of impossibility.
    }
}
