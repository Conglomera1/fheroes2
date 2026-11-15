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
#include <queue>
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
#include "conscription.h"
#include "difficulty.h"
#include "game.h"
#include "heroes.h"
#include "heroes_recruits.h"
#include "kingdom.h"
#include "knight_observer.h"
#include "economy_trace.h"
#include "knight_economy.h"
#include "knight_construction.h"
#include "economy_project.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "monster.h"
#include "payment.h"
#include "profit.h"
#include "economy_trace.h"
#include "race.h"
#include "resource.h"
#include "world.h"
#include "world_pathfinding.h"
#include "world_regions.h"

namespace
{
    // Farm construction in the legacy build orders must not implicitly dismantle
    // Training Grounds. Reconversion needs a dedicated economic decision.
    bool isLegacyFarmReconversion( const Castle & castle, const BuildingType building )
    {
        return building == BUILD_WEL2 && castle.hasTrainingGrounds();
    }

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

    namespace KC = AI::KnightConstruction;
    namespace EP = AI::EconomyProject;
    KC::Resources constructionResources( const Funds & f )
    { return { f.gold, f.wood, f.ore, f.mercury, f.sulfur, f.crystal, f.gems }; }
    const std::array<uint32_t, 10> knightPrimaryOrder{ BUILD_STATUE, DWELLING_MONSTER1, DWELLING_MONSTER2, BUILD_WELL,
        DWELLING_MONSTER3, BUILD_TAVERN, DWELLING_MONSTER4, DWELLING_MONSTER6, DWELLING_MONSTER5, BUILD_WEL2 };
    struct ConstructionContext { const Kingdom * kingdom; KC::Ledger ledger; std::set<int> economic, endangered; };
    thread_local uint32_t emergencyDepth = 0;
    thread_local std::optional<ConstructionContext> constructionContext;

    struct NavalMission { int hero = -1, home = -1, boat = -1, target = -1; MP2::MapObjectType object = MP2::OBJ_NONE; uint32_t day = 0, seed = 0; uint64_t segment = 0; int owner = -1; bool reuse = false; };
    std::map<int, NavalMission> navalMissions;
    struct OpeningExplorer { uint32_t seed = 0, day = 0; uint64_t segment = 0; int owner = -1, hero = -1, home = -1; bool embarked = false; };
    std::map<int, OpeningExplorer> openingExplorers;
    std::map<int, OpeningExplorer> openingLaunches;
    std::map<int, OpeningExplorer> openingRecruits;
    std::map<int, NavalMission> navalLandOrders;
    struct RoleHistory { uint32_t seed = 0, reviewed = 0; uint64_t segment = 0; int primary = -1; };
    std::map<int, RoleHistory> roleHistory;
    struct PopulationRoute {
        uint32_t seed = 0, day = 0;
        uint64_t segment = 0;
        int owner = -1, target = -1, pickup = -1;
        uint32_t quantity = 0;
    };
    std::map<int, PopulationRoute> populationRoutes;
    thread_local const Castle * recruitmentTown = nullptr;
    bool pendingLandOrder( const Heroes & hero )
    {
        const auto it = navalLandOrders.find( hero.GetID() );
        if ( it == navalLandOrders.end() ) return false;
        const auto & m = it->second;
        const bool valid = m.owner == static_cast<int>( hero.GetColor() ) && m.seed == world.GetMapSeed()
            && m.segment == Economy::traceState().segment && world.CountDay() >= m.day && world.CountDay() - m.day <= 7
            && hero.GetIndex() != m.target && !world.getTile( m.target ).isFog( hero.GetColor() )
            && ( m.object == MP2::OBJ_NONE ? Maps::getFogTileCountToBeRevealed( m.target, hero.GetScoutingDistance(), hero.GetColor() ) > 0
                 : world.getTile( m.target ).getMainObjectType( false ) == m.object );
        if ( !valid ) { navalLandOrders.erase( it ); return false; }
        return true;
    }
    bool seaOnly( const AIWorldPathfinder & path, const int target )
    {
        const auto route = path.buildPath( target, false );
        return !route.empty() && std::all_of( route.begin(), route.end(), []( const auto & step ) { return world.getTile( step.GetIndex() ).isWater(); } );
    }
    std::vector<int> seaOptions( const Heroes & hero, const int origin, const AIWorldPathfinder & path )
    {
        std::vector<std::pair<double, int>> ranked;
        for ( int tile = 0; tile < static_cast<int>( world.getSize() ); ++tile ) {
            const auto & cell = world.getTile( tile );
            if ( tile == origin || cell.isFog( hero.GetColor() ) || !cell.isWater() ) continue;
            const auto distance = path.getDistance( tile );
            if ( !distance || distance > 4500 ) continue;
            const auto object = cell.getMainObjectType( false );
            // Expected rewards by TYPE only, never hidden object contents.
            double value = object == MP2::OBJ_FLOTSAM ? 1175 : object == MP2::OBJ_SEA_CHEST ? 1500 : object == MP2::OBJ_SHIPWRECK_SURVIVOR ? 1000 : 0;
            const bool reward = value > 0;
            if ( !reward ) {
                if ( object != MP2::OBJ_NONE ) continue;
                bool frontier = false;
                for ( int dy = -1; dy <= 1 && !frontier; ++dy ) for ( int dx = -1; dx <= 1 && !frontier; ++dx ) {
                    const int x = tile % world.w() + dx, y = tile / world.w() + dy;
                    if ( x >= 0 && x < world.w() && y >= 0 && y < world.h() ) frontier = world.getTile( y * world.w() + x ).isFog( hero.GetColor() );
                }
                if ( !frontier ) continue;
                value = Maps::getFogTileCountToBeRevealed( tile, hero.GetScoutingDistance(), hero.GetColor() );
                if ( value <= 0 ) continue;
            }
            ranked.emplace_back( ( reward ? 1000000.0 : 0.0 ) + value / ( 100 + distance ), tile );
        }
        std::stable_sort( ranked.begin(), ranked.end(), []( const auto & a, const auto & b ) {
            return a.first != b.first ? a.first > b.first : a.second < b.second;
        } );
        std::vector<int> targets;
        size_t checked = 0;
        for ( const auto & entry : ranked ) {
            if ( ++checked > 64 || targets.size() == 12 ) break;
            if ( seaOnly( path, entry.second ) ) targets.push_back( entry.second );
        }
        return targets;
    }
    NavalMission navalUse( const Castle & castle, const bool readyToday = true )
    {
        NavalMission best;
        int boat = castle.getTileIndexToPlaceBoat();
        // An occupied dock may have no placement tile, but its boat can still
        // serve a native route. Never require an empty dock for reuse.
        if ( boat < 0 ) {
            const int y = castle.GetIndex() / world.w() + 2;
            for ( int dx = -1; dx <= 1; ++dx ) {
                const int x = castle.GetIndex() % world.w() + dx;
                if ( x >= 0 && x < world.w() && y < world.h() && world.getTile( y * world.w() + x ).isWater() ) { boat = y * world.w() + x; break; }
            }
        }
        if ( boat < 0 ) return best;
        const auto & kingdom = castle.GetKingdom(); const auto color = kingdom.GetColor();
        if ( constructionContext && constructionContext->endangered.count( castle.GetIndex() ) ) return best;
        std::vector<const Heroes *> candidates;
        for ( const auto * hero : kingdom.GetHeroes() ) {
            if ( !hero || !hero->isActive() || ( readyToday && !hero->CanMove() ) || hero->isShipMaster() || hero->Modes( Heroes::PATROL | Heroes::SLEEPER )
                 || navalMissions.count( hero->GetID() ) ) continue;
            if ( pendingLandOrder( *hero ) ) continue;
            if ( constructionContext && constructionContext->endangered.count( hero->GetIndex() ) ) continue;
            const auto nominee = openingRecruits.find( static_cast<int>( color ) );
            if ( nominee != openingRecruits.end() && nominee->second.seed == world.GetMapSeed()
                 && nominee->second.segment == Economy::traceState().segment && world.CountDay() <= nominee->second.day + 1
                 && nominee->second.home == castle.GetIndex() && hero->GetID() != nominee->second.hero ) continue;
            candidates.push_back( hero );
        }
        if ( candidates.empty() ) return best;
        uint64_t bestCost = UINT64_MAX;
        size_t bestRank = SIZE_MAX;
        for ( const auto * hero : candidates ) {
            AIWorldPathfinder land; land.reset(); land.reEvaluateIfNeeded( *hero );
            uint32_t walk = UINT32_MAX;
            for ( int dy = -1; dy <= 1; ++dy ) for ( int dx = -1; dx <= 1; ++dx ) {
                const int x = boat % world.w() + dx, y = boat / world.w() + dy;
                if ( x < 0 || x >= world.w() || y < 0 || y >= world.h() ) continue;
                const int shore = y * world.w() + x; const auto & cell = world.getTile( shore );
                if ( cell.isFog( color ) || cell.isWater() ) continue;
                const uint32_t distance = land.getDistance( shore );
                if ( shore == hero->GetIndex() || distance ) walk = std::min( walk, distance );
            }
            AIWorldPathfinder sea; sea.reset(); sea.reEvaluateIfNeeded( boat, color, hero->GetArmy().GetStrength(), Skill::Level::BASIC );
            const auto targets = seaOptions( *hero, boat, sea );
            size_t rank = 0;
            for ( const int target : targets ) {
                const auto targetRank = rank++;
                const auto sail = sea.getDistance( target );
                if ( !sail || sail > 4500 ) continue;
                // With the existing fleet, native pathfinding includes ordinary
                // boarding and permitted Summon Boat routes. Prefer these over
                // paying for another boat; don't invoke spells in this policy.
                const auto existing = land.getDistance( target );
                const bool reuse = existing > 0;
                const uint64_t reach = readyToday ? hero->GetMovePoints() : uint64_t{ hero->GetMaxMovePoints() } * 2;
                if ( !reuse && ( walk == UINT32_MAX || uint64_t{ walk } + 100 > reach ) ) continue;
                const uint64_t cost = reuse ? existing : uint64_t{ walk } + sail;
                if ( best.hero >= 0 && ( best.reuse > reuse || ( best.reuse == reuse && ( targetRank > bestRank || ( targetRank == bestRank && cost >= bestCost ) ) ) ) ) continue;
                bestCost = cost; bestRank = targetRank;
                best = { hero->GetID(), castle.GetIndex(), boat, target, world.getTile( target ).getMainObjectType( false ), world.CountDay(), world.GetMapSeed(), Economy::traceState().segment, static_cast<int>( color ) };
                best.reuse = reuse;
            }
        }
        return best;
    }
    bool usefulShipyard( const Castle & castle )
    {
        if ( castle.isBuild( BUILD_SHIPYARD ) ) return false;
        const auto mission = navalUse( castle, false );
        if ( mission.hero < 0 || mission.reuse ) return false;
        const Funds firstVoyage = castle.getBuildingCost( BUILD_SHIPYARD ) + PaymentConditions::BuyBoat();
        return KC::allowsPayment( castle.GetKingdom(), firstVoyage, "shipyard_and_first_boat" );
    }
    std::vector<uint32_t> primaryOrder( const Castle & castle )
    {
        if ( KC::economicTown( castle ) ) return { BUILD_STATUE, BUILD_MARKETPLACE, DWELLING_MONSTER1, BUILD_WELL, BUILD_WEL2 };
        return { knightPrimaryOrder.begin(), knightPrimaryOrder.end() };
    }
    bool woodPreserved( const Castle & castle, const uint32_t building )
    {
        if ( building == BUILD_MARKETPLACE || castle.isBuild( BUILD_MARKETPLACE )
             || world.CountCapturedMines( Resource::WOOD, castle.GetColor() ) > 0 ) return true;
        const auto price = castle.getBuildingCost( building );
        return price.wood <= 0 || castle.GetKingdom().GetFunds().wood - price.wood >= 5;
    }
    bool primaryReady( const Castle & castle, const uint32_t building, const uint32_t day )
    {
        if ( !castle.isCastle() || castle.isBuild( building ) || castle.isBuildingDisabled( building )
             || ( building == BUILD_WEL2 && castle.hasTrainingGrounds() )
             || !Difficulty::allowAIToDevelopCastlesOnDay( Game::getDifficulty(), Game::isCampaign(), day )
             || !Difficulty::allowAIToBuildCastleBuilding( Game::getDifficulty(), Game::isCampaign(), static_cast<BuildingType>( building ) ) ) return false;
        const auto required = static_cast<uint32_t>( fheroes2::getBuildingRequirement( castle.GetRace(), static_cast<BuildingType>( building ) ) );
        for ( uint32_t bit = 1; bit; bit <<= 1 ) if ( ( required & bit ) && !castle.isBuild( bit ) ) return false;
        const auto status = castle.CheckBuyBuilding( building );
        return status == BuildingStatus::ALLOW_BUILD || status == BuildingStatus::LACK_RESOURCES || status == BuildingStatus::NOT_TODAY;
    }

    std::optional<KC::Resources> tomorrowFunds( const Kingdom & kingdom )
    {
        if ( world.CountDay() == UINT32_MAX ) return {};
        const auto projection = Economy::forecast( kingdom.economySnapshot(), Conscription::settings(), 1 );
        if ( !projection.complete ) return {};
        return EP::funds( projection.end );
    }

    void reservePrimary( KC::Ledger & ledger, const Castle & castle, const KC::Resources & tomorrow, const size_t order )
    {
        if ( castle.GetRace() != Race::KNGT || world.CountDay() == UINT32_MAX ) return;
        const auto orderList = primaryOrder( castle );
        for ( size_t i = 0; i < orderList.size(); ++i ) {
            const auto building = orderList[i];
            if ( !primaryReady( castle, building, world.CountDay() + 1 ) || !woodPreserved( castle, building ) ) continue;
            const auto price = constructionResources( castle.getBuildingCost( building ) );
            if ( ledger.book( castle.GetIndex(), KC::Promise{ building, i, order, price }, tomorrow ) ) {
                ECONOMY_TRACE( 1, "CONSTRUCTION_RESERVE", ",\"kind\":\"planning\",\"player\":" << static_cast<int>( castle.GetColor() )
                    << ",\"town\":" << castle.GetIndex() << ",\"building\":" << building << ",\"priority\":" << i
                    << ",\"price\":" << Economy::arrayJson( price ) << ",\"tomorrow\":" << Economy::arrayJson( tomorrow ) );
                return;
            }
        }
    }

    void reconcileConstruction( const Kingdom & kingdom, KC::Ledger & ledger, const KC::Resources & tomorrow )
    {
        for ( auto it = ledger.promises.begin(); it != ledger.promises.end(); ) {
            const auto found = std::find_if( kingdom.GetCastles().begin(), kingdom.GetCastles().end(), [&]( const Castle * c ) {
                return c && c->GetIndex() == it->first && c->GetColor() == kingdom.GetColor();
            } );
            if ( found == kingdom.GetCastles().end() || ( *found )->isBuild( it->second.building )
                 || ( *found )->isBuildingDisabled( it->second.building ) ) it = ledger.promises.erase( it );
            else ++it;
        }
        for ( const int town : ledger.reconcile( tomorrow ) ) {
            ECONOMY_TRACE( 1, "CONSTRUCTION_RESERVE_RELEASED", ",\"town\":" << town << ",\"reason\":\"funds_changed\"" );
        }
    }

    bool constructionAffordable( const Castle & castle, const uint32_t building, KC::Ledger & ledger )
    {
        if ( emergencyDepth ) return true;
        if ( !woodPreserved( castle, building ) ) {
            ECONOMY_TRACE( 1, "CONSTRUCTION_WAIT", ",\"town\":" << castle.GetIndex() << ",\"candidate\":" << building << ",\"reason\":\"preserve_market_wood\"" );
            return false;
        }
        if ( ledger.promises.empty() ) return true;
        const auto baseline = tomorrowFunds( castle.GetKingdom() );
        if ( !baseline ) {
            ECONOMY_TRACE( 1, "CONSTRUCTION_WAIT", ",\"town\":" << castle.GetIndex() << ",\"candidate\":" << building
                << ",\"reason\":\"forecast_unavailable\"" ); return false;
        }
        reconcileConstruction( castle.GetKingdom(), ledger, *baseline );
        if ( ledger.promises.empty() ) return true;
        // A reserved budget cannot be used for speculative marketplace exchanges.
        if ( castle.CheckBuyBuilding( building ) != BuildingStatus::ALLOW_BUILD ) return false;
        auto input = EP::capture( castle.GetKingdom(), world.CountDay() + 1 );
        EP::Action a; a.id = a.project = 1; a.day = world.CountDay(); a.town = castle.GetIndex();
        a.kind = EP::Kind::Build; a.building = building; input.actions = { a };
        EP::WorkBudget work;
        const auto projection = EP::project( input, work );
        auto after = *baseline;
        if ( projection.covered() ) after = EP::funds( projection.end.economy );
        else if ( projection.reason == "unsupported_building" ) {
            // Castle/shipyard have map effects outside the project catalog.
            // Native legality was checked above; project their verified cash effect.
            const auto price = constructionResources( castle.getBuildingCost( building ) );
            auto snapshot = castle.GetKingdom().economySnapshot();
            snapshot.gold -= price[0];
            for ( size_t r = 0; r < snapshot.materials.size(); ++r ) snapshot.materials[r] -= price[r + 1];
            if ( building == BUILD_CASTLE ) snapshot.nonTaxIncome += ProfitConditions::FromBuilding( BUILD_CASTLE, 0 ).gold
                - ProfitConditions::FromBuilding( BUILD_TENT, 0 ).gold;
            const auto cash = Economy::forecast( snapshot, Conscription::settings(), 1 );
            if ( !cash.complete ) return false;
            after = EP::funds( cash.end );
        }
        else {
            ECONOMY_TRACE( 1, "CONSTRUCTION_WAIT", ",\"town\":" << castle.GetIndex() << ",\"candidate\":" << building
                << ",\"reason\":\"projection_incomplete\",\"detail\":" << Economy::jsonQuote( projection.reason ) ); return false;
        }
        const bool allowed = ledger.allows( castle.GetIndex(), building, after );
        if ( !allowed ) {
            ECONOMY_TRACE( 1, "CONSTRUCTION_WAIT", ",\"kind\":\"decision\",\"town\":" << castle.GetIndex() << ",\"candidate\":" << building
                << ",\"reason\":\"preserve_tomorrow_priority\",\"after\":" << Economy::arrayJson( after )
                << ",\"reserved\":" << Economy::arrayJson( ledger.reserved( castle.GetIndex(), building ) ) );
        }
        return allowed;
    }

    enum class PrimaryResult { Fallback, Built, Wait };
    PrimaryResult knightPrimaryDevelopment( Castle & castle )
    {
        if ( !castle.isCastle() || !Difficulty::allowAIToDevelopCastlesOnDay( Game::getDifficulty(), Game::isCampaign(), world.CountDay() ) )
            return PrimaryResult::Fallback;
        if ( !castle.Modes( Castle::ALLOW_TO_BUILD_TODAY ) ) return PrimaryResult::Wait;
        KC::Ledger local;
        auto * ledger = &local;
        if ( constructionContext && constructionContext->kingdom == &castle.GetKingdom() ) ledger = &constructionContext->ledger;
        // First enumerate today's choices in priority order. Reservations are
        // considered only when deciding whether this present choice delays tomorrow.
        std::vector<uint32_t> today;
        for ( const uint32_t building : primaryOrder( castle ) )
            if ( primaryReady( castle, building, world.CountDay() ) && castle.CheckBuyBuilding( building ) == BuildingStatus::ALLOW_BUILD ) today.push_back( building );
        const auto future = tomorrowFunds( castle.GetKingdom() );
        if ( future ) {
            reconcileConstruction( castle.GetKingdom(), *ledger, *future );
            if ( !ledger->promises.count( castle.GetIndex() ) ) reservePrimary( *ledger, castle, *future, ledger->promises.size() );
        }
        for ( const auto building : today ) {
            if ( !constructionAffordable( castle, building, *ledger ) ) continue;
            const bool built = castle.BuyBuilding( building );
            if ( built ) ledger->release( castle.GetIndex(), building );
            ECONOMY_TRACE( 1, "KNIGHT_PRIMARY_BUILD", ",\"town\":" << castle.GetIndex() << ",\"building\":" << building
                << ",\"built\":" << ( built ? "true" : "false" ) );
            return built ? PrimaryResult::Built : PrimaryResult::Wait;
        }
        if ( !castle.isBuild( BUILD_MARKETPLACE ) && castle.CheckBuyBuilding( BUILD_MARKETPLACE ) == BuildingStatus::ALLOW_BUILD
             && std::any_of( today.begin(), today.end(), [&]( uint32_t b ) { return !woodPreserved( castle, b ); } )
             && constructionAffordable( castle, BUILD_MARKETPLACE, *ledger ) ) {
            return castle.BuyBuilding( BUILD_MARKETPLACE ) ? PrimaryResult::Built : PrimaryResult::Wait;
        }
        return PrimaryResult::Fallback;
    }

    bool Build( Castle & castle, const std::vector<BuildOrder> & buildOrderList, const uint32_t multiplier = 1 )
    {
        const int gameDifficulty = Game::getDifficulty();
        const bool isGameCampaign = Game::isCampaign();

        for ( const BuildOrder & order : buildOrderList ) {
            if ( order.building == BUILD_SHIPYARD && !usefulShipyard( castle ) ) continue;
            if ( KC::economicTown( castle ) && ( static_cast<uint32_t>( order.building ) & DWELLING_MONSTERS )
                 && order.building != DWELLING_MONSTER1 ) continue;
            if ( isLegacyFarmReconversion( castle, order.building ) ) {
                continue;
            }

            if ( !Difficulty::allowAIToBuildCastleBuilding( gameDifficulty, isGameCampaign, order.building ) ) {
                continue;
            }

            const uint32_t priority = order.priority;
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
                                   << ", cost " << castle.getBuildingCost( order.building ).String()
                                   << ", resources " << castle.GetKingdom().GetFunds().String() << ", multiplier " << fundsMultiplier );
            }
            if ( castle.CheckBuyBuilding( order.building ) != BuildingStatus::LACK_RESOURCES
                 && !AI::KnightConstruction::canSpend( castle, order.building ) ) continue;
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

    bool CastleDevelopment( Castle & castle, int safetyFactor, int spellLevel )
    {
        if ( !Difficulty::allowAIToDevelopCastlesOnDay( Game::getDifficulty(), Game::isCampaign(), world.CountDay() ) ) {
            return false;
        }

        if ( castle.GetRace() == Race::KNGT ) {
            const auto primary = knightPrimaryDevelopment( castle );
            if ( primary != PrimaryResult::Fallback ) return primary == PrimaryResult::Built;
        }
        if ( AI::KnightEconomy::develop( castle, safetyFactor > 0 ) ) {
            return true;
        }

        if ( castle.GetRace() != Race::KNGT && castle.isCastle() && !castle.isBuild( BUILD_WELL ) && world.CountDay() > 6 ) {
            // If you can't build Well, you won't be able to build anything else
            return AI::BuildIfPossible( castle, BUILD_WELL );
        }

        if ( Build( castle, GetIncomeStructures( castle.GetRace() ) ) ) {
            return true;
        }

        const size_t neighbourRegions = world.getRegion( world.getTile( castle.GetIndex() ).GetRegion() ).getNeighboursCount();
        const bool islandOrPeninsula = neighbourRegions < 3;

        // Force building a shipyard, +1 to cost check since we can have 0 neighbours
        if ( islandOrPeninsula && usefulShipyard( castle ) && AI::BuildIfEnoughFunds( castle, BUILD_SHIPYARD, static_cast<uint32_t>( neighbourRegions + 1 ) ) ) {
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

        // Boats are considered before hero movement, independently of the daily
        // building slot. They must not be purchased here after heroes have moved.

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
    const auto beforeHero = KnightObserver::armyJson( army ), beforeGarrison = KnightObserver::armyJson( castle.GetArmy() );
    const fheroes2::TaxpayerTraceScope selectionTrace( [&]() {
        ECONOMY_TRACE( 1, "CASTLE_DEFENSE_SELECTION", ",\"town\":" << castle.GetIndex() << ",\"hero\":" << hero->GetID()
            << ",\"hero_before\":" << beforeHero << ",\"garrison_before\":" << beforeGarrison
            << ",\"hero_after\":" << KnightObserver::armyJson( army ) << ",\"garrison_after\":" << KnightObserver::armyJson( castle.GetArmy() ) );
    } );

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
            if ( isLegacyFarmReconversion( *castle, order.building ) ) {
                continue;
            }
            const BuildingStatus status = castle->CheckBuyBuilding( order.building );
            if ( status == BuildingStatus::LACK_RESOURCES ) {
                Funds missing = castle->getBuildingCost( order.building ) - kindgomFunds;

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
    const auto task = _priorityTargets.find( castle.GetIndex() );
    const bool defenseTask = task != _priorityTargets.end() && task->second.type == PriorityTaskType::DEFEND;
    if ( taxpayerKeepTrainingPopulation( castle.hasTrainingGrounds(), Conscription::settings().enabled, defenseTask ) ) {
        return true;
    }
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
    if ( const auto * guest = castle.GetHero(); guest && ( AI::KnightConstruction::isOpeningSeaExplorer( *guest ) || AI::KnightConstruction::keepPopulationCargo( *guest ) )
         && !( constructionContext && constructionContext->endangered.count( castle.GetIndex() ) ) ) return;
    const uint32_t region = world.getTile( castle.GetIndex() ).GetRegion();
    const bool urgent = ( constructionContext && constructionContext->endangered.count( castle.GetIndex() ) )
        || ( defensiveStrategy && ( isCriticalTask( castle.GetIndex() ) || ( region < _regions.size() && _regions[region].safetyFactor < 0 ) ) );
    const AI::KnightConstruction::Emergency emergency( urgent );
    if ( urgent && castle.GetHero() ) KC::cancelPopulationRoute( *castle.GetHero(), "military_emergency" );
    if ( urgent ) ECONOMY_TRACE( 1, "BUDGET_EMERGENCY", ",\"town\":" << castle.GetIndex() << ",\"reason\":\"known_military_threat\"" );

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

    const AI::KnightConstruction::Recruitment recruitment( castle );

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
            if ( [this, &castle = std::as_const( castle ), &garrison, &guestHero = std::as_const( *guestHero ), &guestHeroArmy]() {
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
        if ( !urgent ) KC::arrangePopulation( *guestHero, castle );
    }
}

void AI::Planner::CastleTurn( Castle & castle, const bool defensiveStrategy )
{
    const KC::Emergency emergency( defensiveStrategy && constructionContext && constructionContext->endangered.count( castle.GetIndex() ) );
    if ( defensiveStrategy ) {
        // If the castle is potentially under threat, then it makes sense to try to hire the maximum number of troops so that the enemy cannot hire them even if he
        // captures the castle, therefore, it is worth starting with hiring.
        reinforceCastle( castle, true );

        bool investmentSafe = true;
        const auto task = _priorityTargets.find( castle.GetIndex() );
        if ( task != _priorityTargets.end() && task->second.type == PriorityTaskType::DEFEND ) {
            for ( const int32_t tile : task->second.secondaryTaskTileId ) {
                const auto enemy = _enemyArmies.find( tile );
                if ( enemy != _enemyArmies.end() && enemy->second.strength > 0
                     && castle.GetGarrisonStrength( enemy->second.hero ) < taxpayerViableDefenseRatio * enemy->second.strength ) investmentSafe = false;
            }
        }
        if ( castle.GetRace() == Race::KNGT ) {
            const auto primary = knightPrimaryDevelopment( castle );
            if ( primary != PrimaryResult::Fallback ) return;
        }

        // Avoid building monster dwellings when defensive as they might fall into enemy's hands. Instead, try to build defensive structures if there is at least some
        // kind of garrison in the castle.
        if ( castle.GetActualArmy().getTotalCount() > 0 ) {
            Build( castle, defensiveStructures );
            Build( castle, supportingDefensiveStructures );
        }

        if ( !castle.isCastle() ) {
            const auto price = castle.getBuildingCost( BUILD_CASTLE );
            const auto gain = ProfitConditions::FromBuilding( BUILD_CASTLE, castle.GetRace() ).gold
                              - ProfitConditions::FromBuilding( BUILD_TENT, castle.GetRace() ).gold;
            const bool invest = investmentSafe && gain > 0 && taxpayerFarmPaysBackQuickly( gain, price.gold )
                && taxpayerDefensiveFarmAffordable( castle.GetKingdom().GetFunds().gold, price.gold )
                && Difficulty::allowAIToDevelopCastlesOnDay( Game::getDifficulty(), Game::isCampaign(), world.CountDay() )
                && Difficulty::allowAIToBuildCastleBuilding( Game::getDifficulty(), Game::isCampaign(), BUILD_CASTLE )
                && castle.AllowBuyBuilding( BUILD_CASTLE ) && AI::KnightConstruction::canSpend( castle, BUILD_CASTLE );
            ECONOMY_TRACE( 1, "DEFENSIVE_CASTLE_REVIEW", ",\"player\":" << static_cast<int>( castle.GetColor() )
                << ",\"town\":" << castle.GetIndex() << ",\"safe\":" << ( investmentSafe ? "true" : "false" )
                << ",\"status\":" << static_cast<int>( castle.CheckBuyBuilding( BUILD_CASTLE ) )
                << ",\"funds\":" << Economy::fundsJson( castle.GetKingdom().GetFunds() )
                << ",\"income_gain\":" << gain << ",\"invest\":" << ( invest ? "true" : "false" ) );
            if ( invest ) castle.BuyBuilding( BUILD_CASTLE );
        }

        if ( castle.GetRace() == Race::KNGT ) {
            // Log an explicit safety refusal as well as successful economic reviews.
            AI::KnightEconomy::develop( castle, investmentSafe );
        }

    }
    else {
        const uint32_t regionID = world.getTile( castle.GetIndex() ).GetRegion();
        const RegionStats & stats = _regions[regionID];

        CastleDevelopment( castle, stats.safetyFactor, stats.spellLevel );
        recruitTaxpayerPeasants( castle );
    }
}

AI::KnightConstruction::Scope::Scope( const Kingdom & kingdom, const std::vector<const Castle *> & order ) : kingdom_( &kingdom )
{
    if ( fheroes2::taxpayerSimulationDepth ) { kingdom_ = nullptr; return; }
    assert( !constructionContext );
    constructionContext.emplace( ConstructionContext{ &kingdom, {}, {}, {} } );
    std::vector<const Castle *> cities = order;
    if ( cities.empty() ) for ( const Castle * castle : kingdom.GetCastles() ) if ( castle ) cities.push_back( castle );
    // Inventory today's choices before forecasting or reserving tomorrow's funds.
    for ( const Castle * castle : cities ) {
        if ( !castle || castle->GetRace() != Race::KNGT ) continue;
        uint32_t today = 0;
        for ( const auto building : primaryOrder( *castle ) ) {
            if ( primaryReady( *castle, building, world.CountDay() ) && castle->CheckBuyBuilding( building ) == BuildingStatus::ALLOW_BUILD ) {
                today = building; break;
            }
        }
        ECONOMY_TRACE( 1, "CONSTRUCTION_TODAY_CHOICE", ",\"town\":" << castle->GetIndex() << ",\"candidate\":" << today );
    }
    const auto funds = tomorrowFunds( kingdom );
    if ( !funds ) return;
    size_t sequence = 0;
    for ( const Castle * castle : cities ) {
        if ( castle && castle->hasTrainingGrounds() && castle->CheckBuyBuilding( BUILD_WEL2 ) == BuildingStatus::ALLOW_BUILD )
            constructionContext->ledger.book( castle->GetIndex(), KC::Promise{ BUILD_WEL2, 0, sequence++, constructionResources( castle->getBuildingCost( BUILD_WEL2 ) ) }, *funds );
    }
    for ( const Castle * castle : cities ) if ( castle ) reservePrimary( constructionContext->ledger, *castle, *funds, sequence++ );
}
AI::KnightConstruction::Scope::~Scope()
{
    if ( kingdom_ && constructionContext && constructionContext->kingdom == kingdom_ ) constructionContext.reset();
}
bool AI::KnightConstruction::canSpend( const Castle & castle, const uint32_t building )
{
    if ( fheroes2::taxpayerSimulationDepth || emergencyDepth ) return true;
    if ( !woodPreserved( castle, building ) ) return false;
    const auto & kingdom = castle.GetKingdom();
    const bool primary = building == BUILD_CASTLE || building == BUILD_STATUE || building == BUILD_WELL || ( building & DWELLING_MONSTERS )
        || ( castle.GetRace() == Race::KNGT && ( building == BUILD_WEL2 || building == BUILD_TAVERN ) );
    const bool mobilizing = std::any_of( kingdom.GetCastles().begin(), kingdom.GetCastles().end(), []( const Castle * c ) { return c && c->hasTrainingGrounds(); } );
    if ( mobilizing && !primary && int64_t{ kingdom.GetFunds().gold } - castle.getBuildingCost( building ).gold < operatingReserve( kingdom ) ) {
        ECONOMY_TRACE( 1, "CONSTRUCTION_WAIT", ",\"town\":" << castle.GetIndex() << ",\"candidate\":" << building << ",\"reason\":\"mobilization_commitment\"" );
        return false;
    }
    if ( !constructionContext || constructionContext->kingdom != &castle.GetKingdom() ) return true;
    return constructionAffordable( castle, building, constructionContext->ledger );
}

AI::KnightConstruction::Emergency::Emergency( const bool active ) : active_( active ) { if ( active_ ) ++emergencyDepth; }
AI::KnightConstruction::Emergency::~Emergency() { if ( active_ ) --emergencyDepth; }
bool AI::KnightConstruction::economicTown( const Castle & castle )
{
    return constructionContext && constructionContext->kingdom == &castle.GetKingdom() && constructionContext->economic.count( castle.GetIndex() );
}
void AI::KnightConstruction::setRoles( const Kingdom & kingdom, const std::vector<int> & front, const std::vector<int> & danger )
{
    if ( !constructionContext || constructionContext->kingdom != &kingdom ) return;
    auto & economic = constructionContext->economic; economic.clear();
    constructionContext->endangered = { danger.begin(), danger.end() };
    std::vector<const Castle *> cities;
    for ( const auto * c : kingdom.GetCastles() ) if ( c && c->GetRace() == Race::KNGT ) cities.push_back( c );
    if ( cities.empty() ) return;
    auto & history = roleHistory[static_cast<int>( kingdom.GetColor() )];
    const uint32_t day = world.CountDay();
    const auto owned = [&]( int tile ) { return std::any_of( cities.begin(), cities.end(), [&]( const Castle * c ) { return c->GetIndex() == tile; } ); };
    const bool reset = history.seed != world.GetMapSeed() || history.segment != Economy::traceState().segment || day < history.reviewed;
    const bool review = reset || !owned( history.primary ) || day - history.reviewed >= 7;
    std::map<int, uint32_t> costs;
    const auto distance = [&]( const Castle * c ) {
        const auto found = costs.find( c->GetIndex() ); if ( found != costs.end() ) return found->second;
        uint32_t best = UINT32_MAX;
        if ( !front.empty() ) {
            AIWorldPathfinder path; path.reset();
            path.reEvaluateIfNeeded( c->GetIndex(), kingdom.GetColor(), c->GetArmy().GetStrength(), Skill::Level::EXPERT );
            for ( const int target : front ) {
                if ( target == c->GetIndex() ) { best = 0; break; }
                const auto cost = path.getDistance( target ); if ( cost ) best = std::min( best, cost );
            }
        }
        costs[c->GetIndex()] = best; return best;
    };
    const char * reason = "retain_weekly_role";
    if ( review ) {
        std::stable_sort( cities.begin(), cities.end(), [&]( const Castle * a, const Castle * b ) {
            const bool ap = KnightEconomy::commitmentUntil( *a ) > day, bp = KnightEconomy::commitmentUntil( *b ) > day;
            if ( ap != bp ) return ap;
            if ( a->hasTrainingGrounds() != b->hasTrainingGrounds() ) return a->hasTrainingGrounds();
            const auto da = distance( a ), db = distance( b ); if ( da != db ) return da < db;
            const bool ar = a->AllowBuyBuilding( BUILD_TRAINING_GROUNDS ), br = b->AllowBuyBuilding( BUILD_TRAINING_GROUNDS );
            if ( ar != br ) return ar;
            if ( a->getBuildingValue() != b->getBuildingValue() ) return a->getBuildingValue() > b->getBuildingValue();
            return a->GetArmy().GetCountMonsters( Monster::PEASANT ) < b->GetArmy().GetCountMonsters( Monster::PEASANT );
        } );
        history = { world.GetMapSeed(), day, Economy::traceState().segment, cities.front()->GetIndex() };
        reason = reset ? "new_role_plan" : "weekly_role_review";
    }
    for ( const auto * c : cities ) {
        const bool committed = KnightEconomy::commitmentUntil( *c ) > day;
        const bool rear = cities.size() > 1 && c->GetIndex() != history.primary && !committed
            && constructionContext->endangered.count( c->GetIndex() ) == 0;
        if ( rear ) economic.insert( c->GetIndex() );
        ECONOMY_TRACE( 1, "KNIGHT_TOWN_ROLE", ",\"player\":" << static_cast<int>( kingdom.GetColor() ) << ",\"town\":" << c->GetIndex()
            << ",\"primary\":" << ( c->GetIndex() == history.primary ? "true" : "false" ) << ",\"endangered\":" << ( constructionContext->endangered.count( c->GetIndex() ) ? "true" : "false" ) << ",\"economic\":" << ( rear ? "true" : "false" ) << ",\"reason\":" << Economy::jsonQuote( committed ? "funded_mobilization" : reason )
            << ",\"role_until\":" << std::max( history.reviewed + 7, KnightEconomy::commitmentUntil( *c ) )
            << ",\"known_front_cost\":" << ( front.empty() ? -1 : static_cast<int64_t>( distance( c ) ) ) );
    }
    refresh();
}

bool AI::KnightConstruction::populationSafe( const Castle & castle )
{
    return castle.GetKingdom().isControlAI() && castle.GetRace() == Race::KNGT
        && ( !constructionContext || constructionContext->kingdom != &castle.GetKingdom()
             || constructionContext->endangered.count( castle.GetIndex() ) == 0 );
}
uint32_t AI::KnightConstruction::populationReserve( const Castle & castle )
{
    if ( !populationSafe( castle ) ) return 0;
    const auto & cities = castle.GetKingdom().GetCastles();
    const auto knights = std::count_if( cities.begin(), cities.end(), []( const Castle * c ) { return c && c->GetRace() == Race::KNGT; } );
    if ( !castle.hasTrainingGrounds() && ( economicTown( castle ) || knights <= 1 || !active( castle.GetKingdom() ) ) ) return UINT32_MAX;
    const auto & config = Conscription::settings();
    if ( !config.enabled ) return 0;
    uint64_t hut = castle.conscriptionDaySnapshot().hut, need = 0;
    const uint32_t growth = castle.isBuild( DWELLING_MONSTER1 ) ? Monster( Monster::PEASANT ).GetGrown()
        + ( castle.isBuild( BUILD_WELL ) ? Castle::GetGrownWell() : 0 ) : 0;
    // Minimum initial garrison to bridge fourteen days, including the timing
    // of Hut growth. A prospective TG never receives the Farm's growth bonus.
    for ( uint32_t offset = 1; offset <= 14; ++offset ) {
        const uint64_t date = uint64_t{ world.CountDay() } + offset;
        if ( ( date - 1 ) % 7 == 0 ) hut += growth;
        const uint64_t demand = ( date - 1 ) % 7 % 2 == 0 ? config.oddDayLimit : config.evenDayLimit;
        if ( hut >= demand ) hut -= demand;
        else { need += demand - hut; hut = 0; }
    }
    return static_cast<uint32_t>( std::min<uint64_t>( need, UINT32_MAX ) );
}
void AI::KnightConstruction::cancelPopulationRoute( const Heroes & hero, const char * reason )
{
    const auto it = populationRoutes.find( hero.GetID() ); if ( it == populationRoutes.end() ) return;
    ECONOMY_TRACE( 1, "POPULATION_ROUTE", ",\"player\":" << static_cast<int>( hero.GetColor() ) << ",\"hero\":" << hero.GetID()
        << ",\"target\":" << it->second.target << ",\"pickup\":" << it->second.pickup << ",\"quantity\":" << it->second.quantity
        << ",\"reason\":" << Economy::jsonQuote( reason ) );
    populationRoutes.erase( it );
}
int AI::KnightConstruction::populationOrder( const Heroes & hero )
{
    const auto it = populationRoutes.find( hero.GetID() ); if ( it == populationRoutes.end() ) return -1;
    const auto r = it->second;
    const auto & cities = hero.GetKingdom().GetCastles();
    const int waypoint = r.pickup >= 0 ? r.pickup : r.target;
    const auto found = std::find_if( cities.begin(), cities.end(), [&]( const Castle * c ) { return c && c->GetIndex() == waypoint; } );
    const auto final = std::find_if( cities.begin(), cities.end(), [&]( const Castle * c ) { return c && c->GetIndex() == r.target; } );
    const char * failure = nullptr;
    if ( !hero.isControlAI() || r.seed != world.GetMapSeed() || r.segment != Economy::traceState().segment
         || r.owner != static_cast<int>( hero.GetColor() ) || world.CountDay() < r.day ) failure = "stale_route";
    else if ( r.pickup < 0 && !hero.GetArmy().GetCountMonsters( Monster::PEASANT ) ) failure = "cargo_delivered_or_removed";
    else if ( hero.isShipMaster() || hero.Modes( Heroes::PATROL ) ) failure = "incompatible_hero";
    else if ( constructionContext && constructionContext->endangered.count( hero.GetIndex() ) ) failure = "military_emergency";
    else if ( found == cities.end() || !populationSafe( **found ) || !( ( *found )->isBuild( BUILD_WEL2 ) || ( *found )->hasTrainingGrounds() ) ) failure = "destination_unavailable";
    else if ( r.pickup >= 0 && ( !( *found )->isBuild( BUILD_WEL2 ) || ( *found )->hasTrainingGrounds()
              || !( *found )->GetArmy().GetCountMonsters( Monster::PEASANT ) || final == cities.end()
              || !populationSafe( **final ) || !( *final )->hasTrainingGrounds()
              || ( ( *final )->GetHero() && ( *final )->GetHero() != &hero ) ) ) failure = "pickup_unavailable";
    else if ( ( *found )->GetHero() && ( *found )->GetHero() != &hero ) failure = "destination_occupied";
    else if ( !( *found )->GetArmy().HasMonster( Monster::PEASANT ) && ( *found )->GetArmy().GetOccupiedSlotCount() == 5 ) failure = "destination_full";
    else if ( world.CountDay() - r.day > 4 ) failure = "delivery_timeout";
    else if ( hero.GetIndex() != waypoint ) {
        AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( hero );
        const auto route = path.buildPath( waypoint, false );
        if ( !path.getDistance( waypoint ) || route.empty()
             || std::any_of( route.begin(), route.end(), []( const auto & step ) { return world.getTile( step.GetIndex() ).isWater(); } ) ) failure = "route_unreachable";
    }
    if ( failure ) {
        cancelPopulationRoute( hero, failure );
        if ( std::string( failure ) == "stale_route" || std::string( failure ) == "cargo_delivered_or_removed"
             || std::string( failure ) == "incompatible_hero" || std::string( failure ) == "military_emergency"
             || !hero.GetArmy().GetCountMonsters( Monster::PEASANT ) ) return -1;
        // A failed delivery is redirected to a nearby safe productive town,
        // including the source Farm. No blind expiry into ordinary wandering.
        AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( hero );
        uint32_t best = UINT32_MAX; int fallback = -1;
        for ( const Castle * city : cities ) {
            if ( !city || !populationSafe( *city ) || !( city->isBuild( BUILD_WEL2 ) || city->hasTrainingGrounds() )
                 || ( city->GetHero() && city->GetHero() != &hero )
                 || ( !city->GetArmy().HasMonster( Monster::PEASANT ) && city->GetArmy().GetOccupiedSlotCount() == 5 ) ) continue;
            const auto cost = city->GetIndex() == hero.GetIndex() ? 0U : path.getDistance( city->GetIndex() );
            if ( ( !cost && city->GetIndex() != hero.GetIndex() ) || cost >= best || uint64_t{ cost } > uint64_t{ hero.GetMaxMovePoints() } * 2 ) continue;
            const auto route = path.buildPath( city->GetIndex(), false );
            if ( std::any_of( route.begin(), route.end(), []( const auto & step ) { return world.getTile( step.GetIndex() ).isWater(); } ) ) continue;
            best = cost; fallback = city->GetIndex();
        }
        if ( fallback >= 0 ) {
            populationRoutes[hero.GetID()] = { world.GetMapSeed(), world.CountDay(), Economy::traceState().segment, static_cast<int>( hero.GetColor() ), fallback };
            ECONOMY_TRACE( 1, "POPULATION_ROUTE", ",\"hero\":" << hero.GetID() << ",\"target\":" << fallback << ",\"reason\":\"safe_fallback\"" );
        }
        return fallback;
    }
    return waypoint;
}
bool AI::KnightConstruction::keepPopulationCargo( const Heroes & hero )
{
    const int target = populationOrder( hero );
    const auto route = populationRoutes.find( hero.GetID() );
    return target >= 0 && target != hero.GetIndex() && route != populationRoutes.end() && route->second.pickup < 0;
}

uint32_t AI::KnightConstruction::populationSupplyNeed( const Castle & castle, const int exceptHero )
{
    if ( !castle.hasTrainingGrounds() || !populationSafe( castle ) || !Conscription::settings().enabled ) return 0;
    uint64_t incoming = 0;
    for ( const auto * hero : castle.GetKingdom().GetHeroes() ) {
        if ( !hero || hero->GetID() == exceptHero || populationOrder( *hero ) < 0 ) continue;
        const auto route = populationRoutes.find( hero->GetID() );
        if ( route == populationRoutes.end() || route->second.target != castle.GetIndex() ) continue;
        uint64_t cargo = route->second.pickup >= 0 ? route->second.quantity : hero->GetArmy().GetCountMonsters( Monster::PEASANT );
        if ( route->second.pickup >= 0 ) {
            const auto * farm = world.getCastleEntrance( Maps::GetPoint( route->second.pickup ) );
            cargo = farm ? std::min<uint64_t>( cargo, farm->GetArmy().GetCountMonsters( Monster::PEASANT ) ) : 0;
        }
        if ( cargo && hero->GetArmy().GetOccupiedSlotCount() == 1 && hero->GetArmy().HasMonster( Monster::PEASANT ) ) --cargo;
        incoming += cargo;
    }
    const uint64_t held = castle.GetArmy().GetCountMonsters( Monster::PEASANT );
    const uint64_t need = populationReserve( castle );
    return held + incoming >= need ? 0 : static_cast<uint32_t>( need - held - incoming );
}

uint32_t AI::KnightConstruction::populationAvailableStock( const Castle & castle, const int exceptHero )
{
    uint64_t reserved = 0;
    for ( const auto * hero : castle.GetKingdom().GetHeroes() ) {
        if ( !hero || hero->GetID() == exceptHero || populationOrder( *hero ) < 0 ) continue;
        const auto route = populationRoutes.find( hero->GetID() );
        if ( route != populationRoutes.end() && route->second.pickup == castle.GetIndex() ) reserved += route->second.quantity;
    }
    const uint64_t stock = castle.GetArmy().GetCountMonsters( Monster::PEASANT );
    return stock > reserved ? static_cast<uint32_t>( stock - reserved ) : 0;
}

int AI::KnightConstruction::populationPickupDestination( const Heroes & hero, const Castle & source )
{
    if ( populationOrder( hero ) != source.GetIndex() ) return -1;
    const auto route = populationRoutes.find( hero.GetID() );
    if ( route == populationRoutes.end() || route->second.pickup != source.GetIndex() ) return -1;
    const int target = route->second.target;
    cancelPopulationRoute( hero, "pickup_arrived" );
    return target;
}

void AI::KnightConstruction::preparePopulation( Kingdom & kingdom )
{
    if ( fheroes2::taxpayerSimulationDepth || !kingdom.isControlAI() || !active( kingdom ) || !Conscription::settings().enabled
         || !Kingdom::TaxpayerRequiresFarms() || Kingdom::GetTaxpayerGoldPerPeasantPerDay() <= 0 ) return;
    // Existing loads and pending pickups reserve the same destination stock.
    // Only short land trips by existing scouts/couriers are considered.
    for ( const auto * hero : kingdom.GetHeroes() ) if ( hero ) (void)populationOrder( *hero );
    for ( const Castle * target : kingdom.GetCastles() ) {
        if ( !target || !target->hasTrainingGrounds() || !populationSafe( *target ) || target->GetHero()
             || ( !target->GetArmy().HasMonster( Monster::PEASANT ) && target->GetArmy().GetOccupiedSlotCount() == 5 ) ) continue;
        const uint32_t need = populationSupplyNeed( *target );
        if ( !need ) {
            ECONOMY_TRACE( 1, "POPULATION_SUPPLY_REVIEW", ",\"town\":" << target->GetIndex() << ",\"need\":0,\"garrison\":"
                << target->GetArmy().GetCountMonsters( Monster::PEASANT ) << ",\"reserve\":" << populationReserve( *target )
                << ",\"reason\":\"stock_or_incoming_sufficient\"" );
            continue;
        }
        Heroes * chosen = nullptr; const Castle * source = nullptr; uint32_t best = UINT32_MAX, quantity = 0;
        for ( Heroes * hero : kingdom.GetHeroes() ) {
            if ( !hero || !hero->isActive() || !hero->GetMovePoints() || hero->isShipMaster() || hero->Modes( Heroes::PATROL | Heroes::SLEEPER )
                 || hero->isLosingGame() || populationRoutes.count( hero->GetID() ) || isOpeningSeaExplorer( *hero )
                 || navalTarget( *hero ) >= 0 || ( hero->getAIRole() != Heroes::Role::COURIER && hero->getAIRole() != Heroes::Role::SCOUT
                      && hero->getAIRole() != Heroes::Role::HUNTER )
                 || constructionContext->endangered.count( hero->GetIndex() ) ) continue;
            AIWorldPathfinder pickupPath; pickupPath.reset(); pickupPath.reEvaluateIfNeeded( *hero );
            for ( const Castle * farm : kingdom.GetCastles() ) {
                if ( !farm || farm == target || !populationSafe( *farm ) || !economicTown( *farm ) || !farm->isBuild( BUILD_WEL2 )
                     || farm->hasTrainingGrounds() || ( farm->GetHero() && farm->GetHero() != hero ) ) continue;
                const uint32_t stock = populationAvailableStock( *farm );
                if ( !stock ) continue;
                const auto pickup = farm->GetIndex() == hero->GetIndex() ? 0U : pickupPath.getDistance( farm->GetIndex() );
                if ( ( !pickup && farm->GetIndex() != hero->GetIndex() ) || pickup > std::min<uint32_t>( 1000, hero->GetMaxMovePoints() ) ) continue;
                AIWorldPathfinder deliveryPath; deliveryPath.reset();
                deliveryPath.reEvaluateIfNeeded( farm->GetIndex(), kingdom.GetColor(), hero->GetArmy().GetStrength(), hero->GetLevelSkill( Skill::Secondary::PATHFINDING ) );
                const auto delivery = deliveryPath.getDistance( target->GetIndex() );
                const uint32_t limit = hero->getAIRole() == Heroes::Role::COURIER ? 3000 : 1000;
                if ( !delivery || delivery > 2000 || uint64_t{ pickup } + delivery > limit || pickup + delivery >= best ) continue;
                const auto first = pickupPath.buildPath( farm->GetIndex(), false );
                const auto second = deliveryPath.buildPath( target->GetIndex(), false );
                const auto water = []( const auto & step ) { return world.getTile( step.GetIndex() ).isWater(); };
                if ( std::any_of( first.begin(), first.end(), water ) || second.empty() || std::any_of( second.begin(), second.end(), water ) ) continue;
                chosen = hero; source = farm; best = pickup + delivery;
                quantity = std::min( need, stock );
            }
        }
        if ( !chosen || !source || !quantity ) {
            ECONOMY_TRACE( 1, "POPULATION_SUPPLY_REVIEW", ",\"town\":" << target->GetIndex() << ",\"need\":" << need
                << ",\"garrison\":" << target->GetArmy().GetCountMonsters( Monster::PEASANT ) << ",\"reserve\":" << populationReserve( *target )
                << ",\"reason\":\"no_short_safe_courier\"" );
            continue;
        }
        populationRoutes[chosen->GetID()] = { world.GetMapSeed(), world.CountDay(), Economy::traceState().segment,
            static_cast<int>( kingdom.GetColor() ), target->GetIndex(), source->GetIndex(), quantity };
        ECONOMY_TRACE( 1, "POPULATION_SUPPLY_PLAN", ",\"player\":" << static_cast<int>( kingdom.GetColor() ) << ",\"hero\":" << chosen->GetID()
            << ",\"source\":" << source->GetIndex() << ",\"target\":" << target->GetIndex() << ",\"quantity\":" << quantity
            << ",\"need\":" << need << ",\"route_cost\":" << best << ",\"reserve_days\":14" );
        if ( chosen->inCastle() == source ) arrangePopulation( *chosen, *chosen->inCastleMutable() );
    }
}
int AI::KnightConstruction::populationDestination( const Heroes & hero )
{
    if ( !hero.isControlAI() || hero.isShipMaster() || hero.Modes( Heroes::PATROL | Heroes::SLEEPER ) ) return -1;
    if ( const int pending = populationOrder( hero ); pending >= 0 && pending != hero.GetIndex() ) return pending;
    const auto * source = hero.inCastle();
    const bool courier = hero.getAIRole() == Heroes::Role::COURIER;
    const bool scout = hero.getAIRole() == Heroes::Role::SCOUT || hero.getAIRole() == Heroes::Role::HUNTER;
    const bool carries = hero.GetArmy().GetCountMonsters( Monster::PEASANT ) > 0;
    if ( !carries && ( !source || ( !courier && !scout ) ) ) return -1;
    AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( hero );
    const uint32_t limit = courier ? hero.GetMaxMovePoints() * 2 : scout ? hero.GetMaxMovePoints() : std::min<uint32_t>( 300, hero.GetMovePoints() / 4 );
    uint32_t best = UINT32_MAX; int target = -1;
    for ( const Castle * c : hero.GetKingdom().GetCastles() ) {
        if ( !c || c == source || !populationSafe( *c ) || ( c->GetHero() && c->GetHero() != &hero ) ) continue;
        const uint64_t surplus = source && source->GetRace() == Race::KNGT && populationReserve( *source ) != UINT32_MAX
            ? std::max<int64_t>( 0, int64_t{ source->GetArmy().GetCountMonsters( Monster::PEASANT ) } - populationReserve( *source ) ) : 0;
        const uint64_t cargo = hero.GetArmy().GetCountMonsters( Monster::PEASANT ) + surplus;
        const bool fiscal = economicTown( *c ) && c->isBuild( BUILD_WEL2 ) && cargo * std::max( 0, Kingdom::GetTaxpayerGoldPerPeasantPerDay() ) * 7 >= 1000;
        const auto need = populationSupplyNeed( *c, hero.GetID() );
        const bool supply = source && economicTown( *source ) && ( courier || scout ) && c->hasTrainingGrounds()
            && need > 0;
        if ( !fiscal && !supply ) continue;
        if ( !carries && ( !source || source->GetRace() != Race::KNGT || !populationSafe( *source )
             || source->GetArmy().GetCountMonsters( Monster::PEASANT ) <= ( supply ? 1U : populationReserve( *source ) ) ) ) continue;
        if ( !c->GetArmy().HasMonster( Monster::PEASANT ) && c->GetArmy().GetOccupiedSlotCount() >= 4 ) continue;
        const auto cost = path.getDistance( c->GetIndex() );
        if ( !cost || cost > limit || cost >= best ) continue;
        // No naval expedition is created for a population transfer.
        const auto route = path.buildPath( c->GetIndex(), false );
        if ( std::any_of( route.begin(), route.end(), []( const auto & step ) { return world.getTile( step.GetIndex() ).isWater(); } ) ) continue;
        best = cost; target = c->GetIndex();
    }
    return target;
}

void AI::KnightConstruction::notePopulationRoute( const Heroes & hero, const int destination )
{
    if ( destination >= 0 && destination != hero.GetIndex() && hero.GetArmy().GetCountMonsters( Monster::PEASANT ) > 0
         && !populationRoutes.count( hero.GetID() ) )
        populationRoutes[hero.GetID()] = { world.GetMapSeed(), world.CountDay(), Economy::traceState().segment, static_cast<int>( hero.GetColor() ), destination };
}

void AI::KnightConstruction::refresh()
{
    if ( !constructionContext ) return;
    const auto & kingdom = *constructionContext->kingdom;
    const auto future = tomorrowFunds( kingdom );
    if ( !future ) return;
    constructionContext->ledger.promises.clear();
    size_t order = 0;
    for ( const auto * castle : kingdom.GetCastles() ) {
        if ( !castle ) continue;
        if ( castle->hasTrainingGrounds() && castle->CheckBuyBuilding( BUILD_WEL2 ) == BuildingStatus::ALLOW_BUILD )
            constructionContext->ledger.book( castle->GetIndex(), Promise{ BUILD_WEL2, 0, order++, constructionResources( castle->getBuildingCost( BUILD_WEL2 ) ) }, *future );
    }
    for ( const auto * castle : kingdom.GetCastles() ) if ( castle ) reservePrimary( constructionContext->ledger, *castle, *future, order++ );
}
Funds AI::KnightConstruction::spendable( const Kingdom & kingdom )
{
    Funds available = kingdom.GetFunds();
    if ( fheroes2::taxpayerSimulationDepth || emergencyDepth || !kingdom.isControlAI() ) return available;
    const auto floor = operatingReserve( kingdom );
    if ( !floor ) return available;
    auto reserve = reservedDevelopment( kingdom );
    reserve[0] = std::max<int64_t>( reserve[0], floor );
    const auto balance = constructionResources( available );
    Resources result{};
    for ( size_t r = 0; r < result.size(); ++r ) result[r] = std::max<int64_t>( 0, balance[r] - reserve[r] );
    available.gold = static_cast<int32_t>( result[0] ); available.wood = static_cast<int32_t>( result[1] ); available.ore = static_cast<int32_t>( result[2] );
    available.mercury = static_cast<int32_t>( result[3] ); available.sulfur = static_cast<int32_t>( result[4] );
    available.crystal = static_cast<int32_t>( result[5] ); available.gems = static_cast<int32_t>( result[6] );
    return available;
}
AI::KnightConstruction::Resources AI::KnightConstruction::reservedDevelopment( const Kingdom & kingdom )
{
    Resources reserve{};
    if ( constructionContext && constructionContext->kingdom == &kingdom ) {
        for ( const auto & entry : constructionContext->ledger.promises ) {
            const auto found = std::find_if( kingdom.GetCastles().begin(), kingdom.GetCastles().end(), [&]( const Castle * c ) { return c && c->GetIndex() == entry.first; } );
            if ( found == kingdom.GetCastles().end() || ( *found )->isBuild( entry.second.building ) ) continue;
            for ( size_t r = 0; r < reserve.size(); ++r ) reserve[r] += entry.second.cost[r];
        }
    }
    return reserve;
}
int64_t AI::KnightConstruction::operatingReserve( const Kingdom & kingdom )
{
    bool knight = false; int64_t reserve = 2000;
    for ( const auto * c : kingdom.GetCastles() ) if ( c && c->GetRace() == Race::KNGT ) {
        knight = true;
        if ( !c->hasTrainingGrounds() ) continue;
        const auto week = KnightEconomy::weekFunding( *c );
        reserve += c->getBuildingCost( BUILD_WEL2 ).gold + week.fees[0] + ( recruitmentTown == c ? 0 : week.recruits[0] );
    }
    return knight ? reserve : 0;
}
AI::KnightConstruction::Recruitment::Recruitment( const Castle & castle ) : previous_( recruitmentTown ) { recruitmentTown = &castle; }
AI::KnightConstruction::Recruitment::~Recruitment() { recruitmentTown = previous_; }

bool AI::KnightConstruction::canTradeForBuilding( const Castle & castle, const uint32_t building, const Funds & transaction )
{
    const auto & kingdom = castle.GetKingdom();
    if ( fheroes2::taxpayerSimulationDepth || emergencyDepth || !kingdom.isControlAI() ) return true;
    const auto payment = castle.getBuildingCost( building );
    const auto total = constructionResources( transaction + payment );
    auto after = constructionResources( kingdom.GetFunds() );
    for ( size_t r = 0; r < after.size(); ++r ) after[r] -= total[r];
    bool allowed = covers( after, Resources{} ) && after[0] >= operatingReserve( kingdom );
    if ( building != BUILD_MARKETPLACE && !castle.isBuild( BUILD_MARKETPLACE ) && world.CountCapturedMines( Resource::WOOD, castle.GetColor() ) == 0
         && payment.wood > 0 && after[1] < 5 ) allowed = false;
    if ( allowed && constructionContext && constructionContext->kingdom == &kingdom ) {
        auto snapshot = kingdom.economySnapshot(); EP::setFunds( snapshot, after );
        const auto tomorrow = Economy::forecast( snapshot, Conscription::settings(), 1 );
        allowed = tomorrow.complete && constructionContext->ledger.allows( castle.GetIndex(), building, EP::funds( tomorrow.end ) );
    }
    ECONOMY_TRACE( 1, "BUILD_TRADE_REVIEW", ",\"town\":" << castle.GetIndex() << ",\"building\":" << building
        << ",\"total_cost\":" << Economy::arrayJson( total ) << ",\"after\":" << Economy::arrayJson( after )
        << ",\"reserve\":" << operatingReserve( kingdom ) << ",\"allowed\":" << ( allowed ? "true" : "false" ) );
    return allowed;
}
bool AI::KnightConstruction::allowsPayment( const Kingdom & kingdom, const Funds & cost, const char * purpose )
{
    if ( fheroes2::taxpayerSimulationDepth || emergencyDepth || !kingdom.isControlAI() ) return true;
    const auto budget = constructionResources( spendable( kingdom ) ); const auto price = constructionResources( cost );
    const bool allowed = covers( budget, price );
    if ( !allowed ) ECONOMY_TRACE( 1, "BUDGET_WAIT", ",\"player\":" << static_cast<int>( kingdom.GetColor() ) << ",\"purpose\":" << Economy::jsonQuote( purpose )
        << ",\"cost\":" << Economy::arrayJson( price ) << ",\"spendable\":" << Economy::arrayJson( budget ) );
    return allowed;
}
bool AI::KnightConstruction::hasPrimaryChoice( const Castle & castle )
{
    for ( const auto building : primaryOrder( castle ) )
        if ( primaryReady( castle, building, world.CountDay() ) && castle.CheckBuyBuilding( building ) == BuildingStatus::ALLOW_BUILD ) return true;
    return false;
}
bool AI::KnightConstruction::mobilizationPreservesDevelopment( const Castle & castle )
{
    const auto & kingdom = castle.GetKingdom();
    const auto price = constructionResources( castle.getBuildingCost( BUILD_TRAINING_GROUNDS ) );
    auto before = constructionResources( kingdom.GetFunds() ); auto after = before;
    for ( size_t r = 0; r < after.size(); ++r ) after[r] -= price[r];
    const auto income = constructionResources( kingdom.GetIncome( Kingdom::INCOME_ALL ) );
    for ( const auto * c : kingdom.GetCastles() ) {
        if ( !c || c->GetRace() != Race::KNGT ) continue;
        for ( const auto building : primaryOrder( *c ) ) {
            if ( !primaryReady( *c, building, world.CountDay() ) ) continue;
            const auto cost = constructionResources( c->getBuildingCost( building ) );
            int first = -1, changed = -1;
            auto a = before, b = after;
            for ( int day = 0; day <= 7; ++day ) {
                if ( first < 0 && covers( a, cost ) ) first = day;
                if ( changed < 0 && covers( b, cost ) ) changed = day;
                for ( size_t r = 0; r < a.size(); ++r ) { a[r] += income[r]; b[r] += income[r]; }
            }
            if ( first >= 0 && ( changed < 0 || changed > first ) ) return false;
        }
    }
    return true;
}

Heroes * AI::KnightConstruction::openingRecruit( const Castle & port, Heroes * first, Heroes * second )
{
    const auto & kingdom = port.GetKingdom();
    if ( fheroes2::taxpayerSimulationDepth || !kingdom.isControlAI() || world.CountDay() > 7 || kingdom.GetHeroes().size() != 1
         || !port.isBuild( BUILD_SHIPYARD ) || kingdom.GetHeroes().front()->isShipMaster()
         || ( constructionContext && constructionContext->endangered.count( port.GetIndex() ) ) ) return nullptr;
    const auto launched = openingLaunches.find( static_cast<int>( kingdom.GetColor() ) );
    if ( launched != openingLaunches.end() && launched->second.seed == world.GetMapSeed() && launched->second.segment == Economy::traceState().segment ) return nullptr;
    const auto allowed = []( Heroes * h ) { return h && h->isAvailableForHire() && h != world.GetHeroesCondWins() && h != world.GetHeroesCondLoss(); };
    if ( !allowed( first ) ) first = nullptr;
    if ( !allowed( second ) ) second = nullptr;
    if ( !first && !second ) return nullptr;
    int dock = port.getTileIndexToPlaceBoat();
    if ( dock < 0 ) {
        const int y = port.GetIndex() / world.w() + 2;
        for ( int dx = -1; dx <= 1; ++dx ) {
            const int x = port.GetIndex() % world.w() + dx;
            if ( x >= 0 && x < world.w() && y < world.h() && world.getTile( y * world.w() + x ).getMainObjectType( false ) == MP2::OBJ_BOAT ) { dock = y * world.w() + x; break; }
        }
    }
    if ( dock < 0 || world.getTile( dock ).isFog( kingdom.GetColor() ) ) return nullptr;
    const bool existing = world.getTile( dock ).getMainObjectType( false ) == MP2::OBJ_BOAT;
    Funds cost = PaymentConditions::RecruitHero();
    if ( !existing ) cost += PaymentConditions::BuyBoat();
    if ( !port.isBuild( BUILD_STATUE ) ) cost += port.getBuildingCost( BUILD_STATUE );
    const bool mobilizing = std::any_of( kingdom.GetCastles().begin(), kingdom.GetCastles().end(), []( const Castle * c ) { return c && c->hasTrainingGrounds(); } );
    if ( cost.gold < PaymentConditions::RecruitHero().gold + ( existing ? 0 : PaymentConditions::BuyBoat().gold ) + operatingReserve( kingdom ) )
        cost.gold = static_cast<int32_t>( PaymentConditions::RecruitHero().gold + ( existing ? 0 : PaymentConditions::BuyBoat().gold ) + operatingReserve( kingdom ) );
    if ( !kingdom.AllowPayment( cost ) || ( mobilizing && !allowsPayment( kingdom, cost, "opening_hero_and_boat" ) ) ) return nullptr;
    AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( dock, kingdom.GetColor(), kingdom.GetHeroes().front()->GetArmy().GetStrength(), Skill::Level::BASIC );
    if ( seaOptions( *kingdom.GetHeroes().front(), dock, path ).empty() ) return nullptr;
    const auto rank = [&]( const Heroes * h ) { return h->GetRace() == Race::SORC ? 2 : h->GetRace() == port.GetRace() ? 1 : 0; };
    if ( !first ) return second;
    if ( !second ) return first;
    if ( rank( first ) != rank( second ) ) return rank( first ) > rank( second ) ? first : second;
    return first->getRecruitValue() >= second->getRecruitValue() ? first : second;
}
void AI::KnightConstruction::noteOpeningRecruit( const Heroes & hero, const Castle & port )
{
    openingRecruits[static_cast<int>( hero.GetColor() )] = { world.GetMapSeed(), world.CountDay(), Economy::traceState().segment,
        static_cast<int>( hero.GetColor() ), hero.GetID(), port.GetIndex(), false };
    ECONOMY_TRACE( 1, "NAVAL_OPENING_RECRUIT", ",\"player\":" << static_cast<int>( hero.GetColor() ) << ",\"hero\":" << hero.GetID()
        << ",\"town\":" << port.GetIndex() << ",\"race\":" << hero.GetRace() );
}
bool AI::KnightConstruction::isOpeningSeaExplorer( const Heroes & hero )
{
    if ( fheroes2::taxpayerSimulationDepth || !hero.isControlAI() ) return false;
    const auto it = openingExplorers.find( hero.GetID() ); if ( it == openingExplorers.end() ) return false;
    auto & m = it->second;
    if ( m.seed != world.GetMapSeed() || m.segment != Economy::traceState().segment || m.owner != static_cast<int>( hero.GetColor() )
         || world.CountDay() < m.day || ( m.embarked && !hero.isShipMaster() )
         || ( !m.embarked && ( world.CountDay() > 7 || world.CountDay() - m.day > 1 ) ) ) {
        openingExplorers.erase( it ); return false;
    }
    if ( hero.isShipMaster() ) m.embarked = true;
    return true;
}
void AI::KnightConstruction::markOpeningEmbarked( const Heroes & hero )
{
    const auto it = openingExplorers.find( hero.GetID() );
    if ( it != openingExplorers.end() && isOpeningSeaExplorer( hero ) ) it->second.embarked = true;
}
bool AI::KnightConstruction::prepareOpeningExplorer( Heroes & hero, Castle & port, const int target )
{
    if ( fheroes2::taxpayerSimulationDepth || !hero.isControlAI() || hero.isShipMaster() || hero.inCastle() != &port
         || hero.GetColor() != port.GetColor() || !port.isBuild( BUILD_SHIPYARD ) || world.CountDay() > 7
         || ( constructionContext && constructionContext->endangered.count( port.GetIndex() ) ) ) return false;
    const auto launched = openingLaunches.find( static_cast<int>( hero.GetColor() ) );
    if ( launched != openingLaunches.end() && launched->second.seed == world.GetMapSeed() && launched->second.segment == Economy::traceState().segment
         && ( launched->second.hero != hero.GetID() || !isOpeningSeaExplorer( hero ) ) ) return false;
    auto & army = hero.GetArmy(); auto & garrison = port.GetArmy();
    // Use the navigator's faction, including an available upgraded T2 when
    // the base creature is absent. Missing crew leaves both armies untouched.
    const int low = Monster( hero.GetRace(), DWELLING_MONSTER1 ).GetID();
    const int baseHigh = Monster( hero.GetRace(), DWELLING_MONSTER2 ).GetID();
    if ( low == Monster::UNKNOWN || baseHigh == Monster::UNKNOWN ) return false;
    const auto count = [&]( const int id ) { return uint64_t{ army.GetCountMonsters( id ) } + garrison.GetCountMonsters( id ); };
    int high = baseHigh;
    if ( count( high ) < 2 ) {
        const int upgraded = Monster( baseHigh ).GetUpgrade().GetID();
        if ( upgraded != baseHigh && count( upgraded ) >= 2 ) high = upgraded;
    }
    if ( count( low ) < 3 || count( high ) < 2 ) {
        ECONOMY_TRACE( 1, "NAVAL_OPENING_CREW_WAIT", ",\"hero\":" << hero.GetID() << ",\"town\":" << port.GetIndex()
            << ",\"low\":" << low << ",\"high\":" << high << ",\"low_available\":" << count( low )
            << ",\"high_available\":" << count( high ) << ",\"reason\":\"incomplete_mixed_crew\"" );
        return false;
    }
    std::array<int, 5> formation{ low, high, low, high, low };
    if ( hero.GetRace() == Race::KNGT || hero.GetRace() == Race::BARB ) formation = { high, low, low, low, high };
    const Troops beforeArmy( army.getTroops() ), beforeGarrison( garrison.getTroops() );
    Troops shore( garrison.getTroops() ), surplus( army.getTroops() ), crew( army.getTroops() ); crew.Clean();
    for ( size_t n = 0; n < formation.size(); ++n ) {
        bool taken = false;
        for ( Troops * pool : { &surplus, &shore } ) {
            for ( size_t slot = 0; slot < pool->Size(); ++slot ) {
                auto * troop = pool->GetTroop( slot );
                if ( !troop->isValid() || troop->GetID() != formation[n] ) continue;
                crew.GetTroop( n )->Set( Monster( formation[n] ), 1 );
                if ( troop->GetCount() == 1 ) troop->Reset(); else troop->SetCount( troop->GetCount() - 1 );
                taken = true; break;
            }
            if ( taken ) break;
        }
        if ( !taken ) return false;
    }
    for ( size_t slot = 0; slot < surplus.Size(); ++slot ) {
        const auto * troop = surplus.GetTroop( slot );
        if ( troop->isValid() && !shore.JoinTroop( troop->GetMonster(), troop->GetCount(), false ) ) return false;
    }
    const auto armyBefore = KnightObserver::armyJson( army ), townBefore = KnightObserver::armyJson( garrison );
    army.Assign( crew ); garrison.Assign( shore );
    if ( target >= 0 ) {
        AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( hero );
        if ( !path.getDistance( target ) ) { army.Assign( beforeArmy ); garrison.Assign( beforeGarrison ); return false; }
    }
    const OpeningExplorer plan{ world.GetMapSeed(), world.CountDay(), Economy::traceState().segment,
        static_cast<int>( hero.GetColor() ), hero.GetID(), port.GetIndex(), false };
    if ( !isOpeningSeaExplorer( hero ) ) openingExplorers[hero.GetID()] = plan;
    openingLaunches[static_cast<int>( hero.GetColor() )] = plan;
    ECONOMY_TRACE( 1, "OBSERVER_ARMY_TRANSFER", ",\"operation\":\"opening_sea_explorer\",\"player\":" << static_cast<int>( hero.GetColor() )
        << ",\"hero\":" << hero.GetID() << ",\"town\":" << port.GetIndex() << ",\"crew_low\":" << low << ",\"crew_high\":" << high << ",\"crew_stacks\":5"
        << ",\"source_player\":" << static_cast<int>( hero.GetColor() ) << ",\"destination_player\":" << static_cast<int>( hero.GetColor() )
        << ",\"source_tile\":" << port.GetIndex() << ",\"destination_tile\":" << port.GetIndex() << ",\"source_hero\":true,\"destination_hero\":false"
        << ",\"source_before\":" << armyBefore << ",\"source_after\":" << KnightObserver::armyJson( army )
        << ",\"destination_before\":" << townBefore << ",\"destination_after\":" << KnightObserver::armyJson( garrison ) );
    return true;
}

int AI::KnightConstruction::navalTarget( const Heroes & hero, const bool renew )
{
    const auto it = navalMissions.find( hero.GetID() ); if ( it == navalMissions.end() ) return -1;
    auto & m = it->second;
    const bool cancelled = m.owner != static_cast<int>( hero.GetColor() ) || m.seed != world.GetMapSeed() || m.segment != Economy::traceState().segment || world.CountDay() < m.day
        || ( constructionContext && constructionContext->endangered.count( m.home ) )
        || ( !hero.isShipMaster() && constructionContext && constructionContext->endangered.count( hero.GetIndex() ) )
        || std::none_of( hero.GetKingdom().GetCastles().begin(), hero.GetKingdom().GetCastles().end(), [&]( const Castle * c ) { return c && c->GetIndex() == m.home; } )
        || world.getTile( m.target ).isFog( hero.GetColor() );
    const bool complete = renew || hero.GetIndex() == m.target || world.CountDay() - m.day > 7
        || ( m.object != MP2::OBJ_NONE && world.getTile( m.target ).getMainObjectType( false ) != m.object )
        || ( m.object == MP2::OBJ_NONE && Maps::getFogTileCountToBeRevealed( m.target, hero.GetScoutingDistance(), hero.GetColor() ) == 0 );
    if ( !cancelled && complete && hero.isShipMaster() ) {
        AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( hero );
        const auto targets = seaOptions( hero, hero.GetIndex(), path );
        if ( !targets.empty() ) {
            const int previous = m.target;
            m.target = targets.front(); m.object = world.getTile( m.target ).getMainObjectType( false ); m.day = world.CountDay();
            ECONOMY_TRACE( 1, "NAVAL_STAGE", ",\"hero\":" << hero.GetID() << ",\"previous\":" << previous << ",\"target\":" << m.target
                << ",\"object\":" << static_cast<int>( m.object ) << ",\"reason\":\"continue_at_sea\"" );
            return m.target;
        }
    }
    if ( cancelled || complete ) {
        ECONOMY_TRACE( 1, "NAVAL_MISSION_END", ",\"hero\":" << hero.GetID() << ",\"target\":" << m.target << ",\"tile\":" << hero.GetIndex() );
        navalMissions.erase( it ); return -1;
    }
    return m.target;
}

void AI::KnightConstruction::noteLandObjective( const Heroes & hero, const int target )
{
    if ( !hero.isShipMaster() || target < 0 || world.getTile( target ).isFog( hero.GetColor() ) || navalMissions.count( hero.GetID() ) ) return;
    AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( hero );
    if ( !path.getDistance( target ) || seaOnly( path, target ) ) return;
    navalLandOrders[hero.GetID()] = { hero.GetID(), -1, -1, target, world.getTile( target ).getMainObjectType( false ), world.CountDay(),
        world.GetMapSeed(), Economy::traceState().segment, static_cast<int>( hero.GetColor() ) };
    ECONOMY_TRACE( 1, "NAVAL_LAND_OBJECTIVE", ",\"hero\":" << hero.GetID() << ",\"target\":" << target << ",\"reason\":\"honor_disembarkation_goal\"" );
}

std::vector<int> AI::KnightConstruction::prepareNaval( Kingdom & kingdom )
{
    std::vector<int> changed;
    if ( fheroes2::taxpayerSimulationDepth || !kingdom.isControlAI() ) return changed;
    // Clear completed/stale missions before testing hero or fleet availability.
    for ( const auto * hero : kingdom.GetHeroes() ) if ( hero ) (void)navalTarget( *hero );
    const auto color = kingdom.GetColor();
    for ( const auto * castle : kingdom.GetCastles() ) {
        if ( !castle || !castle->isBuild( BUILD_SHIPYARD ) ) continue;
        const auto & recruits = kingdom.GetRecruits();
        if ( openingRecruit( *castle, recruits.GetHero1(), recruits.GetHero2() ) ) continue;
        const auto mission = navalUse( *castle );
        const char * reason = "no_available_hero_or_known_route";
        bool assigned = false;
        if ( mission.hero >= 0 ) {
            // One explorer per connected KNOWN water area is enough. Pending
            // missions count as well, so two ports cannot fund duplicate scouts.
            std::vector<bool> water( world.getSize(), false );
            std::queue<int> frontier; frontier.push( mission.boat ); water[mission.boat] = true;
            while ( !frontier.empty() ) {
                const int tile = frontier.front(); frontier.pop();
                for ( int dy = -1; dy <= 1; ++dy ) for ( int dx = -1; dx <= 1; ++dx ) {
                    const int x = tile % world.w() + dx, y = tile / world.w() + dy;
                    if ( x < 0 || x >= world.w() || y < 0 || y >= world.h() ) continue;
                    const int next = y * world.w() + x;
                    if ( !water[next] && !world.getTile( next ).isFog( color ) && world.getTile( next ).isWater() ) {
                        water[next] = true; frontier.push( next );
                    }
                }
            }
            bool explorer = false;
            for ( const auto * hero : kingdom.GetHeroes() ) {
                if ( hero && hero->isActive() && hero->isShipMaster() && water[hero->GetIndex()] ) explorer = true;
            }
            for ( const auto & [id, activeMission] : navalMissions ) {
                (void)id;
                if ( activeMission.owner == static_cast<int>( color ) && activeMission.seed == world.GetMapSeed()
                     && activeMission.segment == Economy::traceState().segment && water[activeMission.target] ) explorer = true;
            }
            if ( explorer ) reason = "water_already_has_explorer";
            else if ( mission.reuse ) { assigned = true; reason = "reuse_native_route"; }
            else if ( !castle->AllowBuyBoat( false ) ) reason = "dock_unavailable";
            else if ( !allowsPayment( kingdom, PaymentConditions::BuyBoat(), "boat_purchase" ) ) reason = "shared_budget";
            else if ( castle->BuyBoat() ) { changed.push_back( mission.boat ); assigned = true; reason = "purchase_ready_to_board"; }
            if ( assigned ) {
                if ( auto * sailor = world.GetHeroes( mission.hero ); sailor ) {
                    if ( auto * home = sailor->inCastleMutable(); home && home->GetIndex() == mission.home ) {
                        if ( !prepareOpeningExplorer( *sailor, *home, mission.target ) && home->GetRace() == Race::KNGT ) arrangePopulation( *sailor, *home, -1, true );
                    }
                }
                navalMissions[mission.hero] = mission;
                ECONOMY_TRACE( 1, "NAVAL_MISSION", ",\"player\":" << static_cast<int>( color ) << ",\"town\":" << mission.home
                    << ",\"hero\":" << mission.hero << ",\"boat\":" << mission.boat << ",\"target\":" << mission.target
                    << ",\"object\":" << static_cast<int>( mission.object ) << ",\"reuse\":" << ( mission.reuse ? "true" : "false" ) );
            }
        }
        ECONOMY_TRACE( 1, "NAVAL_REVIEW", ",\"player\":" << static_cast<int>( color ) << ",\"town\":" << castle->GetIndex()
            << ",\"hero\":" << mission.hero << ",\"reason\":" << Economy::jsonQuote( reason ) << ",\"assigned\":" << ( assigned ? "true" : "false" ) );
    }
    return changed;
}

bool AI::KnightConstruction::active( const Kingdom & kingdom )
{ return constructionContext && constructionContext->kingdom == &kingdom; }
