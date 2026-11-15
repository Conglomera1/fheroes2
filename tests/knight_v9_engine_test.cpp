// Multi-turn maritime continuity, funded mobilisation and atomic construction.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "ai_hero_action.h"
#include "map_format_info.h"
#include "maps_tiles_helper.h"
#include "players.h"
#include "rand.h"
#include <iostream>

int main( int argc, char ** argv )
{
    assert( argc == 3 ); const std::string mode = argv[1]; Settings::Get().Read( argv[2] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL ); Settings::Get().SetGameType( Game::TYPE_HOTSEAT ); Settings::Get().SetAIMoveSpeed( 0 );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    for ( const auto color : PlayerColorsVector( Color::allPlayerColors() ) ) Players::Get( color )->SetControl( CONTROL_AI );
    Rand::CurrentThreadRandomDevice() = Rand::PCG32( 20261114 );
    world.generateUninitializedMap( 36 ); world.NewDay();
    for ( int tile = 0; tile < static_cast<int>( world.getSize() ); ++tile ) world.getTile( tile ).setIndex( tile );
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    const auto money = [&]( int gold, int wood = 100, int ore = 100 ) {
        kingdom.OddFundsResource( kingdom.GetFunds() ); Funds f; f.gold = gold; f.wood = wood; f.ore = ore;
        f.mercury = f.sulfur = f.crystal = f.gems = 20; kingdom.AddFundsResource( f );
    };
    money( 50000 );
    const auto add = [&]( int tile, const std::vector<int32_t> & buildings, int race = Race::KNGT ) {
        world.addCastle( tile, static_cast<uint8_t>( race ), PlayerColor::BLUE );
        auto * c = world.getCastle( { tile % 36, tile / 36 } ); assert( c ); kingdom.GetCastles().push_back( c );
        world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
        Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
        for ( const auto building : buildings ) meta.builtBuildings.push_back( building );
        c->loadFromResurrectionMap( meta ); c->GetArmy().Clean(); c->SetModes( Castle::ALLOW_TO_BUILD_TODAY ); return c;
    };
    const std::vector<int32_t> complete{ BUILD_CASTLE, BUILD_CAPTAIN, BUILD_STATUE, BUILD_MARKETPLACE, BUILD_WELL, BUILD_TAVERN,
        DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6, BUILD_WEL2 };
    if ( mode == "voyage" || mode == "land_goal" || mode == "sea_defense" || mode == "reward_first" || mode == "sea_only" ) {
        for ( int tile = 0; tile < static_cast<int>( world.getSize() ); ++tile ) {
            world.getTile( tile ).setTerrain( tile / 36 >= 6 ? 0 : Maps::Ground::GRASS_START_IMAGE_INDEX, 0 );
            if ( mode != "reward_first" || tile != 332 ) world.getTile( tile ).ClearFog( static_cast<PlayerColorsSet>( PlayerColor::BLUE ) );
        }
        auto buildings = complete; buildings.push_back( BUILD_SHIPYARD ); auto * c = add( 150, buildings );
        world.getTile( 226 ).setMainObjectType( MP2::OBJ_FLOTSAM );
        world.getTile( 442 ).setMainObjectType( MP2::OBJ_FLOTSAM );
        for ( const int tile : { 226, 442 } ) {
            world.getTile( tile ).getMainObjectPart() = Maps::ObjectPart( Maps::OBJECT_LAYER, 10000 + tile, MP2::OBJ_ICN_TYPE_OBJNWATR, 45 );
            world.getTile( tile ).metadata() = { 5, 200, 0 };
        }
        world.updatePassabilities();
        auto * hero = world.GetHeroes( Heroes::LORDKILBURN ); assert( hero && hero->Recruit( *c ) );
        auto * mainland = world.GetHeroes( Heroes::SIRGALLANTH ); assert( mainland && mainland->Recruit( PlayerColor::BLUE, fheroes2::Point( 8, 4 ) ) );
        mainland->SetModes( Heroes::PATROL );
        KC::noteOpeningRecruit( *hero, *c );
        KC::Scope budget( kingdom );
        if ( mode == "reward_first" ) {
            assert( world.getTile( 332 ).isFog( PlayerColor::BLUE ) );
            AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( 222, PlayerColor::BLUE, hero->GetArmy().GetStrength(), Skill::Level::BASIC );
            assert( path.getDistance( 296 ) < path.getDistance( 226 ) );
            assert( Maps::getFogTileCountToBeRevealed( 296, hero->GetScoutingDistance(), PlayerColor::BLUE ) > 0 );
        }
        assert( KC::prepareNaval( kingdom ).size() == 1 );
        assert( navalMissions.at( hero->GetID() ).target == 226 );
        AIWorldPathfinder first; first.reset(); first.reEvaluateIfNeeded( *hero );
        for ( const auto & step : first.buildPath( 226, false ) ) {
            if ( world.getTile( step.GetIndex() ).getMainObjectType( false ) == MP2::OBJ_BOAT ) { AI::HeroesAction( *hero, step.GetIndex() ); break; }
            hero->Move2Dest( step.GetIndex() );
        }
        assert( hero->isShipMaster() && world.CountDay() == 1 );
        if ( mode == "sea_defense" ) {
            KC::setRoles( kingdom, {}, { 150 } ); assert( KC::navalTarget( *hero ) == -1 );
        }
        else if ( mode == "reward_first" ) {
            // A visible reward must remain ahead of empty exploration points.
            const int target = KC::navalTarget( *hero ); assert( target == 226 );
        }
        else if ( mode == "sea_only" ) {
            AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( *hero );
            assert( seaOnly( path, 226 ) && !seaOnly( path, 150 ) );
        }
        else {
            world.NewDay(); kingdom.ActionNewDayResourceUpdate( nullptr );
            // Execute the real pickup while adjacent, as native movement does.
            hero->Move2Dest( 225 ); AI::HeroesAction( *hero, 226 );
            const int second = KC::navalTarget( *hero );
            assert( second == 442 && hero->isShipMaster() );
            assert( !KC::prepareNaval( kingdom ).size() );
            world.NewDay(); kingdom.ActionNewDayResourceUpdate( nullptr );
            hero->Move2Dest( 441 ); AI::HeroesAction( *hero, 442 );
            assert( KC::navalTarget( *hero ) == -1 && hero->isShipMaster() );
            if ( mode == "land_goal" ) {
                world.getTile( 189 ).setMainObjectType( MP2::OBJ_CAMPFIRE );
                KC::noteLandObjective( *hero, 189 ); assert( pendingLandOrder( *hero ) );
                hero->Move2Dest( 222 ); AI::HeroesAction( *hero, 186 ); assert( !hero->isShipMaster() );
                world.getTile( 226 ).setMainObjectType( MP2::OBJ_FLOTSAM );
                world.NewDay(); kingdom.ActionNewDayResourceUpdate( nullptr );
                const auto cash = kingdom.GetFunds();
                assert( KC::prepareNaval( kingdom ).empty() && navalMissions.empty() && kingdom.GetFunds() == cash );
                // Once the actual land objective is completed, a new voyage is allowed.
                hero->Move2Dest( 189 ); world.getTile( 189 ).setMainObjectType( MP2::OBJ_NONE );
                assert( !pendingLandOrder( *hero ) );
            }
        }
        kingdom.GetHeroes().clear();
    }
    else if ( mode == "direct_secondary" ) {
        auto buildings = complete; buildings.back() = BUILD_TRAINING_GROUNDS;
        auto * army = add( 150, buildings ); assert( army->GetArmy().JoinTroop( Monster::PEASANT, 250, false ) );
        auto * guild = add( 450, { BUILD_CASTLE, BUILD_MARKETPLACE }, Race::SORC );
        money( 6000 ); KC::Scope budget( kingdom ); const auto before = kingdom.GetFunds();
        assert( guild->AllowBuyBuilding( BUILD_MAGEGUILD1 ) );
        assert( !AI::BuildIfPossible( *guild, BUILD_MAGEGUILD1 ) && kingdom.GetFunds() == before );
    }
    else if ( mode == "trade_reject" || mode == "trade_allow" || mode == "trade_emergency" || mode == "trade_log" ) {
        auto buildings = complete; buildings.back() = BUILD_TRAINING_GROUNDS;
        auto * army = add( 150, buildings ); assert( army->GetArmy().JoinTroop( Monster::PEASANT, 250, false ) );
        auto * guild = add( 450, { BUILD_CASTLE, BUILD_MARKETPLACE }, Race::SORC );
        for ( int n = 0; n < ( mode == "trade_log" ? 1 : 7 ); ++n )
            add( 800 + ( n % 4 ) * 6 + ( n / 4 ) * 216, { BUILD_CASTLE, BUILD_MARKETPLACE }, Race::SORC );
        money( mode == "trade_allow" ? 50000 : mode == "trade_log" ? 12518 : 14899, 1, 0 );
        Funds set = kingdom.GetFunds();
        set.mercury = mode == "trade_log" ? 10 : 0; set.sulfur = mode == "trade_log" ? 4 : 0;
        set.crystal = mode == "trade_log" ? 5 : 0; set.gems = mode == "trade_log" ? 4 : 0;
        kingdom.OddFundsResource( kingdom.GetFunds() ); kingdom.AddFundsResource( set );
        KC::Scope budget( kingdom );
        const auto before = kingdom.GetFunds();
        const auto exchange = AI::calculateMarketplaceTransaction( kingdom, guild->getBuildingCost( BUILD_MAGEGUILD1 ) ); assert( exchange );
        const auto total = *exchange + guild->getBuildingCost( BUILD_MAGEGUILD1 );
        if ( mode == "trade_reject" ) assert( KC::allowsPayment( kingdom, *exchange, "test_exchange_alone" ) );
        if ( mode == "trade_log" ) assert( total.gold == 10750 && total.mercury == 6 );
        std::cout << "TRADE total_gold=" << total.gold << " final_gold=" << before.gold - total.gold << '\n';
        const KC::Emergency emergency( mode == "trade_emergency" );
        const bool built = AI::BuildIfPossible( *guild, BUILD_MAGEGUILD1 );
        assert( built == ( mode == "trade_allow" || mode == "trade_emergency" ) );
        if ( !built ) assert( kingdom.GetFunds() == before && !guild->isBuild( BUILD_MAGEGUILD1 ) );
        else assert( kingdom.GetFunds() == before - total );
    }
    else {
        auto * c = add( 150, complete ); assert( c->GetArmy().JoinTroop( Monster::PEASANT, 20, false ) );
        if ( mode == "small_gain" ) {
            for ( int n = 1; n < 11; ++n ) add( 150 + 6 * ( n % 4 ) + 216 * ( n / 4 ), complete );
            assert( c->GetArmy().JoinTroop( Monster::ARCHER, 200, false ) );
        }
        money( mode == "entry_funding" ? 10000 : mode == "small_gain" ? 16463 : 20000 );
        KC::Scope budget( kingdom );
        const auto week = AI::KnightEconomy::weekFunding( *c ); assert( week.complete && week.trained == 17 && week.fees[0] == 1700 );
        const auto before = kingdom.GetFunds();
        const auto decision = AI::KnightEconomy::choose( *c, true );
        std::cout << "DECISION " << decision.reason << " force=" << decision.keepForce << "/" << decision.switchForce << " gold=" << decision.keepGold << "/" << decision.switchGold << '\n';
        if ( mode == "small_gain" ) { assert( !decision.building ); assert( kingdom.GetFunds() == before ); }
        else if ( mode == "entry_funding" ) { assert( !decision.building && decision.reason == "mobilization_funding" ); }
        else if ( mode == "sustained" ) {
            assert( AI::KnightEconomy::develop( *c, true ) && c->hasTrainingGrounds() );
            for ( int day = 0; day < 14; ++day ) {
                world.NewDay(); kingdom.ActionNewDayResourceUpdate( nullptr );
                (void)AI::KnightEconomy::develop( *c, true );
                if ( day < 13 ) assert( c->hasTrainingGrounds() );
                assert( kingdom.GetFunds().gold >= 3500 );
            }
            assert( c->conscriptionDaySnapshot().state.total > 17 );
        }
        else if ( mode == "recovery_reentry" ) {
            assert( AI::KnightEconomy::develop( *c, true ) && c->hasTrainingGrounds() );
            while ( world.CountDay() < 5 ) { world.NewDay(); kingdom.ActionNewDayResourceUpdate( nullptr ); }
            assert( c->conscriptionDaySnapshot().state.total == 10 );
            money( 2124 );
            assert( AI::KnightEconomy::develop( *c, true ) && c->isBuild( BUILD_WEL2 ) );
            while ( world.CountDay() < 8 ) world.NewDay();
            money( 30000 ); KC::refresh();
            assert( !AI::KnightEconomy::develop( *c, true ) && !c->hasTrainingGrounds() );
            assert( AI::KnightEconomy::choose( *c, true ).reason == "recovering_mobilization" );
        }
        else assert( false );
    }
    std::cout << "PASS v9 " << mode << " day=" << world.CountDay() << " gold=" << kingdom.GetFunds().gold << '\n';
    kingdom.GetCastles().clear();
}
