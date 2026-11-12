// Regressions for the invalid v7 forecasts and AoW's delayed naval opening.
// Production decisions, resources, purchases and native routes are exercised.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "map_format_info.h"
#include "players.h"
#include "ai_hero_action.h"
#include "spell_info.h"
#include "rand.h"
#include <iostream>

int main( int argc, char ** argv )
{
    assert( argc == 3 ); const std::string mode = argv[1]; Settings::Get().Read( argv[2] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    Players::Get( PlayerColor::BLUE )->SetControl( CONTROL_AI );
    Rand::CurrentThreadRandomDevice() = Rand::PCG32( 20261112 );
    world.generateUninitializedMap( 36 ); world.NewDay();
    for ( int tile = 0; tile < static_cast<int>( world.getSize() ); ++tile ) world.getTile( tile ).setIndex( tile );
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    const auto money = [&]( const KC::Resources & a ) {
        kingdom.OddFundsResource( kingdom.GetFunds() );
        Funds f; f.gold = static_cast<int32_t>( a[0] ); f.wood = static_cast<int32_t>( a[1] ); f.ore = static_cast<int32_t>( a[2] );
        f.mercury = static_cast<int32_t>( a[3] ); f.sulfur = static_cast<int32_t>( a[4] ); f.crystal = static_cast<int32_t>( a[5] ); f.gems = static_cast<int32_t>( a[6] );
        kingdom.AddFundsResource( f );
    };
    money( { 200000, 1000, 1000, 1000, 1000, 1000, 1000 } );
    const auto add = [&]( int tile, const std::vector<int32_t> & buildings ) {
        world.addCastle( tile, static_cast<uint8_t>( Race::KNGT ), PlayerColor::BLUE );
        auto * c = world.getCastle( { tile % 36, tile / 36 } ); assert( c ); kingdom.GetCastles().push_back( c );
        world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
        Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
        for ( const auto b : buildings ) meta.builtBuildings.push_back( b );
        c->loadFromResurrectionMap( meta ); c->SetModes( Castle::ALLOW_TO_BUILD_TODAY ); c->GetArmy().Clean(); return c;
    };
    const std::vector<int32_t> complete{ BUILD_CASTLE, BUILD_CAPTAIN, BUILD_STATUE, BUILD_MARKETPLACE, BUILD_WELL, BUILD_TAVERN,
        DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6, BUILD_WEL2 };
    if ( mode.rfind( "history_", 0 ) == 0 || mode == "optional_gold" ) {
        auto * c = add( 150, complete ); assert( c->GetArmy().JoinTroop( Monster::PEASANT, 20, false ) );
        KC::Scope budget( kingdom ); (void)AI::KnightEconomy::overhead( kingdom );
        const bool optional = mode == "optional_gold";
        for ( int day = 0; day < 7; ++day ) {
            const Economy::CauseScope cause( optional ? "unit_upgrade" : day % 2 ? "market_trade" : "building_purchase", 150 );
            kingdom.OddFundsResource( Funds( 8, 4, 10, 6, 4, 5, optional ? 10972 : 3179 ) );
            world.NewDay();
        }
        const std::array<KC::Resources, 4> funds{{ { 25030, 28, 23, 9, 6, 2, 6 }, { 17350, 25, 22, 10, 3, 9, 9 },
            { 17880, 31, 23, 2, 0, 2, 4 }, { 17102, 23, 32, 12, 9, 17, 30 } }};
        const auto index = optional ? 0 : static_cast<size_t>( std::stoi( mode.substr( 8 ) ) ); assert( index < funds.size() );
        money( funds[index] ); c->SetModes( Castle::ALLOW_TO_BUILD_TODAY ); KC::refresh();
        const auto rate = AI::KnightEconomy::overhead( kingdom );
        for ( size_t r = 1; r < rate.size(); ++r ) assert( rate[r] == 0 );
        assert( rate[0] == ( optional ? 10972 : 0 ) );
        const auto before = kingdom.GetFunds();
        const auto choice = AI::KnightEconomy::choose( *c, true );
        std::cerr << "REVIEW " << choice.reason << " valid=" << choice.keepValid << "/" << choice.switchValid << "/" << choice.cycleValid << '\n';
        assert( choice.keepValid && choice.switchValid && choice.cycleValid );
        assert( choice.keepGold >= 0 && choice.switchGold >= 0 && choice.cycleGold >= 0 );
        assert( kingdom.GetFunds() == before );
        std::cout << "FORECAST " << mode << " " << choice.reason << " gold=" << choice.keepGold << "/" << choice.switchGold
                  << "/" << choice.cycleGold << " building=" << choice.building << '\n';
    }
    else {
        const bool hidden = mode == "hidden";
        for ( int tile = 0; tile < static_cast<int>( world.getSize() ); ++tile ) {
            world.getTile( tile ).setTerrain( tile / 36 >= 6 ? 0 : Maps::Ground::GRASS_START_IMAGE_INDEX, 0 );
            if ( !hidden || tile / 36 < 6 ) world.getTile( tile ).ClearFog( static_cast<PlayerColorsSet>( PlayerColor::BLUE ) );
        }
        auto * c = add( 150, mode == "shipyard_future" ? std::vector<int32_t>{ BUILD_CASTLE } : std::vector<int32_t>{ BUILD_CASTLE, BUILD_SHIPYARD } );
        const int goal = hidden ? 380 : 226;
        world.getTile( goal ).setMainObjectType( MP2::OBJ_FLOTSAM );
        world.updatePassabilities();
        money( { 7500, 20, 20, 0, 0, 0, 0 } );
        auto * hero = world.GetHeroes( Heroes::LORDKILBURN ); assert( hero && c->RecruitHero( hero ) );
        assert( kingdom.GetFunds().gold == 5000 );
        KC::Scope budget( kingdom );
        if ( mode == "defender" ) KC::setRoles( kingdom, {}, { 150 } );
        if ( mode == "other_defender" ) {
            auto * other = add( 152, { BUILD_CASTLE } );
            hero->SetCenter( other->GetCenter() ); KC::setRoles( kingdom, {}, { 152 } );
        }
        if ( mode == "spent" || mode == "shipyard_future" ) hero->ResetMovePoints();
        if ( mode == "shipyard_future" ) {
            money( { 10000, 60, 20, 0, 0, 0, 0 } ); KC::refresh();
            assert( usefulShipyard( *c ) );
        }
        if ( mode == "existing" || mode == "duplicate" ) world.getTile( 222 ).setBoat( Direction::RIGHT, PlayerColor::BLUE );
        if ( mode == "summon" ) {
            world.getTile( 800 ).setBoat( Direction::RIGHT, PlayerColor::BLUE );
            hero->Move2Dest( 186 );
            assert( hero->SpellBookActivate() ); hero->AppendSpellToBook( Spell( Spell::SUMMONBOAT ), true ); hero->SetSpellPoints( 50 );
            assert( hero->CanCastSpell( Spell( Spell::SUMMONBOAT ) ) );
        }
        if ( mode == "sleeping" ) hero->SetModes( Heroes::SLEEPER );
        const auto before = kingdom.GetFunds();
        const auto changed = KC::prepareNaval( kingdom );
        if ( mode == "same_day" ) {
            assert( changed.size() == 1 && kingdom.GetFunds().gold == before.gold - 1000 && kingdom.GetFunds().wood == 10 );
            assert( navalMissions.at( hero->GetID() ).hero == hero->GetID() && !navalMissions.at( hero->GetID() ).reuse );
            // The boat and Statue use separate slots. Real mutation, no mocked build.
            assert( knightPrimaryDevelopment( *c ) == PrimaryResult::Built && c->isBuild( BUILD_STATUE ) );
            assert( kingdom.GetFunds().gold == 2750 && kingdom.GetFunds().ore == 15 );
            AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( *hero );
            assert( path.getDistance( 226 ) > 0 );
            const auto route = path.buildPath( 226, false );
            bool boards = false;
            for ( const auto & step : route ) if ( step.GetIndex() == changed.front() ) boards = true;
            assert( boards );
            Settings::Get().SetGameType( Game::TYPE_HOTSEAT ); Settings::Get().SetAIMoveSpeed( 0 );
            // Follow the verified native route to its boarding step. Native
            // boarding must consume the day's movement and put the hero at sea.
            for ( const auto & step : route ) {
                if ( step.GetIndex() == changed.front() ) { AI::HeroesAction( *hero, step.GetIndex() ); break; }
                hero->Move2Dest( step.GetIndex() );
            }
            assert( world.CountDay() == 1 && hero->isShipMaster() && hero->GetIndex() == changed.front() && hero->GetMovePoints() == 0 );
        }
        else if ( mode == "existing" || mode == "duplicate" || mode == "summon" ) {
            assert( changed.empty() && kingdom.GetFunds() == before );
            assert( navalMissions.at( hero->GetID() ).reuse );
            if ( mode == "summon" ) {
                assert( fheroes2::getSummonableBoat( *hero ) == 800 );
                // Validate the native summonable route without creating GUI
                // widgets, which require the user's original game assets.
                AIWorldPathfinder path; path.reset(); path.reEvaluateIfNeeded( *hero );
                assert( path.getDistance( 226 ) > 0 && hero->GetSpellPoints() == 50 );
                assert( kingdom.GetFunds() == before );
            }
            if ( mode == "duplicate" ) {
                auto * second = world.GetHeroes( Heroes::SIRGALLANTH ); assert( second && second->Recruit( PlayerColor::BLUE, { 8, 4 } ) );
                assert( KC::prepareNaval( kingdom ).empty() && kingdom.GetFunds() == before );
                assert( navalMissions.size() == 1 );
            }
        }
        else if ( hidden ) {
            assert( world.getTile( goal ).isFog( PlayerColor::BLUE ) );
            for ( const auto & [id, mission] : navalMissions ) { (void)id; assert( mission.target != goal && mission.object != MP2::OBJ_FLOTSAM ); }
        }
        else assert( changed.empty() && navalMissions.empty() && kingdom.GetFunds() == before );
        kingdom.GetHeroes().clear();
    }
    std::cout << "PASS v8 " << mode << " gold=" << kingdom.GetFunds().gold << '\n';
    kingdom.GetCastles().clear();
}
