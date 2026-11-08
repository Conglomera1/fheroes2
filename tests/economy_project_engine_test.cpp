#include "economy_project.h"
#include "army_troop.h"
#include "difficulty.h"
#include "map_format_info.h"
#include "players.h"
#include "settings.h"
#include "world.h"
#include <cassert>
#include <iostream>
#include <memory>

namespace P = AI::EconomyProject;

int main( int argc, char ** argv )
{
    assert( argc == 2 );
    Settings::Get().Read( argv[1] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    world.InitKingdoms();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    std::vector<std::unique_ptr<Castle>> towns;
    for ( int i = 0; i < 3; ++i ) {
        auto town = std::make_unique<Castle>( i + 1, 1, i == 2 ? Race::WRLK : Race::KNGT );
        town->SetColor( PlayerColor::BLUE );
        Maps::Map_Format::CastleMetadata metadata;
        metadata.customBuildings = true;
        metadata.builtBuildings = { BUILD_CASTLE, BUILD_CAPTAIN, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3,
                                   DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6, DWELLING_UPGRADE2,
                                   i == 1 ? BUILD_TRAINING_GROUNDS : BUILD_WEL2 };
        if ( i == 2 ) metadata.builtBuildings.erase( std::find( metadata.builtBuildings.begin(), metadata.builtBuildings.end(), DWELLING_UPGRADE2 ) );
        metadata.availableToHireMonsterCount = { 50, 50, 50, 50, 50, 50 };
        town->loadFromResurrectionMap( metadata );
        town->GetArmy().JoinTroop( Monster( i == 2 ? Monster::CENTAUR : Monster::PEASANT ), i == 0 ? 40 : 2, false );
        town->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
        kingdom.GetCastles().push_back( town.get() ); towns.push_back( std::move( town ) );
    }
    kingdom.OddFundsResource( kingdom.GetFunds() );
    kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 100000 ) );
    world.NewDay();
    auto input = P::capture( kingdom, 7 );
    auto add = [&]( const uint32_t day, const size_t town, const P::Kind kind, const uint32_t value, const size_t pool = 0 ) {
        P::Action action;
        action.id = action.project = input.actions.size() + 1;
        action.day = day; action.town = towns[town]->GetIndex(); action.kind = kind;
        if ( kind == P::Kind::Build ) action.building = value;
        else { action.count = value; action.pool = pool; }
        input.actions.push_back( action );
    };
    add( 1, 0, P::Kind::Build, BUILD_TRAINING_GROUNDS );
    add( 2, 0, P::Kind::Recruit, 1, 1 );
    add( 3, 1, P::Kind::Build, BUILD_WEL2 );
    add( 3, 2, P::Kind::Recruit, 1, 5 );
    add( 4, 0, P::Kind::Build, BUILD_WELL );
    add( 4, 2, P::Kind::Build, BUILD_STATUE );
    add( 5, 0, P::Kind::Build, BUILD_WEL2 );
    add( 6, 0, P::Kind::Build, BUILD_TRAINING_GROUNDS );
    add( 7, 2, P::Kind::Recruit, 2, 0 );
    const Funds originalFunds = kingdom.GetFunds();
    std::cout << "OBS_BEGIN\n";
    P::WorkBudget work;
    const auto predicted = P::project( input, work );
    std::cout << "OBS_END\n";
    if ( !predicted.covered() ) std::cerr << predicted.reason << ' ' << predicted.failedAction << '\n';
    assert( predicted.covered() && kingdom.GetFunds() == originalFunds );
    assert( towns[0]->isBuild( BUILD_WEL2 ) && towns[0]->GetArmy().GetCountMonsters( Monster( Monster::PEASANT ) ) == 40 );
    for ( uint32_t day = 1; day <= 7; ++day ) {
        if ( day > 1 ) {
            world.NewDay();
            for ( auto & town : towns ) town->ActionNewDay();
            kingdom.ActionNewDayResourceUpdate( {} );
        }
        for ( const auto & action : input.actions ) if ( action.day == day ) {
            auto & town = **std::find_if( towns.begin(), towns.end(), [&]( const auto & t ) { return t->GetIndex() == action.town; } );
            if ( action.kind == P::Kind::Build ) assert( town.BuyBuilding( action.building ) );
            else {
                const auto dwelling = input.start.towns[0].dwellings[action.pool].base;
                assert( town.RecruitMonster( Troop( Monster( town.GetRace(), town.GetActualDwelling( dwelling ) ), action.count ), false ) );
            }
        }
        const auto actual = P::capture( kingdom, day );
        assert( P::funds( actual.start.economy ) == predicted.days[day - 1].afterActions );
    }
    const auto actual = P::capture( kingdom, 7 );
    for ( size_t i = 0; i < 3; ++i ) {
        const auto & a = actual.start.economy.towns[i].troops;
        const auto & b = predicted.end.economy.towns[i].troops;
        assert( a.garrison == b.garrison && a.hut == b.hut && a.pools == b.pools );
        assert( a.state.total == b.state.total && a.state.quotas == b.state.quotas && a.state.used == b.state.used );
        assert( a.state.next == b.state.next && a.state.lossRemainder == b.state.lossRemainder );
        assert( actual.start.towns[i].extraPools == predicted.end.towns[i].extraPools );
        assert( actual.start.towns[i].mask == predicted.end.towns[i].mask );
    }
    std::cout << "RESULT";
    for ( const auto n : P::funds( actual.start.economy ) ) std::cout << ' ' << n;
    for ( const auto & town : actual.start.economy.towns ) std::cout << ' ' << town.troops.state.total << ' ' << town.troops.state.lossRemainder << ' ' << town.troops.garrison;
    std::cout << '\n';
    kingdom.GetCastles().clear();
}
