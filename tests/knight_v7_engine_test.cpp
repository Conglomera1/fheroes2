// Targeted regressions from AoW/Utopeia; real engine mutations and shared budgets.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "map_format_info.h"
#include "players.h"
#include <iostream>

int main( int argc, char ** argv )
{
    assert( argc == 3 ); const std::string mode = argv[1]; Settings::Get().Read( argv[2] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    Players::Get( PlayerColor::BLUE )->SetControl( CONTROL_AI );
    world.generateUninitializedMap( 36 ); world.NewDay();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    const auto money = [&]( int gold, int wood = 100 ) {
        kingdom.OddFundsResource( kingdom.GetFunds() ); Funds funds; funds.gold = gold; funds.wood = wood;
        funds.ore = funds.crystal = funds.mercury = funds.sulfur = funds.gems = 100;
        kingdom.AddFundsResource( funds );
    };
    money( 20000 );
    const auto add = [&]( int tile, std::vector<int32_t> buildings ) {
        world.addCastle( tile, static_cast<uint8_t>( Race::KNGT ), PlayerColor::BLUE );
        auto * c = world.getCastle( { tile % 36, tile / 36 } ); assert( c ); kingdom.GetCastles().push_back( c );
        world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
        Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
        for ( const auto b : buildings ) meta.builtBuildings.push_back( b );
        c->loadFromResurrectionMap( meta ); c->SetModes( Castle::ALLOW_TO_BUILD_TODAY ); c->GetArmy().Clean(); return c;
    };
    const std::vector<int32_t> complete{ BUILD_CASTLE, BUILD_CAPTAIN, BUILD_STATUE, BUILD_MARKETPLACE, BUILD_WELL, BUILD_TAVERN,
        DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6, BUILD_WEL2 };
    if ( mode == "wood" || mode == "sawmill" ) {
        auto * c = add( 150, { BUILD_CASTLE, BUILD_STATUE, BUILD_WELL, BUILD_WEL2, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3 } );
        money( 10000, 8 );
        if ( mode == "sawmill" ) { world.getTile( 350 ).setMainObjectType( MP2::OBJ_SAWMILL ); world.CaptureObject( 350, PlayerColor::BLUE ); }
        KC::Scope scope( kingdom ); assert( knightPrimaryDevelopment( *c ) == PrimaryResult::Built );
        assert( c->isBuild( mode == "wood" ? BUILD_MARKETPLACE : BUILD_TAVERN ) );
        assert( kingdom.GetFunds().wood == 3 );
    }
    else if ( mode == "boat_budget" || mode == "mobilization" ) {
        auto buildings = complete; buildings.erase( std::remove( buildings.begin(), buildings.end(), DWELLING_MONSTER5 ), buildings.end() );
        auto * c = add( 150, buildings ); money( 12094, mode == "boat_budget" ? 19 : 11 );
        world.getTile( 350 ).setMainObjectType( MP2::OBJ_SAWMILL ); world.CaptureObject( 350, PlayerColor::BLUE );
        KC::Scope scope( kingdom );
        if ( mode == "boat_budget" ) {
            assert( constructionContext->ledger.promises.at( 150 ).building == DWELLING_MONSTER5 );
            const auto before = kingdom.GetFunds(); assert( !KC::allowsPayment( kingdom, PaymentConditions::BuyBoat(), "boat_purchase" ) );
            assert( !c->BuyBoat() ); assert( kingdom.GetFunds() == before );
            assert( constructionContext->ledger.promises.at( 150 ).building == DWELLING_MONSTER5 );
        }
        else {
            assert( !KC::mobilizationPreservesDevelopment( *c ) );
            const auto decision = AI::KnightEconomy::choose( *c, true );
            assert( !decision.building && decision.reason == "development_delayed" );
        }
    }
    else if ( mode == "recovery" || mode == "unsafe_recovery" ) {
        auto buildings = complete; buildings.back() = BUILD_TRAINING_GROUNDS; auto * c = add( 150, buildings );
        assert( c->GetArmy().JoinTroop( Monster::PEASANT, 100, false ) ); money( 2124 );
        KC::Scope scope( kingdom );
        const auto decision = AI::KnightEconomy::choose( *c, mode == "recovery" );
        if ( mode == "recovery" ) {
            assert( decision.building == BUILD_WEL2 && decision.reason == "cash_recovery" );
            assert( AI::KnightEconomy::develop( *c, true ) && c->isBuild( BUILD_WEL2 ) ); assert( kingdom.GetFunds().gold == 624 );
        }
        else assert( !decision.building && decision.reason == "unsafe_or_other_faction" );
    }
    else if ( mode == "roles" ) {
        auto * front = add( 150, complete ); auto back = complete;
        back.erase( std::remove( back.begin(), back.end(), BUILD_STATUE ), back.end() );
        back.erase( std::remove( back.begin(), back.end(), BUILD_MARKETPLACE ), back.end() ); auto * rear = add( 250, back );
        KC::Scope scope( kingdom ); KC::setRoles( kingdom, { 151 }, {} );
        assert( !KC::economicTown( *front ) && KC::economicTown( *rear ) );
        assert( knightPrimaryDevelopment( *rear ) == PrimaryResult::Built ); assert( rear->isBuild( BUILD_STATUE ) && !rear->isBuild( BUILD_MARKETPLACE ) );
        rear->SetModes( Castle::ALLOW_TO_BUILD_TODAY ); KC::refresh();
        assert( knightPrimaryDevelopment( *rear ) == PrimaryResult::Built && rear->isBuild( BUILD_MARKETPLACE ) );
        KC::setRoles( kingdom, { 151 }, { 250 } ); assert( !KC::economicTown( *rear ) );
    }
    else if ( mode == "statue_wait" || mode == "statue_exception" ) {
        auto buildings = complete;
        buildings.erase( std::remove( buildings.begin(), buildings.end(), BUILD_STATUE ), buildings.end() );
        buildings.erase( std::remove( buildings.begin(), buildings.end(), BUILD_MARKETPLACE ), buildings.end() );
        auto * c = add( 150, buildings ); money( mode == "statue_wait" ? 500 : 800 );
        KC::Scope scope( kingdom ); constructionContext->economic.insert( 150 ); KC::refresh();
        if ( mode == "statue_wait" ) {
            assert( knightPrimaryDevelopment( *c ) == PrimaryResult::Fallback ); assert( !c->isBuild( BUILD_MARKETPLACE ) );
        }
        else { assert( knightPrimaryDevelopment( *c ) == PrimaryResult::Built ); assert( c->isBuild( BUILD_MARKETPLACE ) ); }
    }
    else if ( mode == "routine_budget" || mode == "emergency" || mode == "human" ) {
        auto buildings = complete; buildings.back() = BUILD_TRAINING_GROUNDS; auto * c = add( 150, buildings ); money( 4000 );
        KC::Scope scope( kingdom );
        if ( mode == "human" ) Players::Get( PlayerColor::BLUE )->SetControl( CONTROL_HUMAN );
        const KC::Emergency emergency( mode == "emergency" );
        const Monster archer( Monster::ARCHER ); const auto limit = c->getRecruitLimit( archer, kingdom.GetFunds() );
        assert( limit > 0 ); assert( c->RecruitMonster( Troop( archer, limit ), false ) );
        if ( mode == "routine_budget" ) assert( kingdom.GetFunds().gold >= 3500 );
        else assert( kingdom.GetFunds().gold < 3500 );
    }
    else if ( mode == "shared_forecast" ) {
        Castle * first = nullptr;
        for ( int n = 0; n < 11; ++n ) { auto * c = add( 150 + 6 * ( n % 4 ) + 216 * ( n / 4 ), complete ); if ( !first ) first = c; }
        money( 16463 );
        const auto decision = AI::KnightEconomy::choose( *first, true );
        assert( decision.towns == 11 && decision.projections > 0 );
        assert( decision.reason != "forecast_incomplete" );
        assert( decision.keepGold > 0 && decision.keepGold < 50000 );
        std::cout << "FORECAST towns=" << decision.towns << " keep=" << decision.keepGold << " switch=" << decision.switchGold << '\n';
    }
    else if ( mode == "overhead" ) {
        auto * c = add( 150, complete ); KC::Scope budget( kingdom ); (void)AI::KnightEconomy::overhead( kingdom );
        { const Economy::CauseScope cause( "unit_upgrade", 150 ); kingdom.OddFundsResource( Funds( Resource::GOLD, 700 ) ); }
        world.NewDay(); const auto rate = AI::KnightEconomy::overhead( kingdom ); assert( rate[0] == 700 );
        const auto decision = AI::KnightEconomy::choose( *c, true ); assert( decision.dailyOverhead[0] == 700 );
    }
    else if ( mode == "naval_use" || mode == "hidden_naval_object" ) {
        const int goal = mode == "naval_use" ? 226 : 380;
        for ( int tile = 0; tile < static_cast<int>( world.getSize() ); ++tile ) {
            world.getTile( tile ).setTerrain( tile / 36 >= 6 ? 0 : Maps::Ground::GRASS_START_IMAGE_INDEX, 0 );
            if ( mode == "naval_use" || tile != goal ) world.getTile( tile ).ClearFog( static_cast<PlayerColorsSet>( PlayerColor::BLUE ) );
        }
        auto buildings = complete; buildings.push_back( BUILD_SHIPYARD ); auto * c = add( 150, buildings );
        world.getTile( goal ).setMainObjectType( MP2::OBJ_FLOTSAM );
        world.updatePassabilities();
        auto * hero = world.GetHeroes( Heroes::LORDKILBURN ); assert( hero && hero->Recruit( *c ) );
        if ( mode == "hidden_naval_object" ) assert( world.getTile( goal ).isFog( PlayerColor::BLUE ) );
        KC::Scope scope( kingdom ); const auto mission = navalUse( *c );
        if ( mode == "naval_use" ) {
            assert( mission.hero == hero->GetID() && mission.target == 226 && mission.object == MP2::OBJ_FLOTSAM );
            assert( c->BuyBoat() ); navalMissions[hero->GetID()] = mission;
            assert( KC::navalTarget( *hero ) == 226 );
            AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( *hero ); assert( path.getDistance( 226 ) > 0 );
        }
        else assert( mission.hero < 0 || mission.object != MP2::OBJ_FLOTSAM );
        kingdom.GetHeroes().clear();
    }
    else if ( mode == "no_naval_use" ) {
        auto * c = add( 150, complete ); assert( navalUse( *c ).hero < 0 );
    }
    else assert( false );
    std::cout << "PASS v7 " << mode << " gold=" << kingdom.GetFunds().gold << '\n';
    kingdom.GetCastles().clear();
}
