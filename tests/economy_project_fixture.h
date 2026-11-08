#pragma once
#include "economy_project.h"

namespace P = AI::EconomyProject;

inline P::Action action( const uint64_t id, const uint32_t day, const P::Kind kind, const int32_t town = 42, const uint32_t value = 0 )
{
    P::Action result;
    result.id = result.project = id; result.day = day; result.kind = kind; result.town = town;
    if ( kind == P::Kind::Build ) result.building = value;
    else result.count = value;
    return result;
}

inline P::Input fixture( const bool training = false, const uint32_t day = 7 )
{
    P::Input input;
    input.throughDay = day + 1;
    input.config.requiredBuildings = BUILD_WEL2 | DWELLING_MONSTER2 | DWELLING_MONSTER3 | DWELLING_MONSTER4 | BUILD_CAPTAIN;
    auto & state = input.start;
    state.economy.day = day; state.economy.gold = 10000; state.economy.materials = { 20, 20, 0, 0, 0, 0 };
    state.economy.nonTaxIncome = 1000; state.economy.taxRate = 2; state.economy.peasantCost = 20;
    P::Town town;
    town.tile = 42; town.race = Race::KNGT; town.farmBonus = 8; town.wellBonus = 2;
    town.mask = BUILD_CASTLE | BUILD_CAPTAIN | DWELLING_MONSTER1 | DWELLING_MONSTER2 | DWELLING_MONSTER3 | DWELLING_MONSTER4
                | ( training ? BUILD_TRAINING_GROUNDS : BUILD_WEL2 );
    town.garrison[0] = { Monster::PEASANT, 100 };
    const uint32_t bases[] = { DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6 };
    const int monsters[] = { Monster::PEASANT, Monster::ARCHER, Monster::PIKEMAN, Monster::SWORDSMAN, Monster::CAVALRY, Monster::PALADIN };
    const uint32_t growth[] = { 12, 8, 5, 4, 3, 2 };
    const int64_t prices[] = { 20, 150, 200, 250, 300, 600 };
    for ( size_t i = 0; i < 6; ++i ) {
        town.dwellings[i].base = bases[i];
        town.dwellings[i].basic = { monsters[i], growth[i], { prices[i], 0, 0, 0, 0, 0, 0 } };
        P::Building building;
        building.id = bases[i]; building.price[0] = 1000; building.newPool = static_cast<int>( i );
        town.catalog.emplace( building.id, building );
    }
    for ( const auto buildingId : { BUILD_WEL2, BUILD_TRAINING_GROUNDS, BUILD_WELL, BUILD_STATUE } ) {
        P::Building building;
        building.id = buildingId; building.price[0] = 1000;
        town.catalog.emplace( building.id, building );
    }
    town.dwellings[1].upgrade = DWELLING_UPGRADE2;
    town.dwellings[1].improved = { Monster::RANGER, 8, { 200, 0, 0, 0, 0, 0, 0 } };
    P::Building upgrade;
    upgrade.id = DWELLING_UPGRADE2; upgrade.prerequisites = DWELLING_MONSTER2; upgrade.price[0] = 1000;
    town.catalog.emplace( upgrade.id, upgrade );
    Economy::Town economic;
    economic.tile = 42; economic.troops.garrison = 100; economic.troops.hut = 20;
    state.economy.towns = { economic }; state.towns = { town };
    P::refresh( state );
    return input;
}

inline P::Result run( const P::Input & input ) { P::WorkBudget work; return P::project( input, work ); }

