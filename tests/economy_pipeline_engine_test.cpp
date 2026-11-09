#include "economy_pipeline.h"
namespace O = AI::EconomyPipeline;
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
    assert( argc == 3 ); const std::string mode = argv[2]; const bool routeKnown = mode != "unknown"; const bool hasVisitor = mode == "visitor" || mode == "patrol";
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
    if ( mode == "patrol" ) { visitor.SetModes( Heroes::PATROL ); assert( visitor.GetPatrolDistance() == 0 ); }
    visitor.GetArmy().Reset(); visitor.GetArmy().JoinTroop( Monster::ARCHER, 20, false ); assert( ( towns[0]->GetHero() == &visitor ) == hasVisitor );
    auto & attacker = *world.GetHeroes( Heroes::THUNDAX ); attacker.SetColor( PlayerColor::RED ); attacker.SetCenter( { 4, 2 } );
    attacker.GetArmy().Reset(); attacker.GetArmy().JoinTroop( Monster::PIKEMAN, 10, false );
    AI::EnemyArmy observed( attacker.GetIndex(), &attacker, attacker.GetArmy().GetStrength(), 1000 );
    const auto before = P::capture( kingdom, 15 ); const auto money = kingdom.GetFunds(); O::Work work;
    std::cout << "OBS_BEGIN" << std::endl;
    const auto frame = I::capture( kingdom, 15, { observed }, { towns[1]->GetIndex(), towns[0]->GetIndex() }, true,
        [&]( const I::Enemy & e, int32_t ) { assert( e.observation.enemyHero == attacker.GetID() ); return routeKnown ? 1800U : 0U; }, work.paths );
    I::log( frame, static_cast<int>( kingdom.GetColor() ), work.paths );
    const auto result = O::observe( frame, kingdom, work );
    std::cout << "OBS_END" << std::endl;
    assert( frame.valid && !frame.actionable && frame.queried == 2 && work.paths.paths == 126 && frame.assignments.size() == 2 );
    assert( frame.context.base.config.goldPerPeasant == Conscription::settings().goldPerPeasant );
    assert( hasVisitor ? ( frame.context.base.start.towns[0].hero == visitor.GetID() && P::count( frame.context.base.start.towns[0].visitor, Monster::ARCHER ) == 20 ) : frame.context.base.start.towns[0].hero < 0 );
    assert( frame.defenses.threats.size() == 2 && frame.defenses.coverageComplete == ( routeKnown && ( !hasVisitor || mode == "patrol" ) ) );
    if ( mode == "known" || mode == "patrol" ) {
        assert( result.searched && result.selection.certified && result.selection.selected && !result.actionable );
        assert( work.evaluation.projections < 256 && result.preparation.released.empty() );
        assert( work.evaluation.projections == 64 );
        const auto & selectedActions = result.selection.search.plans[*result.selection.selected].candidate.actions;
        const bool containsTransfer = std::any_of( selectedActions.begin(), selectedActions.end(),
            []( const auto & a ) { return a.kind != P::Kind::Build && a.kind != P::Kind::Recruit; } );
        assert( result.registry.registry && result.registry.status == AI::EconomyRegistryObservation::Status::Staged
                && result.registry.registry->pending() > 0 && result.registry.registry->view().permissions.empty() );
        assert( result.registry.links.size() == result.registry.registry->pending() );
        if ( containsTransfer ) {
            assert( mode == "patrol" ); const auto registered = result.registry.registry->view();
            for ( const auto & a : registered.context->base.actions ) if ( a.kind == P::Kind::ToGarrison || a.kind == P::Kind::ToVisitor )
                assert( a.town == towns[0]->GetIndex() && result.registry.registry->boundVisitor(a.project) == visitor.GetID() );
        }
        if ( result.widened ) assert( result.frame.context.base.throughDay == 29 && work.paths.paths == 126 );
    }
    else { assert( !result.searched && result.status == O::Status::Held && work.evaluation.projections == 256 && !result.registry.registry ); }
    const auto after = P::capture( kingdom, 15 ); assert( kingdom.GetFunds() == money && ( ( towns[0]->GetHero() == &visitor ) == hasVisitor ) );
    for ( size_t t = 0; t < 2; ++t ) for ( size_t s = 0; s < 5; ++s ) {
        assert( before.start.towns[t].garrison[s].monster == after.start.towns[t].garrison[s].monster && before.start.towns[t].garrison[s].count == after.start.towns[t].garrison[s].count );
        assert( before.start.towns[t].visitor[s].monster == after.start.towns[t].visitor[s].monster && before.start.towns[t].visitor[s].count == after.start.towns[t].visitor[s].count );
    }
    std::cout << "RESULT " << mode << " threats=" << frame.defenses.threats.size() << " queries=" << frame.queried
        << " searched=" << result.searched << " widened=" << result.widened << " proposals=" << result.proposals.projects.size() << " projections_left=" << work.evaluation.projections
        << " registry=" << AI::EconomyRegistryObservation::name( result.registry.status ) << '\n';
    kingdom.GetCastles().clear();
}
