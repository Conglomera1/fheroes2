/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "taxpayer_test.h"
#include "taxpayer_policy.h"
#include <limits>

#include "ai_common.h"
#include "ai_planner.h" // IWYU pragma: associated
#include "army.h"
#include "army_troop.h"
#include "castle.h"
#include "castle_building_info.h"
#include "difficulty.h"
#include "game.h"
#include "heroes.h"
#include "kingdom.h"
#include "logging.h"
#include "maps_tiles.h"
#include "monster.h"
#include "payment.h"
#include "race.h"
#include "resource.h"
#include "world.h"
#include "world_regions.h"

namespace
{
    const char * taxpayerBuildingStatus( const BuildingStatus status )
    {
        switch ( status ) {
        case BuildingStatus::ALLOW_BUILD: return "allowed";
        case BuildingStatus::NOT_TODAY: return "daily building already used";
        case BuildingStatus::ALREADY_BUILT: return "already built";
        case BuildingStatus::NEED_CASTLE: return "Castle required";
        case BuildingStatus::BUILD_DISABLE: return "disabled on map";
        case BuildingStatus::SHIPYARD_NOT_ALLOWED: return "no sea access";
        case BuildingStatus::UNKNOWN_UPGRADE: return "invalid upgrade";
        case BuildingStatus::REQUIRES_BUILD: return "missing prerequisite";
        case BuildingStatus::LACK_RESOURCES: return "insufficient resources";
        default: return "unknown";
        }
    }

    struct BuildOrder
    {
        const BuildingType building = BUILD_NOTHING;
        const uint32_t priority = 1;

        BuildOrder() = default;
        BuildOrder( const BuildingType b, const uint32_t p )
            : building( b )
            , priority( p )
        {}
    };

    const std::vector<BuildOrder> defensiveStructures = { { BUILD_LEFTTURRET, 1 }, { BUILD_RIGHTTURRET, 1 }, { BUILD_MOAT, 1 }, { BUILD_CAPTAIN, 1 } };
    const std::vector<BuildOrder> supportingDefensiveStructures = { { BUILD_MAGEGUILD1, 1 }, { BUILD_SPEC, 2 }, { BUILD_TAVERN, 1 } };

    const std::vector<BuildOrder> & GetIncomeStructures( int type )
    {
        static const std::vector<BuildOrder> standard = { { BUILD_CASTLE, 1 }, { BUILD_STATUE, 1 }, { BUILD_MARKETPLACE, 1 } };
        static const std::vector<BuildOrder> warlock = { { BUILD_CASTLE, 1 }, { BUILD_STATUE, 1 }, { BUILD_MARKETPLACE, 1 }, { BUILD_SPEC, 1 } };

        return ( type == Race::WRLK ) ? warlock : standard;
    }

    const std::vector<BuildOrder> & GetBuildOrder( int type )
    {
        static const std::vector<BuildOrder> genericBuildOrder
            = { { BUILD_CASTLE, 2 },      { BUILD_STATUE, 1 },      { BUILD_MARKETPLACE, 1 }, { DWELLING_UPGRADE7, 1 },   { DWELLING_UPGRADE6, 1 },
                { DWELLING_MONSTER6, 1 }, { DWELLING_UPGRADE5, 1 }, { DWELLING_MONSTER5, 1 }, { DWELLING_UPGRADE4, 1 },   { DWELLING_MONSTER4, 1 },
                { DWELLING_UPGRADE3, 2 }, { DWELLING_MONSTER3, 2 }, { DWELLING_UPGRADE2, 3 }, { DWELLING_MONSTER2, 3 },   { DWELLING_MONSTER1, 4 },
                { BUILD_MAGEGUILD1, 2 },  { BUILD_WEL2, 10 },       { BUILD_TAVERN, 5 },      { BUILD_THIEVESGUILD, 10 }, { BUILD_MAGEGUILD2, 3 },
                { BUILD_MAGEGUILD3, 4 },  { BUILD_MAGEGUILD4, 5 },  { BUILD_MAGEGUILD5, 5 },  { BUILD_SHIPYARD, 4 } };

        static const std::vector<BuildOrder> barbarianBuildOrder
            = { { BUILD_CASTLE, 2 },        { BUILD_STATUE, 1 },      { BUILD_MARKETPLACE, 1 }, { DWELLING_MONSTER6, 1 }, { DWELLING_UPGRADE5, 1 },
                { DWELLING_MONSTER5, 1 },   { DWELLING_UPGRADE4, 1 }, { DWELLING_MONSTER4, 1 }, { DWELLING_MONSTER3, 1 }, { DWELLING_UPGRADE2, 2 },
                { DWELLING_MONSTER2, 2 },   { DWELLING_MONSTER1, 4 }, { BUILD_MAGEGUILD1, 3 },  { BUILD_WEL2, 10 },       { BUILD_TAVERN, 5 },
                { BUILD_THIEVESGUILD, 10 }, { BUILD_MAGEGUILD2, 4 },  { BUILD_MAGEGUILD3, 5 },  { BUILD_MAGEGUILD4, 6 },  { BUILD_MAGEGUILD5, 7 },
                { BUILD_SHIPYARD, 4 } };

        static const std::vector<BuildOrder> sorceressBuildOrder
            = { { BUILD_CASTLE, 2 },        { BUILD_STATUE, 1 },      { BUILD_MARKETPLACE, 1 }, { DWELLING_MONSTER6, 1 }, { DWELLING_MONSTER5, 1 },
                { DWELLING_MONSTER4, 1 },   { BUILD_MAGEGUILD1, 1 },  { DWELLING_MONSTER3, 1 }, { DWELLING_UPGRADE4, 1 }, { DWELLING_UPGRADE3, 2 },
                { DWELLING_UPGRADE2, 5 },   { DWELLING_MONSTER2, 2 }, { BUILD_TAVERN, 2 },      { DWELLING_MONSTER1, 4 }, { BUILD_WEL2, 10 },
                { BUILD_THIEVESGUILD, 10 }, { BUILD_MAGEGUILD2, 3 },  { BUILD_MAGEGUILD3, 4 },  { BUILD_MAGEGUILD4, 5 },  { BUILD_MAGEGUILD5, 5 },
                { BUILD_SHIPYARD, 4 } };

        // De-prioritizing dwelling 5 (you can reach 6 without it), 1 and upgrades of 3 and 4
        // Well, tavern and Archery upgrade are more important
        static const std::vector<BuildOrder> knightBuildOrder
            = { { BUILD_CASTLE, 2 },      { BUILD_STATUE, 1 },      { BUILD_MARKETPLACE, 1 },   { DWELLING_UPGRADE6, 2 }, { DWELLING_MONSTER6, 1 },
                { DWELLING_UPGRADE5, 2 }, { DWELLING_MONSTER5, 2 }, { DWELLING_UPGRADE4, 2 },   { DWELLING_MONSTER4, 1 }, { DWELLING_UPGRADE3, 2 },
                { DWELLING_MONSTER3, 1 }, { DWELLING_UPGRADE2, 1 }, { DWELLING_MONSTER2, 3 },   { DWELLING_MONSTER1, 4 }, { BUILD_WELL, 1 },
                { BUILD_TAVERN, 1 },      { BUILD_MAGEGUILD1, 2 },  { BUILD_MAGEGUILD2, 3 },    { BUILD_MAGEGUILD3, 5 },  { BUILD_MAGEGUILD4, 5 },
                { BUILD_MAGEGUILD5, 5 },  { BUILD_SPEC, 5 },        { BUILD_THIEVESGUILD, 10 }, { BUILD_WEL2, 20 },       { BUILD_SHIPYARD, 4 } };

        // Priority on Dwellings 5/6 and Mage guild level 2
        static const std::vector<BuildOrder> necromancerBuildOrder
            = { { BUILD_CASTLE, 2 },      { BUILD_STATUE, 1 },       { BUILD_MARKETPLACE, 1 }, { DWELLING_UPGRADE6, 1 }, { DWELLING_MONSTER6, 1 },
                { DWELLING_UPGRADE5, 2 }, { DWELLING_MONSTER5, 1 },  { BUILD_MAGEGUILD1, 1 },  { DWELLING_UPGRADE4, 2 }, { DWELLING_MONSTER4, 1 },
                { DWELLING_UPGRADE3, 3 }, { DWELLING_MONSTER3, 3 },  { DWELLING_UPGRADE2, 4 }, { DWELLING_MONSTER2, 2 }, { DWELLING_MONSTER1, 3 },
                { BUILD_MAGEGUILD2, 2 },  { BUILD_THIEVESGUILD, 2 }, { BUILD_WEL2, 8 },        { BUILD_MAGEGUILD3, 4 },  { BUILD_MAGEGUILD4, 5 },
                { BUILD_MAGEGUILD5, 5 },  { BUILD_SHRINE, 10 },      { BUILD_SHIPYARD, 4 } };

        // Priority on Mage tower/guild and library
        static const std::vector<BuildOrder> wizardBuildOrder
            = { { BUILD_CASTLE, 2 },        { BUILD_STATUE, 1 },      { BUILD_MARKETPLACE, 1 }, { DWELLING_UPGRADE6, 1 }, { DWELLING_MONSTER6, 1 },
                { DWELLING_UPGRADE5, 1 },   { DWELLING_MONSTER5, 1 }, { DWELLING_MONSTER4, 1 }, { DWELLING_MONSTER3, 1 }, { DWELLING_MONSTER2, 1 },
                { DWELLING_MONSTER1, 1 },   { BUILD_MAGEGUILD1, 1 },  { DWELLING_UPGRADE3, 4 }, { BUILD_SPEC, 2 },        { BUILD_WEL2, 8 },
                { BUILD_MAGEGUILD2, 3 },    { BUILD_MAGEGUILD3, 4 },  { BUILD_MAGEGUILD4, 4 },  { BUILD_MAGEGUILD5, 4 },  { BUILD_TAVERN, 10 },
                { BUILD_THIEVESGUILD, 10 }, { BUILD_SHIPYARD, 4 } };

        switch ( type ) {
        case Race::KNGT:
            return knightBuildOrder;
        case Race::NECR:
            return necromancerBuildOrder;
        case Race::WZRD:
            return wizardBuildOrder;
        case Race::SORC:
            return sorceressBuildOrder;
        case Race::BARB:
            return barbarianBuildOrder;
        default:
            break;
        }

        return genericBuildOrder;
    }

    bool Build( Castle & castle, const std::vector<BuildOrder> & buildOrderList, const uint32_t multiplier = 1 )
    {
        const int gameDifficulty = Game::getDifficulty();
        const bool isGameCampaign = Game::isCampaign();

        for ( const BuildOrder & order : buildOrderList ) {
            if ( !Difficulty::allowAIToBuildCastleBuilding( gameDifficulty, isGameCampaign, order.building ) ) {
                continue;
            }

            uint32_t priority = order.priority;
            if ( order.building == BUILD_WEL2 && castle.GetRace() == Race::KNGT && Kingdom::TaxpayerRequiresFarms()
                 && Kingdom::GetTaxpayerGoldPerPeasantPerDay() > 0 ) {
                // Prioritize the Farm when it unlocks Taxpayer income for this Knight town.
                priority = 2;
            }

            const uint32_t fundsMultiplier = priority * multiplier;
            // Trace Knight town construction in both Taxpayer modes so their logs can be compared.
            const bool traceKnightBuild = castle.GetRace() == Race::KNGT && Settings::Get().taxpayerTestDiagnostics() >= 2;

            if ( traceKnightBuild ) {
                COUT( Logging::GetTimeString() << ": [TAXPAYER AI TEST] " <<
                           "Day " << world.CountDay() << ", Knight town tile " << castle.GetIndex() << ": considering building id "
                                  << static_cast<uint32_t>( order.building ) << ", priority " << order.priority << " -> " << priority
                                  << ", funds multiplier " << fundsMultiplier );
            }

            if ( castle.GetRace() == Race::KNGT ) {
                TAXPAYER_TRACE( 2, "Day " << world.CountDay() << ", player " << Color::String( castle.GetColor() ) << ", town " << castle.GetIndex()
                                   << ": build candidate " << order.building << ", status " << taxpayerBuildingStatus( castle.CheckBuyBuilding( order.building ) )
                                   << ", prerequisite mask " << fheroes2::getBuildingRequirement( castle.GetRace(), order.building )
                                   << ", cost " << PaymentConditions::BuyBuilding( castle.GetRace(), order.building ).String()
                                   << ", resources " << castle.GetKingdom().GetFunds().String() << ", multiplier " << fundsMultiplier );
            }
            bool built = false;
            if ( fundsMultiplier == 1 ) {
                built = AI::BuildIfPossible( castle, order.building );
            }
            else {
                built = AI::BuildIfEnoughFunds( castle, order.building, fundsMultiplier );
            }

            if ( traceKnightBuild ) {
                COUT( Logging::GetTimeString() << ": [TAXPAYER AI TEST] " <<
                           "Day " << world.CountDay() << ", Knight town tile " << castle.GetIndex() << ": building id "
                                  << static_cast<uint32_t>( order.building ) << ( built ? " BUILT" : " not selected (fund threshold or building unavailable)" ) );
            }

            if ( built ) {
                return true;
            }
        }

        return false;
    }

    // Economic recruitment is deliberately separate from military reinforcement. It uses only spare gold and
    // never fills the last free garrison slot with a new Peasant stack.
    void recruitTaxpayerPeasants( Castle & castle )
    {
        const char * decision = "ineligible: faction, Farm, dwelling, tax mode or zero rate";
        const fheroes2::TaxpayerTraceScope recruitmentTrace( [&castle, &decision]() {
            TAXPAYER_TRACE( 2, "Day " << world.CountDay() << ", player " << Color::String( castle.GetColor() ) << ", town " << castle.GetIndex()
                               << ": economic recruitment " << decision << ", stock " << castle.getMonstersInDwelling( DWELLING_MONSTER1 )
                               << ", treasury " << castle.GetKingdom().GetFunds().gold << ", occupied garrison slots " << castle.GetArmy().GetOccupiedSlotCount()
                               << ", rate " << Kingdom::GetTaxpayerGoldPerPeasantPerDay() );
        } );
        if ( castle.GetRace() != Race::KNGT || !castle.isBuild( BUILD_WEL2 ) || !castle.isBuild( DWELLING_MONSTER1 )
             || !Kingdom::TaxpayerRequiresFarms() || Kingdom::GetTaxpayerGoldPerPeasantPerDay() <= 0 ) {
            return;
        }

        const Monster peasant( Monster::PEASANT );
        const Funds peasantCost = peasant.GetCost();

        decision = "rejected: payback exceeds 14 days or invalid cost";
        // A Peasant bought solely as a taxpayer should repay its purchase within two weeks.
        if ( peasantCost.gold <= 0 || static_cast<int64_t>( peasantCost.gold ) > static_cast<int64_t>( Kingdom::GetTaxpayerGoldPerPeasantPerDay() ) * 14 ) {
            return;
        }

        decision = "rejected: garrison capacity / last free slot reserved";
        const Army & garrison = castle.GetArmy();
        const uint32_t occupiedSlots = garrison.GetOccupiedSlotCount();
        if ( occupiedSlots >= 5 || ( occupiedSlots >= 4 && garrison.GetCountMonsters( peasant ) == 0 ) ) {
            return;
        }

        // Reserve at least 2000 gold and at least 75% of the current treasury for other AI needs.
        const Funds & treasury = castle.GetKingdom().GetFunds();
        const int32_t goldBudget = std::min( std::max( 0, treasury.gold - 2000 ), treasury.gold / 4 );
        decision = "rejected: reserve of 2000 gold / 25 percent investment budget";
        TAXPAYER_TRACE( 2, "Day " << world.CountDay() << ", town " << castle.GetIndex() << ": economic gold budget " << goldBudget
                           << ", unit cost " << peasantCost.gold );
        if ( goldBudget < peasantCost.gold ) {
            return;
        }

        Funds budget = treasury;
        budget.gold = goldBudget;
        decision = "rejected: no stock or recruitment failed";
        const uint32_t peasantCount = castle.getRecruitLimit( peasant, budget );
        if ( peasantCount == 0 || !castle.RecruitMonster( Troop( peasant, peasantCount ), false ) ) {
            return;
        }

        decision = "accepted";
        if ( Settings::Get().taxpayerTestDiagnostics() >= 1 ) {
            COUT( Logging::GetTimeString() << ": [TAXPAYER AI ECONOMY] Day " << world.CountDay() << ", Knight town tile " << castle.GetIndex()
                                        << ": recruited " << peasantCount << " Peasants for tax, estimated daily income +"
                                        << static_cast<uint64_t>( peasantCount ) * static_cast<uint64_t>( Kingdom::GetTaxpayerGoldPerPeasantPerDay() ) );
        }
    }

    bool buildQuickPaybackFarm( Castle & castle )
    {
        if ( castle.GetRace() != Race::KNGT || castle.isBuild( BUILD_WEL2 ) ) {
            return false;
        }

        const int32_t rate = Kingdom::GetTaxpayerGoldPerPeasantPerDay();
        if ( !Kingdom::TaxpayerRequiresFarms() || rate <= 0 ) {
            return false;
        }

        const Heroes * guest = castle.GetHero();
        const uint64_t peasants = static_cast<uint64_t>( castle.GetArmy().GetCountMonsters( Monster::PEASANT ) )
                                  + ( guest ? guest->GetArmy().GetCountMonsters( Monster::PEASANT ) : 0 );
        const uint64_t dailyIncome = peasants * static_cast<uint64_t>( rate );
        const uint32_t farmCost = PaymentConditions::BuyBuilding( castle.GetRace(), BUILD_WEL2 ).gold;
        const bool quickPayback = AI::taxpayerFarmPaysBackQuickly( dailyIncome, farmCost );
        const uint64_t paybackDays = dailyIncome > 0 ? farmCost / dailyIncome + ( farmCost % dailyIncome != 0 ) : 0;
        const BuildingStatus status = castle.CheckBuyBuilding( BUILD_WEL2 );
        const bool difficultyAllows = Difficulty::allowAIToBuildCastleBuilding( Game::getDifficulty(), Game::isCampaign(), BUILD_WEL2 );
        const bool built = quickPayback && difficultyAllows && AI::BuildIfPossible( castle, BUILD_WEL2 );

        TAXPAYER_TRACE( 2, "Day " << world.CountDay() << ", player " << Color::String( castle.GetColor() ) << ", town " << castle.GetIndex()
                           << ": early Farm investment, Peasants " << peasants << ", daily tax " << dailyIncome << ", cost " << farmCost
                           << ", payback days " << paybackDays << ", quick payback " << quickPayback << ", difficulty allows " << difficultyAllows
                           << ", status " << taxpayerBuildingStatus( status ) << ", funds multiplier 1, built " << built );
        if ( built && Settings::Get().taxpayerTestDiagnostics() >= 2 ) {
            COUT( Logging::GetTimeString() << ": [TAXPAYER AI TEST] Day " << world.CountDay() << ", Knight town tile " << castle.GetIndex()
                                         << ": building id " << static_cast<uint32_t>( BUILD_WEL2 ) << " BUILT" );
        }
        return built;
    }

    bool CastleDevelopment( Castle & castle, int safetyFactor, int spellLevel )
    {
        if ( !Difficulty::allowAIToDevelopCastlesOnDay( Game::getDifficulty(), Game::isCampaign(), world.CountDay() ) ) {
            return false;
        }

        // Defensive turns never enter this development path. Keep all building prerequisites,
        // but examine a rapidly profitable Farm before ordinary development and budget multipliers.
        if ( buildQuickPaybackFarm( castle ) ) {
            return true;
        }

        if ( castle.isCastle() && !castle.isBuild( BUILD_WELL ) && world.CountDay() > 6 ) {
            // If you can't build Well, you won't be able to build anything else
            return AI::BuildIfPossible( castle, BUILD_WELL );
        }

        if ( Build( castle, GetIncomeStructures( castle.GetRace() ) ) ) {
            return true;
        }

        const size_t neighbourRegions = world.getRegion( world.getTile( castle.GetIndex() ).GetRegion() ).getNeighboursCount();
        const bool islandOrPeninsula = neighbourRegions < 3;

        // Force building a shipyard, +1 to cost check since we can have 0 neighbours
        if ( islandOrPeninsula && AI::BuildIfEnoughFunds( castle, BUILD_SHIPYARD, static_cast<uint32_t>( neighbourRegions + 1 ) ) ) {
            return true;
        }

        if ( Build( castle, GetBuildOrder( castle.GetRace() ) ) ) {
            return true;
        }

        if ( castle.GetLevelMageGuild() < spellLevel && safetyFactor > 0 ) {
            static const std::vector<BuildOrder> magicGuildUpgrades
                = { { BUILD_MAGEGUILD2, 2 }, { BUILD_MAGEGUILD3, 2 }, { BUILD_MAGEGUILD4, 1 }, { BUILD_MAGEGUILD5, 1 } };
            if ( Build( castle, magicGuildUpgrades ) ) {
                return true;
            }
        }

        // Check whether it is worth buying a boat
        const bool buyBoat = [&castle, islandOrPeninsula]() {
            // Check if it is possible to build a boat in this castle at all (except for the solvency check)
            if ( !castle.AllowBuyBoat( false ) ) {
                return false;
            }

            Kingdom & kingdom = castle.GetKingdom();

            // If there are no active heroes in the kingdom, then there will be no one to use the boat
            if ( kingdom.GetHeroes().empty() ) {
                return false;
            }

            // Perhaps the kingdom already has the necessary supply of resources
            const Funds requiredFunds = PaymentConditions::BuyBoat() * ( islandOrPeninsula ? 2 : 4 );
            if ( kingdom.AllowPayment( requiredFunds ) ) {
                return true;
            }

            // Even if the kingdom does not have the necessary supply of these resources right now, there may be enough resources of another type available to get the
            // missing resources as a result of resource exchange
            if ( !AI::calculateMarketplaceTransaction( kingdom, requiredFunds ) ) {
                return false;
            }

            // There are resources available to buy a boat right now
            if ( kingdom.AllowPayment( PaymentConditions::BuyBoat() ) ) {
                return true;
            }

            // There are no available resources, but it is possible to make a resource exchange
            if ( AI::tradeAtMarketplace( kingdom, PaymentConditions::BuyBoat() ) ) {
                return true;
            }

            assert( 0 );
            return false;
        }();

        // If yes, then buy a boat
        if ( buyBoat ) {
            castle.BuyBoat();
        }

        if ( Build( castle, defensiveStructures, 10 ) ) {
            return true;
        }

        return Build( castle, supportingDefensiveStructures, 10 );
    }
}

void AI::Planner::CastlePreBattle( Castle & castle )
{
    Heroes * hero = world.GetHero( castle );
    if ( hero == nullptr ) {
        return;
    }

    Army & army = hero->GetArmy();

    if ( !army.ArrangeForCastleDefense( castle.GetArmy() ) ) {
        return;
    }

    // Optimization cannot be performed if we have not received any reinforcements from the garrison, otherwise the actual placement of units during the battle will
    // differ from that observed by the enemy player before the start of the battle
    OptimizeTroopsOrder( army );
}

void AI::Planner::updateKingdomBudget( const Kingdom & kingdom )
{
    // clean up first
    for ( BudgetEntry & budgetEntry : _budget ) {
        budgetEntry.reset();
    }

    const Funds & kindgomFunds = kingdom.GetFunds();
    Funds requirements;
    for ( const Castle * castle : kingdom.GetCastles() ) {
        if ( !castle ) {
            continue;
        }

        const int race = castle->GetRace();
        const std::vector<BuildOrder> & buildOrder = GetBuildOrder( race );
        for ( const BuildOrder & order : buildOrder ) {
            const BuildingStatus status = castle->CheckBuyBuilding( order.building );
            if ( status == BuildingStatus::LACK_RESOURCES ) {
                Funds missing = PaymentConditions::BuyBuilding( race, order.building ) - kindgomFunds;

                requirements = requirements.max( missing );
            }
        }

        if ( castle->isBuild( DWELLING_MONSTER6 ) ) {
            Funds bestUnitCost = Monster( race, DWELLING_MONSTER6 ).GetUpgrade().GetCost();
            for ( BudgetEntry & budgetEntry : _budget ) {
                if ( bestUnitCost.Get( budgetEntry.resource ) > 0 ) {
                    budgetEntry.recurringCost = true;
                }
            }
        }
    }

    for ( BudgetEntry & budgetEntry : _budget ) {
        budgetEntry.missing = requirements.Get( budgetEntry.resource );

        if ( budgetEntry.missing ) {
            budgetEntry.priority = true;
        }
    }
}

bool AI::Planner::shouldKeepTaxpayersInGarrison( const Castle & castle ) const
{
    return castle.GetRace() == Race::KNGT && castle.isBuild( BUILD_WEL2 ) && Kingdom::TaxpayerRequiresFarms()
           && Kingdom::GetTaxpayerGoldPerPeasantPerDay() > 0;
}

bool AI::Planner::shouldPreserveTaxpayerPeasants( const Castle & castle, const Troop & candidate, const Troop & incoming ) const
{
    if ( !shouldKeepTaxpayersInGarrison( castle ) || candidate.GetID() != Monster::PEASANT ) {
        return false;
    }
    const uint64_t weeklyTax = static_cast<uint64_t>( candidate.GetCount() ) * static_cast<uint64_t>( Kingdom::GetTaxpayerGoldPerPeasantPerDay() ) * 7;
    const uint64_t recruitmentValue = static_cast<uint64_t>( std::max<int32_t>( 1000, incoming.GetTotalCost().gold ) );
    double currentRatio = std::numeric_limits<double>::max();
    double replacementRatio = std::numeric_limits<double>::max();
    unsigned knownThreats = 0;
    const auto task = _priorityTargets.find( castle.GetIndex() );
    if ( task != _priorityTargets.end() && task->second.type == PriorityTaskType::DEFEND ) {
        for ( const int32_t enemyIndex : task->second.secondaryTaskTileId ) {
            const auto enemy = _enemyArmies.find( enemyIndex );
            if ( enemy == _enemyArmies.end() || enemy->second.strength <= 0 ) {
                continue;
            }
            ++knownThreats;
            currentRatio = std::min( currentRatio, castle.GetGarrisonStrength( enemy->second.hero ) / enemy->second.strength );
            replacementRatio = std::min( replacementRatio,
                                         castle.GetGarrisonStrength( enemy->second.hero, &candidate, &incoming ) / enemy->second.strength );
        }
    }
    bool outsideHero = false;
    for ( const Heroes * hero : castle.GetKingdom().GetHeroes() ) {
        if ( hero != nullptr && hero->isActive() && hero->GetIndex() != castle.GetIndex() ) {
            outsideHero = true;
            break;
        }
    }
    const bool lastStand = taxpayerLastStand( static_cast<unsigned>( castle.GetKingdom().GetCastles().size() ), outsideHero, knownThreats > 0 );
    const TaxpayerDefenseDecision decision = taxpayerDefenseDecision( currentRatio, replacementRatio, lastStand, knownThreats > 0 );
    const bool preserve = decision == TaxpayerDefenseDecision::Preserve;
    TAXPAYER_TRACE( 1, "Day " << world.CountDay() << ", player " << Color::String( castle.GetColor() ) << ", town " << castle.GetIndex()
                       << ": taxpayer replacement decision " << static_cast<int>( decision ) << ", preserve " << preserve
                       << ", count " << candidate.GetCount() << ", weekly tax " << weeklyTax << ", incoming gold value " << recruitmentValue
                       << ", known defense threats " << knownThreats << ", current defense ratio " << ( knownThreats ? currentRatio : -1 )
                       << ", replacement defense ratio " << ( knownThreats ? replacementRatio : -1 ) << ", last stand " << lastStand );
    return preserve;
}

void AI::Planner::reinforceCastle( Castle & castle, const bool defensiveStrategy )
{
    assert( castle.isControlAI() );

    const uint32_t taxpayerBeforeGarrison = castle.GetArmy().GetCountMonsters( Monster::PEASANT );
    const Heroes * taxpayerGuest = castle.GetHero();
    const uint32_t taxpayerBeforeHero = taxpayerGuest ? taxpayerGuest->GetArmy().GetCountMonsters( Monster::PEASANT ) : 0;
    const fheroes2::TaxpayerTraceScope reinforcementTrace( [&]() {
        const Heroes * guest = castle.GetHero();
        TAXPAYER_TRACE( 2, "Day " << world.CountDay() << ", player " << Color::String( castle.GetColor() ) << ", town " << castle.GetIndex()
                           << ": reinforcement balance garrison " << taxpayerBeforeGarrison << " -> " << castle.GetArmy().GetCountMonsters( Monster::PEASANT )
                           << ", hero " << taxpayerBeforeHero << " -> " << ( guest ? guest->GetArmy().GetCountMonsters( Monster::PEASANT ) : 0 )
                           << ", guest id " << ( guest ? guest->GetID() : -1 ) << ", defensive strategy " << defensiveStrategy
                           << ", critical military task " << isCriticalTask( castle.GetIndex() ) << ", regional safety "
                           << _regions.at( world.getTile( castle.GetIndex() ).GetRegion() ).safetyFactor
                           << ", economic protection " << shouldKeepTaxpayersInGarrison( castle ) );
    } );


    const auto recruitMonster = [&castle]( const Troop & troop ) {
        // This method can hire a unit to both the castle garrison and the guest hero's army (depending on the availability of suitable slots)
        if ( castle.RecruitMonster( troop, false ) ) {
            DEBUG_LOG( DBG_AI, DBG_INFO, castle.GetName() << " hires " << troop.GetCount() << " " << troop.GetPluralName( troop.GetCount() ) )

            return true;
        }

        return false;
    };

    // First of all, upgrade the existing units in the garrison and in the guest hero's army (if there is one) and merge the same ones to free
    // up as much slots as possible for new units

    Army & garrison = castle.GetArmy();

    garrison.UpgradeTroops( castle );
    garrison.MergeSameMonsterTroops();

    Heroes * guestHero = castle.GetHero();
    if ( guestHero ) {
        assert( guestHero->isActive() && guestHero->isControlAI() && guestHero->GetColor() == castle.GetColor() );

        Army & guestHeroArmy = guestHero->GetArmy();

        guestHeroArmy.UpgradeTroops( castle );
        guestHeroArmy.MergeSameMonsterTroops();
    }

    // It is allowed to hire non-upgraded units even if an upgraded dwelling is built
    static const std::array<uint32_t, 12> castleDwellings{ DWELLING_UPGRADE7, DWELLING_UPGRADE6, DWELLING_MONSTER6, DWELLING_UPGRADE5,
                                                           DWELLING_MONSTER5, DWELLING_UPGRADE4, DWELLING_MONSTER4, DWELLING_UPGRADE3,
                                                           DWELLING_MONSTER3, DWELLING_UPGRADE2, DWELLING_MONSTER2, DWELLING_MONSTER1 };

    for ( const uint32_t dwelling : castleDwellings ) {
        if ( !castle.isBuild( dwelling ) ) {
            continue;
        }

        const Monster monster( castle.GetRace(), dwelling );

        const uint32_t count = castle.getRecruitLimit( monster, castle.GetKingdom().GetFunds() );
        if ( count == 0 ) {
            continue;
        }

        const Troop troop( monster, count );
        if ( recruitMonster( troop ) ) {
            continue;
        }

        Troop * weakestTroop = nullptr;

        if ( guestHero ) {
            Army & guestHeroArmy = guestHero->GetArmy();

            // If there is a guest hero in the castle, and there is no slot for a new unit, then we can try to free up a slot by transferring some unit to the guest hero
            if ( [this, &castle = std::as_const( castle ), &garrison, &guestHero = std::as_const( *guestHero ), &guestHeroArmy, &troop]() {
                     for ( size_t i = 0; i < garrison.Size(); ++i ) {
                         Troop * garrisonTroop = garrison.GetTroop( i );
                         // All garrison slots should be occupied here because we just couldn't find a place for a new unit
                         assert( garrisonTroop != nullptr && garrisonTroop->isValid() );

                         if ( garrisonTroop->GetID() == Monster::PEASANT && shouldKeepTaxpayersInGarrison( castle ) ) {
                             continue;
                         }

                         if ( guestHeroArmy.JoinTroop( *garrisonTroop ) ) {
                             DEBUG_LOG( DBG_AI, DBG_TRACE,
                                        castle.GetName() << " transfers " << garrisonTroop->GetCount() << " " << garrisonTroop->GetPluralName( garrisonTroop->GetCount() )
                                                         << " to the army of the guest hero " << guestHero.GetName() )
#ifndef WITH_DEBUG
                             (void)castle;
                             (void)guestHero;
#endif

                             garrisonTroop->Reset();

                             return true;
                         }
                     }

                     return false;
                 }() ) {
                if ( !recruitMonster( troop ) ) {
                    // We have just successfully transferred a unit from the garrison to the guest hero's army, but we still can't hire a new unit. This shouldn't happen.
                    assert( 0 );
                }

                continue;
            }

            weakestTroop = guestHeroArmy.GetWeakestTroop();
            // All slots in the guest hero's army should be occupied here because we just couldn't find a place for a new unit
            assert( weakestTroop != nullptr );
        }

        {
            Troop * weakestGarrisonTroop = garrison.GetWeakestTroop();
            // All garrison slots should be occupied here because we just couldn't find a place for a new unit
            assert( weakestGarrisonTroop != nullptr );

            // We need to compare a strength of troops themselves here (excluding commanding hero's stats)
            weakestTroop = weakestTroop && Troop( *weakestTroop ).GetStrength() < Troop( *weakestGarrisonTroop ).GetStrength() ? weakestTroop : weakestGarrisonTroop;
        }

        // If we still can't find a slot, let's try to dismiss the weakest unit of those that are present in the garrison and in the army of the guest hero - provided
        // that it is weaker than the unit to hire

        assert( weakestTroop != nullptr );

        // We need to compare a strength of troops themselves here (excluding commanding hero's stats)
        if ( shouldPreserveTaxpayerPeasants( castle, *weakestTroop, troop ) ) {
            if ( Settings::Get().taxpayerTestDiagnostics() >= 1 ) {
                COUT( Logging::GetTimeString() << ": [TAXPAYER AI ECONOMY] Day " << world.CountDay() << ", Knight town tile " << castle.GetIndex()
                                            << ": kept " << weakestTroop->GetCount() << " Peasants over recruitment of " << troop.GetCount()
                                            << " " << troop.GetPluralName( troop.GetCount() ) );
            }
            continue;
        }

        if ( Troop( *weakestTroop ).GetStrength() > troop.GetStrength() ) {
            DEBUG_LOG( DBG_AI, DBG_TRACE,
                       castle.GetName() << " skips hiring " << troop.GetCount() << " " << troop.GetPluralName( troop.GetCount() )
                                        << " because the weakest unit consisting of " << weakestTroop->GetCount() << " "
                                        << weakestTroop->GetPluralName( weakestTroop->GetCount() ) << " is stronger" )

            continue;
        }

        DEBUG_LOG( DBG_AI, DBG_TRACE, castle.GetName() << " dismisses " << weakestTroop->GetCount() << " " << weakestTroop->GetPluralName( weakestTroop->GetCount() ) )

        if ( weakestTroop->GetID() == Monster::PEASANT && castle.GetRace() == Race::KNGT && castle.isBuild( BUILD_WEL2 )
             && Kingdom::TaxpayerRequiresFarms() && Kingdom::GetTaxpayerGoldPerPeasantPerDay() > 0 ) {
            if ( Settings::Get().taxpayerTestDiagnostics() >= 1 ) {
                COUT( Logging::GetTimeString() << ": [TAXPAYER AI ECONOMY] Day " << world.CountDay() << ", Knight town tile " << castle.GetIndex()
                                            << ": dismissed " << weakestTroop->GetCount() << " Peasants for military recruitment of "
                                            << troop.GetCount() << " " << troop.GetPluralName( troop.GetCount() ) );
            }
        }

        weakestTroop->Reset();

        if ( !recruitMonster( troop ) ) {
            // We have just successfully dismissed a unit, but we still can't hire a new unit. This shouldn't happen.
            assert( 0 );
        }
    }

    if ( guestHero ) {
        Army & guestHeroArmy = guestHero->GetArmy();

        // Keep the taxable population out of routine military transfers. Otherwise a later
        // deposit takes back the same Peasants after every reinforcement, even on the same day.
        const Monster peasant( Monster::PEASANT );
        const uint32_t taxpayers = garrison.GetCountMonsters( peasant );
        if ( taxpayers > 0 && shouldKeepTaxpayersInGarrison( castle ) ) {
            const Troops garrisonBackup{ garrison.getTroops() };
            const Troops heroBackup{ guestHeroArmy.getTroops() };

            for ( size_t slot = 0; slot < garrison.Size(); ++slot ) {
                Troop * troop = garrison.GetTroop( slot );
                assert( troop != nullptr );
                if ( troop->isValid() && troop->GetID() == Monster::PEASANT ) {
                    troop->Reset();
                }
            }

            guestHeroArmy.JoinStrongestFromArmy( garrison );
            if ( garrison.JoinTroop( peasant, taxpayers, false ) ) {
                if ( Settings::Get().taxpayerTestDiagnostics() >= 1 ) {
                    COUT( Logging::GetTimeString() << ": [TAXPAYER AI ECONOMY] Day " << world.CountDay() << ", Knight town tile " << castle.GetIndex()
                                                << ": retained " << taxpayers << " Peasants during reinforcement of hero " << guestHero->GetName() );
                }
            }
            else {
                // If the military rearrangement needs all garrison slots, restore both armies
                // and cancel the rearrangement. Slot pressure alone must not export taxpayers.
                garrison.Assign( garrisonBackup );
                guestHeroArmy.Assign( heroBackup );
                if ( Settings::Get().taxpayerTestDiagnostics() >= 1 ) {
                    COUT( Logging::GetTimeString() << ": [TAXPAYER AI ECONOMY] Day " << world.CountDay() << ", Knight town tile " << castle.GetIndex()
                                                << ": cancelled military rearrangement to preserve taxpayers and troop counts" );
                }
            }
        }
        else {
            guestHeroArmy.JoinStrongestFromArmy( garrison );
        }

        // Check if we should leave some troops in the garrison
        // TODO: amount of troops left could depend on region's safetyFactor
        if ( const uint32_t regionID = world.getTile( castle.GetIndex() ).GetRegion();
             castle.isCastle() && _regions.at( regionID ).safetyFactor <= 100 && !garrison.isValid() ) {
            auto [troopForTransferToGarrison, transferHalf]
                = [guestHeroRole = guestHero->getAIRole(), &guestHeroArmy = std::as_const( guestHeroArmy )]() -> std::pair<Troop *, bool> {
                const bool isFighterRole = ( guestHeroRole == Heroes::Role::FIGHTER || guestHeroRole == Heroes::Role::CHAMPION );

                // We need to compare a strength of troops themselves here (excluding commanding hero's stats)
                const double troopsStrength = Troops( guestHeroArmy.getTroops() ).GetStrength();
                const double significanceRatio = isFighterRole ? 20.0 : 10.0;

                {
                    Troop * candidateTroop = guestHeroArmy.GetSlowestTroop();
                    assert( candidateTroop != nullptr );

                    // We need to compare a strength of troops themselves here (excluding commanding hero's stats)
                    if ( Troop( *candidateTroop ).GetStrength() <= troopsStrength / significanceRatio ) {
                        return { candidateTroop, false };
                    }
                }

                // if this is an important hero, then all his troops are significant
                if ( isFighterRole ) {
                    return {};
                }

                {
                    Troop * candidateTroop = guestHeroArmy.GetWeakestTroop();
                    assert( candidateTroop != nullptr );

                    // We need to compare a strength of troops themselves here (excluding commanding hero's stats)
                    if ( Troop( *candidateTroop ).GetStrength() <= troopsStrength / significanceRatio ) {
                        return { candidateTroop, true };
                    }
                }

                return {};
            }();

            if ( troopForTransferToGarrison ) {
                assert( guestHeroArmy.GetOccupiedSlotCount() > 1 );

                const uint32_t initialCount = troopForTransferToGarrison->GetCount();
                const uint32_t countToTransfer = transferHalf ? initialCount / 2 : initialCount;

                if ( garrison.JoinTroop( troopForTransferToGarrison->GetMonster(), countToTransfer, true ) ) {
                    if ( countToTransfer == initialCount ) {
                        troopForTransferToGarrison->Reset();
                    }
                    else {
                        troopForTransferToGarrison->SetCount( initialCount - countToTransfer );
                    }
                }
            }
        }
    }

    OptimizeTroopsOrder( garrison );

    if ( guestHero ) {
        OptimizeTroopsOrder( guestHero->GetArmy() );
    }
}

void AI::Planner::CastleTurn( Castle & castle, const bool defensiveStrategy )
{
    if ( defensiveStrategy ) {
        // If the castle is potentially under threat, then it makes sense to try to hire the maximum number of troops so that the enemy cannot hire them even if he
        // captures the castle, therefore, it is worth starting with hiring.
        reinforceCastle( castle, true );

        // Avoid building monster dwellings when defensive as they might fall into enemy's hands. Instead, try to build defensive structures if there is at least some
        // kind of garrison in the castle.
        if ( castle.GetActualArmy().getTotalCount() > 0 ) {
            Build( castle, defensiveStructures );
            Build( castle, supportingDefensiveStructures );
        }
    }
    else {
        const uint32_t regionID = world.getTile( castle.GetIndex() ).GetRegion();
        const RegionStats & stats = _regions[regionID];

        CastleDevelopment( castle, stats.safetyFactor, stats.spellLevel );
        recruitTaxpayerPeasants( castle );
    }
}
