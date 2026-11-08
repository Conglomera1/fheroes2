#include "economy_military.h"
#include "ai_planner.h"
#include "army_troop.h"
#include "battle_tower.h"
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

namespace D = AI::EconomyDefense;
namespace M = AI::EconomyMilitary;
namespace P = AI::EconomyProject;
namespace G = AI::EconomyCandidates;
namespace {
size_t checks = 0;
void close( const double a, const double b )
{ assert( std::abs( a - b ) <= 1e-9 * std::max( 1.0, std::abs( b ) ) ); ++checks; }
// Pre-extraction siege arithmetic, using the actual Battle::Tower type as independent oracle.
double oldSiege( const Castle & castle, double army, const Heroes * attacker )
{
    if ( castle.isCastle() && army > 0.1 ) {
        const Battle::Tower tower( castle, Battle::TowerType::TWR_CENTER, 0 );
        const double strength = tower.GetStrengthWithBonus( tower.GetAttackBonus(), 0 );
        army += strength;
        if ( castle.isBuild( BUILD_LEFTTURRET ) ) army += strength / 2;
        if ( castle.isBuild( BUILD_RIGHTTURRET ) ) army += strength / 2;
        if ( attacker && !attacker->HasSecondarySkill( Skill::Secondary::BALLISTICS ) && attacker->GetArmy().isMeleeDominantArmy() )
            army *= castle.isBuild( BUILD_MOAT ) ? 1.45 : 1.25;
        else army *= castle.isBuild( BUILD_MOAT ) ? 1.2 : 1.15;
    }
    return army;
}
P::Town townState( const Kingdom & kingdom, const Castle & castle )
{
    const auto input = P::capture( kingdom, 15 );
    return *std::find_if( input.start.towns.begin(), input.start.towns.end(), [&]( const auto & t ) { return t.tile == castle.GetIndex(); } );
}
void configure( Castle & castle, const bool captain, const uint32_t structures, const uint32_t peasants )
{
    Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true;
    meta.builtBuildings = { BUILD_CASTLE, DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4,
                            DWELLING_MONSTER5, DWELLING_MONSTER6, BUILD_WEL2, BUILD_MAGEGUILD1 };
    if ( captain ) meta.builtBuildings.push_back( BUILD_CAPTAIN );
    for ( uint32_t bit = 1; bit; bit <<= 1 ) if ( structures & bit ) meta.builtBuildings.push_back( bit );
    meta.availableToHireMonsterCount = { 50, 50, 50, 50, 50, 50 };
    castle.loadFromResurrectionMap( meta );
    castle.GetArmy().Reset();
    if ( peasants ) castle.GetArmy().JoinTroop( Monster::PEASANT, peasants, false );
    castle.SetModes( Castle::ALLOW_TO_BUILD_TODAY );
}
}
int main( int argc, char ** argv )
{
    assert( argc == 2 ); Settings::Get().Read( argv[1] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() ); world.generateUninitializedMap( 36 ); world.NewDay();
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    kingdom.AddFundsResource( Funds( 100, 100, 100, 100, 100, 100, 100000 ) );
    auto & melee = *world.GetHeroes( Heroes::THUNDAX );
    auto & ranged = *world.GetHeroes( Heroes::FINEOUS );
    auto & ballistic = *world.GetHeroes( Heroes::JOJOSH );
    for ( auto * attacker : { &melee, &ranged, &ballistic } ) {
        attacker->SetColor( PlayerColor::RED ); attacker->GetArmy().Reset();
        attacker->SetCenter( { 4 + attacker->GetID(), 2 } );
        attacker->GetArmy().JoinTroop( attacker == &ranged ? Monster::ARCHER : Monster::PIKEMAN, 100, false );
    }
    ballistic.GetSecondarySkills().AddSkill( Skill::Secondary( Skill::Secondary::BALLISTICS, Skill::Level::EXPERT ) );
    const int races[] = { Race::KNGT, Race::BARB, Race::SORC, Race::WRLK, Race::WZRD, Race::NECR };
    std::vector<Castle *> towns;
    for ( size_t i = 0; i < 6; ++i ) {
        const int tile = 150 + static_cast<int>( i ) * 180;
        world.addCastle( tile, static_cast<uint8_t>( races[i] ), PlayerColor::BLUE );
        auto * castle = world.getCastle( { tile % 36, tile / 36 } );
        world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
        kingdom.GetCastles().push_back( castle ); towns.push_back( castle );
    }
    for ( auto * castle : towns ) for ( const bool captain : { false, true } )
        for ( const uint32_t structures : { 0U, uint32_t( BUILD_LEFTTURRET ), uint32_t( BUILD_LEFTTURRET | BUILD_RIGHTTURRET ),
                                          uint32_t( BUILD_LEFTTURRET | BUILD_RIGHTTURRET | BUILD_MOAT ),
                                          uint32_t( BUILD_LEFTTURRET | BUILD_RIGHTTURRET | BUILD_MOAT | BUILD_SPEC ) } )
            for ( const uint32_t population : { 0U, 100U } ) {
                configure( *castle, captain, structures, population );
                auto town = townState( kingdom, *castle );
                for ( const Heroes * attacker : { static_cast<Heroes *>( nullptr ), &melee, &ranged, &ballistic } ) {
                    const AI::EnemyArmy observed( attacker ? attacker->GetIndex() : 80, attacker, 100, 1500 );
                    const auto threat = D::snapshotThreat( observed, 1, town.tile, 0, 2, true, true );
                    const auto value = D::localDefenseValue( kingdom, town, threat ); assert( value.complete );
                    close( value.force, oldSiege( *castle, castle->GetArmy().GetStrength(), attacker ) );
                    close( castle->GetGarrisonStrength( attacker ), oldSiege( *castle, castle->GetArmy().GetStrength(), attacker ) );
                }
            }
    // Projected turret/moat/fortifications must match real purchases, not just current castle structures.
    auto & home = *towns[0]; configure( home, true, 0, 100 );
    auto threat = D::snapshotThreat( AI::EnemyArmy( melee.GetIndex(), &melee, 100, 1500 ), 1, home.GetIndex(), 0, 2, true, true );
    for ( const auto building : { BUILD_LEFTTURRET, BUILD_RIGHTTURRET, BUILD_MOAT, BUILD_SPEC } ) {
        auto projected = townState( kingdom, home ); projected.mask |= building;
        const auto future = D::localDefenseValue( kingdom, projected, threat ); assert( future.complete );
        home.SetModes( Castle::ALLOW_TO_BUILD_TODAY ); assert( home.BuyBuilding( building ) );
        close( future.force, home.GetGarrisonStrength( &melee ) );
    }
    // A visible attack profile is frozen; an unverified hero is never inspected.
    const auto hidden = D::snapshotThreat( AI::EnemyArmy( melee.GetIndex(), &melee, 100, 1500 ), 2, home.GetIndex(), 0, {}, false, false );
    assert( hidden.enemyHero == -1 && !D::localDefenseValue( kingdom, townState( kingdom, home ), hidden ).complete ); ++checks;
    // Real funding/army model, with a known deadline at the end of our day 2.
    for ( auto * castle : towns ) configure( *castle, true, 0, 100 );
    auto baseline = townState( kingdom, home );
    const auto baseForce = D::localDefenseValue( kingdom, baseline, threat ); assert( baseForce.complete );
    baseline.garrison[1] = { Monster::ARCHER, 1 };
    const auto reinforcedForce = D::localDefenseValue( kingdom, baseline, threat ); assert( reinforcedForce.complete );
    threat.enemyStrength = ( baseForce.force + reinforcedForce.force ) / ( 2 * D::viableRatio );
    D::Registry registry{ { threat }, true };
    G::Context context; context.base = P::capture( kingdom, 15 );
    std::vector<M::Assignment> uses;
    for ( const auto & town : context.base.start.towns ) uses.push_back( { uint64_t( uses.size() + 1 ), town.tile, M::Slot::TownArmy, 1, 15, true, true } );
    const auto scorer = [&]( const auto & town, const auto slot ) { return M::localArmyValue( kingdom, town, slot ); };
    const auto defender = [&]( const auto & town, const auto & attack ) { return D::localDefenseValue( kingdom, town, attack ); };
    M::Budget budget; M::Session session( context, uses, scorer, budget, registry, defender );
    G::Candidate c; c.id = 1; c.actions = { G::recruit( home.GetIndex(), 1, 1, 2 ) };
    const auto fundsBefore = kingdom.GetFunds();
    const auto before = context.base.start;
    std::cout << "OBS_BEGIN" << std::endl;
    const auto early = M::evaluateObserved( session, c, static_cast<int>( kingdom.GetColor() ), budget );
    c.id = 2; c.actions = { G::recruit( home.GetIndex(), 1, 5, 3 ) };
    const auto late = M::evaluateObserved( session, c, static_cast<int>( kingdom.GetColor() ), budget );
    registry.threats[0].lastSafeDay.reset(); M::Budget unknownBudget;
    M::Session unknown( context, uses, scorer, unknownBudget, registry, defender );
    c.id = 3;
    const auto uncertain = M::evaluateObserved( unknown, c, static_cast<int>( kingdom.GetColor() ), unknownBudget );
    std::cout << "OBS_END" << std::endl;
    assert( early.defense.complete() && early.defense.cities[0].viable == true && early.baselineDefense.cities[0].viable == false ); ++checks;
    assert( late.defense.cities[0].viable == false && late.utility.value() > early.utility.value() ); ++checks;
    assert( M::compareDefense( early, late ) == D::Order::Better && M::compareDefense( early, uncertain ) == D::Order::Incomparable ); ++checks;
    assert( !uncertain.defense.complete() && !uncertain.defense.checks[0].force ); ++checks;
    assert( kingdom.GetFunds() == fundsBefore );
    const auto after = P::capture( kingdom, 15 ).start;
    for ( size_t i = 0; i < after.towns.size(); ++i ) {
        assert( after.towns[i].mask == before.towns[i].mask );
        for ( size_t j = 0; j < 5; ++j ) assert( after.towns[i].garrison[j].monster == before.towns[i].garrison[j].monster && after.towns[i].garrison[j].count == before.towns[i].garrison[j].count );
    }
    ++checks;
    std::cout << "RESULT checks=" << checks << " D_early=1 D_late=0 D_unknown=null priority=early\n";
    kingdom.GetCastles().clear();
}
