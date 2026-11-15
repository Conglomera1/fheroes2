// Real budgets, armies and boarding; regressions from Kalindra and multi-town roles.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "ai_hero_action.h"
#include "map_format_info.h"
#include "players.h"
#include "rand.h"
#include <iostream>

int main( int argc, char ** argv )
{
    assert( argc == 3 ); const std::string mode = argv[1]; Settings::Get().Read( argv[2] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL ); Settings::Get().SetGameType( Game::TYPE_HOTSEAT ); Settings::Get().SetAIMoveSpeed( 0 );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    for ( const auto color : PlayerColorsVector( Color::allPlayerColors() ) ) Players::Get( color )->SetControl( CONTROL_AI );
    Rand::CurrentThreadRandomDevice() = Rand::PCG32( 20261115 ); world.generateUninitializedMap( 36 ); world.NewDay();
    const bool boarding = mode.find( "boarding" ) == 0;
    for ( int tile = 0; tile < static_cast<int>( world.getSize() ); ++tile ) {
        world.getTile( tile ).setIndex( tile );
        world.getTile( tile ).setTerrain( boarding && tile / 36 >= 6 ? 0 : Maps::Ground::GRASS_START_IMAGE_INDEX, 0 );
        world.getTile( tile ).ClearFog( static_cast<PlayerColorsSet>( PlayerColor::BLUE ) );
    }
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    const auto money = [&]( int gold ) {
        kingdom.OddFundsResource( kingdom.GetFunds() ); Funds f; f.gold = gold;
        f.wood = f.ore = f.mercury = f.sulfur = f.crystal = f.gems = 100; kingdom.AddFundsResource( f );
    };
    money( 50000 );
    const std::vector<int32_t> complete{ BUILD_CASTLE, BUILD_CAPTAIN, BUILD_STATUE, BUILD_MARKETPLACE, BUILD_WELL, BUILD_TAVERN,
        DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6, BUILD_WEL2 };
    const auto add = [&]( int tile, std::vector<int32_t> buildings, int race = Race::KNGT ) {
        world.addCastle( tile, static_cast<uint8_t>( race ), PlayerColor::BLUE );
        auto * c = world.getCastle( { tile % 36, tile / 36 } ); assert( c ); kingdom.GetCastles().push_back( c );
        world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
        Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true; meta.availableToHireMonsterCount.fill( 0 );
        if ( race == Race::SORC ) meta.availableToHireMonsterCount[1] = 200;
        for ( auto b : buildings ) meta.builtBuildings.push_back( b );
        c->loadFromResurrectionMap( meta ); c->GetArmy().Clean(); c->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
        return c;
    };
    auto tg = complete; tg.back() = BUILD_TRAINING_GROUNDS;
    if ( mode == "role_stable" || mode == "capture" ) {
        auto * first = add( 150, complete ); auto * rear = add( 258, complete ); world.updatePassabilities();
        KC::Scope scope( kingdom ); KC::setRoles( kingdom, { 151 }, {} );
        assert( !KC::economicTown( *first ) && KC::economicTown( *rear ) );
        world.NewDay(); KC::setRoles( kingdom, { 259 }, {} );
        assert( !KC::economicTown( *first ) && KC::economicTown( *rear ) );
        if ( mode == "capture" ) {
            kingdom.GetCastles().erase( kingdom.GetCastles().begin() ); first->SetColor( PlayerColor::RED );
            KC::setRoles( kingdom, { 259 }, {} ); assert( !KC::economicTown( *rear ) );
        }
        else {
            while ( world.CountDay() < 8 ) world.NewDay();
            KC::setRoles( kingdom, { 259 }, {} ); assert( KC::economicTown( *first ) && !KC::economicTown( *rear ) );
        }
    }
    else if ( mode == "kalindra" || mode == "early_budget" || mode == "primary_slot" ) {
        auto buildings = complete;
        if ( mode == "primary_slot" ) buildings.erase( std::remove( buildings.begin(), buildings.end(), DWELLING_MONSTER6 ), buildings.end() );
        auto * c = add( 150, buildings ); assert( c->GetArmy().JoinTroop( Monster::PEASANT, 20, false ) );
        money( mode == "primary_slot" ? 50000 : 20000 ); KC::Scope scope( kingdom );
        KC::setRoles( kingdom, {}, {} ); const auto before = kingdom.GetFunds();
        AI::KnightEconomy::prepare( kingdom, { 150 } );
        if ( mode == "primary_slot" ) { assert( !c->hasTrainingGrounds() && kingdom.GetFunds() == before ); }
        else {
            assert( c->hasTrainingGrounds() );
            if ( mode == "kalindra" ) {
                auto * rear = add( 258, complete ); world.NewDay(); kingdom.ActionNewDayResourceUpdate( nullptr );
                KC::setRoles( kingdom, { 259 }, {} ); assert( !KC::economicTown( *c ) );
                // Even an explicit economic reclassification cannot dismantle the funded plan.
                constructionContext->economic.insert( 150 );
                const auto cash = kingdom.GetFunds(); assert( !AI::KnightEconomy::develop( *c, true ) );
                assert( c->hasTrainingGrounds() && kingdom.GetFunds() == cash );
                assert( AI::KnightEconomy::choose( *c, true ).reason == "funded_mobilization_plan" );
                assert( rear->isBuild( BUILD_WEL2 ) );
                while ( world.CountDay() < 14 ) {
                    world.NewDay(); kingdom.ActionNewDayResourceUpdate( nullptr );
                    assert( !AI::KnightEconomy::develop( *c, true ) && c->hasTrainingGrounds() );
                }
                assert( c->conscriptionDaySnapshot().state.total > 25 );
            }
            else {
                const auto reserved = KC::operatingReserve( kingdom );
                assert( reserved > 5000 && KC::spendable( kingdom ).gold == kingdom.GetFunds().gold - reserved );
                // A local producing town can pay for its infantry; another city cannot consume its envelope.
                { const KC::Recruitment own( *c ); assert( KC::operatingReserve( kingdom ) < reserved ); }
                assert( KC::operatingReserve( kingdom ) == reserved );
                auto * other = add( 450, { BUILD_CASTLE, DWELLING_MONSTER1, DWELLING_MONSTER2 }, Race::SORC );
                const Monster unit( Race::SORC, DWELLING_MONSTER2 );
                const auto count = other->getRecruitLimit( unit, kingdom.GetFunds() ); assert( count > 0 );
                assert( other->RecruitMonster( Troop( unit, count ), false ) );
                assert( kingdom.GetFunds().gold >= reserved );
                assert( !KC::allowsPayment( kingdom, Funds( Resource::GOLD, kingdom.GetFunds().gold - reserved + 1 ), "other_town_recruitment" ) );
            }
        }
    }
    else if ( mode == "buffer" || mode == "economic_compared" ) {
        auto * c = add( 150, tg ); KC::Scope scope( kingdom );
        if ( mode == "buffer" ) {
            const auto reserve = KC::populationReserve( *c ); assert( reserve > 0 && reserve < 35 );
            assert( c->GetArmy().JoinTroop( Monster::PEASANT, reserve, false ) );
            for ( int n = 0; n < 14; ++n ) { world.NewDay(); kingdom.ActionNewDayResourceUpdate( nullptr ); }
            assert( c->conscriptionDaySnapshot().state.total == 34 );
        }
        else {
            assert( c->GetArmy().JoinTroop( Monster::PEASANT, 20, false ) );
            constructionContext->economic.insert( 150 ); const auto d = AI::KnightEconomy::choose( *c, true );
            assert( d.projections > 0 && d.reason != "economic_role_recovery" );
        }
    }
    else if ( boarding ) {
        auto buildings = mode == "boarding_prefarm" ? std::vector<int32_t>{ BUILD_CASTLE, BUILD_STATUE } : complete;
        buildings.push_back( BUILD_SHIPYARD ); auto * c = add( 150, buildings );
        world.getTile( 226 ).setMainObjectType( MP2::OBJ_FLOTSAM ); world.updatePassabilities();
        auto * hero = world.GetHeroes( Heroes::LORDKILBURN ); assert( hero && hero->Recruit( *c ) );
        auto * mainland = world.GetHeroes( Heroes::SIRGALLANTH ); assert( mainland && mainland->Recruit( PlayerColor::BLUE, fheroes2::Point( 8, 4 ) ) );
        mainland->SetModes( Heroes::PATROL );
        KC::noteOpeningRecruit( *hero, *c );
        hero->GetArmy().Clean(); assert( hero->GetArmy().JoinTroop( Monster::PEASANT, 40, false ) );
        if ( mode != "boarding_last" ) assert( hero->GetArmy().JoinTroop( Monster::ARCHER, 5, false ) );
        KC::Scope scope( kingdom ); assert( KC::prepareNaval( kingdom ).size() == 1 );
        const auto kept = 5U;
        assert( hero->GetArmy().GetCountMonsters( Monster::PEASANT ) == kept );
        assert( c->GetArmy().GetCountMonsters( Monster::PEASANT ) == 40 - kept );
        AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( *hero );
        for ( const auto & step : path.buildPath( 226, false ) ) {
            if ( world.getTile( step.GetIndex() ).getMainObjectType( false ) == MP2::OBJ_BOAT ) { AI::HeroesAction( *hero, step.GetIndex() ); break; }
            hero->Move2Dest( step.GetIndex() );
        }
        assert( hero->isShipMaster() && world.CountDay() == 1 && hero->GetArmy().isValid() );
        AI::OptimizeTroopsOrder( hero->GetArmy() );
        assert( hero->GetArmy().GetOccupiedSlotCount() == 5 );
        if ( mode != "boarding_last" ) assert( c->GetArmy().GetCountMonsters( Monster::ARCHER ) == 5 );
        kingdom.GetHeroes().clear();
    }
    else {
        auto * front = add( 150, tg ); auto * rear = add( 258, complete ); world.updatePassabilities();
        const bool supply = mode == "supply";
        auto * home = supply ? rear : front;
        auto * hero = world.GetHeroes( Heroes::LORDKILBURN ); assert( hero && hero->Recruit( *home ) );
        hero->setAIRole( Heroes::Role::COURIER ); hero->GetArmy().Clean(); assert( hero->GetArmy().JoinTroop( Monster::ARCHER, 10, false ) );
        assert( home->GetArmy().JoinTroop( Monster::PEASANT, 200, false ) );
        KC::Scope scope( kingdom ); KC::setRoles( kingdom, { 151 }, mode == "danger" ? std::vector<int>{ 150 } : std::vector<int>{} );
        if ( mode == "human" ) Players::Get( PlayerColor::BLUE )->SetControl( CONTROL_HUMAN );
        const uint32_t reserve = KC::populationReserve( *front );
        KC::arrangePopulation( *hero, *home );
        if ( mode == "danger" || mode == "human" ) {
            assert( hero->GetArmy().GetCountMonsters( Monster::PEASANT ) == 0 && home->GetArmy().GetCountMonsters( Monster::PEASANT ) == 200 );
        }
        else {
            assert( mode == "tax_export" || supply );
            const auto cargo = supply ? reserve : 200 - reserve;
            assert( cargo > 0 && hero->GetArmy().GetCountMonsters( Monster::PEASANT ) == cargo );
            assert( home->GetArmy().GetCountMonsters( Monster::PEASANT ) == 200 - cargo );
            auto * destination = supply ? front : rear;
            assert( KC::populationDestination( *hero ) == destination->GetIndex() );
            hero->Move2Dest( 186 );
            assert( KC::populationDestination( *hero ) == destination->GetIndex() );
            hero->Move2Dest( destination->GetIndex() ); KC::arrangePopulation( *hero, *destination );
            assert( hero->GetArmy().GetCountMonsters( Monster::PEASANT ) == 0 );
            assert( destination->GetArmy().GetCountMonsters( Monster::PEASANT ) == cargo );
        }
        kingdom.GetHeroes().clear();
    }
    std::cout << "PASS v10 " << mode << " day=" << world.CountDay() << " gold=" << kingdom.GetFunds().gold << '\n';
    kingdom.GetCastles().clear();
}
