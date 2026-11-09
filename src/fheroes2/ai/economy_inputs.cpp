#include "economy_inputs.h"
#include "ai_planner.h"
#include "kingdom.h"
#include "economy_trace.h"

AI::EconomyInputs::Frame AI::EconomyInputs::capture( const Kingdom & kingdom, const uint32_t throughDay, const std::vector<EnemyArmy> & observed,
    const std::vector<int32_t> & priorities, const bool complete, const Distance & distance, Work & work,
    const std::vector<EconomyMissions::Target> & targets, const std::vector<EconomyMissions::Need> & needs )
{
    G::Context context; context.base = P::capture( kingdom, throughDay );
    std::vector<Enemy> enemies;
    if ( observed.size() > 1024 ) { Frame invalid; invalid.gaps.insert( "enemy_input_limit" ); return invalid; }
    for ( const auto & enemy : observed ) enemies.push_back( { D::snapshotThreat( enemy, 0, -1, 0, {}, true, false ), enemy.movePoints, enemy.hero != nullptr } );
    auto missions = EconomyMissions::capture( kingdom, context.base.start, targets, needs );
    if ( !missions.valid ) { Frame invalid; invalid.gaps = missions.gaps; invalid.missions = std::move( missions ); return invalid; }
    auto frame = assemble( std::move( context ), std::move( enemies ), priorities, complete, distance, work, missions.stationaryVisitors );
    frame.gaps.insert( missions.gaps.begin(), missions.gaps.end() );
    frame.defenses.coverageComplete = frame.defenses.coverageComplete && frame.gaps.empty();
    frame.risks.coverageComplete = frame.risks.coverageComplete && frame.gaps.empty(); frame.risks.gaps = frame.gaps;
    frame.missions = std::move( missions ); return frame;
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
    if ( frame.missions ) {
        const auto & missions = *frame.missions;
        ECONOMY_TRACE( 2, "PLANNER_MISSIONS", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
            << ",\"day\":" << missions.day << ",\"valid\":" << ( missions.valid ? "true" : "false" )
            << ",\"heroes\":" << missions.heroes.size() << ",\"targets\":" << missions.targets.size() << ",\"resource_hints\":" << missions.needs.size()
            << ",\"future_commitments_complete\":false,\"military_outlets_exhausted\":false,\"actionable\":false" );
        for ( const auto & hero : missions.heroes )
            ECONOMY_TRACE( 2, "PLANNER_HERO_INTENT", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                << ",\"hero\":" << hero.id << ",\"tile\":" << hero.tile << ",\"town\":" << hero.town << ",\"role\":" << Economy::jsonQuote( EconomyMissions::name( hero.role ) )
                << ",\"use\":" << Economy::jsonQuote( EconomyMissions::name( hero.use ) ) << ",\"path_target\":" << hero.pathTarget
                << ",\"movement\":" << hero.movement << ",\"patrol\":" << ( hero.patrol ? "true" : "false" ) << ",\"patrol_radius\":" << hero.patrolRadius
                << ",\"sleeping\":" << ( hero.sleeping ? "true" : "false" ) << ",\"delivery_verified\":false" );
        for ( const auto & target : missions.targets ) {
            ECONOMY_TRACE( 2, "PLANNER_PRIORITY_INTENT", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                << ",\"tile\":" << target.tile << ",\"intent\":" << Economy::jsonQuote( EconomyMissions::name( target.intent ) )
                << ",\"related_count\":" << target.related.size() << ",\"hero_allocated\":false" );
            for ( const auto related : target.related )
                ECONOMY_TRACE( 2, "PLANNER_PRIORITY_LINK", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                    << ",\"tile\":" << target.tile << ",\"related_tile\":" << related );
        }
        for ( const auto & need : missions.needs )
            ECONOMY_TRACE( 2, "PLANNER_RESOURCE_HINT", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                << ",\"resource\":" << need.resource << ",\"missing\":" << need.missing << ",\"priority\":" << ( need.priority ? "true" : "false" )
                << ",\"recurring\":" << ( need.recurring ? "true" : "false" ) << ",\"reserved_funds\":false" );
        for ( const auto & limitation : missions.limitations )
            ECONOMY_TRACE( 2, "PLANNER_MISSION_LIMITATION", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << parent
                << ",\"limitation\":" << Economy::jsonQuote( limitation ) );
    }
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
