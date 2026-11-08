#include "economy_inputs.h"
#include "ai_planner.h"
#include "economy_trace.h"

AI::EconomyInputs::Frame AI::EconomyInputs::capture( const Kingdom & kingdom, const uint32_t throughDay, const std::vector<EnemyArmy> & observed,
    const std::vector<int32_t> & priorities, const bool complete, const Distance & distance, Work & work )
{
    G::Context context; context.base = P::capture( kingdom, throughDay );
    std::vector<Enemy> enemies;
    if ( observed.size() > 1024 ) { Frame invalid; invalid.gaps.insert( "enemy_input_limit" ); return invalid; }
    for ( const auto & enemy : observed ) enemies.push_back( { D::snapshotThreat( enemy, 0, -1, 0, {}, true, false ), enemy.movePoints, enemy.hero != nullptr } );
    return assemble( std::move( context ), std::move( enemies ), priorities, complete, distance, work );
}

void AI::EconomyInputs::log( const Frame & frame, const int player, const Work & work )
{
    ECONOMY_TRACE( 2, "PLANNER_INPUTS", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"phase\":\"before_hero_planning\""
        << ",\"valid\":" << ( frame.valid ? "true" : "false" ) << ",\"actionable\":false,\"towns\":" << frame.context.base.start.towns.size()
        << ",\"assignments\":" << frame.assignments.size() << ",\"threats\":" << frame.defenses.threats.size()
        << ",\"coverage_complete\":" << ( frame.defenses.coverageComplete ? "true" : "false" ) << ",\"path_queries\":" << frame.queried
        << ",\"paths_left\":" << work.paths << ",\"outside_horizon\":" << frame.outsideHorizon
        << ",\"through_day\":" << frame.context.base.throughDay );
    const auto parent = Economy::traceState().sequence;
    for ( const auto & route : frame.routes )
        ECONOMY_TRACE( 2, "PLANNER_ROUTE", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
            << ",\"enemy_tile\":" << route.enemy << ",\"town\":" << route.town << ",\"distance\":" << route.distance << ",\"daily_movement\":" << route.movement );
    for ( const auto & assignment : frame.assignments )
        ECONOMY_TRACE( 2, "PLANNER_ASSIGNMENT", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
            << ",\"town\":" << assignment.town << ",\"verified\":" << ( assignment.verified ? "true" : "false" ) << ",\"scope\":\"local_town_only\"" );
    for ( const auto & t : frame.defenses.threats )
        ECONOMY_TRACE( 2, "PLANNER_THREAT", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
            << ",\"id\":" << t.id << ",\"town\":" << t.town << ",\"enemy_tile\":" << t.enemyTile << ",\"enemy_hero\":" << t.enemyHero
            << ",\"priority\":" << t.priority << ",\"strength\":" << t.enemyStrength << ",\"reachable\":" << ( t.reachable ? "true" : "false" )
            << ",\"last_safe_day\":" << ( t.lastSafeDay ? std::to_string( *t.lastSafeDay ) : "null" ) );
    for ( const auto & gap : frame.gaps )
        ECONOMY_TRACE( 2, "PLANNER_INPUT_GAP", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent << ",\"gap\":" << Economy::jsonQuote( gap ) );
}
