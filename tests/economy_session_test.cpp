#include "economy_trace.h"
#include "game_io.h"
#include "game.h"
#include "ground.h"
#include "players.h"
#include "settings.h"
#include "world.h"
#include <cassert>
#include <iostream>

int main( int argc, char ** argv )
{
    assert( argc == 3 );
    auto & settings = Settings::Get();
    settings.Read( argv[1] );
    settings.SetGameType( Game::TYPE_STANDARD );
    settings.GetPlayers().Init( Color::allPlayerColors() );
    world.generateBattleOnlyMap( Maps::Ground::GRASS );
    world.NewDay();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    kingdom.AddFundsResource( Funds( Resource::GOLD, 42 ) );
    const int32_t savedGold = kingdom.GetFunds().gold;
    assert( Game::Save( argv[2] ) );
    kingdom.AddFundsResource( Funds( Resource::GOLD, 99 ) );
    assert( Game::Load( argv[2] ) == fheroes2::GameMode::START_GAME );
    auto & restored = world.GetKingdom( PlayerColor::BLUE );
    assert( restored.GetFunds().gold == savedGold );
    restored.AddFundsResource( Funds( Resource::WOOD, 1 ) );
    {
        const Economy::CauseScope outer( "outer", 1 );
        {
            const Economy::CauseScope inner( "inner", 2 );
            assert( std::string( Economy::traceState().cause ) == "inner" );
        }
        assert( std::string( Economy::traceState().cause ) == "outer" );
    }
    assert( std::string( Economy::traceState().cause ) == "unattributed" );
    assert( Economy::jsonQuote( std::string( "a\"\\\n" ) + char( 255 ) ) == "\"a\\\"\\\\\\u000a\\u00ff\"" );
    std::cout << "RESULT " << world.CountDay() << ' ' << restored.GetFunds().gold << ' ' << restored.GetFunds().wood << '\n';
}
