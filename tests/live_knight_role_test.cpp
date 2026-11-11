// Actual Farm/TG decision adapter. Construction ordering has its own integration cases.
// Link without ai_planner_castle.o and fheroes2.o.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "map_format_info.h"
#include "players.h"
#include "settings.h"
#include "difficulty.h"
#include <iostream>

int main( int argc, char ** argv )
{
    assert( argc == 3 ); const std::string mode = argv[2]; Settings::Get().Read( argv[1] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL ); Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    world.generateUninitializedMap( 36 ); world.NewDay();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    kingdom.OddFundsResource( kingdom.GetFunds() );
    kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, mode == "poor" ? 3000 : 20000 ) );
    constexpr int tile = 150; world.addCastle( tile, static_cast<uint8_t>( Race::KNGT ), PlayerColor::BLUE );
    auto * castle = world.getCastle( { tile % 36, tile / 36 } ); assert( castle ); kingdom.GetCastles().push_back( castle );
    world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
    Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
    meta.builtBuildings = { BUILD_CASTLE, BUILD_WELL, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4,
                           ( mode == "recover" || mode == "expensive" ) ? BUILD_TRAINING_GROUNDS : BUILD_WEL2 };
    if ( mode != "prerequisite" ) meta.builtBuildings.push_back( BUILD_CAPTAIN );
    castle->loadFromResurrectionMap( meta ); castle->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
    castle->GetArmy().Clean(); assert( castle->GetArmy().JoinTroop( Monster::PEASANT, ( mode == "recover" || mode == "expensive" ) ? 100 : 20, false ) );
    if ( mode == "weekday" ) world.NewDay();
    if ( mode == "mobile" ) {
        auto * h = world.GetHeroes( Heroes::LORDKILBURN ); h->SetColor( PlayerColor::BLUE ); h->SetCenter( castle->GetCenter() );
        kingdom.GetHeroes().push_back( h ); assert( castle->GetHero() == h );
    }
    const auto before = kingdom.GetFunds(); const auto armyBefore = castle->conscriptionDaySnapshot();
    const auto decision = AI::KnightEconomy::choose( *castle, mode != "danger" );
    assert( kingdom.GetFunds() == before && castle->conscriptionDaySnapshot().garrison == armyBefore.garrison );
    std::cout << "CHOICE mode=" << mode << " reason=" << decision.reason << " building=" << decision.building
        << " force=" << decision.keepForce << "/" << decision.switchForce << " gold=" << decision.keepGold << "/" << decision.switchGold
        << " projections=" << decision.projections << std::endl;
    std::cout << "LIVE_BEGIN" << std::endl;
    if ( mode == "rich" || mode == "mobile" || mode == "recover" || mode == "expensive" || mode == "custom_cost" ) {
        const auto expected = ( mode == "rich" || mode == "mobile" || mode == "custom_cost" ) ? BUILD_TRAINING_GROUNDS : BUILD_WEL2;
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
