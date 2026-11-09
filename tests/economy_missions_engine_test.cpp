#include "economy_inputs.h"
#include "heroes.h"
#include "difficulty.h"
#include "map_format_info.h"
#include "maps_tiles.h"
#include "players.h"
#include "settings.h"
#include "world.h"
#include <cassert>
#include <iostream>
namespace I = AI::EconomyInputs;
namespace X = AI::EconomyMissions;
namespace P = AI::EconomyProject;
int main( int argc, char ** argv )
{
    assert( argc == 2 ); Settings::Get().Read( argv[1] ); Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() ); world.generateUninitializedMap( 36 ); world.NewDay();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE ); kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 10000 ) );
    const int tile = 150; world.addCastle( tile, Race::KNGT, PlayerColor::BLUE );
    auto * town = world.getCastle( { tile % 36, tile / 36 } ); kingdom.GetCastles().push_back( town );
    world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
    Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true; meta.builtBuildings = { BUILD_CASTLE, BUILD_CAPTAIN, BUILD_WEL2, DWELLING_MONSTER1 };
    town->loadFromResurrectionMap( meta ); town->GetArmy().Reset(); town->GetArmy().JoinTroop( Monster::PEASANT, 40, false );
    const int ids[] = { Heroes::LORDKILBURN, Heroes::SIRGALLANTH, Heroes::ECTOR, Heroes::GVENNETH, Heroes::TYRO, Heroes::AMBROSE };
    const Heroes::Role roles[] = { Heroes::Role::FIGHTER, Heroes::Role::SCOUT, Heroes::Role::COURIER, Heroes::Role::HUNTER, Heroes::Role::FIGHTER, Heroes::Role::CHAMPION };
    for ( size_t i = 0; i < 6; ++i ) {
        auto & hero = *world.GetHeroes( ids[i] ); hero.SetColor( PlayerColor::BLUE ); hero.setAIRole( roles[i] );
        hero.SetCenter( i ? fheroes2::Point{ 3 + static_cast<int>( i ), 3 } : town->GetCenter() );
        hero.GetArmy().Reset(); hero.GetArmy().JoinTroop( Monster::ARCHER, 10 + static_cast<uint32_t>( i ), false );
        kingdom.GetHeroes().push_back( &hero );
    }
    auto & visitor = *world.GetHeroes( ids[0] ); visitor.SetModes( Heroes::PATROL );
    auto & courier = *world.GetHeroes( ids[2] ); courier.GetPath().push_back( Route::Step( 100, courier.GetIndex(), Direction::RIGHT, 100 ) );
    auto & sleeper = *world.GetHeroes( ids[3] ); sleeper.SetModes( Heroes::SLEEPER ); sleeper.ResetMovePoints();
    assert( town->GetHero() == &visitor && visitor.GetPatrolDistance() == 0 );
    std::vector<X::Target> targets{ { 200, X::Intent::Attack, { tile } }, { tile, X::Intent::Defend, {200} }, { 300, X::Intent::Reinforce, {} } };
    std::vector<X::Need> needs;
    for ( size_t i = 0; i < 7; ++i ) needs.push_back( { i, static_cast<int64_t>( 321 + i ), i == 0, i == 6 } );
    const auto money = kingdom.GetFunds(); const auto before = P::capture( kingdom, 15 );
    I::Work work;
    std::cout << "OBS_BEGIN" << std::endl;
    auto frame = I::capture( kingdom, 15, {}, {tile}, true, {}, work, targets, needs );
    I::log( frame, static_cast<int>( kingdom.GetColor() ), work );
    assert( frame.valid && frame.gaps.empty() && frame.missions && frame.missions->valid );
    const auto & snapshot = *frame.missions;
    assert( snapshot.day == 1 && snapshot.heroes.size() == 6 && snapshot.targets.size() == 3 && snapshot.needs.size() == 7 );
    assert( snapshot.stationaryVisitors == std::set<int32_t>{ ids[0] } && frame.assignments[0].verified && frame.context.base.actions.empty() );
    std::set<X::Role> seen;
    for ( const auto & hero : snapshot.heroes ) {
        seen.insert( hero.role );
        assert( hero.id == ids[0] ? hero.use == X::Use::FixedLocal : hero.use == X::Use::MobileRole );
        if ( hero.id == ids[2] ) assert( hero.pathTarget == 100 && hero.role == X::Role::Courier );
        if ( hero.id == ids[3] ) assert( hero.sleeping && hero.movement == 0 );
    }
    assert( seen.size() == 5 && work.paths == 128 && snapshot.needs[0].missing == 321 );
    // Changing the live mission cannot silently rewrite this frozen frame.
    visitor.ResetModes( Heroes::PATROL );
    const auto wide = I::widen( frame, work );
    I::log( wide, static_cast<int>( kingdom.GetColor() ), work );
    assert( wide.valid && wide.gaps.empty() && wide.missions->stationaryVisitors == snapshot.stationaryVisitors && wide.assignments[0].verified );
    assert( wide.missions->heroes[0].role == snapshot.heroes[0].role && wide.missions->targets[0].tile == snapshot.targets[0].tile );
    const auto fresh = I::capture( kingdom, 15, {}, {tile}, true, {}, work, targets, needs );
    I::log( fresh, static_cast<int>( kingdom.GetColor() ), work );
    assert( fresh.valid && fresh.gaps.count( "visitor_assignment_not_committed" ) && !fresh.assignments[0].verified );
    visitor.SetModes( Heroes::PATROL );
    const auto after = P::capture( kingdom, 15 );
    assert( money == kingdom.GetFunds() && before.config.goldPerPeasant == after.config.goldPerPeasant && visitor.Modes( Heroes::PATROL ) );
    assert( courier.GetPath().GetDestinationIndex() == 100 && courier.getAIRole() == Heroes::Role::COURIER );
    for ( size_t i = 0; i < 5; ++i ) {
        assert( before.start.towns[0].garrison[i].count == after.start.towns[0].garrison[i].count );
        assert( before.start.towns[0].visitor[i].count == after.start.towns[0].visitor[i].count );
    }
    std::cout << "OBS_END\nRESULT missions heroes=6 roles=5 targets=3 hints=7 paths_left=" << work.paths << " stale_fixed=1 fresh_held=1\n";
    kingdom.GetHeroes().clear(); kingdom.GetCastles().clear();
}
