#include "economy_stability.h"
namespace T = AI::EconomyStability;
#include "army_troop.h"
#include "difficulty.h"
#include "map_format_info.h"
#include "players.h"
#include "settings.h"
#include "world.h"
#include <cassert>
#include <iostream>
#include <memory>
namespace S = AI::EconomySearch;
namespace R = AI::EconomyRisk;
namespace G = AI::EconomyCandidates;
namespace P = AI::EconomyProject;
namespace M = AI::EconomyMilitary;
int main( int argc, char ** argv )
{
    assert( argc == 3 ); const std::string mode = argv[2]; Settings::Get().Read( argv[1] ); Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() ); world.InitKingdoms();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE ); std::vector<std::unique_ptr<Castle>> towns;
    for ( int i = 0; i < 3; ++i ) {
        auto town = std::make_unique<Castle>( i + 1, 1, i == 2 ? Race::WRLK : Race::KNGT ); town->SetColor( PlayerColor::BLUE );
        Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
        meta.builtBuildings = { BUILD_CASTLE, BUILD_CAPTAIN, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4,
                               DWELLING_MONSTER5, DWELLING_MONSTER6, i == 1 ? BUILD_TRAINING_GROUNDS : BUILD_WEL2 };
        town->loadFromResurrectionMap( meta ); // Loader fills one normal week; do not assume metadata stock overrides.
        town->GetArmy().JoinTroop( Monster( i == 2 ? Monster::CENTAUR : Monster::PEASANT ), i ? 2 : 40, false );
        town->SetModes( Castle::ALLOW_TO_BUILD_TODAY ); kingdom.GetCastles().push_back( town.get() ); towns.push_back( std::move( town ) );
    }
    kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 100000 ) ); world.NewDay();
    G::Context context; context.base = P::capture( kingdom, 15 ); G::Budget generation;
    auto proposals = G::generate( context, generation ); assert( proposals.valid && !proposals.projects.empty() );
    if ( mode != "generated" ) {
        G::Candidate bundle; bundle.id = 1;
        P::Action build; build.kind = P::Kind::Build; build.day = 1; build.town = towns[0]->GetIndex(); build.building = BUILD_TRAINING_GROUNDS;
        bundle.actions = { build };
        if ( mode == "irrelevant" ) bundle.actions.push_back( G::recruit( towns[2]->GetIndex(), 1, 6, 2 ) );
        else { assert( mode == "productive" ); bundle.actions.push_back( G::recruit( towns[0]->GetIndex(), 1, 11, 4 ) ); }
        proposals.projects = { bundle }; proposals.longerHorizonNeeded = false;
    }
    std::vector<M::Assignment> uses;
    for ( const auto & town : context.base.start.towns ) uses.push_back( { uint64_t( uses.size() + 1 ), town.tile, M::Slot::TownArmy, 1, 15, true, true } );
    M::Budget budget; S::Work work;
    R::Session evaluator( context, uses, [&]( const auto & town, const auto slot ) { return M::localArmyValue( kingdom, town, slot ); }, budget,
                         { {}, true }, {}, { {}, true, {} } );
    const auto before = kingdom.GetFunds();
    std::cout << "OBS_BEGIN" << std::endl;
    const auto stability = T::observe( evaluator, proposals, budget, work, static_cast<int>( kingdom.GetColor() ) );
    const auto & search = stability.search;
    std::cout << "OBS_END" << std::endl;
    assert( stability.selected && stability.certified && !stability.actionable && search.beam.size() <= 8 && search.passes <= 6 && !search.actionable );
    const auto & winner = search.plans[*stability.selected]; assert( winner.score.verified && winner.score.robust );
    if ( mode == "irrelevant" ) { assert( winner.candidate.actions.empty() && stability.proofs.size() == 1 && !stability.proofs[0].decision.accepted ); }
    if ( mode == "productive" ) { assert( !winner.candidate.actions.empty() && stability.proofs.size() == 1 && stability.proofs[0].decision.accepted
            && stability.proofs[0].without.verified && !stability.proofs[0].repairs.empty() ); }
    assert( kingdom.GetFunds() == before && towns[0]->GetArmy().GetCountMonsters( Monster::PEASANT ) == 40 );
    P::Input plan; assert( G::materialize( context, winner.candidate, plan ) ); P::WorkBudget independent;
    const auto replay = P::project( plan, independent ); assert( replay.covered() );
    assert( before.gold - replay.days[0].afterActions[0] == winner.score.immediateGold );
    size_t actions = 0;
    for ( const auto & action : plan.actions ) if ( action.day == 1 ) {
        auto & town = **std::find_if( towns.begin(), towns.end(), [&]( const auto & t ) { return t->GetIndex() == action.town; } );
        if ( action.kind == P::Kind::Build ) assert( town.BuyBuilding( action.building ) );
        else { assert( action.kind == P::Kind::Recruit ); const auto dwelling = context.base.start.towns[0].dwellings[action.pool].base;
            assert( town.RecruitMonster( Troop( Monster( town.GetRace(), town.GetActualDwelling( dwelling ) ), action.count ), false ) ); }
        ++actions;
    }
    const auto actual = P::capture( kingdom, 15 ); assert( P::funds( actual.start.economy ) == replay.days[0].afterActions );
    // One current-day replay; future special-week effects are not simulated here.
    P::Input today = plan; today.throughDay = 1;
    today.actions.erase( std::remove_if( today.actions.begin(), today.actions.end(), []( const auto & a ) { return a.day > 1; } ), today.actions.end() );
    P::WorkBudget todayBudget; const auto expected = P::project( today, todayBudget ); assert( expected.covered() );
    for ( size_t i = 0; i < towns.size(); ++i ) for ( size_t j = 0; j < 5; ++j ) {
        assert( actual.start.towns[i].garrison[j].monster == expected.end.towns[i].garrison[j].monster && actual.start.towns[i].garrison[j].count == expected.end.towns[i].garrison[j].count );
    }
    std::cout << "RESULT mode=" << mode << " plans=" << search.plans.size() << " evaluations=" << search.counts.evaluations << " cache=" << search.counts.cacheHits
              << " proofs=" << stability.proofs.size() << " proof_evals=" << stability.evaluations << " passes=" << search.passes << " day1_actions=" << actions << " immediate=" << winner.score.immediateGold << " remaining=" << budget.projections << '\n';
    kingdom.GetCastles().clear();
}
