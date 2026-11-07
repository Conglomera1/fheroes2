// Integration test: include the planner implementation to exercise its internal
// legacy build paths without exporting test-only production entry points.
// Link with the game objects except fheroes2.o and ai_planner_castle.o.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"

#include "conscription.h"
#include "map_format_info.h"
#include "players.h"
#include <iostream>
#include <memory>

namespace
{
    std::unique_ptr<Castle> makeCastle( const int race, const bool training, const bool market = false )
    {
        auto castle = std::make_unique<Castle>( 1, 1, race );
        castle->SetColor( PlayerColor::BLUE );
        Maps::Map_Format::CastleMetadata metadata;
        metadata.customBuildings = true;
        metadata.builtBuildings = { BUILD_CASTLE, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, BUILD_CAPTAIN };
        if ( training ) metadata.builtBuildings.push_back( BUILD_TRAINING_GROUNDS );
        if ( market ) metadata.builtBuildings.push_back( BUILD_MARKETPLACE );
        castle->loadFromResurrectionMap( metadata );
        return castle;
    }

    void setFunds( const Funds & funds )
    {
        auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
        kingdom.OddFundsResource( kingdom.GetFunds() );
        kingdom.AddFundsResource( funds );
    }
}

int main()
{
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    world.InitKingdoms();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );

    // A captured TG must survive both legacy Farm paths, even with profitable
    // taxpayers and enough funds. Skipping it must still allow another building.
    auto training = makeCastle( Race::KNGT, true );
    training->GetArmy().JoinTroop( Monster( Monster::PEASANT ), 100, false );
    setFunds( Funds( 100, 100, 100, 100, 100, 100, 100000 ) );
    const Funds before = kingdom.GetFunds();
    assert( !buildQuickPaybackFarm( *training ) );
    assert( !Build( *training, { { BUILD_WEL2, 1 } } ) );
    assert( training->hasTrainingGrounds() && !training->isBuild( BUILD_WEL2 ) );
    assert( kingdom.GetFunds() == before );
    assert( Build( *training, { { BUILD_WEL2, 1 }, { BUILD_WELL, 1 } } ) );
    assert( training->hasTrainingGrounds() && training->isBuild( BUILD_WELL ) );

    // Existing ordinary Farm investment remains possible.
    auto normal = makeCastle( Race::KNGT, false );
    normal->GetArmy().JoinTroop( Monster( Monster::PEASANT ), 100, false );
    setFunds( Funds( 100, 100, 0, 0, 0, 0, 10000 ) );
    assert( buildQuickPaybackFarm( *normal ) );
    assert( normal->isBuild( BUILD_WEL2 ) );
    const Funds trainingCost = normal->getBuildingCost( BUILD_TRAINING_GROUNDS );
    setFunds( trainingCost );
    normal->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
    assert( AI::BuildIfPossible( *normal, BUILD_TRAINING_GROUNDS ) );
    assert( normal->hasTrainingGrounds() && kingdom.GetFunds() == Funds() );

    // The execution helper stays available for an explicit future reconversion
    // policy. Check all configured resources and multiplier boundaries.
    auto explicitConversion = makeCastle( Race::KNGT, true );
    const Funds cost = explicitConversion->getBuildingCost( BUILD_WEL2 );
    assert( cost.gold == Conscription::settings().farmGold );
    assert( cost.wood == Conscription::settings().farmWood );
    assert( cost.ore == Conscription::settings().farmOre );
    Funds threshold = cost * 2;
    if ( threshold.gold > 0 ) {
        --threshold.gold;
        setFunds( threshold );
        assert( !AI::BuildIfEnoughFunds( *explicitConversion, BUILD_WEL2, 2 ) );
        assert( kingdom.GetFunds() == threshold && explicitConversion->hasTrainingGrounds() );
    }
    setFunds( cost * 2 );
    assert( AI::BuildIfEnoughFunds( *explicitConversion, BUILD_WEL2, 2 ) );
    assert( kingdom.GetFunds() == cost );
    assert( explicitConversion->isBuild( BUILD_WEL2 ) && !explicitConversion->hasTrainingGrounds() );

    // With a market, exact gold and no required wood/ore: the real transaction
    // must obtain the configured resources before reconversion can succeed.
    auto market = makeCastle( Race::KNGT, true, true );
    kingdom.GetCastles().push_back( market.get() );
    setFunds( Funds( 0, 0, 1000, 1000, 1000, 1000, cost.gold ) );
    assert( AI::BuildIfPossible( *market, BUILD_WEL2 ) );
    assert( market->isBuild( BUILD_WEL2 ) && !market->hasTrainingGrounds() );
    kingdom.GetCastles().clear();

    auto insufficient = makeCastle( Race::KNGT, true );
    if ( cost.gold > 0 ) {
        Funds tooLittle = cost;
        --tooLittle.gold;
        setFunds( tooLittle );
        assert( !AI::BuildIfPossible( *insufficient, BUILD_WEL2 ) );
        assert( kingdom.GetFunds() == tooLittle && insufficient->hasTrainingGrounds() );
    }

    // Other factions' growth buildings are not classified as TG reconversions.
    auto barbarian = makeCastle( Race::BARB, false );
    assert( !isLegacyFarmReconversion( *barbarian, BUILD_WEL2 ) );
    assert( barbarian->getBuildingCost( BUILD_WEL2 ) == PaymentConditions::BuyBuilding( Race::BARB, BUILD_WEL2 ) );
    setFunds( Funds( 100, 100, 100, 100, 100, 100, 100000 ) );
    assert( Build( *barbarian, { { BUILD_WEL2, 1 } } ) );
    assert( barbarian->isBuild( BUILD_WEL2 ) );
    std::cout << "AI building integration checks passed\n";
}
