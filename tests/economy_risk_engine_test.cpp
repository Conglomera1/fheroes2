#include "economy_risk.h"
#include "army_troop.h"
#include "difficulty.h"
#include "map_format_info.h"
#include "players.h"
#include "settings.h"
#include "world.h"
#include <cassert>
#include <iostream>
#include <memory>
namespace R = AI::EconomyRisk;
namespace M = AI::EconomyMilitary;
namespace P = AI::EconomyProject;
namespace G = AI::EconomyCandidates;
namespace D = AI::EconomyDefense;
int main( int argc, char ** argv )
{
    assert( argc == 2 ); Settings::Get().Read( argv[1] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL ); Settings::Get().GetPlayers().Init( Color::allPlayerColors() ); world.InitKingdoms();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    std::vector<std::unique_ptr<Castle>> towns;
    for ( int i = 0; i < 2; ++i ) {
        auto town = std::make_unique<Castle>( i + 1, 1, Race::KNGT ); town->SetColor( PlayerColor::BLUE );
        Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
        meta.builtBuildings = { BUILD_CASTLE, BUILD_CAPTAIN, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4,
                               i ? BUILD_TRAINING_GROUNDS : BUILD_WEL2 };
        meta.availableToHireMonsterCount = { 50, 50, 50, 50, 50, 50 };
        town->loadFromResurrectionMap( meta ); town->GetArmy().JoinTroop( Monster::PEASANT, 40, false ); town->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
        kingdom.GetCastles().push_back( town.get() ); towns.push_back( std::move( town ) );
    }
    kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 100000 ) ); world.NewDay();
    auto input = P::capture( kingdom, 7 );
    assert( input.start.towns[0].incomeAttributionKnown && input.start.towns[0].recurringIncome[0] == 1000 );
    P::Action statue; statue.id = statue.project = 1; statue.day = 2; statue.town = towns[0]->GetIndex(); statue.building = BUILD_STATUE;
    P::Action recruit; recruit.id = recruit.project = 2; recruit.day = 4; recruit.town = towns[1]->GetIndex(); recruit.kind = P::Kind::Recruit; recruit.pool = 1; recruit.count = 2;
    input.actions = { statue, recruit }; input.losses = { { towns[0]->GetIndex(), 3 } };
    P::WorkBudget budget;
    const auto projected = P::project( input, budget ); assert( projected.covered() );
    // Audit observation and its JSON before mutating the real fixture to replay the capture.
    G::Context context; context.base = P::capture( kingdom, 15 );
    D::Threat threat; threat.id = 1; threat.town = towns[0]->GetIndex(); threat.enemyTile = 9; threat.enemyStrength = 100;
    threat.known = threat.reachable = true; threat.lastSafeDay = 2;
    D::Registry defenses{ { threat }, true };
    R::Registry risks{ { { 1, R::Focus::Income, input.losses, { 1 }, false, "Farm captured" } }, true, {} };
    std::vector<M::Assignment> uses;
    for ( const auto & t : context.base.start.towns ) uses.push_back( { uint64_t( uses.size() + 1 ), t.tile, M::Slot::TownArmy, 1, 15, true, true } );
    M::Budget riskBudget;
    R::Session session( context, uses, [&]( const auto & town, const auto slot ) { return M::localArmyValue( kingdom, town, slot ); }, riskBudget,
                       defenses, [&]( const auto & town, const auto & attack ) { return D::localDefenseValue( kingdom, town, attack ); }, risks );
    G::Candidate candidate; candidate.id = 1; candidate.actions = { G::recruit( towns[1]->GetIndex(), 1, 2, 4 ) };
    const auto fundsBefore = kingdom.GetFunds();
    std::cout << "OBS_BEGIN" << std::endl;
    const auto evaluation = R::evaluateObserved( session, candidate, static_cast<int>( kingdom.GetColor() ), riskBudget );
    std::cout << "OBS_END" << std::endl;
    assert( evaluation.status == R::Status::Scored && evaluation.robustUtility && evaluation.variants.size() == 2 );
    assert( *evaluation.robustUtility <= *evaluation.variants[0].evaluation.utility );
    assert( kingdom.GetFunds() == fundsBefore && towns[0]->GetArmy().GetCountMonsters( Monster::PEASANT ) == 40 && towns[0]->isBuild( BUILD_WEL2 ) );
    // Real ownership loss before our day-3 income; the Statue built day 2 must disappear from future income too.
    size_t checks = 0;
    for ( uint32_t day = 1; day <= 7; ++day ) {
        if ( day > 1 ) {
            world.NewDay();
            if ( day == 3 ) { towns[0]->SetColor( PlayerColor::RED ); kingdom.GetCastles().erase( kingdom.GetCastles().begin() ); }
            for ( auto * town : kingdom.GetCastles() ) town->ActionNewDay();
            kingdom.ActionNewDayResourceUpdate( {} );
        }
        if ( day == 2 ) assert( towns[0]->BuyBuilding( BUILD_STATUE ) );
        if ( day == 4 ) assert( towns[1]->RecruitMonster( Troop( Monster::ARCHER, 2 ), false ) );
        const auto actual = P::capture( kingdom, day );
        assert( P::funds( actual.start.economy ) == projected.days[day - 1].afterActions ); ++checks;
    }
    const auto actual = P::capture( kingdom, 7 );
    assert( !projected.end.towns[0].available && !projected.end.economy.towns[0].training && !projected.end.economy.towns[0].farm ); ++checks;
    const auto & a = actual.start.economy.towns[0].troops;
    const auto & b = projected.end.economy.towns[1].troops;
    assert( a.garrison == b.garrison && a.hut == b.hut && a.pools == b.pools && a.state.total == b.state.total && a.state.lossRemainder == b.state.lossRemainder ); ++checks;
    assert( actual.start.economy.nonTaxIncome == projected.end.economy.nonTaxIncome ); ++checks;
    std::cout << "RESULT checks=" << checks;
    for ( const auto n : P::funds( actual.start.economy ) ) std::cout << ' ' << n;
    std::cout << ' ' << a.state.total << ' ' << a.state.lossRemainder << '\n';
    kingdom.GetCastles().clear();
}
