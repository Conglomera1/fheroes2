// Actual Farm/TG decision adapter. Construction ordering has its own integration cases.
// Link without ai_planner_castle.o and fheroes2.o.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "map_format_info.h"
#include "players.h"
#include "settings.h"
#include "difficulty.h"
#include <iostream>

namespace AI
{
    struct KnightPolicyTestAccess
    {
        static void reinforce( Castle & castle, const bool urgent, const bool lastStand )
        {
            auto & planner = Planner::Get();
            planner._regions.resize( world.getSize() );
            planner._regions[world.getTile( castle.GetIndex() ).GetRegion()].safetyFactor = 117;
            if ( urgent ) {
                constexpr int enemyTile = 149;
                planner._priorityTargets.emplace( castle.GetIndex(), PriorityTask( PriorityTaskType::DEFEND, enemyTile ) );
                const double threat = castle.GetGarrisonStrength( nullptr ) * ( lastStand ? 2.0 : 0.5 );
                planner._enemyArmies.emplace( enemyTile, EnemyArmy( enemyTile, nullptr, threat, 1000 ) );
            }
            KnightConstruction::Scope scope( castle.GetKingdom() );
            KnightConstruction::setRoles( castle.GetKingdom(), {}, urgent ? std::vector<int>{ castle.GetIndex() } : std::vector<int>{} );
            planner.reinforceCastle( castle );
        }
    };
}

int main( int argc, char ** argv )
{
    assert( argc == 3 ); const std::string mode = argv[2]; Settings::Get().Read( argv[1] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL ); Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    world.generateUninitializedMap( 36 ); world.NewDay();
    const bool checkReinforcement = mode == "training_reinforcement" || mode == "training_last_slot" || mode == "training_quiet";
    if ( checkReinforcement ) {
        while ( world.CountDay() < 138 ) {
            world.NewDay();
        }
    }
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    kingdom.OddFundsResource( kingdom.GetFunds() );
    kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, mode == "poor" ? 3000 : 20000 ) );
    constexpr int tile = 150; world.addCastle( tile, static_cast<uint8_t>( Race::KNGT ), PlayerColor::BLUE );
    auto * castle = world.getCastle( { tile % 36, tile / 36 } ); assert( castle ); kingdom.GetCastles().push_back( castle );
    world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
    if ( checkReinforcement ) {
        assert( Conscription::settings().mode == 2 );
        Players::Get( PlayerColor::BLUE )->SetControl( CONTROL_AI );

        // Timberhill J138 from the autoplay log, reconstructed without a saved game.
        const bool reconstructedGarrison = mode == "training_reinforcement";
        const bool urgent = mode != "training_quiet";
        Maps::Map_Format::CastleMetadata training;
        training.customBuildings = true;
        training.builtBuildings = { BUILD_CASTLE, BUILD_CAPTAIN, BUILD_WELL, BUILD_TRAINING_GROUNDS, DWELLING_MONSTER1, DWELLING_MONSTER2,
                                   DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6, DWELLING_UPGRADE2, DWELLING_UPGRADE3,
                                   DWELLING_UPGRADE4, DWELLING_UPGRADE5, DWELLING_UPGRADE6 };
        if ( !reconstructedGarrison ) {
            training.builtBuildings = { BUILD_CASTLE, BUILD_CAPTAIN, BUILD_WELL, BUILD_TRAINING_GROUNDS, DWELLING_MONSTER4, DWELLING_UPGRADE4 };
        }
        castle->loadFromResurrectionMap( training );
        castle->ActionNewWeek();
        castle->ActionNewWeek();

        const std::array<int, 6> troops{ Monster::PEASANT, Monster::RANGER, Monster::VETERAN_PIKEMAN, Monster::MASTER_SWORDSMAN, Monster::CHAMPION,
                                         Monster::CRUSADER };
        const std::array<uint32_t, 6> stocks = reconstructedGarrison ? std::array<uint32_t, 6>{ 36, 20, 14, 12, 10, 8 }
                                                                    : std::array<uint32_t, 6>{ 0, 0, 0, 12, 0, 0 };
        auto & garrison = castle->GetArmy();
        for ( size_t dwelling = 0; dwelling < troops.size(); ++dwelling ) {
            const Monster unit( troops[dwelling] );
            const auto available = castle->getMonstersInDwelling( unit.GetDwelling() );
            assert( available >= stocks[dwelling] );
            if ( available > stocks[dwelling] ) {
                garrison.Clean();
                assert( castle->RecruitMonster( Troop( unit, available - stocks[dwelling] ), false ) );
            }
        }
        kingdom.OddFundsResource( kingdom.GetFunds() );
        kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 41147 ) );
        garrison.Clean();
        assert( garrison.JoinTroop( Monster::RANGER, reconstructedGarrison ? 20 : 100, false ) );
        assert( garrison.JoinTroop( Monster::EARTH_ELEMENT, reconstructedGarrison ? 2 : 200, false ) );
        assert( garrison.JoinTroop( Monster::PEASANT, 77, false ) );
        if ( !reconstructedGarrison ) {
            assert( garrison.JoinTroop( Monster::BLACK_DRAGON, 20, false ) );
            assert( garrison.JoinTroop( Monster::TITAN, 20, false ) );
        }

        auto * atlas = world.GetHeroes( Heroes::ATLAS );
        assert( atlas->Recruit( *castle ) );
        auto & visitingArmy = atlas->GetArmy();
        visitingArmy.Clean();
        assert( visitingArmy.JoinTroop( Monster::WAR_TROLL, 40, false ) );
        assert( visitingArmy.JoinTroop( Monster::ROYAL_MUMMY, 86, false ) );
        assert( visitingArmy.JoinTroop( Monster::ORC_CHIEF, 138, false ) );
        assert( visitingArmy.JoinTroop( Monster::POWER_LICH, 31, false ) );
        assert( visitingArmy.JoinTroop( Monster::OGRE_LORD, 60, false ) );
        const auto combinedCount = [&]( const int monster ) {
            return garrison.GetCountMonsters( monster ) + visitingArmy.GetCountMonsters( monster );
        };

        AI::KnightPolicyTestAccess::reinforce( *castle, urgent, mode == "training_last_slot" );
        std::cout << "REINFORCEMENT case=" << mode << " garrison_peasants=" << garrison.GetCountMonsters( Monster::PEASANT )
                  << " master_swordsmen=" << combinedCount( Monster::MASTER_SWORDSMAN ) << std::endl;
        assert( combinedCount( Monster::MASTER_SWORDSMAN ) == ( urgent ? 12 : 0 ) );
        if ( reconstructedGarrison ) {
            const auto before = castle->conscriptionDaySnapshot();
            assert( before.garrison >= 77 && combinedCount( Monster::PEASANT ) + before.hut == 113 );
            assert( castle->getMonstersInDwelling( DWELLING_UPGRADE3 ) == 14 );
            castle->setConscriptionOrder( 22 );
            world.NewDay();
            kingdom.ActionNewDayResourceUpdate( {} );
            const auto next = castle->conscriptionDaySnapshot();
            assert( next.garrison == before.garrison - 22 && next.hut == before.hut && next.state.total == 22 );
        }
        else assert( combinedCount( Monster::PEASANT ) == ( urgent ? 0 : 77 ) );
        kingdom.GetHeroes().clear();
        kingdom.GetCastles().clear();
        return 0;
    }
    Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
    const bool alreadyTraining = mode == "recover" || mode == "expensive" || mode == "idle_training";
    meta.builtBuildings = { BUILD_CASTLE, BUILD_WELL, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4,
                           alreadyTraining ? BUILD_TRAINING_GROUNDS : BUILD_WEL2 };
    if ( mode == "stocked" ) meta.availableToHireMonsterCount = { 0, 10, 7, 6, 0, 0 };
    if ( mode == "priority_recruitment" || mode == "idle_training" ) meta.availableToHireMonsterCount = { 0, 30, 30, 30, 0, 0 };
    if ( mode != "prerequisite" ) meta.builtBuildings.push_back( BUILD_CAPTAIN );
    castle->loadFromResurrectionMap( meta ); castle->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
    castle->GetArmy().Clean(); assert( castle->GetArmy().JoinTroop( Monster::PEASANT, alreadyTraining ? 100 : 20, false ) );
    if ( mode == "priority_recruitment" ) {
        assert( Conscription::settings().mode == 2 );
        Players::Get( PlayerColor::BLUE )->SetControl( CONTROL_AI );
        constexpr int trainingTile = 170;
        world.addCastle( trainingTile, static_cast<uint8_t>( Race::KNGT ), PlayerColor::BLUE );
        auto * training = world.getCastle( { trainingTile % 36, trainingTile / 36 } );
        assert( training );
        kingdom.GetCastles().push_back( training );
        world.getTile( trainingTile ).setMainObjectType( MP2::OBJ_CASTLE );
        meta.builtBuildings.back() = BUILD_STATUE;
        std::replace( meta.builtBuildings.begin(), meta.builtBuildings.end(), BUILD_WEL2, BUILD_TRAINING_GROUNDS );
        meta.availableToHireMonsterCount = {};
        training->loadFromResurrectionMap( meta );
        training->GetArmy().Clean();
        assert( training->GetArmy().JoinTroop( Monster::PEASANT, 20, false ) );
        kingdom.OddFundsResource( kingdom.GetFunds() );
        kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 5500 ) );
        const auto funds = kingdom.GetFunds();
        {
            AI::KnightConstruction::Scope scope( kingdom );
            AI::KnightConstruction::setRoles( kingdom, {}, { tile } );
            assert( AI::KnightConstruction::reinforcementNeed( *castle ) == AI::KnightConstruction::ReinforcementNeed::Defense );
            AI::KnightEconomy::planDailyOrders( kingdom );
            assert( training->getConscriptionOrder() == 0 );
            AI::KnightConstruction::setRoles( kingdom, {}, {} );
            assert( AI::KnightConstruction::reinforcementNeed( *castle ) == AI::KnightConstruction::ReinforcementNeed::None );
            AI::KnightEconomy::planDailyOrders( kingdom );
            assert( training->getConscriptionOrder() > 0 );
        }
        assert( kingdom.GetFunds() == funds && training->GetArmy().GetCountMonsters( Monster::PEASANT ) == 20 );
        const auto order = training->getConscriptionOrder();
        world.NewDay();
        kingdom.ActionNewDayResourceUpdate( {} );
        assert( training->GetArmy().GetCountMonsters( Monster::PEASANT ) == 20 - order );
        std::cout << "RESULT priority_recruitment defense=0 quiet=" << order << " executed=" << order << '\n';
        kingdom.GetCastles().clear();
        return 0;
    }
    if ( mode == "weekday" ) world.NewDay();
    if ( mode == "mobile" ) {
        auto * h = world.GetHeroes( Heroes::LORDKILBURN ); h->SetColor( PlayerColor::BLUE ); h->SetCenter( castle->GetCenter() );
        kingdom.GetHeroes().push_back( h ); assert( castle->GetHero() == h );
    }
    const auto before = kingdom.GetFunds(); const auto armyBefore = castle->conscriptionDaySnapshot();
    const auto decision = AI::KnightEconomy::choose( *castle, mode != "danger" );
    assert( kingdom.GetFunds() == before && castle->conscriptionDaySnapshot().garrison == armyBefore.garrison );
    if ( mode == "stocked" ) {
        assert( decision.comparisonMode == 1 && decision.comparisonWeekTrained == 17 );
        assert( decision.projections > 0 && decision.keepValid && decision.switchValid );
        assert( castle->getConscriptionOrder() == 0 );
    }
    std::cout << "CHOICE mode=" << mode << " reason=" << decision.reason << " building=" << decision.building
        << " force=" << decision.keepForce << "/" << decision.switchForce << " gold=" << decision.keepGold << "/" << decision.switchGold
        << " projections=" << decision.projections << std::endl;
    std::cout << "LIVE_BEGIN" << std::endl;
    if ( mode == "rich" || mode == "mobile" || alreadyTraining || mode == "custom_cost" || ( mode == "stocked" && decision.building ) ) {
        const auto expected = alreadyTraining ? BUILD_WEL2 : BUILD_TRAINING_GROUNDS;
        assert( decision.building == expected ); const auto price = castle->getBuildingCost( expected );
        assert( AI::KnightEconomy::develop( *castle, true ) ); // Real production role adapter, diagnostics may be zero.
        assert( castle->isBuild( expected ) && !castle->isBuild( expected == BUILD_TRAINING_GROUNDS ? BUILD_WEL2 : BUILD_TRAINING_GROUNDS ) );
        assert( kingdom.GetFunds() == before - price && !castle->Modes( Castle::ALLOW_TO_BUILD_TODAY ) );
        assert( castle->conscriptionDaySnapshot().garrison == armyBefore.garrison && castle->conscriptionDaySnapshot().pools == armyBefore.pools );
        const auto after = kingdom.GetFunds(); assert( !AI::KnightEconomy::develop( *castle, true ) && kingdom.GetFunds() == after );
    }
    else {
        assert( !decision.building ); assert( !AI::KnightEconomy::develop( *castle, mode != "danger" ) && kingdom.GetFunds() == before );
        assert( castle->isBuild( BUILD_WEL2 ) && !castle->hasTrainingGrounds() );
    }
    std::cout << "LIVE_END\nRESULT mode=" << mode << " farm=" << castle->isBuild( BUILD_WEL2 ) << " tg=" << castle->hasTrainingGrounds()
        << " paid=" << before.gold - kingdom.GetFunds().gold << " projections=" << decision.projections << '\n';
    kingdom.GetHeroes().clear(); kingdom.GetCastles().clear();
}
