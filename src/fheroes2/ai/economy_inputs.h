// C3D2: frozen, local planner inputs; no economic decision is executed.
#pragma once
#include "economy_risk.h"

namespace AI { struct EnemyArmy; }
namespace AI::EconomyInputs
{
    namespace P = EconomyProject;
    namespace G = EconomyCandidates;
    namespace M = EconomyMilitary;
    namespace D = EconomyDefense;
    namespace R = EconomyRisk;
    struct Work { size_t paths = 128; };
    struct Enemy { D::Threat observation; uint32_t movePoints = 0; bool mobile = false; };
    using Distance = std::function<uint32_t( const Enemy &, int32_t )>;
    struct Frame
    {
        G::Context context;
        D::Registry defenses;
        R::Registry risks;
        std::vector<M::Assignment> assignments;
        std::set<std::string> gaps;
        size_t queried = 0, outsideHorizon = 0;
        bool valid = false, actionable = false;
    };
    inline Frame assemble( G::Context context, std::vector<Enemy> enemies, const std::vector<int32_t> & priorityOrder,
                           const bool observationsComplete, const Distance & distance, Work & work )
    {
        Frame out; work.paths = std::min<size_t>( work.paths, 128 );
        out.context = std::move( context );
        const auto & input = out.context.base; const auto today = input.start.economy.day;
        if ( !today || input.throughDay <= today || ( uint64_t{ input.throughDay } - today != 14 && uint64_t{ input.throughDay } - today != 28 )
             || input.start.towns.size() > 256 || enemies.size() > 1024 || priorityOrder.size() > 256 ) {
            out.gaps.insert( "invalid_input_frame_or_limit" ); return out;
        }
        std::map<int32_t, uint32_t> priorities;
        for ( size_t i = 0; i < priorityOrder.size(); ++i )
            if ( !priorities.emplace( priorityOrder[i], static_cast<uint32_t>( i ) ).second ) { out.gaps.insert( "duplicate_town_priority" ); return out; }
        std::set<int32_t> towns;
        for ( const auto & town : input.start.towns ) {
            if ( !town.available || !towns.insert( town.tile ).second || !priorities.count( town.tile ) ) {
                out.gaps.insert( "town_priority_or_ownership_unverified" ); return out;
            }
            // Only local garrison+visitor defense/readiness: no credit at a distant front.
            out.assignments.push_back( { uint64_t( out.assignments.size() + 1 ), town.tile, M::Slot::TownArmy, today, input.throughDay, true, true } );
        }
        if ( priorities.size() != towns.size() ) { out.gaps.insert( "foreign_priority_town" ); return out; }
        std::sort( enemies.begin(), enemies.end(), []( const auto & a, const auto & b ) { return a.observation.enemyTile < b.observation.enemyTile; } );
        std::set<int32_t> locations;
        for ( const auto & enemy : enemies ) if ( enemy.observation.enemyTile < 0 || !locations.insert( enemy.observation.enemyTile ).second ) {
            out.gaps.insert( "invalid_enemy_identity" ); return out;
        }
        if ( !observationsComplete ) out.gaps.insert( "enemy_observation_coverage_unverified" );
        size_t pairs = 0;
        for ( const auto & enemy : enemies ) for ( const auto town : towns ) {
            if ( ++pairs > 1024 ) { out.gaps.insert( "enemy_town_pair_limit" ); break; }
            auto threat = enemy.observation; threat.id = pairs; threat.town = town; threat.priority = priorities.at( town );
            threat.lastSafeDay.reset(); threat.reachable = false;
            if ( !threat.known ) { out.gaps.insert( "enemy_observation_unverified" ); continue; }
            if ( !std::isfinite( threat.enemyStrength ) || threat.enemyStrength <= 0 ) { out.gaps.insert( "enemy_strength_unverified" ); continue; }
            if ( !enemy.mobile ) out.gaps.insert( "enemy_recruitment_threat_unverified" );
            else if ( !enemy.movePoints ) out.gaps.insert( "enemy_movement_unverified" );
            else if ( !distance ) out.gaps.insert( "enemy_path_provider_missing" );
            else if ( !work.paths ) out.gaps.insert( "shared_economy_path_limit" );
            else {
                --work.paths; ++out.queried;
                const auto length = distance( enemy, town );
                if ( !length ) out.gaps.insert( "enemy_path_unverified" );
                else {
                    const uint64_t days = ( uint64_t{ length } + enemy.movePoints - 1 ) / enemy.movePoints;
                    const uint64_t deadline = uint64_t{ today } + days - 1;
                    if ( deadline > input.throughDay ) { ++out.outsideHorizon; continue; }
                    threat.lastSafeDay = static_cast<uint32_t>( deadline ); threat.reachable = true;
                }
            }
            out.defenses.threats.push_back( threat );
            if ( threat.lastSafeDay && *threat.lastSafeDay < input.throughDay ) {
                const auto economic = std::find_if( input.start.economy.towns.begin(), input.start.economy.towns.end(), [&]( const auto & t ) { return t.tile == town; } );
                const auto physical = std::find_if( input.start.towns.begin(), input.start.towns.end(), [&]( const auto & t ) { return t.tile == town; } );
                const bool income = physical->recurringIncome[0] > 0 || ( economic != input.start.economy.towns.end() && economic->farm && input.start.economy.taxRate > 0 );
                out.risks.scenarios.push_back( { threat.id, income ? R::Focus::Income : R::Focus::Destination,
                    { { town, *threat.lastSafeDay + 1 } }, { threat.id }, false, "known_local_destination_exposure", threat.priority } );
            }
        }
        out.defenses.coverageComplete = out.gaps.empty();
        out.risks.coverageComplete = out.gaps.empty(); out.risks.gaps = out.gaps;
        out.valid = true; return out;
    }
    Frame capture( const Kingdom &, uint32_t throughDay, const std::vector<EnemyArmy> &, const std::vector<int32_t> & priorityOrder,
                   bool observationsComplete, const Distance &, Work & );
    void log( const Frame &, int player, const Work & );
}
