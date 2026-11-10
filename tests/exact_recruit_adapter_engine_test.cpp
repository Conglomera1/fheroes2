// C3D5H native fixture. Fault callbacks and synthetic oracles are labelled below.
#include "support/exact_recruit_fixture_adapter.h"
#include "difficulty.h"
#include "map_format_info.h"
#include "maps_tiles.h"
#include "players.h"
#include "settings.h"
#include <cassert>
#include <iostream>
#include <set>

namespace F = RecruitFixture;
namespace P = F::P;
namespace V = F::V;
namespace C = F::C;
namespace M = F::M;

int main( int argc, char ** argv )
{
    assert( argc == 3 );
    const std::string mode = argv[2];
    const std::set<std::string> modes{ "exact", "merge", "visitor", "visitor_merge", "both_full", "full", "overstock", "empty", "poor",
        "zero", "pool", "unbuilt", "upgrade", "overflow", "visitor_overflow", "no_accept", "legacy", "foreign", "stale", "forged", "order",
        "changed_funds", "changed_visitor", "changed_mission", "changed_role", "identity", "budget", "coverage", "two", "false_clean", "true_empty", "false_effect",
        "wrong_cost", "wrong_stock", "wrong_destination", "slot_reorder", "record_fault", "throw_after", "reentrant", "pure" };
    assert( modes.count( mode ) );
    Settings::Get().Read( argv[1] ); Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() ); world.generateUninitializedMap( 36 ); world.NewDay();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 100000 ) );
    constexpr int tile = 150;
    world.addCastle( tile, static_cast<uint8_t>( Race::KNGT ), PlayerColor::BLUE );
    auto * town = world.getCastle( { tile % 36, tile / 36 } ); assert( town );
    kingdom.GetCastles().push_back( town ); world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
    Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true; meta.builtBuildings = { BUILD_CASTLE, DWELLING_MONSTER1 };
    town->loadFromResurrectionMap( meta ); town->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
    assert( town->BuyBuilding( DWELLING_MONSTER2 ) );
    town->GetArmy().Reset(); assert( town->GetArmy().JoinTroop( Monster::PEASANT, 1, false ) );
    const auto fill = []( Army & army ) {
        army.Reset();
        for ( const int unit : { Monster::PEASANT, Monster::PIKEMAN, Monster::SWORDSMAN, Monster::CAVALRY, Monster::PALADIN } )
            assert( army.JoinTroop( unit, 1, false ) );
    };
    const bool hasVisitor = mode == "visitor" || mode == "visitor_merge" || mode == "both_full" || mode == "visitor_overflow"
        || mode == "changed_visitor" || mode == "changed_mission" || mode == "changed_role" || mode == "wrong_destination";
    Heroes * visitor = nullptr;
    if ( hasVisitor ) {
        visitor = world.GetHeroes( Heroes::LORDKILBURN ); visitor->SetColor( PlayerColor::BLUE ); visitor->SetCenter( town->GetCenter() );
        visitor->SetModes( Heroes::PATROL ); assert( visitor->GetPatrolDistance() == 0 );
        visitor->GetArmy().Reset(); assert( visitor->GetArmy().JoinTroop( Monster::PEASANT, 1, false ) );
        kingdom.GetHeroes().push_back( visitor ); assert( town->GetHero() == visitor );
    }
    if ( mode == "full" || mode == "visitor" || mode == "visitor_merge" || mode == "both_full" || mode == "visitor_overflow" ) fill( town->GetArmy() );
    if ( mode == "both_full" ) fill( visitor->GetArmy() );
    if ( mode == "merge" ) assert( town->GetArmy().JoinTroop( Monster::ARCHER, 2, false ) );
    if ( mode == "visitor_merge" ) assert( visitor->GetArmy().JoinTroop( Monster::ARCHER, 2, false ) );
    if ( mode == "overflow" ) assert( town->GetArmy().JoinTroop( Monster::ARCHER, UINT32_MAX, false ) );
    if ( mode == "visitor_overflow" ) assert( visitor->GetArmy().JoinTroop( Monster::ARCHER, UINT32_MAX, false ) );
    if ( mode == "empty" ) assert( town->RecruitMonster( Troop( Monster::ARCHER, 8 ), false ) );
    if ( mode == "poor" ) kingdom.OddFundsResource( kingdom.GetFunds() );
    if ( mode == "upgrade" ) {
        for ( const auto building : { BUILD_WELL, BUILD_TAVERN, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_UPGRADE2 } ) {
            town->SetModes( Castle::ALLOW_TO_BUILD_TODAY ); assert( town->BuyBuilding( building ) );
        }
    }
    C::Program program; program.town = tile;
    P::Action action; action.kind = P::Kind::Recruit; action.town = tile; action.day = world.CountDay(); action.pool = 1; action.count = 8; action.required = true;
    if ( mode == "overstock" ) action.count = 9;
    if ( mode == "empty" ) action.count = 1;
    if ( mode == "zero" ) action.count = 0;
    if ( mode == "pool" ) action.pool = 6;
    if ( mode == "unbuilt" ) action.pool = 2;
    if ( mode == "two" || mode == "order" ) action.count = 3;
    program.pending = { action };
    if ( mode == "two" || mode == "order" ) { auto second = action; second.count = 2; program.pending.push_back( second ); }
    if ( mode == "legacy" ) program.source = C::Source::LegacyHint;

    F::Owner owner( kingdom, *town, 9001 );
    const auto before = F::capture( *town );
    M::Budget work;
    size_t pureChecks = 0;
    F::Result result;
    std::cout << "ADAPTER_BEGIN" << std::endl;
    if ( mode == "no_accept" ) {
        result = owner.prepare( work ); assert( result.status == F::Status::Held && !result.ticket && owner.pending() == 0 );
    }
    else if ( mode == "zero" || mode == "pool" || mode == "legacy" ) {
        assert( !owner.acceptProgram( program ) ); assert( owner.pending() == 0 );
        result = owner.prepare( work ); assert( result.status == F::Status::Held );
    }
    else if ( mode == "pure" ) {
        // Synthetic classifier/arithmetic tests, independently specified expectations.
        auto context = P::capture( kingdom, world.CountDay() + 14 );
        const auto checked = F::check( *town, context.start.towns[0], action, before ); assert( checked.valid );
        assert( F::classify( true, before, checked, checked.expected ) == V::Outcome::Executed ); ++pureChecks;
        assert( F::classify( false, before, checked, before ) == V::Outcome::Failed ); ++pureChecks;
        assert( F::classify( true, before, checked, before ) == V::Outcome::Uncertain ); ++pureChecks;
        assert( F::classify( false, before, checked, checked.expected ) == V::Outcome::Uncertain ); ++pureChecks;
        assert( F::classify( true, before, checked, {} ) == V::Outcome::Uncertain ); ++pureChecks;
        assert( F::classify( false, before, checked, {} ) == V::Outcome::Uncertain ); ++pureChecks;
        for ( size_t r = 0; r < 7; ++r ) {
            F::Resources unit{}, funds{}, total{}; unit[r] = 2; funds[r] = 5;
            assert( F::checkedCost( unit, 2, funds, total ) && total[r] == 4 ); ++pureChecks;
            assert( !F::checkedCost( unit, 3, funds, total ) ); ++pureChecks;
            funds[r] = INT32_MAX; unit[r] = INT32_MAX;
            assert( !F::checkedCost( unit, 2, funds, total ) ); ++pureChecks;
            auto wrong = checked.expected; ++wrong.money[r];
            assert( F::classify( true, before, checked, wrong ) == V::Outcome::Uncertain ); ++pureChecks;
        }
        auto wrong = checked.expected; ++wrong.stock[0]; assert( F::classify( true, before, checked, wrong ) == V::Outcome::Uncertain ); ++pureChecks;
        wrong = checked.expected; ++wrong.visitor; assert( F::classify( true, before, checked, wrong ) == V::Outcome::Uncertain ); ++pureChecks;
        auto projected = context.start.towns[0]; projected.dwellings[1].basic.id = Monster::ORC;
        assert( !F::check( *town, projected, action, before ).valid ); ++pureChecks;
        projected = context.start.towns[0]; ++projected.dwellings[1].basic.price[0];
        assert( !F::check( *town, projected, action, before ).valid ); ++pureChecks;
    }
    else {
        assert( owner.acceptProgram( program ) );
        assert( !owner.acceptProgram( program ) ); // No extension/reacceptance, even before execution.
        if ( mode == "budget" ) work.projections = 0;
        auto preparation = owner.prepare( work, mode != "coverage" );
        const bool refused = mode == "both_full" || mode == "full" || mode == "overstock" || mode == "empty" || mode == "poor"
            || mode == "unbuilt" || mode == "overflow" || mode == "visitor_overflow";
        if ( refused ) {
            assert( preparation.status == F::Status::Failed && !preparation.ticket && owner.pending() == 1 ); result = preparation;
        }
        else if ( mode == "budget" || mode == "coverage" ) {
            assert( preparation.status == F::Status::Held && !preparation.ticket && owner.pending() == 1 ); result = preparation;
        }
        else {
            assert( preparation.status == F::Status::Prepared && preparation.ticket );
            assert( work.projections == 254 ); // Real common Recourse/Risk baseline + reference.
            const auto ticket = *preparation.ticket;
            if ( mode == "identity" ) {
                for ( int field = 0; field < 12; ++field ) {
                    auto bad = ticket;
                    switch ( field ) {
                    case 0: ++bad.registry; break;
                    case 1: ++bad.stamp.player; break;
                    case 2: ++bad.stamp.day; break;
                    case 3: ++bad.stamp.revision; break;
                    case 4: ++bad.visitor; break;
                    case 5: ++bad.action.project; break;
                    case 6: ++bad.action.id; break;
                    case 7: ++bad.action.town; break;
                    case 8: ++bad.action.pool; break;
                    case 9: bad.action.kind = P::Kind::Build; break;
                    case 10: ++bad.action.count; break;
                    default: bad.action.required = !bad.action.required; break;
                    }
                    assert( owner.run( bad ).status == F::Status::Held && owner.calls() == 0 && owner.pending() == 1 );
                    assert( F::capture( *town ) == before );
                }
            }
            auto supplied = ticket;
            if ( mode == "foreign" ) ++supplied.registry;
            if ( mode == "forged" ) ++supplied.action.count;
            if ( mode == "order" ) ++supplied.action.id;
            if ( mode == "stale" ) { auto newer = owner.prepare( work ); assert( newer.ticket && newer.ticket->stamp.revision != ticket.stamp.revision ); }
            if ( mode == "changed_funds" ) kingdom.AddFundsResource( Funds( Resource::GOLD, 1 ) );
            if ( mode == "changed_mission" ) visitor->ResetModes( Heroes::PATROL );
            if ( mode == "changed_role" ) visitor->setAIRole( Heroes::Role::COURIER );
            if ( mode == "changed_visitor" ) {
                visitor->SetCenter( { 3, 3 } );
                auto * replacement = world.GetHeroes( Heroes::SIRGALLANTH ); replacement->SetColor( PlayerColor::BLUE );
                replacement->SetCenter( town->GetCenter() ); replacement->SetModes( Heroes::PATROL );
                replacement->GetArmy().Reset(); assert( replacement->GetArmy().JoinTroop( Monster::PEASANT, 1, false ) );
                kingdom.GetHeroes().push_back( replacement ); assert( town->GetHero() == replacement );
            }
            const auto immediatelyBefore = F::capture( *town );
            F::Owner::Call fault;
            if ( mode == "false_clean" ) fault = []( Castle &, const Troop & ) { return false; };
            if ( mode == "true_empty" ) fault = []( Castle &, const Troop & ) { return true; };
            if ( mode == "false_effect" ) fault = []( Castle & t, const Troop & q ) { assert( t.RecruitMonster( q, false ) ); return false; };
            if ( mode == "wrong_cost" ) fault = [&]( Castle & t, const Troop & q ) { assert( t.RecruitMonster( q, false ) ); kingdom.AddFundsResource( Funds( Resource::GOLD, 1 ) ); return true; };
            if ( mode == "wrong_stock" ) fault = []( Castle & t, const Troop & q ) { return t.RecruitMonster( Troop( q.GetMonster(), 1 ), false ); };
            if ( mode == "wrong_destination" ) fault = [&]( Castle & t, const Troop & q ) {
                assert( t.RecruitMonster( q, false ) );
                for ( size_t i = 0; i < t.GetArmy().Size(); ++i ) if ( t.GetArmy().GetTroop( i )->GetID() == Monster::ARCHER ) t.GetArmy().GetTroop( i )->Reset();
                assert( visitor->GetArmy().JoinTroop( Monster::ARCHER, 8, false ) ); return true;
            };
            if ( mode == "slot_reorder" ) fault = []( Castle & t, const Troop & q ) {
                assert( t.RecruitMonster( q, false ) ); const Troop first = *t.GetArmy().GetTroop( 0 );
                t.GetArmy().GetTroop( 0 )->Set( *t.GetArmy().GetTroop( 1 ) ); t.GetArmy().GetTroop( 1 )->Set( first ); return true;
            };
            if ( mode == "record_fault" ) fault = [&]( Castle & t, const Troop & q ) { assert( t.RecruitMonster( q, false ) ); owner.invalidateForTest(); return true; };
            if ( mode == "throw_after" ) fault = []( Castle & t, const Troop & q ) -> bool { assert( t.RecruitMonster( q, false ) ); throw 42; };
            if ( mode == "reentrant" ) fault = [&]( Castle & t, const Troop & q ) { assert( owner.run( ticket ).status == F::Status::Blocked ); return t.RecruitMonster( q, false ); };
            result = owner.run( supplied, fault );
            const bool rejected = mode == "foreign" || mode == "stale" || mode == "forged" || mode == "order"
                || mode == "changed_funds" || mode == "changed_visitor" || mode == "changed_mission" || mode == "changed_role";
            const bool uncertain = mode == "true_empty" || mode == "false_effect" || mode == "wrong_cost" || mode == "wrong_stock"
                || mode == "wrong_destination" || mode == "slot_reorder" || mode == "record_fault" || mode == "throw_after";
            if ( rejected ) {
                assert( result.status == F::Status::Held && owner.calls() == 0 && F::capture( *town ) == immediatelyBefore );
                assert( owner.pending() == ( mode == "order" ? 2 : 1 ) );
            }
            else if ( uncertain ) {
                assert( result.status == F::Status::Blocked && owner.blocked() && owner.calls() == 1 && owner.pending() == 1 );
                const auto after = F::capture( *town );
                assert( owner.run( ticket ).status == F::Status::Blocked );
                assert( owner.prepare( work ).status == F::Status::Blocked );
                assert( F::capture( *town ) == after && owner.calls() == 1 );
            }
            else if ( mode == "false_clean" ) {
                assert( result.status == F::Status::Failed && owner.pending() == 1 && F::capture( *town ) == before );
                assert( owner.run( ticket ).status == F::Status::Held && owner.calls() == 1 );
                auto retry = owner.prepare( work ); assert( retry.ticket && retry.ticket->stamp.revision != ticket.stamp.revision );
                // Explicit next invocation after re-verification; no automatic retry.
                assert( owner.run( *retry.ticket ).status == F::Status::Executed && owner.pending() == 0 && owner.calls() == 2 );
            }
            else {
                assert( result.status == F::Status::Executed && !owner.blocked() && owner.calls() == 1 );
                assert( owner.pending() == ( mode == "two" ? 1 : 0 ) );
                const auto after = F::capture( *town );
                assert( owner.run( ticket ).status == F::Status::Held && F::capture( *town ) == after && owner.calls() == 1 );
                auto refreshed = owner.prepare( work );
                if ( mode == "two" ) {
                    assert( refreshed.ticket && refreshed.ticket->action.id != ticket.action.id );
                    assert( owner.run( *refreshed.ticket ).status == F::Status::Executed && owner.pending() == 0 && owner.calls() == 2 );
                }
                else assert( refreshed.status == F::Status::Empty && F::capture( *town ) == after );
            }
        }
    }
    std::cout << "ADAPTER_END" << std::endl;
    const auto after = F::capture( *town );
    if ( owner.calls() == 0 && mode != "changed_funds" && mode != "changed_visitor" && mode != "changed_mission" && mode != "changed_role" ) assert( after == before );
    const auto total = []( const F::Army & army, const int id ) { uint64_t n = 0; for ( const auto & s : army ) if ( s.first == id ) n += s.second; return n; };
    const int creature = mode == "upgrade" ? Monster::RANGER : Monster::ARCHER;
    std::cout << "RESULT mode=" << mode << " status=" << static_cast<int>( result.status ) << " calls=" << owner.calls()
        << " pending=" << owner.pending() << " blocked=" << owner.blocked() << " gold_spent=" << before.money[0] - after.money[0]
        << " stock_removed=" << before.stock[1] - after.stock[1]
        << " garrison_added=" << total( after.garrison, creature ) - total( before.garrison, creature )
        << " visitor_added=" << total( after.visitorArmy, creature ) - total( before.visitorArmy, creature )
        << " projections_left=" << work.projections << " synthetic_oracles=" << pureChecks << '\n';
    kingdom.GetHeroes().clear(); kingdom.GetCastles().clear();
}
