// Real production construction code; link without ai_planner_castle.o/fheroes2.o.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "map_format_info.h"
#include "players.h"
#include <iostream>

int main( int argc, char ** argv )
{
    assert( argc == 2 || argc == 3 ); const std::string mode = argv[1];
    if ( argc == 3 ) Settings::Get().Read( argv[2] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    world.generateUninitializedMap( 36 ); world.NewDay();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    kingdom.OddFundsResource( kingdom.GetFunds() );
    const int gold = mode == "castle_income" ? 6000 : mode == "futurewood" ? 5000 : mode == "farm_1000" ? 1000 : mode == "wait" ? 2500 : mode == "shared" ? 1500 : 20000;
    const int wood = mode == "castle_income" ? 40 : mode == "futurewood" ? 19 : mode == "farm_1000" ? 0 : mode == "wait" || mode == "shared" ? 20 : 100;
    Funds starting; starting.gold = gold; starting.wood = wood; starting.ore = 100; starting.mercury = 100; starting.sulfur = 100; starting.gems = 100;
    kingdom.AddFundsResource( starting );
    const auto add = [&]( int tile ) {
        world.addCastle( tile, static_cast<uint8_t>( Race::KNGT ), PlayerColor::BLUE );
        Castle * castle = world.getCastle( { tile % 36, tile / 36 } ); assert( castle );
        kingdom.GetCastles().push_back( castle ); world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
        Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
        meta.builtBuildings = {};
        if ( mode != "castle_income" || tile != 250 ) meta.builtBuildings.push_back( BUILD_CASTLE );
        if ( mode != "hut" ) meta.builtBuildings.push_back( DWELLING_MONSTER1 );
        if ( mode != "statue" ) meta.builtBuildings.push_back( BUILD_STATUE );
        if ( mode != "hut" && mode != "statue" && mode != "archer" ) meta.builtBuildings.push_back( DWELLING_MONSTER2 );
        if ( mode != "hut" && mode != "statue" && mode != "archer" && mode != "well" ) {
            for ( const auto b : { BUILD_WELL, BUILD_TAVERN, DWELLING_MONSTER3, DWELLING_MONSTER4 } ) meta.builtBuildings.push_back( b );
            if ( mode == "growth" ) {
                meta.builtBuildings.push_back( DWELLING_MONSTER5 ); meta.builtBuildings.push_back( DWELLING_MONSTER6 );
            }
        }
        castle->loadFromResurrectionMap( meta ); castle->SetModes( Castle::ALLOW_TO_BUILD_TODAY ); castle->GetArmy().Clean();
        return castle;
    };
    Castle * first = add( 150 ); Castle * second = ( mode == "shared" || mode == "castle_income" ) ? add( 250 ) : nullptr;
    if ( mode == "futurewood" ) {
        world.getTile( 350 ).setMainObjectType( MP2::OBJ_SAWMILL ); world.CaptureObject( 350, PlayerColor::BLUE );
        assert( kingdom.GetIncome( Kingdom::INCOME_ALL ).wood == 2 );
    }
    const auto before = kingdom.GetFunds();
    {
        AI::KnightConstruction::Scope budget( kingdom );
        if ( mode == "castle_income" ) {
            assert( !second->isCastle() );
            assert( AI::KnightConstruction::canSpend( *second, BUILD_CASTLE ) );
            assert( second->BuyBuilding( BUILD_CASTLE ) );
            const auto tomorrow = tomorrowFunds( kingdom ); assert( tomorrow && ( *tomorrow )[0] == 3500 );
            assert( constructionContext->ledger.reserved()[0] == 3000 );
        }
        else if ( mode == "shared" ) {
            assert( knightPrimaryDevelopment( *first ) == PrimaryResult::Fallback );
            assert( kingdom.GetFunds() == before );
            assert( knightPrimaryDevelopment( *second ) == PrimaryResult::Built );
            assert( second->isBuild( BUILD_WEL2 ) && !first->isBuild( BUILD_WEL2 ) );
            const auto tomorrow = tomorrowFunds( kingdom ); assert( tomorrow );
            assert( ( *tomorrow )[0] >= 3000 && ( *tomorrow )[1] >= 20 );
            assert( constructionContext->ledger.promises.size() == 1 );
        }
        else if ( mode == "wait" ) {
            assert( knightPrimaryDevelopment( *first ) == PrimaryResult::Fallback ); assert( kingdom.GetFunds() == before );
            // Farm would delay the dwelling. A cheaper moat preserves its funds.
            assert( AI::KnightConstruction::canSpend( *first, BUILD_MOAT ) );
            assert( Build( *first, defensiveStructures ) && first->isBuild( BUILD_MOAT ) );
            const auto tomorrow = tomorrowFunds( kingdom ); assert( tomorrow && ( *tomorrow )[0] >= 3000 );
        }
        else {
            const auto expected = mode == "statue" ? BUILD_STATUE : mode == "hut" ? DWELLING_MONSTER1 : mode == "archer" ? DWELLING_MONSTER2 : mode == "well" ? BUILD_WELL : BUILD_WEL2;
            assert( knightPrimaryDevelopment( *first ) == PrimaryResult::Built ); assert( first->isBuild( expected ) );
            assert( kingdom.GetFunds() == before - first->getBuildingCost( expected ) );
            assert( !first->Modes( Castle::ALLOW_TO_BUILD_TODAY ) );
            assert( !first->isBuild( BUILD_CAPTAIN ) );
            if ( mode == "futurewood" ) {
                const auto tomorrow = tomorrowFunds( kingdom ); assert( tomorrow && ( *tomorrow )[1] == 21 );
                assert( constructionContext->ledger.promises.at( 150 ).building == DWELLING_MONSTER5 );
            }
        }
    }
    assert( !constructionContext );
    std::cout << "PASS real construction " << mode << " gold=" << kingdom.GetFunds().gold << '\n';
    kingdom.GetCastles().clear();
}
