// Native engine oracles: compare hypothetical forces with real commander/army contexts.
#include "economy_military.h"
#include "army_troop.h"
#include "difficulty.h"
#include "heroes.h"
#include "map_format_info.h"
#include "maps_tiles.h"
#include "players.h"
#include "settings.h"
#include "world.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace M = AI::EconomyMilitary;
namespace P = AI::EconomyProject;
namespace G = AI::EconomyCandidates;
namespace {
size_t checks = 0;
void close( const double actual, const double expected )
{
    assert( std::abs( actual - expected ) <= 1e-9 * std::max( 1.0, std::abs( expected ) ) ); ++checks;
}
// Frozen pre-C3B1 formula. The RAW captain pointer matters even if its quarters are absent.
double legacyStrength( const Army & army, const HeroBase * commander )
{
    double total = 0;
    bool troops = false;
    const auto archery = commander ? commander->GetSecondarySkillValue( Skill::Secondary::ARCHERY ) : 0;
    for ( size_t i = 0; i < 5; ++i ) {
        const auto * troop = army.GetTroop( i );
        if ( troop->isEmpty() ) continue;
        troops = true;
        double value = troop->GetStrengthWithBonus( commander ? commander->GetAttack() : 0, commander ? commander->GetDefense() : 0 );
        if ( archery && troop->isArchers() ) value *= std::sqrt( 1 + static_cast<double>( archery ) / 100 );
        const int morale = army.GetMorale();
        if ( troop->isAffectedByMorale() ) value *= 1 + ( morale < 0 ? morale / 12.0 : morale / 24.0 );
        value *= 1 + army.GetLuck() / 24.0;
        total += value;
    }
    if ( commander && troops && commander->GetSpellPoints() ) {
        double magic = 0;
        for ( const auto & spell : commander->getAllSpells() ) if ( spell.isCombat() )
            magic = std::max( magic, spell.getStrategicValue( total, commander->GetSpellPoints(), commander->GetPower() ) );
        total += magic;
    }
    return total;
}
void setArmy( Army & army, const P::Army & stacks )
{
    for ( size_t i = 0; i < stacks.size(); ++i ) {
        if ( stacks[i].count ) army.GetTroop( i )->Set( Monster( stacks[i].monster ), stacks[i].count );
        else army.GetTroop( i )->Reset();
    }
}
P::Town captureTown( const Kingdom & kingdom, const Castle & castle )
{
    const auto input = P::capture( kingdom, world.CountDay() + 14 );
    return *std::find_if( input.start.towns.begin(), input.start.towns.end(), [&]( const auto & t ) { return t.tile == castle.GetIndex(); } );
}
Castle & makeTown( Kingdom & kingdom, const int tile, const int race, const bool captain )
{
    world.addCastle( tile, static_cast<uint8_t>( race ), kingdom.GetColor() );
    auto & castle = *world.getCastle( { tile % world.w(), tile / world.w() } );
    world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
    Maps::Map_Format::CastleMetadata metadata;
    metadata.customBuildings = true;
    metadata.builtBuildings = { BUILD_CASTLE, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4,
                                DWELLING_MONSTER5, DWELLING_MONSTER6, BUILD_WEL2 };
    if ( captain ) metadata.builtBuildings.push_back( BUILD_CAPTAIN );
    metadata.availableToHireMonsterCount = { 50, 50, 50, 50, 50, 50 };
    castle.loadFromResurrectionMap( metadata );
    castle.SetModes( Castle::ALLOW_TO_BUILD_TODAY );
    kingdom.GetCastles().push_back( &castle );
    return castle;
}
}
int main( int argc, char ** argv )
{
    assert( argc == 2 );
    Settings::Get().Read( argv[1] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    world.generateUninitializedMap( 36 );
    world.NewDay();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 100000 ) );
    const int races[] = { Race::KNGT, Race::BARB, Race::SORC, Race::WRLK, Race::WZRD, Race::NECR };
    std::vector<Castle *> towns;
    for ( size_t i = 0; i < 6; ++i ) for ( const bool captain : { false, true } )
        towns.push_back( &makeTown( kingdom, 150 + static_cast<int>( i ) * 180 + ( captain ? 10 : 0 ), races[i], captain ) );

    for ( auto * castle : towns ) {
        auto & army = castle->GetArmy();
        auto & captain = castle->GetCaptain();
        captain.SpellBookActivate(); captain.AppendSpellToBook( Spell( Spell::LIGHTNINGBOLT ), true );
        for ( const uint32_t mana : { 0, 50 } ) {
            captain.SetSpellPoints( mana );
            for ( const P::Army stacks : { P::Army{}, P::Army{ { { Monster::ARCHER, 20 } } },
                  P::Army{ { { Monster::PEASANT, 10 }, { Monster::ARCHER, 20 }, { Monster::CENTAUR, 8 }, { Monster::SKELETON, 7 } } } } ) {
                setArmy( army, stacks );
                close( army.GetStrength(), legacyStrength( army, &captain ) );
                const auto value = M::localArmyValue( kingdom, captureTown( kingdom, *castle ), M::Slot::TownArmy );
                assert( value.complete ); close( value.force, army.GetStrength() );
            }
        }
    }
    // Forecast castle morale/luck/power bonuses and compare with the real construction.
    for ( const size_t index : { 1, 3, 5, 11 } ) {
        auto & castle = *towns[index];
        const uint32_t building = index == 1 ? BUILD_TAVERN : BUILD_SPEC;
        auto changed = captureTown( kingdom, castle ); changed.mask |= building;
        const auto predicted = M::localArmyValue( kingdom, changed, M::Slot::TownArmy );
        assert( predicted.complete );
        castle.SetModes( Castle::ALLOW_TO_BUILD_TODAY ); assert( castle.BuyBuilding( building ) );
        close( predicted.force, castle.GetArmy().GetStrength() );
    }
    // A new captain has default skills/mana; use only spells already visible in an existing guild.
    auto & newHome = *towns[0];
    newHome.GetCaptain().SpellBookDeactivate(); newHome.GetCaptain().SpellBookActivate();
    newHome.SetModes( Castle::ALLOW_TO_BUILD_TODAY ); assert( newHome.BuyBuilding( BUILD_MAGEGUILD1 ) );
    newHome.SetModes( Castle::ALLOW_TO_BUILD_TODAY );
    setArmy( newHome.GetArmy(), P::Army{ { { Monster::ARCHER, 20 }, { Monster::PIKEMAN, 5 } } } );
    auto future = captureTown( kingdom, newHome ); future.mask |= BUILD_CAPTAIN;
    const auto futureCaptain = M::localArmyValue( kingdom, future, M::Slot::TownArmy );
    assert( futureCaptain.complete );
    assert( newHome.BuyBuilding( BUILD_CAPTAIN ) );
    close( futureCaptain.force, newHome.GetArmy().GetStrength() );
    auto hiddenGuild = captureTown( kingdom, newHome ); hiddenGuild.mask |= BUILD_MAGEGUILD2;
    assert( !M::localArmyValue( kingdom, hiddenGuild, M::Slot::TownArmy ).complete ); ++checks;
    auto hiddenLibrary = captureTown( kingdom, *towns[8] ); hiddenLibrary.mask |= BUILD_SPEC;
    assert( !M::localArmyValue( kingdom, hiddenLibrary, M::Slot::TownArmy ).complete ); ++checks;

    // Real hero with archery, leadership, magic, mixed alignments, undead and maximum-morale artifact.
    auto & hero = *world.GetHeroes( Heroes::LORDKILBURN );
    hero.SetColor( kingdom.GetColor() ); hero.SetCenter( newHome.GetCenter() );
    hero.GetSecondarySkills().AddSkill( Skill::Secondary( Skill::Secondary::ARCHERY, Skill::Level::EXPERT ) );
    hero.GetSecondarySkills().AddSkill( Skill::Secondary( Skill::Secondary::LEADERSHIP, Skill::Level::EXPERT ) );
    hero.SpellBookActivate(); hero.AppendSpellToBook( Spell( Spell::LIGHTNINGBOLT ), true ); hero.SetSpellPoints( 50 );
    assert( newHome.GetHero() == &hero && hero.inCastle() == &newHome );
    setArmy( hero.GetArmy(), P::Army{ { { Monster::ARCHER, 20 }, { Monster::PIKEMAN, 3 } } } );
    close( hero.GetArmy().GetStrength(), legacyStrength( hero.GetArmy(), &hero ) );
    for ( const bool maximum : { false, true } ) {
        if ( maximum ) assert( hero.GetBagArtifacts().PushArtifact( Artifact( Artifact::BATTLE_GARB ) ) );
        for ( const auto monster : { Monster::SWORDSMAN, Monster::CENTAUR, Monster::SKELETON } ) {
            auto town = captureTown( kingdom, newHome );
            town.visitor[2] = { monster, 7 }; town.garrison = {};
            const auto predicted = M::localArmyValue( kingdom, town, M::Slot::Visitor );
            assert( predicted.complete );
            const auto original = captureTown( kingdom, newHome ).visitor;
            setArmy( hero.GetArmy(), town.visitor );
            close( predicted.force, hero.GetArmy().GetStrength() );
            setArmy( hero.GetArmy(), original );
        }
    }
    // Combining the garrison uses the hero once, with the engine's defense stack selection.
    auto town = captureTown( kingdom, newHome );
    Army expected( &hero ); setArmy( expected, town.visitor );
    Army garrison; setArmy( garrison, town.garrison ); expected.ArrangeForCastleDefense( garrison );
    P::Army selected{};
    for ( size_t i = 0; i < selected.size(); ++i ) if ( expected.GetTroop( i )->isValid() )
        selected[i] = { expected.GetTroop( i )->GetID(), expected.GetTroop( i )->GetCount() };
    const auto combined = M::localArmyValue( kingdom, town, M::Slot::TownArmy );
    assert( combined.complete );
    setArmy( hero.GetArmy(), selected ); close( combined.force, hero.GetArmy().GetStrength() );
    setArmy( hero.GetArmy(), town.visitor );
    // Retain current, known commanders for the whole read-only evaluation pass.
    G::Context context; context.base = P::capture( kingdom, world.CountDay() + 14 );
    std::vector<M::Assignment> assignments;
    for ( const auto & t : context.base.start.towns ) assignments.push_back( { static_cast<uint64_t>( assignments.size() + 1 ), t.tile, M::Slot::TownArmy, 1, 15, true, true } );
    M::Budget budget;
    M::Session session( context, assignments, [&]( const auto & t, const auto slot ) { return M::localArmyValue( kingdom, t, slot ); }, budget );
    G::Candidate recruit; recruit.id = 1; recruit.actions = { G::recruit( towns[1]->GetIndex(), 1, 1, 2 ) };
    const auto fundsBefore = kingdom.GetFunds();
    const auto armiesBefore = context.base.start;
    std::cout << "OBS_BEGIN" << std::endl;
    const auto early = M::evaluateObserved( session, recruit, static_cast<int>( kingdom.GetColor() ), budget );
    recruit.id = 2; recruit.actions[0].day = 9;
    const auto late = M::evaluateObserved( session, recruit, static_cast<int>( kingdom.GetColor() ), budget );
    std::cout << "OBS_END" << std::endl;
    assert( early.utility && late.utility && *early.utility > 0 );
    close( *late.utility, *early.utility / 2 );
    assert( kingdom.GetFunds() == fundsBefore );
    const auto after = P::capture( kingdom, 15 ).start;
    for ( size_t i = 0; i < after.towns.size(); ++i ) {
        assert( after.towns[i].mask == armiesBefore.towns[i].mask );
        for ( size_t j = 0; j < 5; ++j ) {
            assert( after.towns[i].garrison[j].monster == armiesBefore.towns[i].garrison[j].monster && after.towns[i].garrison[j].count == armiesBefore.towns[i].garrison[j].count );
            assert( after.towns[i].visitor[j].monster == armiesBefore.towns[i].visitor[j].monster && after.towns[i].visitor[j].count == armiesBefore.towns[i].visitor[j].count );
        }
    }
    ++checks;
    std::cout << "RESULT checks=" << checks << " early=" << *early.utility << " late=" << *late.utility << '\n';
    kingdom.GetCastles().clear();
}
