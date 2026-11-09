// Frozen planner intent is evidence, not an assignment, payment or permission to cancel.
#pragma once
#include "economy_project.h"

namespace AI::EconomyMissions
{
    namespace P = EconomyProject;
    enum class Role { Scout, Courier, Hunter, Fighter, Champion, Unknown };
    enum class Intent { Attack, Defend, Reinforce, Unknown };
    enum class Use { FixedLocal, MobilePatrol, MobileRole };
    struct Hero
    {
        int32_t id = -1, tile = -1, town = -1, pathTarget = -1;
        Role role = Role::Unknown;
        uint32_t movement = 0, patrolRadius = 0;
        bool patrol = false, sleeping = false;
        Use use = Use::MobileRole;
    };
    struct Target { int32_t tile = -1; Intent intent = Intent::Unknown; std::set<int32_t> related; };
    // Resource index uses Project::Resources: gold, wood, ore, mercury, sulfur, crystal, gems.
    struct Need { size_t resource = 0; int64_t missing = 0; bool priority = false, recurring = false; };
    struct Snapshot
    {
        uint32_t day = 0;
        std::vector<Hero> heroes;
        std::vector<Target> targets;
        std::vector<Need> needs;
        std::set<int32_t> stationaryVisitors;
        std::set<std::string> gaps, limitations;
        bool valid = false, actionable = false;
    };
    inline const char * name( const Role role )
    {
        switch ( role ) {
        case Role::Scout: return "scout";
        case Role::Courier: return "courier";
        case Role::Hunter: return "hunter";
        case Role::Fighter: return "fighter";
        case Role::Champion: return "champion";
        default: return "unknown";
        }
    }
    inline const char * name( const Intent intent )
    {
        switch ( intent ) {
        case Intent::Attack: return "attack";
        case Intent::Defend: return "defend";
        case Intent::Reinforce: return "reinforce";
        default: return "unknown";
        }
    }
    inline const char * name( const Use use )
    { return use == Use::FixedLocal ? "fixed_local" : use == Use::MobilePatrol ? "mobile_patrol" : "mobile_role"; }
    inline Snapshot classify( const P::State & state, std::vector<Hero> heroes, std::vector<Target> targets, std::vector<Need> needs )
    {
        Snapshot out; out.day = state.economy.day;
        if ( !out.day || state.towns.size() > 256 || heroes.size() > 256 || targets.size() > 1024 || needs.size() > 7 ) {
            out.gaps.insert( "mission_snapshot_limit_or_date" ); return out;
        }
        // No date/quantity/beneficiary or persistent project is supplied by these legacy hints.
        out.limitations.insert( "economic_commitments_not_supplied" );
        std::map<int32_t, int32_t> visitors;
        std::set<int32_t> towns;
        for ( const auto & town : state.towns ) {
            if ( town.tile < 0 || !town.available || !towns.insert( town.tile ).second
                 || ( town.hero >= 0 && !visitors.emplace( town.hero, town.tile ).second ) ) {
                out.gaps.insert( "mission_town_identity_invalid" ); return out;
            }
        }
        std::set<int32_t> heroIds, heroTiles;
        for ( auto & hero : heroes ) {
            if ( hero.id < 0 || hero.tile < 0 || hero.pathTarget < -1 || hero.role < Role::Scout || hero.role > Role::Champion
                 || !heroIds.insert( hero.id ).second || !heroTiles.insert( hero.tile ).second ) {
                out.gaps.insert( "mission_hero_identity_or_role_invalid" ); return out;
            }
            const auto visitor = visitors.find( hero.id );
            const int32_t actualTown = visitor == visitors.end() ? -1 : visitor->second;
            if ( hero.town != actualTown || ( actualTown >= 0 && hero.tile != actualTown )
                 || ( actualTown < 0 && towns.count( hero.tile ) ) ) {
                out.gaps.insert( "mission_visitor_mismatch" ); return out;
            }
            hero.use = hero.patrol && hero.patrolRadius == 0 && actualTown >= 0 ? Use::FixedLocal
                : hero.patrol ? Use::MobilePatrol : Use::MobileRole;
            if ( hero.use == Use::FixedLocal ) out.stationaryVisitors.insert( hero.id );
            else {
                out.limitations.insert( "mobile_missions_not_dated_or_funded" );
                if ( actualTown >= 0 ) out.gaps.insert( "visitor_assignment_not_committed" );
            }
            if ( hero.pathTarget >= 0 ) out.limitations.insert( "existing_path_is_not_delivery_commitment" );
        }
        for ( const auto & visitor : visitors ) if ( !heroIds.count( visitor.first ) ) {
            out.gaps.insert( "mission_visitor_missing" ); return out;
        }
        std::set<int32_t> targetTiles;
        size_t links = 0;
        for ( const auto & target : targets ) {
            if ( target.tile < 0 || target.intent < Intent::Attack || target.intent > Intent::Reinforce || !targetTiles.insert( target.tile ).second ) {
                out.gaps.insert( "mission_target_invalid" ); return out;
            }
            for ( const auto related : target.related ) if ( related < 0 || ++links > 2048 ) {
                out.gaps.insert( "mission_target_links_invalid_or_limit" ); return out;
            }
        }
        if ( !targets.empty() ) out.limitations.insert( "priority_targets_not_allocated_to_heroes" );
        std::set<size_t> resources;
        for ( const auto & need : needs ) if ( need.resource >= 7 || need.missing < 0 || !resources.insert( need.resource ).second ) {
            out.gaps.insert( "mission_resource_hint_invalid" ); return out;
        }
        if ( !needs.empty() ) out.limitations.insert( "resource_hints_are_not_reserved_funds" );
        // Stable records independent of the planner's unordered containers; never choose a new target.
        std::sort( heroes.begin(), heroes.end(), []( const auto & a, const auto & b ) { return a.id < b.id; } );
        std::sort( targets.begin(), targets.end(), []( const auto & a, const auto & b ) { return a.tile < b.tile; } );
        std::sort( needs.begin(), needs.end(), []( const auto & a, const auto & b ) { return a.resource < b.resource; } );
        out.heroes = std::move( heroes ); out.targets = std::move( targets ); out.needs = std::move( needs ); out.valid = true;
        return out;
    }
    Snapshot capture( const Kingdom &, const P::State &, const std::vector<Target> &, const std::vector<Need> & );
}
