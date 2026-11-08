#include "economy_defense.h"
#include "economy_military.h"
#include "ai_planner.h"
#include "heroes.h"
#include "kingdom.h"

AI::EconomyDefense::Threat AI::EconomyDefense::snapshotThreat( const EnemyArmy & enemy, const uint64_t id, const int32_t town,
                                                              const uint32_t priority, const std::optional<uint32_t> lastSafeDay,
                                                              const bool known, const bool reachable )
{
    Threat threat;
    threat.id = id; threat.town = town; threat.enemyTile = enemy.index;
    threat.priority = priority; threat.lastSafeDay = lastSafeDay; threat.enemyStrength = enemy.strength;
    threat.known = known; threat.reachable = reachable;
    // Never inspect an unverified/hidden hero in order to fill a missing observation.
    if ( known && enemy.hero ) {
        threat.enemyHero = enemy.hero->GetID();
        threat.meleeWithoutBallistics = !enemy.hero->HasSecondarySkill( Skill::Secondary::BALLISTICS ) && enemy.hero->GetArmy().isMeleeDominantArmy();
    }
    return threat;
}

AI::EconomyDefense::Value AI::EconomyDefense::localDefenseValue( const Kingdom & kingdom, const P::Town & town, const Threat & threat )
{
    if ( !threat.known || !threat.reachable || threat.town != town.tile ) return { false, 0, "unverified_local_defense" };
    const auto army = EconomyMilitary::localArmyValue( kingdom, town, EconomyMilitary::Slot::TownArmy );
    if ( !army.complete ) return { false, 0, army.reason };
    const auto castle = std::find_if( kingdom.GetCastles().begin(), kingdom.GetCastles().end(), [&]( const Castle * c ) { return c && c->GetIndex() == town.tile; } );
    if ( castle == kingdom.GetCastles().end() ) return { false, 0, "defended_town_context_changed" };
    uint32_t guild = 0;
    for ( uint32_t bit = BUILD_MAGEGUILD1; bit <= BUILD_MAGEGUILD5 && ( town.mask & bit ); bit <<= 1 ) ++guild;
    const double force = Castle::garrisonStrengthWithStructures( army.force, town.mask, ( *castle )->countBuildings( town.mask ), guild,
                                                                threat.meleeWithoutBallistics );
    if ( !std::isfinite( force ) || force < 0 ) return { false, 0, "invalid_structure_force" };
    return { true, force, "known_local_siege_conditional_no_battle_forecast" };
}
