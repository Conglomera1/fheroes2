// Real rendering / town event handlers. Owned game data and MP2 map required.
#include "game.h"
#include "game_init.h"
#include "game_language.h"
#include "game_interface.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "image_tool.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "knight_observer.h"
#include "kingdom.h"
#include "players.h"
#include "world.h"
#include "castle.h"
#include "serialize.h"
#include "screen.h"
#include "localevent.h"
#include "settings.h"
#include <SDL.h>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>

std::vector<uint8_t> snapshot()
{
    RWStreamBuf bytes; bytes << world;
    return { bytes.data(), bytes.data() + bytes.size() };
}
void key( SDL_Keycode value )
{
    SDL_Event e{}; e.type = SDL_KEYDOWN; e.key.keysym.sym = value; SDL_PushEvent( &e );
    std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
    e.type = SDL_KEYUP; SDL_PushEvent( &e );
    std::this_thread::sleep_for( std::chrono::milliseconds( 70 ) );
}
void click( int x, int y )
{
    SDL_Event e{}; e.type = SDL_MOUSEMOTION; e.motion.x = x; e.motion.y = y; SDL_PushEvent( &e );
    e = {}; e.type = SDL_MOUSEBUTTONDOWN; e.button.button = SDL_BUTTON_LEFT; e.button.x = x; e.button.y = y; SDL_PushEvent( &e );
    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    e.type = SDL_MOUSEBUTTONUP; SDL_PushEvent( &e );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
}
int main( int argc, char ** argv )
{
    assert( argc == 3 );
    auto hardware = Game::createHardwareComponent(); Game::initLogging(); Game::initDataDir(); Game::initConfigDir( argv[0] );
    auto core = Game::createCoreComponent(); auto display = Game::createDisplayComponent(); auto data = Game::createDataComponent();
    Game::initPalette(); Game::initTranslations(); Game::initEventHandler(); Game::initAnimation(); Game::initHotKeys();
    auto & conf = Settings::Get(); conf.SetGameType( Game::TYPE_STANDARD ); conf.SetSoundVolume( 0 ); conf.SetMusicVolume( 0 );
    assert( conf.isKnightObserverBuild() && conf.isTaxpayerTestMapRevealEnabled() && conf.taxpayerTestDiagnostics() == 1 );
    Maps::FileInfo map; assert( map.readMP2Map( argv[1], false ) ); conf.setCurrentMapInfo( map ); conf.GetPlayers().Init( map ); conf.GetPlayers().SetStartGame();
    Players::SetPlayerControl( PlayerColor::BLUE, CONTROL_HUMAN );
    assert( world.LoadMapMP2( argv[1], map.version == GameVersion::SUCCESSION_WARS ) ); world.NewDay();
    for ( auto color : PlayerColorsVector( conf.GetPlayers().GetColors() ) ) world.ClearFog( color ); // Ordinary game startup scouting.
    conf.SetCurrentColor( PlayerColor::BLUE );
    auto & enemy = world.GetKingdom( PlayerColor::RED ); assert( enemy.isControlAI() && !enemy.GetCastles().empty() );
    Castle & town = *enemy.GetCastles().front();
    town.GetArmy().JoinTroop( Monster::PEASANT, 20, false ); town.GetArmy().JoinTroop( Monster::ARCHER, 5, false );
    uint32_t enemyHidden = 0, humanHidden = 0;
    for ( size_t i = 0; i < world.getSize(); ++i ) {
        enemyHidden += world.getTile( static_cast<int32_t>( i ) ).isFog( PlayerColor::RED );
        humanHidden += world.getTile( static_cast<int32_t>( i ) ).isFog( PlayerColor::BLUE );
    }
    assert( enemyHidden && humanHidden && enemyHidden < world.getSize() && humanHidden < world.getSize() );
    auto & ui = Interface::AdventureMap::Get(); ui.reset(); ui.getStatusPanel().Reset(); ui.getRadar().Build(); ui.getRadar().SetHide( false );
    Interface::GameArea::updateMapFogDirections();
    const auto before = snapshot();
    KnightObserver::kingdomState( enemy, "isolation_before" );
    ui.getGameArea().SetCenter( town.GetCenter() ); ui.redraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );
    assert( fheroes2::Save( fheroes2::Display::instance(), std::string( argv[2] ) + "-map.png" ) );
    assert( before == snapshot() );
    std::thread input( []() {
        std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
        click( 145, 280 ); click( 245, 280 ); // Never join/move these troops.
        key( SDLK_b ); key( SDLK_1 ); key( SDLK_SPACE ); key( SDLK_LEFT ); key( SDLK_RIGHT );
        for ( int i = 0; i < 8; ++i ) key( SDLK_ESCAPE );
    } );
    assert( town.OpenDialog( true, true, false, true, true ) == Castle::CastleDialogReturnValue::Close ); input.join();
    assert( fheroes2::Save( fheroes2::Display::instance(), std::string( argv[2] ) + "-town.png" ) );
    assert( before == snapshot() );
    // Exercise the actual Ctrl-click entry, without selecting or taking over a town.
    std::thread close( []() { std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) ); for ( int i = 0; i < 5; ++i ) key( SDLK_ESCAPE ); } );
    SDL_SetModState( KMOD_LCTRL ); ui.mouseCursorAreaClickLeft( town.GetIndex() ); SDL_SetModState( KMOD_NONE ); close.join();
    assert( before == snapshot() );
    KnightObserver::kingdomState( enemy, "isolation_after" );
    std::cout << "PASS observer map, read-only town, Ctrl-click, purchase/army/navigation hotkeys; full serialized world unchanged; AI hidden="
        << enemyHidden << " human hidden=" << humanHidden << " bytes=" << before.size() << std::endl;
    _exit( 0 );
}
