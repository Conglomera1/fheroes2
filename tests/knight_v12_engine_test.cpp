// Real budgets, armies and boarding; regressions from Kalindra and multi-town roles.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "ai_hero_action.h"
#include "map_format_info.h"
#include "maps_tiles_helper.h"
#include "players.h"
#include "rand.h"
#include <iostream>
namespace AI {
struct KnightPolicyTestAccess {
 static Planner & planner() { auto & p=Planner::Get(); p._regions.resize(world.getSize()); return p; }
 static void reinforce(Castle & c) { planner().reinforceCastle(c); }
 static void defend(Castle & c) {
     auto & p=planner(); p._priorityTargets[c.GetIndex()]=PriorityTask(PriorityTaskType::DEFEND,151);
     p._enemyArmies[151]=EnemyArmy(151,nullptr,1e9,1500);p.CastleTurn(c,true);
 }
 static int target(Heroes & h) { double score=0; return planner().getPriorityTarget(h,score); }
 static bool recruit(Castle & c) { return planner().recruitHero(c,false); }
};
}

int main( int argc, char ** argv )
{
    assert( argc == 3 ); const std::string mode = argv[1]; Settings::Get().Read( argv[2] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL ); Settings::Get().SetGameType( Game::TYPE_HOTSEAT ); Settings::Get().SetAIMoveSpeed( 0 );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    for ( const auto color : PlayerColorsVector( Color::allPlayerColors() ) ) Players::Get( color )->SetControl( CONTROL_AI );
    Rand::CurrentThreadRandomDevice() = Rand::PCG32( 20261115 ); world.generateUninitializedMap( 36 ); world.NewDay();
    const bool boarding = mode.find( "naval" ) == 0;
    for ( int tile = 0; tile < static_cast<int>( world.getSize() ); ++tile ) {
        world.getTile( tile ).setIndex( tile );
        world.getTile( tile ).setTerrain( boarding && tile / 36 >= 6 ? 0 : Maps::Ground::GRASS_START_IMAGE_INDEX, 0 );
        world.getTile( tile ).ClearFog( static_cast<PlayerColorsSet>( PlayerColor::BLUE ) );
    }
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    const auto money = [&]( int gold ) {
        kingdom.OddFundsResource( kingdom.GetFunds() ); Funds f; f.gold = gold;
        f.wood = f.ore = f.mercury = f.sulfur = f.crystal = f.gems = 100; kingdom.AddFundsResource( f );
    };
    money( 50000 );
    const std::vector<int32_t> complete{ BUILD_CASTLE, BUILD_CAPTAIN, BUILD_STATUE, BUILD_MARKETPLACE, BUILD_WELL, BUILD_TAVERN,
        DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6, BUILD_WEL2 };
    const auto add = [&]( int tile, std::vector<int32_t> buildings, int race = Race::KNGT ) {
        world.addCastle( tile, static_cast<uint8_t>( race ), PlayerColor::BLUE );
        auto * c = world.getCastle( { tile % 36, tile / 36 } ); assert( c ); kingdom.GetCastles().push_back( c );
        world.getTile( tile ).setMainObjectType( MP2::OBJ_CASTLE );
        Maps::Map_Format::CastleMetadata meta; meta.customBuildings = true; meta.availableToHireMonsterCount.fill( 0 );
        if ( race == Race::SORC ) meta.availableToHireMonsterCount[1] = 200;
        for ( auto b : buildings ) meta.builtBuildings.push_back( b );
        c->loadFromResurrectionMap( meta ); c->GetArmy().Clean(); c->SetModes( Castle::ALLOW_TO_BUILD_TODAY );
        return c;
    };

    if(mode.find("cargo")==0) {
        auto * front=add(150,complete); auto * rear=add(258,complete);
        auto * hero=world.GetHeroes(Heroes::LORDKILBURN); assert(hero->Recruit(*front));
        hero->GetArmy().Clean(); hero->GetArmy().JoinTroop(Monster::ARCHER,5,false);hero->setAIRole(Heroes::Role::COURIER);
        front->GetArmy().JoinTroop(Monster::PEASANT,200,false);world.updatePassabilities();
        KC::Scope scope(kingdom);KC::setRoles(kingdom,{151},{});
        AI::KnightPolicyTestAccess::reinforce(*front);
        const auto cargo=hero->GetArmy().GetCountMonsters(Monster::PEASANT);assert(cargo>150);
        assert(KC::populationOrder(*hero)==258);
        if(mode=="cargo_reinforce") {
            front->GetArmy().JoinTroop(Monster::SWORDSMAN,200,false);
            AI::KnightPolicyTestAccess::reinforce(*front);
            AI::transferSlowestTroopsToGarrison(hero,front);
            assert(hero->GetArmy().GetCountMonsters(Monster::PEASANT)==cargo);
            assert(AI::KnightPolicyTestAccess::target(*hero)==258);
        }
        else if(mode=="cargo_meeting") {
            auto * fighter=world.GetHeroes(Heroes::SIRGALLANTH);assert(fighter->Recruit(PlayerColor::BLUE,fheroes2::Point(7,4)));
            fighter->setAIRole(Heroes::Role::FIGHTER);AI::HeroesAction(*hero,151);
            assert(hero->hasMetWithHero(fighter->GetID()));
            assert(hero->GetArmy().GetCountMonsters(Monster::PEASANT)==cargo);
        }
        else if(mode=="cargo_deliver" || mode=="cargo_arrive") {
            hero->Move2Dest(186);world.NewDay();hero->setAIRole(Heroes::Role::FIGHTER);
            assert(AI::KnightPolicyTestAccess::target(*hero)==258);
            hero->Move2Dest(258); if(mode=="cargo_arrive")AI::KnightPolicyTestAccess::reinforce(*rear);else KC::arrangePopulation(*hero,*rear);
            assert(rear->GetArmy().GetCountMonsters(Monster::PEASANT)>=cargo);
            assert(hero->GetArmy().GetCountMonsters(Monster::PEASANT)==0 && KC::populationOrder(*hero)<0);
        }
        else if(mode=="cargo_captured") {
            kingdom.GetCastles().pop_back();rear->SetColor(PlayerColor::RED);
            assert(KC::populationOrder(*hero)==150);KC::arrangePopulation(*hero,*front);
            assert(hero->GetArmy().GetCountMonsters(Monster::PEASANT)==0);
        }
        else if(mode=="cargo_timeout") {
            while(world.CountDay()<6)world.NewDay();
            assert(KC::populationOrder(*hero)==150);KC::arrangePopulation(*hero,*front);
            assert(hero->GetArmy().GetCountMonsters(Monster::PEASANT)==0);
        }
        else if(mode=="cargo_danger") {
            constructionContext->endangered.insert(258);
            assert(KC::populationOrder(*hero)==150);KC::arrangePopulation(*hero,*front);
            assert(hero->GetArmy().GetCountMonsters(Monster::PEASANT)==0);
        }
        kingdom.GetHeroes().clear();
    }
    else if(mode.find("naval")==0) {
        auto * port=add(150,{BUILD_CASTLE,BUILD_SHIPYARD});
        world.getTile(226).setMainObjectType(MP2::OBJ_FLOTSAM);world.updatePassabilities();
        auto * main=world.GetHeroes(Heroes::SIRGALLANTH);assert(main->Recruit(*port));
        const auto landArmy=KnightObserver::armyJson(main->GetArmy());
        auto * knight=world.GetHeroes(Heroes::LORDKILBURN);auto * sorc=world.GetHeroes(Heroes::ARIEL);auto * wizard=world.GetHeroes(Heroes::MYRINI);
        if(mode=="naval_poor")money(3000);else money(7500);
        if(mode=="naval_late")while(world.CountDay()<8)world.NewDay();
        
        if(mode=="naval_no_goal")world.getTile(226).setMainObjectType(MP2::OBJ_NONE);
        KC::Scope scope(kingdom);
        if(mode=="naval_danger")constructionContext->endangered.insert(150);
        const bool blocked=mode=="naval_poor" || mode=="naval_late" || mode=="naval_danger" || mode=="naval_no_goal";
        if(mode=="naval_origin") assert(KC::openingRecruit(*port,wizard,knight)==knight);
        else if(blocked) assert(!KC::openingRecruit(*port,knight,sorc));
        else {
            assert(KC::openingRecruit(*port,knight,sorc)==sorc);
            auto & recruits=kingdom.GetCurrentRecruits();recruits.SetHero1(knight);recruits.SetHero2(sorc);
            assert(KC::prepareNaval(kingdom).empty());assert(KnightObserver::armyJson(main->GetArmy())==landArmy);
            main->Move2Dest(152);
            assert(AI::KnightPolicyTestAccess::recruit(*port));assert(port->GetHero()==sorc);
            assert(port->GetArmy().JoinTroop(Monster::PEASANT,40,false));
            assert(KC::prepareNaval(kingdom).size()==1);
            assert(sorc->GetArmy().GetCountMonsters(Monster::SPRITE)==5 && sorc->GetArmy().GetOccupiedSlotCount()==5);
            assert(KnightObserver::armyJson(main->GetArmy())==landArmy);
            assert(port->GetArmy().GetCountMonsters(Monster::DWARF)>0);
            assert(port->BuyBuilding(BUILD_STATUE));assert(kingdom.GetFunds().gold==2750);
            AIWorldPathfinder path;path.reset();path.reEvaluateIfNeeded(*sorc);
            for(const auto & step:path.buildPath(226,false)) {
                if(world.getTile(step.GetIndex()).getMainObjectType(false)==MP2::OBJ_BOAT){AI::HeroesAction(*sorc,step.GetIndex());break;}
                sorc->Move2Dest(step.GetIndex());
            }
            assert(sorc->isShipMaster() && world.CountDay()==1);AI::OptimizeTroopsOrder(sorc->GetArmy());assert(sorc->GetArmy().GetOccupiedSlotCount()==5);
        }
        kingdom.GetHeroes().clear();
    }
    else if(mode.find("cash")==0) {
        auto * c=add(150,complete);c->GetArmy().JoinTroop(Monster::PEASANT,20,false);KC::Scope scope(kingdom);
        assert(AI::KnightEconomy::develop(*c,true) && c->hasTrainingGrounds());
        if(mode=="cash_income") {
            for(int tile=300;tile<315;++tile)add(tile,{BUILD_CASTLE},Race::SORC);
            world.NewDay(); money(1607);assert(AI::KnightEconomy::choose(*c,true).reason=="funded_mobilization_plan");
            assert(!AI::KnightEconomy::develop(*c,true) && c->hasTrainingGrounds());
        }
        else {
            while(world.CountDay()<20)world.NewDay();
            money(1607);assert(AI::KnightEconomy::choose(*c,true).reason=="cash_recovery");
            assert(AI::KnightEconomy::develop(*c,true) && c->isBuild(BUILD_WEL2));
            while(world.CountDay()<22)world.NewDay();money(30000);KC::refresh();
            assert(AI::KnightEconomy::choose(*c,true).reason=="recovering_mobilization");
        }
    }
    else if(mode.find("defense")==0) {
        auto buildings=complete;buildings.erase(std::remove(buildings.begin(),buildings.end(),BUILD_CAPTAIN),buildings.end());
        buildings.erase(std::remove(buildings.begin(),buildings.end(),BUILD_WEL2),buildings.end());
        if(mode=="defense_well")buildings.erase(std::remove(buildings.begin(),buildings.end(),BUILD_WELL),buildings.end());
        auto * c=add(150,buildings);c->GetArmy().JoinTroop(Monster::ARCHER,10,false);KC::Scope scope(kingdom);
        constructionContext->endangered.insert(150);AI::KnightPolicyTestAccess::defend(*c);
        assert(!c->isBuild(BUILD_CAPTAIN));assert(c->isBuild(mode=="defense_well" ? BUILD_WELL : BUILD_WEL2));
    }
    else if(mode.find("adventure")==0) {
        auto b=complete;b.back()=BUILD_TRAINING_GROUNDS;auto * c=add(150,b);c->GetArmy().JoinTroop(Monster::PEASANT,20,false);
        auto * hero=world.GetHeroes(Heroes::LORDKILBURN);assert(hero->Recruit(*c));hero->GetArmy().Clean();hero->GetArmy().JoinTroop(Monster::MEDUSA,1,false);
        auto & tile=world.getTile(186);tile.setMainObjectType(MP2::OBJ_RUINS);Maps::setMonsterCountOnTile(tile,20);world.updatePassabilities();hero->Move2Dest(186);
        money(10000);KC::Scope scope(kingdom);const auto floor=KC::operatingReserve(kingdom);
        if(mode=="adventure_human")Players::Get(PlayerColor::BLUE)->SetControl(CONTROL_HUMAN);
        {KC::Emergency emergency(mode=="adventure_emergency");AI::HeroesAction(*hero,186);}
        if(mode=="adventure")assert(kingdom.GetFunds().gold>=floor && hero->GetArmy().GetCountMonsters(Monster::MEDUSA)>1);
        else assert(kingdom.GetFunds().gold<floor);
        kingdom.GetHeroes().clear();
    }
    else assert(false);
    std::cout<<"PASS v12 "<<mode<<" day="<<world.CountDay()<<" gold="<<kingdom.GetFunds().gold<<'\n';
    kingdom.GetCastles().clear();
}
