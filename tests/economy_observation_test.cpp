#include "economy_forecast.h"
#include "difficulty.h"
#include "kingdom.h"
#include "map_format_info.h"
#include "players.h"
#include "race.h"
#include "settings.h"
#include "world.h"
#include "ai_common.h"
#include <cassert>
#include <iostream>
#include <memory>

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
        auto town = std::make_unique<Castle>( i + 1, 1, Race::KNGT );
        town->SetColor( PlayerColor::BLUE );
        Maps::Map_Format::CastleMetadata metadata;
        metadata.customBuildings = true;
        metadata.builtBuildings = { BUILD_CASTLE, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4,
                                   i == 0 ? BUILD_WEL2 : BUILD_TRAINING_GROUNDS, BUILD_MARKETPLACE };
        metadata.availableToHireMonsterCount = { 20, 0, 0, 0, 0, 0 };
        town->loadFromResurrectionMap( metadata );
        if ( i < 2 ) town->GetArmy().JoinTroop( Monster( Monster::PEASANT ), i == 0 ? 100 : 2, false );
        kingdom.GetCastles().push_back( town.get() );
        towns.push_back( std::move( town ) );
    }
    kingdom.OddFundsResource( kingdom.GetFunds() );
    kingdom.AddFundsResource( Funds( Resource::GOLD, 340 ) );
    kingdom.AddFundsResource( kingdom.GetFunds() ); // Exercise aliased credit logging too.
    world.NewDay(); // Day 1 skips daily income and conscription.
    for ( int i = 0; i < 6; ++i ) {
        const auto before = kingdom.economySnapshot();
        std::vector<Economy::ScheduledEvent> knownEvents;
        if ( i == 1 ) {
            EventDate event;
            event.firstOccurrenceDay = 3;
            event.colors = Color::allPlayerColors();
            event.isApplicableForAIPlayers = true;
            event.resource.gold = -100;
            event.resource.wood = 7;
            event.resource.ore = 8;
            event.resource.mercury = 9;
            event.resource.sulfur = 10;
            event.resource.crystal = 11;
            event.resource.gems = 12;
            world.AddEventDate( event );
            knownEvents.push_back( { 3, -100, { 7, 8, 9, 10, 11, 12 } } );
        }
        const auto predicted = Economy::forecast( before, Conscription::settings(), 1, 200000, knownEvents );
        assert( predicted.complete );
        const auto unchanged = kingdom.economySnapshot();
        assert( unchanged.gold == before.gold && unchanged.towns[1].troops.state.total == before.towns[1].troops.state.total );
        world.NewDay();
        kingdom.ActionNewDayResourceUpdate( {} );
        const auto actual = kingdom.economySnapshot();
        assert( actual.gold == predicted.end.gold );
        assert( actual.materials == predicted.end.materials );
        assert( Economy::taxIncome( actual ) == Economy::taxIncome( predicted.end ) );
        for ( size_t n = 0; n < actual.towns.size(); ++n ) {
            const auto & a = actual.towns[n].troops;
            const auto & b = predicted.end.towns[n].troops;
            assert( a.garrison == b.garrison && a.hut == b.hut && a.pools == b.pools );
            assert( a.state.total == b.state.total && a.state.quotas == b.state.quotas && a.state.used == b.state.used );
            assert( a.state.next == b.state.next && a.state.lossRemainder == b.state.lossRemainder );
        }
    }
    kingdom.AddFundsResource( Funds( Resource::GOLD, 100000 ) );
    towns[0]->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
    assert( towns[0]->BuyBuilding( BUILD_WELL ) );
    assert( towns[0]->RecruitMonster( Troop( Monster( Monster::PEASANT ), 1 ), false ) );
    assert( AI::tradeAtMarketplace( kingdom, Funds( Resource::GEMS, 13 ) ) );
    const auto final = kingdom.economySnapshot();
    std::cout << "RESULT " << final.gold;
    for ( const auto & town : final.towns ) std::cout << ' ' << town.troops.state.total << ' ' << town.troops.hut << ' ' << town.troops.garrison
                                                  << ' ' << town.troops.state.lossRemainder;
    std::cout << '\n';
    kingdom.GetCastles().clear();
}
