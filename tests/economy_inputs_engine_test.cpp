#include "economy_inputs.h"
#include "ai_planner.h"
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
namespace P = AI::EconomyProject;
namespace M = AI::EconomyMilitary;
namespace D = AI::EconomyDefense;
namespace R = AI::EconomyRisk;
int main( int argc, char ** argv )
{
    assert( argc == 3 ); const std::string mode = argv[2]; const bool routeKnown = mode != "unknown"; const bool hasVisitor = mode == "visitor";
    Settings::Get().Read( argv[1] ); Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() ); world.generateUninitializedMap( 36 ); world.NewDay();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 100000 ) );
    std::vector<Castle *> towns;
    for ( int i = 0; i < 2; ++i ) {
        const int tile = 150 + i * 180;
        world.addCastle( tile, static_cast<uint8_t>( i ? Race::WRLK : Race::KNGT ), PlayerColor::BLUE );
        auto * town = world.getCastle( { tile % 36, tile / 36 } ); kingdom.GetCastles().push_back( town );
        world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
        Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
        meta.builtBuildings = { BUILD_CASTLE, BUILD_CAPTAIN, BUILD_WEL2, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4 };
        town->loadFromResurrectionMap( meta ); town->GetArmy().Reset(); town->GetArmy().JoinTroop( i ? Monster::CENTAUR : Monster::PEASANT, 40, false );
        town->SetModes( Castle::ALLOW_TO_BUILD_TODAY ); towns.push_back( town );
    }
    auto & visitor = *world.GetHeroes( Heroes::LORDKILBURN ); visitor.SetColor( PlayerColor::BLUE ); visitor.SetCenter( hasVisitor ? towns[0]->GetCenter() : fheroes2::Point{ 3, 3 } );
    visitor.GetArmy().Reset(); visitor.GetArmy().JoinTroop( Monster::ARCHER, 20, false ); assert( ( towns[0]->GetHero() == &visitor ) == hasVisitor );
    auto & attacker = *world.GetHeroes( Heroes::THUNDAX ); attacker.SetColor( PlayerColor::RED ); attacker.SetCenter( { 4, 2 } );
    attacker.GetArmy().Reset(); attacker.GetArmy().JoinTroop( Monster::PIKEMAN, 10, false );
    AI::EnemyArmy observed( attacker.GetIndex(), &attacker, attacker.GetArmy().GetStrength(), 1000 );
    const auto before = P::capture( kingdom, 15 ); const auto money = kingdom.GetFunds(); I::Work work;
    std::cout << "OBS_BEGIN" << std::endl;
    const auto frame = I::capture( kingdom, 15, { observed }, { towns[1]->GetIndex(), towns[0]->GetIndex() }, true,
        [&]( const I::Enemy & e, int32_t ) { assert( e.observation.enemyHero == attacker.GetID() ); return routeKnown ? 1800U : 0U; }, work );
    I::log( frame, static_cast<int>( kingdom.GetColor() ), work );
    std::cout << "OBS_END" << std::endl;
    assert( frame.valid && !frame.actionable && frame.queried == 2 && work.paths == 126 && frame.assignments.size() == 2 );
    assert( frame.context.base.config.goldPerPeasant == Conscription::settings().goldPerPeasant );
    assert( hasVisitor ? ( frame.context.base.start.towns[0].hero == visitor.GetID() && P::count( frame.context.base.start.towns[0].visitor, Monster::ARCHER ) == 20 ) : frame.context.base.start.towns[0].hero < 0 );
    assert( frame.defenses.threats.size() == 2 && frame.defenses.coverageComplete == ( routeKnown && !hasVisitor ) );
    M::Budget budget;
    R::Session evaluator( frame.context, frame.assignments, [&]( const auto & town, auto slot ) { return M::localArmyValue( kingdom, town, slot ); }, budget,
        frame.defenses, [&]( const auto & town, const auto & threat ) { return D::localDefenseValue( kingdom, town, threat ); }, frame.risks );
    AI::EconomyCandidates::Candidate nothing; nothing.id = 1; const auto result = evaluator.evaluate( nothing );
    if ( routeKnown && !hasVisitor ) {
        assert( frame.risks.scenarios.size() == 2 && frame.defenses.threats[0].lastSafeDay == 2 );
        assert( result.status == R::Status::Scored && result.robustUtility == 0 && result.variants.size() == 3 );
    }
    else assert( result.status == R::Status::Incomplete && !result.robustUtility );
    const auto after = P::capture( kingdom, 15 ); assert( kingdom.GetFunds() == money && ( ( towns[0]->GetHero() == &visitor ) == hasVisitor ) );
    for ( size_t t = 0; t < 2; ++t ) for ( size_t s = 0; s < 5; ++s ) {
        assert( before.start.towns[t].garrison[s].monster == after.start.towns[t].garrison[s].monster && before.start.towns[t].garrison[s].count == after.start.towns[t].garrison[s].count );
        assert( before.start.towns[t].visitor[s].monster == after.start.towns[t].visitor[s].monster && before.start.towns[t].visitor[s].count == after.start.towns[t].visitor[s].count );
    }
    std::cout << "RESULT " << mode << " threats=" << frame.defenses.threats.size() << " queries=" << frame.queried
        << " variants=" << result.variants.size() << " projections_left=" << budget.projections << '\n';
    kingdom.GetCastles().clear();
}
