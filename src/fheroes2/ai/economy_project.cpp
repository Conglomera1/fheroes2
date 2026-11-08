#include "economy_project.h"
#include "army_troop.h"
#include "castle_building_info.h"
#include "heroes.h"
#include "kingdom.h"
#include "payment.h"
#include "profit.h"
#include "difficulty.h"
#include "game.h"
#include "players.h"

namespace
{
    AI::EconomyProject::Resources resources( const Funds & value )
    { return { value.gold, value.wood, value.ore, value.mercury, value.sulfur, value.crystal, value.gems }; }

    AI::EconomyProject::Army army( const Troops & troops )
    {
        AI::EconomyProject::Army result{};
        for ( size_t i = 0; i < result.size() && i < troops.Size(); ++i ) {
            const auto * troop = troops.GetTroop( i );
            if ( troop && troop->isValid() ) result[i] = { troop->GetID(), troop->GetCount() };
        }
        return result;
    }
    AI::EconomyProject::Creature creature( const int race, const uint32_t dwelling )
    {
        if ( !dwelling ) return {};
        const Monster unit( race, dwelling );
        if ( !unit.isValid() ) return {};
        return { unit.GetID(), unit.GetGrown(), resources( unit.GetCost() ), unit.GetMonsterStrength() };
    }
}

AI::EconomyProject::Input AI::EconomyProject::capture( const Kingdom & kingdom, const uint32_t throughDay )
{
    Input result;
    result.start.economy = kingdom.economySnapshot();
    result.config = Conscription::settings();
    result.throughDay = throughDay;
    const uint32_t bases[] = { DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6 };
    const uint32_t upgrades[] = { 0, DWELLING_UPGRADE2, DWELLING_UPGRADE3, DWELLING_UPGRADE4, DWELLING_UPGRADE5, DWELLING_UPGRADE6 };
    for ( const auto * castle : kingdom.GetCastles() ) {
        if ( !castle ) continue;
        Town town;
        town.tile = castle->GetIndex(); town.race = castle->GetRace();
        town.buildAvailable = castle->Modes( Castle::ALLOW_TO_BUILD_TODAY );
        town.wellBonus = Castle::GetGrownWell(); town.farmBonus = Castle::GetGrownWel2();
        const auto * player = Players::Get( kingdom.GetColor() );
        town.incomeAttributionKnown = player && player->getHandicapStatus() == Player::HandicapStatus::NONE
            && ( !kingdom.isControlAI() || Game::getDifficulty() == Difficulty::NORMAL || Game::getDifficulty() == Difficulty::HARD );
        Funds recurring = ProfitConditions::FromBuilding( castle->isCastle() ? BUILD_CASTLE : BUILD_TENT, 0 );
        if ( castle->isBuild( BUILD_STATUE ) ) recurring += ProfitConditions::FromBuilding( BUILD_STATUE, 0 );
        if ( town.race == Race::WRLK && castle->isBuild( BUILD_SPEC ) ) recurring += ProfitConditions::FromBuilding( BUILD_SPEC, Race::WRLK );
        // This scenario makes a trapped visitor unavailable too; remove its known personal recurring income.
        if ( const auto * visitor = castle->GetHero() ) {
            recurring.gold += visitor->GetSecondarySkillValue( Skill::Secondary::ESTATES );
            for ( const auto & artifact : visitor->GetBagArtifacts() ) recurring += ProfitConditions::FromArtifact( artifact.GetID() );
        }
        town.recurringIncome = resources( recurring );
        town.garrison = army( castle->GetArmy() );
        if ( const auto * hero = castle->GetHero() ) { town.hero = hero->GetID(); town.visitor = army( hero->GetArmy() ); }
        for ( uint32_t bit = 1; bit; bit <<= 1 ) {
            if ( castle->isBuild( bit ) ) town.mask |= bit;
            if ( castle->isBuildingDisabled( bit ) ) town.disabled |= bit;
        }
        for ( size_t pool = 0; pool < 6; ++pool ) {
            auto & dwelling = town.dwellings[pool];
            dwelling.base = bases[pool]; dwelling.upgrade = upgrades[pool];
            dwelling.secondUpgrade = pool == 5 && town.race == Race::WRLK ? uint32_t{ DWELLING_UPGRADE7 } : 0;
            dwelling.basic = creature( town.race, dwelling.base );
            dwelling.improved = creature( town.race, dwelling.upgrade );
            dwelling.second = creature( town.race, dwelling.secondUpgrade );
            // Unsupported upgrades can map to the base monster; do not offer them as buildings.
            if ( dwelling.improved.id == dwelling.basic.id ) { dwelling.upgrade = 0; dwelling.improved = {}; }
            if ( pool >= 4 ) town.extraPools[pool - 4] = castle->getMonstersInDwelling( bases[pool] );
        }
        for ( uint32_t bit = 1; bit; bit <<= 1 ) {
            // Castle construction/shipyard change map state; shrine rules depend on game version.
            if ( bit == BUILD_CASTLE || bit == BUILD_TENT || bit == BUILD_SHIPYARD
                 || ( bit == BUILD_SHRINE && town.race != Race::KNGT )
                 || ( bit == BUILD_TAVERN && town.race == Race::NECR ) ) continue;
            Building building;
            building.id = bit;
            building.prerequisites = fheroes2::getBuildingRequirement( town.race, static_cast<BuildingType>( bit ) );
            if ( bit >= BUILD_MAGEGUILD2 && bit <= BUILD_MAGEGUILD5 ) building.prerequisites |= bit >> 1;
            building.price = resources( PaymentConditions::BuyBuilding( town.race, bit ) );
            if ( bit == BUILD_STATUE || ( bit == BUILD_SPEC && town.race == Race::WRLK ) )
                building.income = resources( ProfitConditions::FromBuilding( bit, town.race ) );
            bool supported = true;
            if ( bit & ( DWELLING_MONSTERS | DWELLING_UPGRADES ) ) {
                supported = false;
                for ( size_t pool = 0; pool < 6; ++pool ) {
                    const auto & dwelling = town.dwellings[pool];
                    if ( bit == dwelling.base ) { building.newPool = static_cast<int>( pool ); supported = dwelling.basic.id != 0; }
                    if ( ( bit == dwelling.upgrade && dwelling.improved.id ) || ( bit == dwelling.secondUpgrade && dwelling.second.id ) ) supported = true;
                }
            }
            if ( supported ) town.catalog.emplace( bit, building );
        }
        result.start.towns.push_back( std::move( town ) );
    }
    return result;
}
