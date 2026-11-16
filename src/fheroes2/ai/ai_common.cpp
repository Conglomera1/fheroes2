/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2026                                             *
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

#include "taxpayer_test.h"

#include "ai_common.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "army.h"
#include "economy_trace.h"
#include "army_troop.h"
#include "artifact_ultimate.h"
#include "castle.h"
#include "color.h"
#include "difficulty.h"
#include "game.h"
#include "game_static.h"
#include "heroes.h"
#include "kingdom.h"
#include "knight_observer.h"
#include "knight_construction.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "monster.h"
#include "mp2.h"
#include "players.h"
#include "puzzle.h"
#include "resource.h"
#include "resource_trading.h"
#include "world.h"

bool AI::BuildIfPossible( Castle & castle, const BuildingType building )
{
    const auto status = castle.CheckBuyBuilding( building );
    if ( status != BuildingStatus::LACK_RESOURCES && !AI::KnightConstruction::canSpend( castle, building ) ) return false;
    switch ( status ) {
    case BuildingStatus::LACK_RESOURCES: {
        const Funds payment = castle.getBuildingCost( building );
        const auto exchange = AI::calculateMarketplaceTransaction( castle.GetKingdom(), payment );
        // Review the signed exchange and the final building as ONE operation,
        // before either mutation. Passing the exchange alone is insufficient.
        if ( !exchange || !AI::KnightConstruction::canTradeForBuilding( castle, building, *exchange ) ) return false;

        if ( !AI::tradeAtMarketplace( castle.GetKingdom(), payment ) ) {
            return false;
        }

        break;
    }
    case BuildingStatus::ALLOW_BUILD:
        break;
    default:
        return false;
    }

    const bool result = castle.BuyBuilding( building );
    assert( result );

    return result;
}

bool AI::BuildIfEnoughFunds( Castle & castle, const BuildingType building, const uint32_t fundsMultiplier )
{
    if ( fundsMultiplier < 1 || fundsMultiplier > 99 ) {
        return false;
    }

    const Kingdom & kingdom = castle.GetKingdom();
    const Funds requiredFunds = castle.getBuildingCost( building ) * fundsMultiplier;

    // Perhaps the kingdom already has the necessary supply of resources
    if ( !kingdom.AllowPayment( requiredFunds ) ) {
        // Even if the kingdom does not have the necessary supply of these resources right now, there may be enough resources of another type available to get the
        // missing resources as a result of resource exchange
        if ( !AI::calculateMarketplaceTransaction( kingdom, requiredFunds ) ) {
            return false;
        }
    }

    return BuildIfPossible( castle, building );
}

void AI::OptimizeTroopsOrder( Army & army )
{
    if ( !army.isValid() ) {
        return;
    }

    const auto * commander = army.GetCommander();
    if ( commander && commander->isHeroes() && KnightConstruction::isOpeningSeaExplorer( static_cast<const Heroes &>( *commander ) ) ) {
        // Preserve the five weak stacks through the native pre-battle pass,
        // including difficulties which otherwise merge identical creatures.
        bool singles = true;
        for ( size_t slot = 0; slot < army.Size(); ++slot ) {
            const auto * troop = army.GetTroop( slot );
            if ( troop->isValid() && troop->GetCount() != 1 ) singles = false;
        }
        if ( singles ) return;
    }
    army.MergeSameMonsterTroops();

    std::vector<Troop> archers;
    std::vector<Troop> others;

    for ( size_t slot = 0; slot < Army::maximumTroopCount; ++slot ) {
        const Troop * troop = army.GetTroop( slot );
        if ( troop && troop->isValid() ) {
            if ( troop->isArchers() ) {
                archers.push_back( *troop );
            }
            else {
                others.push_back( *troop );
            }
        }
    }

    assert( archers.size() + others.size() <= 5 );

    // Sort troops by tactical priority. For melee units, the order of comparison is as follows:
    // 1. Comparison by speed (faster units first);
    // 2. Comparison by type (flying units first);
    // 3. Comparison by strength.
    std::sort( others.begin(), others.end(), []( const Troop & left, const Troop & right ) {
        if ( left.GetSpeed() != right.GetSpeed() ) {
            return left.GetSpeed() < right.GetSpeed();
        }

        if ( left.isFlying() != right.isFlying() ) {
            return right.isFlying();
        }

        return left.GetStrength() < right.GetStrength();
    } );

    // Archers are sorted solely by strength
    std::sort( archers.begin(), archers.end(), []( const Troop & left, const Troop & right ) { return left.GetStrength() < right.GetStrength(); } );

    std::array<size_t, 5> slotOrder = { 2, 1, 3, 0, 4 };
    switch ( archers.size() ) {
    case 1:
        if ( others.size() == 1 ) {
            slotOrder = { 0, 2, 1, 3, 4 };
        }
        else {
            slotOrder = { 0, 1, 2, 3, 4 };
        }
        break;
    case 2:
        if ( others.size() == 3 ) {
            slotOrder = { 0, 2, 3, 1, 4 };
        }
        else {
            slotOrder = { 0, 4, 2, 1, 3 };
        }
        break;
    case 3:
        if ( others.size() == 1 ) {
            slotOrder = { 0, 4, 2, 1, 3 };
        }
        else {
            slotOrder = { 0, 3, 2, 1, 4 };
        }
        break;
    case 4:
        slotOrder = { 0, 4, 2, 3, 1 };
        break;
    case 5:
        slotOrder = { 0, 4, 1, 2, 3 };
        break;
    default:
        break;
    }

    army.Clean();

    for ( const size_t slot : slotOrder ) {
        if ( !archers.empty() ) {
            army.GetTroop( slot )->Set( archers.back() );

            archers.pop_back();
        }
        else if ( !others.empty() ) {
            army.GetTroop( slot )->Set( others.back() );

            others.pop_back();
        }
        else {
            break;
        }
    }

    if ( Difficulty::allowAIToSplitWeakStacks( Game::getDifficulty() ) ) {
        // Complicate the task of a potential attacker
        army.splitStackOfWeakestUnitsIntoFreeSlots();
    }
}

namespace
{
    // Prepare both armies on copies. Removing the cargo first may free the
    // courier slot needed to receive one entire garrison stack in exchange.
    bool populationTransfer( Troops & source, Troops & target, const uint32_t quantity )
    {
        const Monster peasant( Monster::PEASANT );
        if ( !quantity || source.GetCountMonsters( peasant ) < quantity
             || uint64_t{ target.GetCountMonsters( peasant ) } + quantity > UINT32_MAX ) return false;
        uint32_t left = quantity;
        for ( size_t slot = 0; slot < source.Size() && left; ++slot ) {
            auto * troop = source.GetTroop( slot );
            if ( troop->GetID() != Monster::PEASANT ) continue;
            const auto remove = std::min( left, troop->GetCount() );
            if ( remove == troop->GetCount() ) troop->Reset(); else troop->SetCount( troop->GetCount() - remove );
            left -= remove;
        }
        if ( !target.CanJoinTroop( peasant ) ) {
            Troop * choice = nullptr;
            for ( size_t slot = 0; slot < target.Size(); ++slot ) {
                auto * troop = target.GetTroop( slot );
                if ( troop->isValid() && source.CanJoinTroop( troop->GetMonster() )
                     && uint64_t{ source.GetCountMonsters( troop->GetMonster() ) } + troop->GetCount() <= UINT32_MAX
                     && ( !choice || troop->GetStrength() < choice->GetStrength() ) ) choice = troop;
            }
            if ( !choice || !source.JoinTroop( choice->GetMonster(), choice->GetCount(), false ) ) return false;
            choice->Reset();
        }
        return target.JoinTroop( peasant, quantity, false );
    }
}

bool AI::KnightConstruction::canReceivePopulation( const Heroes & hero, const Castle & castle )
{
    Troops source( hero.GetArmy().getTroops() ), target( castle.GetArmy().getTroops() );
    if ( !source.HasMonster( Monster::PEASANT ) ) {
        // A future load can replace one courier troop at the source Farm.
        if ( !source.CanJoinTroop( Monster::PEASANT ) ) source.GetWeakestTroop()->Reset();
        if ( !source.JoinTroop( Monster::PEASANT, 1, false ) ) return false;
    }
    return populationTransfer( source, target, source.GetCountMonsters( Monster::PEASANT ) );
}

bool AI::KnightConstruction::canLoadPopulation( const Heroes & hero, const Castle & castle, const uint32_t quantity )
{
    Troops source( castle.GetArmy().getTroops() ), target( hero.GetArmy().getTroops() );
    return populationTransfer( source, target, quantity );
}

void AI::KnightConstruction::arrangePopulation( Heroes & hero, Castle & castle, int destination, const bool embarking )
{
    if ( fheroes2::taxpayerSimulationDepth || !hero.isControlAI() || !populationSafe( castle ) || hero.inCastle() != &castle
         || hero.GetColor() != castle.GetColor()
         || !Kingdom::TaxpayerRequiresFarms() || Kingdom::GetTaxpayerGoldPerPeasantPerDay() <= 0 ) return;
    if ( !castle.isBuild( BUILD_WEL2 ) && !castle.hasTrainingGrounds() ) return;
    if ( isOpeningSeaExplorer( hero ) || keepOpeningLandPeasants( hero ) || keepPopulationCargo( hero ) ) return;
    if ( const int pickup = populationPickupDestination( hero, castle ); pickup >= 0 ) destination = pickup;
    auto & garrison = castle.GetArmy(); auto & army = hero.GetArmy();
    const Monster peasant( Monster::PEASANT );
    const auto transfer = [&]( Army & source, Army & target, uint32_t quantity, const bool fromHero, const char * reason ) {
        quantity = std::min( quantity, UINT32_MAX - target.GetCountMonsters( peasant ) );
        if ( !quantity ) return;
        const std::string beforeSource = KnightObserver::armyJson( source ), beforeTarget = KnightObserver::armyJson( target );
        Troops stagedSource( source.getTroops() ), stagedTarget( target.getTroops() );
        if ( !populationTransfer( stagedSource, stagedTarget, quantity ) ) return;
        source.Assign( stagedSource ); target.Assign( stagedTarget );
        ECONOMY_TRACE( 1, "OBSERVER_ARMY_TRANSFER", ",\"operation\":\"knight_population\",\"player\":" << static_cast<int>( hero.GetColor() )
            << ",\"hero\":" << hero.GetID() << ",\"town\":" << castle.GetIndex() << ",\"quantity\":" << quantity
            << ",\"reason\":" << Economy::jsonQuote( reason ) << ",\"source_player\":" << static_cast<int>( hero.GetColor() )
            << ",\"destination_player\":" << static_cast<int>( hero.GetColor() )
            << ",\"source_hero\":" << ( fromHero ? "true" : "false" ) << ",\"destination_hero\":" << ( fromHero ? "false" : "true" )
            << ",\"source_tile\":" << castle.GetIndex() << ",\"destination_tile\":" << castle.GetIndex()
            << ",\"source_before\":" << beforeSource << ",\"source_after\":" << KnightObserver::armyJson( source )
            << ",\"destination_before\":" << beforeTarget << ",\"destination_after\":" << KnightObserver::armyJson( target ) );
    };
    const bool delivered = populationOrder( hero ) == castle.GetIndex();
    const uint32_t reserve = delivered ? UINT32_MAX : populationReserve( castle );
    const uint32_t wanted = embarking ? UINT32_MAX : reserve;
    const uint32_t held = garrison.GetCountMonsters( peasant );
    const uint32_t carried = army.GetCountMonsters( peasant );
    const uint32_t movable = army.GetOccupiedSlotCount() == 1 && carried ? carried - 1 : carried;
    if ( wanted > held ) transfer( army, garrison, std::min( movable, wanted - held ), true, embarking ? "before_embarkation" : "population_deposit" );
    if ( delivered ) {
        if ( army.GetCountMonsters( peasant ) <= ( army.GetOccupiedSlotCount() == 1 ? 1U : 0U ) ) cancelPopulationRoute( hero, "delivered" );
        return;
    }
    if ( !embarking ) {
        if ( destination < 0 ) destination = populationDestination( hero );
        const auto found = std::find_if( castle.GetKingdom().GetCastles().begin(), castle.GetKingdom().GetCastles().end(),
            [&]( const Castle * c ) { return c && c->GetIndex() == destination; } );
        if ( found != castle.GetKingdom().GetCastles().end() && populationSafe( **found ) ) {
            const auto & target = **found; uint32_t quantity = 0;
            const uint32_t stock = garrison.GetCountMonsters( peasant );
            if ( reserve != UINT32_MAX && economicTown( target ) && target.isBuild( BUILD_WEL2 ) && stock > reserve ) quantity = stock - reserve;
            else if ( castle.isBuild( BUILD_WEL2 ) && target.hasTrainingGrounds() ) {
                quantity = std::min( populationAvailableStock( castle, hero.GetID() ), populationSupplyNeed( target, hero.GetID() ) );
            }
            if ( quantity && canReceivePopulation( hero, target ) ) {
                transfer( garrison, army, quantity, false, target.hasTrainingGrounds() ? "supply_conscription" : "export_taxpayer_surplus" );
                notePopulationRoute( hero, destination );
            }
        }
    }
    ECONOMY_TRACE( 1, "KNIGHT_POPULATION", ",\"player\":" << static_cast<int>( hero.GetColor() ) << ",\"hero\":" << hero.GetID()
        << ",\"town\":" << castle.GetIndex() << ",\"destination\":" << destination << ",\"reserve_days\":14,\"reserve\":" << reserve
        << ",\"garrison\":" << garrison.GetCountMonsters( peasant ) << ",\"carried\":" << army.GetCountMonsters( peasant )
        << ",\"embarking\":" << ( embarking ? "true" : "false" ) );
}

void AI::depositTaxpayerPeasants( Heroes * hero, Castle * castle )
{
    assert( hero && castle );
    KnightConstruction::arrangePopulation( *hero, *castle );
}

void AI::transferSlowestTroopsToGarrison( Heroes * hero, Castle * castle )
{
    assert( hero != nullptr && castle != nullptr );

    if ( KnightConstruction::keepPopulationCargo( *hero ) || KnightConstruction::isOpeningSeaExplorer( *hero ) || KnightConstruction::keepOpeningLandPeasants( *hero ) ) return;
    Army & army = hero->GetArmy();
    Army & garrison = castle->GetArmy();

    // Make efforts to get free slots in the garrison to move troops there
    garrison.MergeSameMonsterTroops();

    std::vector<Troop *> armyTroops;
    armyTroops.reserve( army.Size() );

    for ( size_t i = 0; i < army.Size(); ++i ) {
        Troop * troop = army.GetTroop( i );
        assert( troop != nullptr );

        if ( troop->isEmpty() ) {
            continue;
        }

        armyTroops.push_back( troop );
    }

    assert( !armyTroops.empty() );

    // Move the slowest units first
    std::sort( armyTroops.begin(), armyTroops.end(), Army::SlowestTroop );

    // At least one of the fastest units should remain in the hero's army
    armyTroops.pop_back();

    for ( Troop * troop : armyTroops ) {
        if ( !garrison.JoinTroop( *troop ) ) {
            break;
        }

        troop->Reset();
    }

    assert( army.isValid() );
}

std::optional<Funds> AI::calculateMarketplaceTransaction( const Kingdom & kingdom, const Funds & fundsToObtain )
{
    const uint32_t marketplaceCount = kingdom.GetCountMarketplace();
    if ( marketplaceCount == 0 ) {
        return {};
    }

    std::map<int, int32_t> plannedBalance;
    {
        const Funds fundsDiff = kingdom.GetFunds() - fundsToObtain;

        Resource::forEach( Resource::ALL, [&plannedBalance, &fundsDiff]( const int res ) {
            if ( const auto [dummy, inserted] = plannedBalance.try_emplace( res, fundsDiff.Get( res ) ); !inserted ) {
                assert( 0 );
            }
        } );

        assert( !plannedBalance.empty() );
    }

    Funds plannedTransaction;

    const auto exchangeResources = [marketplaceCount, &plannedBalance, &plannedTransaction]( const int fromRes, const int toRes ) {
        assert( fromRes != toRes );

        auto fromResBalanceIter = plannedBalance.find( fromRes );
        auto toResBalanceIter = plannedBalance.find( toRes );

        assert( fromResBalanceIter != plannedBalance.end() && toResBalanceIter != plannedBalance.end() );

        auto & [fromResDummy, fromResBalanceAmount] = *fromResBalanceIter;
        auto & [toResDummy, toResBalanceAmount] = *toResBalanceIter;

        assert( fromResBalanceAmount > 0 && toResBalanceAmount < 0 );

        const int32_t tradeCost = fheroes2::getTradeCost( marketplaceCount, fromRes, toRes );
        assert( tradeCost > 0 );

        int32_t * fromResTransactionAmount = plannedTransaction.GetPtr( fromRes );
        int32_t * toResTransactionAmount = plannedTransaction.GetPtr( toRes );

        assert( fromResTransactionAmount != nullptr && toResTransactionAmount != nullptr && fromResTransactionAmount != toResTransactionAmount );

        // If resources are exchanged for gold, then by giving away one unit of the resource we get several units of gold
        if ( toRes == Resource::GOLD ) {
            fromResBalanceAmount -= 1;
            toResBalanceAmount += tradeCost;

            // Since the planned transaction must be deducted from the funds of the kingdom, the volume of exchanged resources should be positive, and the volume of
            // acquired resources should be negative
            *fromResTransactionAmount += 1;
            *toResTransactionAmount -= tradeCost;
        }
        // Otherwise (when exchanging gold for a resource or a resource for a resource) by giving away several units of a resource, we get only one unit of another
        // resource
        else {
            fromResBalanceAmount -= tradeCost;
            toResBalanceAmount += 1;

            // Since the planned transaction must be deducted from the funds of the kingdom, the volume of exchanged resources should be positive, and the volume of
            // acquired resources should be negative
            *fromResTransactionAmount += tradeCost;
            *toResTransactionAmount -= 1;
        }

        assert( fromResBalanceAmount >= 0 );
    };

    for ( const auto & [missingRes, missingResAmount] : plannedBalance ) {
        if ( missingResAmount >= 0 ) {
            continue;
        }

        while ( missingResAmount < 0 ) {
            // The exchange of resources takes place one unit at a time. Each time, the resource whose relative stock is currently maximum is selected for exchange
            // (that is, the resource whose current stock can be exchanged for the maximum amount of the target resource).

            int resToExchange = Resource::UNKNOWN;
            int32_t maxPotentialRevenue = 0;

            for ( const auto & [res, resAmount] : plannedBalance ) {
                if ( res == missingRes ) {
                    continue;
                }

                if ( resAmount <= 0 ) {
                    continue;
                }

                const int32_t tradeCost = fheroes2::getTradeCost( marketplaceCount, res, missingRes );
                assert( tradeCost > 0 );

                const int32_t potentialRevenue = ( missingRes == Resource::GOLD ? resAmount * tradeCost : resAmount / tradeCost );
                if ( maxPotentialRevenue < potentialRevenue ) {
                    resToExchange = res;
                    maxPotentialRevenue = potentialRevenue;
                }
            }

            if ( resToExchange == Resource::UNKNOWN ) {
                return {};
            }

            exchangeResources( resToExchange, missingRes );
        }

        // If we exchange resources for gold, we can get a little more than the amount we need. In other cases, we should get exactly the amount we need.
        assert( missingRes == Resource::GOLD ? missingResAmount >= 0 : missingResAmount == 0 );
    }

    // If the trade is successful, then we should not have any "debts" in the planned balance...
    assert( std::none_of( plannedBalance.cbegin(), plannedBalance.cend(), []( const auto & item ) { return ( item.second < 0 ); } ) );

    // ... and the kingdom should be able to conduct the planned transaction
    assert( kingdom.AllowPayment( plannedTransaction ) );

    return plannedTransaction;
}

bool AI::tradeAtMarketplace( Kingdom & kingdom, const Funds & fundsToObtain )
{
    const std::optional<Funds> transaction = calculateMarketplaceTransaction( kingdom, fundsToObtain );
    if ( !transaction ) {
        DEBUG_LOG( DBG_AI, DBG_TRACE,
                   Color::String( kingdom.GetColor() ) << " having funds " << kingdom.GetFunds().String() << " failed to obtain funds " << fundsToObtain.String() )

        return false;
    }

    assert( kingdom.AllowPayment( *transaction ) );
    if ( !AI::KnightConstruction::allowsPayment( kingdom, *transaction, "market_trade" ) ) return false;

    DEBUG_LOG( DBG_AI, DBG_INFO,
               Color::String( kingdom.GetColor() ) << " having funds " << kingdom.GetFunds().String() << " obtains funds " << fundsToObtain.String()
                                                   << " through a transaction " << transaction->String() )

    const Economy::CauseScope cause( "market_trade", static_cast<int>( kingdom.GetColor() ) );
    kingdom.OddFundsResource( *transaction );
    ECONOMY_TRACE( 1, "MARKET_TRADE", ",\"kind\":\"actual\",\"player\":" << static_cast<int>( kingdom.GetColor() )
                   << ",\"goal\":" << Economy::fundsJson( fundsToObtain ) << ",\"transaction\":" << Economy::fundsJson( *transaction ) );

    return true;
}

void AI::shareObjectVisitInfoWithAllies( const Kingdom & kingdom, const int32_t tileIndex )
{
    if ( !Difficulty::isObjectVisitInfoSharingAllowedForAI( Game::getDifficulty() ) ) {
        return;
    }

    if ( !kingdom.isControlAI() ) {
        return;
    }

    const PlayerColorsSet friendColors = Players::GetPlayerFriends( kingdom.GetColor() );
    if ( friendColors == 0 ) {
        // No allies.
        return;
    }

    if ( ( friendColors & Players::HumanColors() ) != 0 ) {
        // Some allies are human players.
        return;
    }

    const MP2::MapObjectType objectType = world.getTile( tileIndex ).getMainObjectType( false );

    const PlayerColorsVector playerColors( friendColors );
    for ( const PlayerColor color : playerColors ) {
        ColorBase( color ).GetKingdom().SetVisited( tileIndex, objectType );
    }
}

bool AI::isUltimateArtifactAvailableToHero( const UltimateArtifact & art, const Heroes & hero )
{
    if ( art.isFound() ) {
        return false;
    }

    // The hero's kingdom must have a fully open obelisk map
    if ( !hero.GetKingdom().PuzzleMaps().all() ) {
        return false;
    }

    if ( hero.IsFullBagArtifacts() ) {
        return false;
    }

    const int32_t idx = art.getPosition();

    return Maps::isValidAbsIndex( idx ) && world.getTile( idx ).isSuitableForUltimateArtifact();
}

bool AI::isValuableAdventureMapObject( const Kingdom & kingdom, const MP2::MapObjectType objectType, const int32_t tileIndex )
{
    if ( !MP2::isInGameActionObject( objectType ) ) {
        // Only action objects have any value.
        return false;
    }

    switch ( objectType ) {
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.getCastleEntrance( Maps::GetPoint( tileIndex ) );
        if ( castle == nullptr ) {
            // How is it possible that a castle does not exist?
            assert( 0 );
            return false;
        }

        if ( kingdom.GetColor() == castle->GetColor() ) {
            return true;
        }

        // We can interact only with enemy castles.
        return !Players::isFriends( kingdom.GetColor(), static_cast<PlayerColorsSet>( castle->GetColor() ) );
    }
    case MP2::OBJ_HERO: {
        const Heroes * otherHero = world.getTile( tileIndex ).getHero();
        if ( otherHero == nullptr ) {
            // It could happen when adding new heroes at the start of game.
            return false;
        }

        if ( kingdom.GetColor() == otherHero->GetColor() ) {
            return true;
        }

        // We can interact only with enemy heroes.
        return !Players::isFriends( kingdom.GetColor(), static_cast<PlayerColorsSet>( otherHero->GetColor() ) );
    }
    case MP2::OBJ_MAGELLANS_MAPS:
        // Magellan's Maps object should be visited only once.
        return !kingdom.isVisited( objectType );
    case MP2::OBJ_ALCHEMIST_LAB:
    case MP2::OBJ_LIGHTHOUSE:
    case MP2::OBJ_MINE:
    case MP2::OBJ_SAWMILL:
        // Heroes cannot re-capture their own or ally's objects.
        return !Players::isFriends( kingdom.GetColor(), static_cast<PlayerColorsSet>( Maps::getColorFromTile( world.getTile( tileIndex ) ) ) );
    case MP2::OBJ_DAEMON_CAVE:
    case MP2::OBJ_LEAN_TO:
    case MP2::OBJ_SKELETON:
    case MP2::OBJ_WAGON:
        // Single visit objects with no resources have no value.
        return Maps::doesTileContainValuableItems( world.getTile( tileIndex ) );
    case MP2::OBJ_OBSERVATION_TOWER: {
        // If Observation Tower uncovers nothing, then we skip it.
        const int32_t fogCountToUncoverByTower
            = Maps::getFogTileCountToBeRevealed( tileIndex, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ), kingdom.GetColor() );

        return fogCountToUncoverByTower > 0;
    }
    case MP2::OBJ_OBELISK:
        // An obelisk should be visited only once.
        return !kingdom.isVisited( tileIndex, objectType );
    case MP2::OBJ_TRAVELLER_TENT:
        // Traveller's Tent should be visited only once.
        return !kingdom.isTravellerTentVisited( Maps::getBarrierColorFromTile( world.getTile( tileIndex ) ) );
    case MP2::OBJ_DERELICT_SHIP:
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_PYRAMID:
    case MP2::OBJ_SHIPWRECK:
        // Single visit objects with no resources have no value.
        return !kingdom.isVisited( tileIndex, objectType ) && Maps::doesTileContainValuableItems( world.getTile( tileIndex ) );
    case MP2::OBJ_HUT_OF_MAGI:
        // A Hut of Magi should uncover all areas at once so visiting any other similar object makes no sense.
        return !kingdom.isVisited( objectType ) && Maps::doesObjectExistOnMap( MP2::OBJ_EYE_OF_MAGI );
    case MP2::OBJ_BOAT:
    case MP2::OBJ_EYE_OF_MAGI:
    case MP2::OBJ_ORACLE:
    case MP2::OBJ_SIGN:
    case MP2::OBJ_STONE_LITHS:
    case MP2::OBJ_WHIRLPOOL:
        // These objects aren't considered valuable.
        return false;
    default:
        break;
    }

    return true;
}
