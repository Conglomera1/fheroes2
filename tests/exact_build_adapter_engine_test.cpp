// C3D5I: real isolated Farm/TG operations. Fault callbacks are synthetic injections.
#include "support/exact_build_fixture_adapter.h"
#include "difficulty.h"
#include "map_format_info.h"
#include "maps_tiles.h"
#include "players.h"
#include "settings.h"
#include <cassert>
#include <iostream>
#include <set>

namespace B = BuildFixture;
namespace F = RecruitFixture;
namespace P = F::P;
namespace V = F::V;
namespace C = F::C;
namespace M = F::M;

int main( int argc, char ** argv )
{
    assert( argc == 4 ); const std::string mode = argv[2], profile = argv[3];
    const std::set<std::string> modes{ "to_tg", "to_farm", "first_farm", "visitor_to_tg", "visitor_to_farm", "progress_to_farm",
        "poor_gold_tg", "poor_wood_tg", "poor_ore_tg", "poor_gold_farm", "poor_wood_farm", "poor_ore_farm", "already", "not_today",
        "missing_farm", "missing_captain", "missing_moat", "disabled_farm", "disabled_tg", "disabled_source_farm", "unsupported", "shape",
        "no_accept", "stale", "forged", "changed_flag", "changed_army", "budget", "coverage", "two_builds", "false_clean", "true_empty",
        "false_effect", "wrong_cost", "wrong_building", "wrong_stock", "wrong_flag", "wrong_army", "record_fault", "throw_after", "reentrant", "pure" };
    const std::set<std::string> profiles{ "default", "custom", "disabled", "extra_requirement", "free", "compatibility" };
    assert( modes.count( mode ) && profiles.count( profile ) );
    Settings::Get().Read( argv[1] ); Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() ); world.generateUninitializedMap( 36 ); world.NewDay();
    const auto & config = Conscription::settings();
    const bool custom = profile == "custom", free = profile == "free";
    assert( config.trainingGold == ( custom ? 4321 : free ? 0 : 2500 ) );
    assert( config.trainingWood == ( custom ? 7 : free ? 0 : 10 ) && config.trainingOre == ( custom ? 13 : free ? 0 : 10 ) );
    assert( config.farmGold == ( custom ? 1234 : free ? 0 : 1500 ) );
    assert( config.farmWood == ( custom ? 3 : 0 ) && config.farmOre == ( custom ? 5 : 0 ) );
    assert( config.enabled == ( profile != "disabled" ) );
    const std::set<std::string> reverse{ "to_farm", "visitor_to_farm", "progress_to_farm", "poor_gold_farm", "poor_wood_farm", "poor_ore_farm", "disabled_farm", "first_farm" };
    const bool farmTarget = reverse.count( mode );
    const bool startTG = ( farmTarget && mode != "first_farm" ) || mode == "already";
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE ); kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 100000 ) );
    constexpr int tile = 150; world.addCastle( tile, static_cast<uint8_t>( Race::KNGT ), PlayerColor::BLUE );
    auto * town = world.getCastle( { tile % 36, tile / 36 } ); assert( town );
    kingdom.GetCastles().push_back( town ); world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
    Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
    meta.builtBuildings = { BUILD_CASTLE, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, BUILD_WELL, BUILD_TAVERN };
    if ( mode != "missing_captain" ) meta.builtBuildings.push_back( BUILD_CAPTAIN );
    if ( startTG ) meta.builtBuildings.push_back( BUILD_TRAINING_GROUNDS );
    else if ( mode != "first_farm" && mode != "missing_farm" ) meta.builtBuildings.push_back( BUILD_WEL2 );
    if ( mode == "disabled_farm" || mode == "disabled_source_farm" ) meta.bannedBuildings.push_back( BUILD_WEL2 );
    if ( mode == "disabled_tg" ) meta.bannedBuildings.push_back( BUILD_TRAINING_GROUNDS );
    town->loadFromResurrectionMap( meta ); town->GetArmy().Reset();
    assert( town->GetArmy().JoinTroop( Monster::PEASANT, 20, false ) );
    assert( town->GetArmy().JoinTroop( Monster::PIKEMAN, 2, false ) );
    if ( mode == "visitor_to_tg" || mode == "visitor_to_farm" ) {
        auto * hero = world.GetHeroes( Heroes::LORDKILBURN ); hero->SetColor( PlayerColor::BLUE ); hero->SetCenter( town->GetCenter() );
        hero->SetModes( Heroes::PATROL ); hero->GetArmy().Clean(); assert( hero->GetArmy().JoinTroop( Monster::PEASANT, 3, false ) );
        assert( hero->GetArmy().GetCountMonsters( Monster::PEASANT ) == 3 );
        kingdom.GetHeroes().push_back( hero ); assert( town->GetHero() == hero );
    }
    if ( mode == "progress_to_farm" ) {
        world.NewDay();
        // Seed a nonzero real training cycle before acceptance; do not simulate it.
        if ( town->conscriptionDaySnapshot().state.total == 0 ) Castle::processDailyConscription( kingdom );
        assert( town->conscriptionDaySnapshot().state.total == 3 );
    }
    town->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
    if ( mode == "not_today" ) town->ResetModes( Castle::ALLOW_TO_BUILD_TODAY );
    if ( mode.rfind( "poor_gold", 0 ) == 0 ) kingdom.OddFundsResource( Funds( Resource::GOLD, kingdom.GetFunds().gold ) );
    if ( mode.rfind( "poor_wood", 0 ) == 0 ) kingdom.OddFundsResource( Funds( Resource::WOOD, kingdom.GetFunds().wood ) );
    if ( mode.rfind( "poor_ore", 0 ) == 0 ) kingdom.OddFundsResource( Funds( Resource::ORE, kingdom.GetFunds().ore ) );
    P::Action action; action.kind = P::Kind::Build; action.town = tile; action.day = world.CountDay(); action.required = true;
    action.building = farmTarget ? BUILD_WEL2 : BUILD_TRAINING_GROUNDS;
    if ( mode == "unsupported" ) action.building = BUILD_MAGEGUILD1;
    if ( mode == "shape" ) action.count = 1;
    C::Program program; program.town = tile; program.pending = { action };
    if ( mode == "two_builds" ) { auto next = action; next.building = BUILD_WEL2; program.pending.push_back( next ); }
    const auto before = B::capture( *town );
    const auto listedCost = F::resources( town->getBuildingCost( action.building ) );
    B::Owner owner( kingdom, *town, 9101 ); M::Budget work; F::Result result; size_t synthetic = 0;
    std::cout << "BUILD_BEGIN" << std::endl;
    if ( mode == "no_accept" ) { result = owner.prepare( work ); assert( result.status == F::Status::Held && owner.pending() == 0 ); }
    else if ( mode == "unsupported" || mode == "shape" ) {
        assert( !owner.acceptProgram( program ) ); result = owner.prepare( work ); assert( result.status == F::Status::Held && owner.pending() == 0 );
    }
    else if ( mode == "pure" ) {
        auto projected = P::capture( kingdom, world.CountDay() + 14 );
        auto checked = B::check( *town, projected.start.towns[0], action, before ); assert( checked.valid );
        assert( B::classify( true, before, checked, checked.expected ) == V::Outcome::Executed ); ++synthetic;
        assert( B::classify( false, before, checked, before ) == V::Outcome::Failed ); ++synthetic;
        assert( B::classify( true, before, checked, before ) == V::Outcome::Uncertain ); ++synthetic;
        assert( B::classify( false, before, checked, checked.expected ) == V::Outcome::Uncertain ); ++synthetic;
        assert( B::classify( true, before, checked, {} ) == V::Outcome::Uncertain ); ++synthetic;
        for ( size_t r = 0; r < 7; ++r ) { auto bad = checked.expected; ++bad.money[r]; assert( B::classify( true, before, checked, bad ) == V::Outcome::Uncertain ); ++synthetic; }
        for ( size_t p = 0; p < 6; ++p ) { auto bad = checked.expected; ++bad.stock[p]; assert( B::classify( true, before, checked, bad ) == V::Outcome::Uncertain ); ++synthetic; }
        auto bad = checked.expected; bad.canBuild = true; assert( B::classify( true, before, checked, bad ) == V::Outcome::Uncertain ); ++synthetic;
        bad = checked.expected; bad.farm = true; assert( B::classify( true, before, checked, bad ) == V::Outcome::Uncertain ); ++synthetic;
        bad = checked.expected; bad.training = false; assert( B::classify( true, before, checked, bad ) == V::Outcome::Uncertain ); ++synthetic;
        bad = checked.expected; ++bad.hutGrowth; assert( B::classify( true, before, checked, bad ) == V::Outcome::Uncertain ); ++synthetic;
        bad = checked.expected; ++bad.tax; assert( B::classify( true, before, checked, bad ) == V::Outcome::Uncertain ); ++synthetic;
        bad = checked.expected; ++bad.income[0]; assert( B::classify( true, before, checked, bad ) == V::Outcome::Uncertain ); ++synthetic;
        bad = checked.expected; ++std::get<0>( bad.progress ); assert( B::classify( true, before, checked, bad ) == V::Outcome::Uncertain ); ++synthetic;
        auto missing = projected.start.towns[0]; missing.catalog.erase( BUILD_TRAINING_GROUNDS );
        assert( !B::check( *town, missing, action, before ).valid ); ++synthetic;
        auto other = action; other.building = DWELLING_MONSTER5; assert( !B::check( *town, projected.start.towns[0], other, before ).valid ); ++synthetic;
    }
    else {
        assert( owner.acceptProgram( program ) && !owner.acceptProgram( program ) );
        if ( mode == "budget" ) work.projections = 0;
        auto preparation = owner.prepare( work, mode != "coverage" ); result = preparation;
        if ( preparation.status == F::Status::Prepared ) {
            assert( preparation.ticket && work.projections == 254 ); const auto ticket = *preparation.ticket; auto supplied = ticket;
            if ( mode == "stale" ) { auto newer = owner.prepare( work ); assert( newer.ticket && newer.ticket->stamp.revision != ticket.stamp.revision ); }
            if ( mode == "forged" ) supplied.action.building = BUILD_WEL2;
            if ( mode == "changed_flag" ) town->ResetModes( Castle::ALLOW_TO_BUILD_TODAY );
            if ( mode == "changed_army" ) assert( town->GetArmy().JoinTroop( Monster::PEASANT, 1, false ) );
            const auto immediatelyBefore = B::capture( *town ); B::Owner::Call fault;
            if ( mode == "false_clean" ) fault = []( Castle &, uint32_t ) { return false; };
            if ( mode == "true_empty" ) fault = []( Castle &, uint32_t ) { return true; };
            if ( mode == "false_effect" ) fault = []( Castle & t, uint32_t b ) { assert( t.BuyBuilding( b ) ); return false; };
            if ( mode == "wrong_cost" ) fault = [&]( Castle & t, uint32_t b ) { assert( t.BuyBuilding( b ) ); kingdom.AddFundsResource( Funds( Resource::GOLD, 1 ) ); return true; };
            if ( mode == "wrong_building" ) fault = []( Castle & t, uint32_t ) { return t.BuyBuilding( BUILD_STATUE ); };
            if ( mode == "wrong_stock" ) fault = []( Castle & t, uint32_t b ) { assert( t.BuyBuilding( b ) ); assert( t.RecruitMonster( Troop( Monster::ARCHER, 1 ), false ) ); return true; };
            if ( mode == "wrong_flag" ) fault = []( Castle & t, uint32_t b ) { assert( t.BuyBuilding( b ) ); t.SetModes( Castle::ALLOW_TO_BUILD_TODAY ); return true; };
            if ( mode == "wrong_army" ) fault = []( Castle & t, uint32_t b ) { assert( t.BuyBuilding( b ) ); assert( t.GetArmy().JoinTroop( Monster::PEASANT, 1, false ) ); return true; };
            if ( mode == "record_fault" ) fault = [&]( Castle & t, uint32_t b ) { assert( t.BuyBuilding( b ) ); owner.invalidateForTest(); return true; };
            if ( mode == "throw_after" ) fault = []( Castle & t, uint32_t b ) -> bool { assert( t.BuyBuilding( b ) ); throw 42; };
            if ( mode == "reentrant" ) fault = [&]( Castle & t, uint32_t b ) { assert( owner.run( ticket ).status == F::Status::Blocked ); return t.BuyBuilding( b ); };
            result = owner.run( supplied, fault );
            const bool changed = mode == "stale" || mode == "forged" || mode == "changed_flag" || mode == "changed_army";
            const bool uncertain = mode == "true_empty" || mode == "false_effect" || mode.rfind( "wrong_", 0 ) == 0 || mode == "record_fault" || mode == "throw_after";
            if ( changed ) { assert( result.status == F::Status::Held && owner.calls() == 0 && owner.pending() == 1 && B::capture( *town ) == immediatelyBefore ); }
            else if ( uncertain ) {
                assert( result.status == F::Status::Blocked && owner.blocked() && owner.calls() == 1 && owner.pending() == 1 );
                const auto after = B::capture( *town );
                assert( owner.run( ticket ).status == F::Status::Blocked && owner.prepare( work ).status == F::Status::Blocked );
                assert( B::capture( *town ) == after && owner.calls() == 1 );
            }
            else if ( mode == "false_clean" ) {
                assert( result.status == F::Status::Failed && owner.pending() == 1 && B::capture( *town ) == before );
                assert( owner.run( ticket ).status == F::Status::Held && owner.calls() == 1 );
                const auto retry = owner.prepare( work ); assert( retry.ticket );
                assert( owner.run( *retry.ticket ).status == F::Status::Executed && owner.calls() == 2 && owner.pending() == 0 );
            }
            else {
                assert( result.status == F::Status::Executed && owner.pending() == 0 && owner.calls() == 1 );
                const auto after = B::capture( *town );
                assert( owner.run( ticket ).status == F::Status::Held && owner.calls() == 1 && B::capture( *town ) == after );
                assert( owner.prepare( work ).status == F::Status::Empty && B::capture( *town ) == after );
                assert( before.stock == after.stock && before.garrison == after.garrison && before.visitorArmy == after.visitorArmy && before.progress == after.progress );
                assert( after.canBuild == false && after.farm == farmTarget && after.training != farmTarget );
            }
        }
        else {
            assert( !preparation.ticket && ( preparation.status == F::Status::Failed || preparation.status == F::Status::Held ) );
            assert( owner.calls() == 0 && B::capture( *town ) == before && owner.pending() == program.pending.size() );
        }
    }
    std::cout << "BUILD_END" << std::endl;
    const auto after = B::capture( *town );
    if ( owner.calls() == 0 && mode != "changed_flag" && mode != "changed_army" ) assert( after == before );
    std::cout << "RESULT profile=" << profile << " mode=" << mode << " status=" << static_cast<int>( result.status ) << " calls=" << owner.calls()
        << " pending=" << owner.pending() << " blocked=" << owner.blocked() << " gold_spent=" << before.money[0] - after.money[0]
        << " wood_spent=" << before.money[1] - after.money[1] << " ore_spent=" << before.money[2] - after.money[2]
        << " farm_before=" << before.farm << " farm_after=" << after.farm << " tg_before=" << before.training << " tg_after=" << after.training
        << " can_build_after=" << after.canBuild << " tax_before=" << before.tax << " tax_after=" << after.tax
        << " income_delta=" << after.income[0] - before.income[0] << " hut_growth_before=" << before.hutGrowth << " hut_growth_after=" << after.hutGrowth
        << " stock_same=" << ( before.stock == after.stock ) << " progress_before=" << std::get<0>( before.progress )
        << " progress_same=" << ( before.progress == after.progress ) << " list_gold=" << listedCost[0] << " list_wood=" << listedCost[1] << " list_ore=" << listedCost[2]
        << " projections_left=" << work.projections << " synthetic_oracles=" << synthetic << '\n';
    kingdom.GetHeroes().clear(); kingdom.GetCastles().clear();
}
