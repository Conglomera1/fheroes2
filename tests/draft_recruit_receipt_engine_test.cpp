// C3D5F DRAFT: not compiled/run yet. Characterize the real API before wiring receipts.
#include "castle.h"
#include "kingdom.h"
#include "army_troop.h"
#include "difficulty.h"
#include "map_format_info.h"
#include "maps_tiles.h"
#include "players.h"
#include "settings.h"
#include "world.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

int main( int argc, char ** argv )
{
    assert( argc == 3 );
    const std::string mode = argv[2];
    assert( mode == "raw_exact" || mode == "raw_overstock" || mode == "guarded_overstock" );
    Settings::Get().Read( argv[1] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    world.generateUninitializedMap( 36 );
    world.NewDay();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 100000 ) );
    constexpr int tile = 150;
    world.addCastle( tile, static_cast<uint8_t>( Race::KNGT ), PlayerColor::BLUE );
    auto * town = world.getCastle( { tile % 36, tile / 36 } );
    assert( town );
    kingdom.GetCastles().push_back( town );
    world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
    Maps::Map_Format::CastleMetadata meta;
    meta.customBuildings = true;
    meta.builtBuildings = { BUILD_CASTLE, DWELLING_MONSTER1 };
    town->loadFromResurrectionMap( meta );
    town->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
    assert( town->BuyBuilding( DWELLING_MONSTER2 ) ); // Seed real stock through the normal API.
    assert( !town->GetHero() );
    town->GetArmy().Reset();
    assert( town->GetArmy().JoinTroop( Monster::PEASANT, 1, false ) );
    const uint32_t stockBefore = town->getMonstersInDwelling( DWELLING_MONSTER2 );
    assert( stockBefore > 0 && stockBefore < std::numeric_limits<uint32_t>::max() );
    const uint32_t requested = stockBefore + ( mode == "raw_exact" ? 0U : 1U );
    const Troop request( Monster::ARCHER, requested );
    const auto moneyBefore = kingdom.GetFunds();
    const uint32_t unitsBefore = town->GetArmy().GetCountMonsters( Monster::ARCHER );
    const bool guarded = mode == "guarded_overstock";
    // TEST-LOCAL demonstration only, not a production executor or a receipt authorization.
    const bool invoke = !guarded || requested <= stockBefore;
    const bool accepted = invoke && town->RecruitMonster( request, false );
    const uint32_t stockAfter = town->getMonstersInDwelling( DWELLING_MONSTER2 );
    const uint32_t unitsAfter = town->GetArmy().GetCountMonsters( Monster::ARCHER );
    const auto moneyAfter = kingdom.GetFunds();
    const int64_t added = int64_t{unitsAfter} - unitsBefore;
    const int64_t removed = int64_t{stockBefore} - stockAfter;
    const bool unchanged = moneyAfter == moneyBefore && unitsAfter == unitsBefore && stockAfter == stockBefore;
    const bool exact = added == requested && removed == requested
        && moneyBefore - moneyAfter == request.GetTotalCost()
        && town->GetArmy().GetCountMonsters( Monster::PEASANT ) == 1;
    const bool consistent = accepted ? exact : unchanged;
    std::cout << "RESULT mode=" << mode << " invoked=" << invoke << " accepted=" << accepted
              << " requested=" << requested << " stock_before=" << stockBefore << " stock_after=" << stockAfter
              << " units_added=" << added << " stock_removed=" << removed
              << " gold_spent=" << int64_t{moneyBefore.gold} - moneyAfter.gold
              << " receipt_consistent=" << consistent << '\n';
    if ( mode == "raw_exact" ) assert( accepted && exact );
    if ( guarded ) assert( !invoke && !accepted && unchanged );
    kingdom.GetCastles().clear();
    // raw_overstock is a diagnostic probe: 2 means inconsistency, NOT a passing regression.
    return consistent ? 0 : 2;
}
