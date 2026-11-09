#include "economy_missions.h"
#include "heroes.h"
#include "kingdom.h"

AI::EconomyMissions::Snapshot AI::EconomyMissions::capture( const Kingdom & kingdom, const P::State & state,
    const std::vector<Target> & targets, const std::vector<Need> & needs )
{
    if ( kingdom.GetHeroes().size() > 256 || kingdom.GetCastles().size() > 256 ) {
        Snapshot invalid; invalid.gaps.insert( "mission_engine_input_limit" ); return invalid;
    }
    std::map<int32_t, const Heroes *> sources;
    for ( const auto * hero : kingdom.GetHeroes() ) {
        if ( !hero || hero->GetColor() != kingdom.GetColor() || !sources.emplace( hero->GetID(), hero ).second ) {
            Snapshot invalid; invalid.gaps.insert( "mission_engine_hero_identity_invalid" ); return invalid;
        }
    }
    // Include directly observed local visitors; a roster entry is not proof of a stationary mission.
    for ( const auto * town : kingdom.GetCastles() ) if ( town ) {
        if ( const auto * hero = town->GetHero() ) {
            if ( hero->GetColor() != kingdom.GetColor() ) {
                Snapshot invalid; invalid.gaps.insert( "mission_engine_visitor_owner_invalid" ); return invalid;
            }
            const auto [it, inserted] = sources.emplace( hero->GetID(), hero );
            if ( !inserted && it->second != hero ) {
                Snapshot invalid; invalid.gaps.insert( "mission_engine_visitor_identity_invalid" ); return invalid;
            }
        }
    }
    std::vector<Hero> heroes;
    for ( const auto & entry : sources ) {
        const auto & hero = *entry.second;
        Hero record; record.id = hero.GetID(); record.tile = hero.GetIndex(); record.movement = hero.GetMovePoints();
        record.pathTarget = hero.GetPath().GetDestinationIndex(); record.patrol = hero.Modes( Heroes::PATROL );
        record.patrolRadius = hero.GetPatrolDistance(); record.sleeping = hero.Modes( Heroes::SLEEPER );
        for ( const auto & town : state.towns ) if ( town.hero == record.id ) record.town = town.tile;
        switch ( hero.getAIRole() ) {
        case Heroes::Role::SCOUT: record.role = Role::Scout; break;
        case Heroes::Role::COURIER: record.role = Role::Courier; break;
        case Heroes::Role::HUNTER: record.role = Role::Hunter; break;
        case Heroes::Role::FIGHTER: record.role = Role::Fighter; break;
        case Heroes::Role::CHAMPION: record.role = Role::Champion; break;
        default: break;
        }
        heroes.push_back( record );
    }
    return classify( state, std::move( heroes ), targets, needs );
}
