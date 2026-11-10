// C3D5J. Native copies/JoinTroop/Assign, with explicit test-only fault callbacks.
#include "support/exact_transfer_fixture_adapter.h"
#include "difficulty.h"
#include "map_format_info.h"
#include "maps_tiles.h"
#include "players.h"
#include "settings.h"
#include <cassert>
#include <iostream>
#include <set>

namespace T = TransferFixture;
namespace F = RecruitFixture;
namespace P = F::P;
namespace V = F::V;
namespace C = F::C;
namespace M = F::M;

int main( int argc, char ** argv )
{
    assert( argc == 5 ); const std::string mode = argv[2], direction = argv[3], profile = argv[4];
    const std::set<std::string> modes{ "exact", "new_slot", "full_merge", "source_split", "destination_split", "all_peasants", "empty_garrison",
        "last_unit", "leave_one", "split_last", "insufficient", "no_source", "full", "overflow", "overflow_alternate", "zero", "no_binding",
        "wrong_binding", "mobile", "no_accept", "unsupported", "stale", "forged_visitor", "forged_count", "changed_visitor", "changed_mission",
        "changed_source", "budget", "coverage", "two", "reverse", "false_clean", "true_empty", "false_effect", "wrong_quantity", "wrong_creature",
        "wrong_funds", "rollback_false", "rollback_true", "record_fault", "throw_after", "reentrant", "pure", "progress", "no_funds" };
    assert( modes.count( mode ) && ( direction == "garrison" || direction == "visitor" ) );
    assert( profile == "default" || profile == "tg" || profile == "custom" || profile == "compatibility" );
    const bool deposit = direction == "garrison";
    const bool training = profile == "tg" || profile == "custom" || mode == "progress";
    Settings::Get().Read( argv[1] ); Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() ); world.generateUninitializedMap( 36 ); world.NewDay();
    const auto & config = Conscription::settings();
    assert( config.goldPerPeasant == ( profile == "custom" ? 137U : 100U ) );
    assert( config.evenDayLimit == ( profile == "custom" ? 4U : 3U ) );
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE ); kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 100000 ) );
    constexpr int tile = 150; world.addCastle( tile, static_cast<uint8_t>( Race::KNGT ), PlayerColor::BLUE );
    auto * town = world.getCastle( { tile % 36, tile / 36 } ); assert( town );
    kingdom.GetCastles().push_back( town ); world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
    Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
    meta.builtBuildings = { BUILD_CASTLE, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, BUILD_WELL, BUILD_CAPTAIN };
    if ( training ) meta.builtBuildings.push_back( BUILD_TRAINING_GROUNDS );
    else if ( profile != "compatibility" ) meta.builtBuildings.push_back( BUILD_WEL2 );
    town->loadFromResurrectionMap( meta );
    auto * visitor = world.GetHeroes( Heroes::LORDKILBURN ); visitor->SetColor( PlayerColor::BLUE ); visitor->SetCenter( town->GetCenter() );
    visitor->SetModes( Heroes::PATROL ); assert( visitor->GetPatrolDistance() == 0 ); kingdom.GetHeroes().push_back( visitor );
    if ( mode == "progress" ) {
        town->GetArmy().Clean(); assert( town->GetArmy().JoinTroop( Monster::PEASANT, 20, false ) );
        world.NewDay(); if ( town->conscriptionDaySnapshot().state.total == 0 ) Castle::processDailyConscription( kingdom );
        assert( town->conscriptionDaySnapshot().state.total == ( profile == "custom" ? 4U : 3U ) );
    }
    // Explicit troop fixtures after any history seeding. Army::Reset would add a
    // hero's initial unit, so use Clean and exact slots here.
    auto & garrison = town->GetArmy(); auto & heroArmy = visitor->GetArmy();
    garrison.Clean(); heroArmy.Clean();
    assert( garrison.JoinTroop( Monster::PEASANT, 20, false ) && garrison.JoinTroop( Monster::PIKEMAN, 2, false ) );
    assert( heroArmy.JoinTroop( Monster::PEASANT, 10, false ) && heroArmy.JoinTroop( Monster::ARCHER, 1, false ) );
    ::Army & source = deposit ? heroArmy : garrison; ::Army & destination = deposit ? garrison : heroArmy;
    const auto split = []( ::Army & army, uint32_t first, uint32_t second, bool other ) {
        army.Clean(); army.GetTroop( 0 )->Set( Monster::PEASANT, first ); army.GetTroop( 2 )->Set( Monster::PEASANT, second );
        if ( other ) army.GetTroop( 1 )->Set( Monster::ARCHER, 1 );
    };
    if ( mode == "source_split" ) split( source, 4, 6, true );
    if ( mode == "destination_split" ) split( destination, 3, 7, true );
    if ( mode == "split_last" ) split( heroArmy, 4, 6, false );
    if ( mode == "last_unit" || mode == "leave_one" ) { heroArmy.Clean(); assert( heroArmy.JoinTroop( Monster::PEASANT, 10, false ) ); }
    if ( mode == "new_slot" || mode == "no_source" ) {
        auto & target = mode == "new_slot" ? destination : source;
        target.Clean(); assert( target.JoinTroop( Monster::ARCHER, 1, false ) );
    }
    if ( mode == "full" || mode == "full_merge" ) {
        destination.Clean();
        for ( const int unit : { mode == "full" ? Monster::PALADIN : Monster::PEASANT, Monster::ARCHER, Monster::PIKEMAN, Monster::SWORDSMAN, Monster::CAVALRY } )
            assert( destination.JoinTroop( unit, unit == Monster::PEASANT ? 3 : 1, false ) );
    }
    if ( mode == "overflow" || mode == "overflow_alternate" ) {
        destination.Clean(); destination.GetTroop( 0 )->Set( Monster::PEASANT, UINT32_MAX );
        destination.GetTroop( 1 )->Set( Monster::ARCHER, 1 );
        if ( mode == "overflow_alternate" ) destination.GetTroop( 2 )->Set( Monster::PEASANT, 3 );
        assert( Kingdom::GetTaxpayerGoldPerPeasantPerDay() == 0 ); // Counter stress does not exercise saturated-income arithmetic.
    }
    if ( mode == "empty_garrison" ) {
        garrison.Clean(); if ( !deposit ) assert( garrison.JoinTroop( Monster::PEASANT, 20, false ) );
    }
    if ( mode == "mobile" ) visitor->ResetModes( Heroes::PATROL );
    if ( mode == "no_funds" ) kingdom.OddFundsResource( kingdom.GetFunds() );
    P::Action a; a.kind = deposit ? P::Kind::ToGarrison : P::Kind::ToVisitor; a.count = 5; a.day = world.CountDay(); a.town = tile; a.required = true;
    if ( mode == "source_split" ) a.count = 7;
    if ( mode == "all_peasants" || ( mode == "empty_garrison" && !deposit ) ) a.count = static_cast<uint32_t>( T::population( F::army( source ), true ) );
    if ( mode == "last_unit" && deposit ) a.count = 10;
    if ( mode == "split_last" && deposit ) a.count = 10;
    if ( mode == "leave_one" ) a.count = deposit ? 9 : 19;
    if ( mode == "insufficient" ) a.count = static_cast<uint32_t>( T::population( F::army( source ), true ) ) + 1;
    if ( mode == "zero" ) a.count = 0;
    if ( mode == "unsupported" ) { a.kind = P::Kind::Recruit; a.pool = 1; }
    if ( mode == "two" ) a.count = 3;
    C::Program program; program.town = tile; program.visitor = visitor->GetID(); program.pending = { a };
    if ( mode == "no_binding" ) program.visitor = -1;
    if ( mode == "wrong_binding" ) ++program.visitor;
    if ( mode == "two" || mode == "reverse" ) {
        auto next = a; next.count = mode == "two" ? 2 : 5;
        if ( mode == "reverse" ) next.kind = deposit ? P::Kind::ToVisitor : P::Kind::ToGarrison;
        program.pending.push_back( next );
    }
    const auto before = T::capture( *town ); const auto taxBefore = kingdom.GetIncome( Kingdom::INCOME_TAXPAYER ).gold;
    T::Owner owner( kingdom, *town, 9201 ); M::Budget work; F::Result result; size_t synthetic = 0;
    std::cout << "TRANSFER_BEGIN" << std::endl;
    if ( mode == "no_accept" ) { result = owner.prepare( work ); assert( result.status == F::Status::Held ); }
    else if ( mode == "no_binding" || mode == "wrong_binding" || mode == "mobile" || mode == "zero" || mode == "unsupported" ) {
        assert( !owner.acceptProgram( program ) ); result = owner.prepare( work ); assert( result.status == F::Status::Held && owner.pending() == 0 );
    }
    else if ( mode == "pure" ) {
        const auto projected = P::capture( kingdom, world.CountDay() + 14 ); const auto c = T::check( *town, projected.start.towns[0], a, before ); assert( c.valid );
        assert( T::classify( true, before, c, c.expected ) == V::Outcome::Executed ); ++synthetic;
        assert( T::classify( false, before, c, before ) == V::Outcome::Failed ); ++synthetic;
        assert( T::classify( true, before, c, before ) == V::Outcome::Uncertain ); ++synthetic;
        assert( T::classify( false, before, c, c.expected ) == V::Outcome::Uncertain ); ++synthetic;
        assert( T::classify( true, before, c, {} ) == V::Outcome::Uncertain ); ++synthetic;
        for ( size_t r = 0; r < 7; ++r ) { auto bad = c.expected; ++bad.money[r]; assert( T::classify( true, before, c, bad ) == V::Outcome::Uncertain ); ++synthetic; }
        for ( size_t p = 0; p < 6; ++p ) { auto bad = c.expected; ++bad.stock[p]; assert( T::classify( true, before, c, bad ) == V::Outcome::Uncertain ); ++synthetic; }
        auto bad = c.expected; ++bad.visitor; assert( T::classify( true, before, c, bad ) == V::Outcome::Uncertain ); ++synthetic;
        bad = c.expected; ++std::get<0>( bad.progress ); assert( T::classify( true, before, c, bad ) == V::Outcome::Uncertain ); ++synthetic;
        bad = c.expected; ++bad.garrison[0].second; assert( T::classify( true, before, c, bad ) == V::Outcome::Uncertain ); ++synthetic;
        auto other = projected.start.towns[0]; ++other.hero; assert( !T::check( *town, other, a, before ).valid ); ++synthetic;
        T::Army wide{}; wide[0] = wide[1] = { Monster::PEASANT, UINT32_MAX };
        assert( T::population( wide, true ) == uint64_t{ UINT32_MAX } * 2 ); ++synthetic;
        assert( F::slot( wide, Monster::PEASANT, 1 ) == -2 ); ++synthetic;
        auto mismatched = a; ++mismatched.count; assert( !T::apply( *town, mismatched, c ) && T::capture( *town ) == before ); ++synthetic;
        mismatched = a; mismatched.kind = deposit ? P::Kind::ToVisitor : P::Kind::ToGarrison;
        assert( !T::apply( *town, mismatched, c ) && T::capture( *town ) == before ); ++synthetic;
    }
    else {
        assert( owner.acceptProgram( program ) && !owner.acceptProgram( program ) );
        if ( mode == "budget" ) work.projections = 0;
        auto preparation = owner.prepare( work, mode != "coverage" ); result = preparation;
        if ( preparation.status == F::Status::Prepared ) {
            assert( preparation.ticket && preparation.ticket->visitor == visitor->GetID() && work.projections == 254 );
            const auto ticket = *preparation.ticket; auto supplied = ticket;
            if ( mode == "stale" ) { const auto newer = owner.prepare( work ); assert( newer.ticket && newer.ticket->stamp.revision != ticket.stamp.revision ); }
            if ( mode == "forged_visitor" ) ++supplied.visitor;
            if ( mode == "forged_count" ) ++supplied.action.count;
            if ( mode == "changed_mission" ) visitor->ResetModes( Heroes::PATROL );
            if ( mode == "changed_source" ) source.GetTroop( 0 )->SetCount( source.GetTroop( 0 )->GetCount() + 1 );
            if ( mode == "changed_visitor" ) {
                visitor->SetCenter( { 3, 3 } ); auto * h = world.GetHeroes( Heroes::SIRGALLANTH ); h->SetColor( PlayerColor::BLUE ); h->SetCenter( town->GetCenter() );
                h->SetModes( Heroes::PATROL ); h->GetArmy().Clean(); assert( h->GetArmy().JoinTroop( Monster::PEASANT, 10, false ) );
                assert( h->GetArmy().JoinTroop( Monster::ARCHER, 1, false ) ); kingdom.GetHeroes().push_back( h ); assert( town->GetHero() == h );
            }
            const auto immediatelyBefore = T::capture( *town ); T::Owner::Call fault;
            if ( mode == "false_clean" ) fault = []( Castle &, const P::Action &, const T::Checked & ) { return false; };
            if ( mode == "true_empty" ) fault = []( Castle &, const P::Action &, const T::Checked & ) { return true; };
            if ( mode == "false_effect" ) fault = []( Castle & t, const P::Action & op, const T::Checked & c ) { assert( T::apply( t, op, c ) ); return false; };
            if ( mode == "wrong_quantity" ) fault = [&]( Castle & t, const P::Action & op, const T::Checked & ) {
                auto wrong = op; ++wrong.count; const auto p = P::capture( kingdom, world.CountDay() + 14 );
                const auto c = T::check( t, p.start.towns[0], wrong, T::capture( t ) ); assert( c.valid ); return T::apply( t, wrong, c );
            };
            if ( mode == "wrong_creature" ) fault = []( Castle & t, const P::Action & op, const T::Checked & c ) {
                assert( T::apply( t, op, c ) ); assert( t.GetArmy().JoinTroop( Monster::ARCHER, 1, false ) ); return true;
            };
            if ( mode == "wrong_funds" ) fault = [&]( Castle & t, const P::Action & op, const T::Checked & c ) {
                assert( T::apply( t, op, c ) ); kingdom.OddFundsResource( Funds( Resource::GOLD, 1 ) ); return true;
            };
            if ( mode == "rollback_false" || mode == "rollback_true" ) fault = [&]( Castle & t, const P::Action & op, const T::Checked & c ) {
                const Troops gb( t.GetArmy() ), hb( visitor->GetArmy() ); assert( T::apply( t, op, c ) );
                t.GetArmy().Assign( gb ); visitor->GetArmy().Assign( hb ); return mode == "rollback_true";
            };
            if ( mode == "record_fault" ) fault = [&]( Castle & t, const P::Action & op, const T::Checked & c ) { assert( T::apply( t, op, c ) ); owner.invalidateForTest(); return true; };
            if ( mode == "throw_after" ) fault = []( Castle & t, const P::Action & op, const T::Checked & c ) -> bool { assert( T::apply( t, op, c ) ); throw 42; };
            if ( mode == "reentrant" ) fault = [&]( Castle & t, const P::Action & op, const T::Checked & c ) { assert( owner.run( ticket ).status == F::Status::Blocked ); return T::apply( t, op, c ); };
            result = owner.run( supplied, fault );
            const bool rejected = mode == "stale" || mode == "forged_visitor" || mode == "forged_count" || mode == "changed_visitor" || mode == "changed_mission" || mode == "changed_source";
            const bool uncertain = mode == "true_empty" || mode == "false_effect" || mode.rfind( "wrong_", 0 ) == 0 || mode == "rollback_true" || mode == "record_fault" || mode == "throw_after";
            if ( rejected ) assert( result.status == F::Status::Held && owner.calls() == 0 && owner.pending() == 1 && T::capture( *town ) == immediatelyBefore );
            else if ( uncertain ) {
                assert( result.status == F::Status::Blocked && owner.blocked() && owner.calls() == 1 && owner.pending() == 1 );
                const auto after = T::capture( *town );
                assert( owner.run( ticket ).status == F::Status::Blocked && owner.prepare( work ).status == F::Status::Blocked );
                assert( owner.calls() == 1 && T::capture( *town ) == after );
            }
            else if ( mode == "false_clean" || mode == "rollback_false" ) {
                assert( result.status == F::Status::Failed && owner.pending() == 1 && owner.calls() == 1 && T::capture( *town ) == before );
                assert( owner.run( ticket ).status == F::Status::Held && owner.calls() == 1 );
                const auto retry = owner.prepare( work ); assert( retry.ticket && retry.ticket->stamp.revision != ticket.stamp.revision );
                if ( mode == "false_clean" ) assert( owner.run( *retry.ticket ).status == F::Status::Executed && owner.calls() == 2 && owner.pending() == 0 );
            }
            else {
                assert( result.status == F::Status::Executed && owner.calls() == 1 && owner.pending() == ( program.pending.size() - 1 ) );
                const auto after = T::capture( *town );
                assert( T::population( before.garrison, true ) + T::population( before.visitorArmy, true ) == T::population( after.garrison, true ) + T::population( after.visitorArmy, true ) );
                assert( visitor->GetArmy().isValid() );
                if ( mode == "source_split" ) { const auto & src = deposit ? after.visitorArmy : after.garrison; assert( src[0].second == 0 && src[2].second == 3 ); }
                if ( mode == "destination_split" ) { const auto & dst = deposit ? after.garrison : after.visitorArmy; assert( dst[0].second == 8 && dst[2].second == 7 ); }
                assert( owner.run( ticket ).status == F::Status::Held && owner.calls() == 1 && T::capture( *town ) == after );
                const auto refreshed = owner.prepare( work );
                if ( mode == "two" || mode == "reverse" ) {
                    assert( refreshed.ticket && owner.pending() == 1 );
                    assert( owner.run( *refreshed.ticket ).status == F::Status::Executed && owner.calls() == 2 && owner.pending() == 0 );
                }
                else assert( refreshed.status == F::Status::Empty && T::capture( *town ) == after );
            }
        }
        else assert( !preparation.ticket && ( preparation.status == F::Status::Failed || preparation.status == F::Status::Held ) && owner.calls() == 0 && owner.pending() == program.pending.size() && T::capture( *town ) == before );
    }
    std::cout << "TRANSFER_END" << std::endl;
    const auto after = T::capture( *town );
    if ( !owner.calls() && mode != "changed_visitor" && mode != "changed_mission" && mode != "changed_source" ) assert( after == before );
    std::cout << "RESULT profile=" << profile << " mode=" << mode << " direction=" << direction << " status=" << static_cast<int>( result.status )
        << " calls=" << owner.calls() << " pending=" << owner.pending() << " blocked=" << owner.blocked()
        << " garrison_before=" << T::population( before.garrison, true ) << " garrison_after=" << T::population( after.garrison, true )
        << " visitor_before=" << T::population( before.visitorArmy, true ) << " visitor_after=" << T::population( after.visitorArmy, true )
        << " gold_spent=" << before.money[0] - after.money[0] << " funds_same=" << ( before.money == after.money )
        << " stocks_same=" << ( before.stock == after.stock ) << " buildings_same=" << ( before.buildings == after.buildings && before.canBuild == after.canBuild )
        << " progress_before=" << std::get<0>( before.progress ) << " progress_same=" << ( before.progress == after.progress )
        << " tax_before=" << taxBefore << " tax_after=" << kingdom.GetIncome( Kingdom::INCOME_TAXPAYER ).gold
        << " projections_left=" << work.projections << " synthetic_oracles=" << synthetic << '\n';
    kingdom.GetHeroes().clear(); kingdom.GetCastles().clear();
}
