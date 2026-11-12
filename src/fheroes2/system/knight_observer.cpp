#include "knight_observer.h"
#include "army.h"
#include "artifact.h"
#include "castle.h"
#include "economy_project.h"
#include "economy_trace.h"
#include "heroes.h"
#include "maps_tiles.h"
#include "route.h"
#include "settings.h"
#include <sstream>

std::string KnightObserver::armyJson( const Army & army )
{
    std::ostringstream out; out << '[';
    for ( size_t i = 0; i < army.Size(); ++i ) {
        if ( i ) out << ',';
        const auto * troop = army.GetTroop( i );
        out << '[' << ( troop && troop->isValid() ? troop->GetID() : 0 ) << ',' << ( troop ? troop->GetCount() : 0 ) << ']';
    }
    return out.str() + ']';
}

void KnightObserver::kingdomState( const Kingdom & kingdom, const char * phase )
{
    if ( !Settings::Get().isKnightObserverBuild() || fheroes2::taxpayerSimulationDepth ) return;
    const int player = static_cast<int>( kingdom.GetColor() );
    uint64_t fogHash = 14695981039346656037ULL; uint32_t hidden = 0;
    for ( size_t tile = 0; tile < world.getSize(); ++tile ) {
        const bool fog = world.getTile( static_cast<int32_t>( tile ) ).isFog( kingdom.GetColor() );
        hidden += fog; fogHash = ( fogHash ^ static_cast<uint64_t>( fog ) ) * 1099511628211ULL;
    }
    ECONOMY_TRACE( 1, "OBSERVER_KINGDOM", ",\"kind\":\"actual\",\"phase\":" << Economy::jsonQuote( phase ) << ",\"player\":" << player
        << ",\"control\":" << kingdom.GetControl() << ",\"race\":" << kingdom.GetRace() << ",\"funds\":" << Economy::fundsJson( kingdom.GetFunds() )
        << ",\"daily_income_all\":" << Economy::fundsJson( kingdom.GetIncome( Kingdom::INCOME_ALL ) )
        << ",\"difficulty\":" << Settings::Get().GameDifficulty() << ",\"map_width\":" << world.w() << ",\"map_height\":" << world.h()
        << ",\"hidden_tiles\":" << hidden << ",\"gameplay_fog_hash\":\"" << fogHash << "\",\"observer_reveals_gameplay_fog\":false"
        << ",\"towns\":" << kingdom.GetCastles().size() << ",\"heroes\":" << kingdom.GetHeroes().size()
        << ",\"build\":\"KNIGHT_OBSERVER_20261110\",\"ai_revision\":7,\"compiled\":" << Economy::jsonQuote( __DATE__ " " __TIME__ ) );
    auto captured = AI::EconomyProject::capture( kingdom, world.CountDay() );
    for ( size_t i = 0; i < captured.start.towns.size(); ++i ) {
        const auto & town = captured.start.towns[i];
        const auto * castle = world.getCastle( { town.tile % world.w(), town.tile / world.w() } );
        if ( !castle ) continue;
        const auto training = castle->conscriptionDaySnapshot().state;
        std::ostringstream dwellings; dwellings << '[';
        for ( size_t tier = 0; tier < 6; ++tier ) {
            if ( tier ) dwellings << ',';
            const auto & creature = town.dwellings[tier].creature( town.mask );
            dwellings << "{\"tier\":" << tier + 1 << ",\"built\":" << ( ( town.mask & town.dwellings[tier].base ) ? "true" : "false" )
                << ",\"monster\":" << creature.id << ",\"stock\":" << AI::EconomyProject::stock( captured.start, i, tier )
                << ",\"growth_base\":" << creature.growth << ",\"unit_cost\":" << Economy::arrayJson( creature.price ) << '}';
        }
        dwellings << ']';
        ECONOMY_TRACE( 1, "OBSERVER_TOWN", ",\"kind\":\"actual\",\"phase\":" << Economy::jsonQuote( phase ) << ",\"player\":" << player
            << ",\"town\":" << town.tile << ",\"name\":" << Economy::jsonQuote( castle->GetName() ) << ",\"race\":" << town.race
            << ",\"buildings_mask\":" << town.mask << ",\"disabled_mask\":" << town.disabled << ",\"can_build_today\":" << ( town.buildAvailable ? "true" : "false" )
            << ",\"farm\":" << ( castle->isBuild( BUILD_WEL2 ) ? "true" : "false" ) << ",\"training_grounds\":" << ( castle->hasTrainingGrounds() ? "true" : "false" )
            << ",\"farm_cost\":" << Economy::fundsJson( castle->getBuildingCost( BUILD_WEL2 ) ) << ",\"tg_cost\":" << Economy::fundsJson( castle->getBuildingCost( BUILD_TRAINING_GROUNDS ) )
            << ",\"well_growth_bonus\":" << town.wellBonus << ",\"farm_growth_bonus\":" << town.farmBonus
            << ",\"garrison\":" << armyJson( castle->GetArmy() ) << ",\"visitor\":" << town.hero
            << ",\"dwellings\":" << dwellings.str() << ",\"trained_total\":" << training.total
            << ",\"cycle_quotas\":" << Economy::arrayJson( training.quotas ) << ",\"cycle_used\":" << Economy::arrayJson( training.used )
            << ",\"cycle_next\":" << training.next << ",\"loss_remainder\":" << training.lossRemainder );
    }
    for ( const auto * hero : kingdom.GetHeroes() ) {
        std::ostringstream skills, artifacts, path; skills << '['; artifacts << '['; path << '[';
        for ( int skill = 1; skill <= 14; ++skill ) { if ( skill > 1 ) skills << ','; skills << hero->GetLevelSkill( skill ); }
        bool first = true;
        for ( const auto & artifact : hero->GetBagArtifacts() ) { if ( !first ) artifacts << ','; first = false; artifacts << artifact.GetID(); }
        first = true;
        for ( const auto & step : hero->GetPath() ) { if ( !first ) path << ','; first = false; path << step.GetIndex(); }
        skills << ']'; artifacts << ']'; path << ']';
        ECONOMY_TRACE( 1, "OBSERVER_HERO", ",\"kind\":\"actual\",\"phase\":" << Economy::jsonQuote( phase ) << ",\"player\":" << player
            << ",\"hero\":" << hero->GetID() << ",\"name\":" << Economy::jsonQuote( hero->GetName() ) << ",\"tile\":" << hero->GetIndex()
            << ",\"race\":" << hero->GetRace() << ",\"level\":" << hero->GetLevel() << ",\"experience\":" << hero->GetExperience()
            << ",\"attack\":" << hero->GetAttack() << ",\"defense\":" << hero->GetDefense() << ",\"power\":" << hero->GetPower() << ",\"knowledge\":" << hero->GetKnowledge()
            << ",\"morale\":" << hero->GetMorale() << ",\"luck\":" << hero->GetLuck() << ",\"mana\":" << hero->GetSpellPoints()
            << ",\"moves\":" << hero->GetMovePoints() << ",\"max_moves\":" << hero->GetMaxMovePoints() << ",\"ship\":" << ( hero->isShipMaster() ? "true" : "false" )
            << ",\"role\":" << static_cast<int>( hero->getAIRole() ) << ",\"sleeper\":" << ( hero->Modes( Heroes::SLEEPER ) ? "true" : "false" )
            << ",\"army\":" << armyJson( hero->GetArmy() ) << ",\"skills\":" << skills.str() << ",\"artifacts\":" << artifacts.str()
            << ",\"target\":" << hero->GetPath().GetDestinationIndex() << ",\"path\":" << path.str() );
    }
}
