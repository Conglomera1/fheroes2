// Physical anticipatory resupply, shared cargo reservations, and six opening crews.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "ai_hero_action.h"
#include "map_format_info.h"
#include "players.h"
#include "rand.h"
#include <iostream>
namespace AI {
struct KnightPolicyTestAccess {
 static Planner & planner() { auto & p=Planner::Get();p._regions.resize(world.getSize());return p; }
 static int target(Heroes & h) { double score=0;return planner().getPriorityTarget(h,score); }
 static void reinforce(Castle & c) { planner().reinforceCastle(c); }
 static void emergency(int tile) { planner()._priorityTargets[tile]=PriorityTask(PriorityTaskType::DEFEND,151); }
};
}
int main(int argc,char ** argv) {
    assert(argc==3);const std::string mode=argv[1];Settings::Get().Read(argv[2]);
    Settings::Get().SetGameDifficulty(Difficulty::NORMAL);Settings::Get().SetGameType(Game::TYPE_HOTSEAT);Settings::Get().SetAIMoveSpeed(0);
    Settings::Get().GetPlayers().Init(Color::allPlayerColors());
    for(const auto color:PlayerColorsVector(Color::allPlayerColors()))Players::Get(color)->SetControl(CONTROL_AI);
    Rand::CurrentThreadRandomDevice()=Rand::PCG32(20261115);world.generateUninitializedMap(36);world.NewDay();
    for(int t=0;t<static_cast<int>(world.getSize());++t) {
        world.getTile(t).setIndex(t);world.getTile(t).setTerrain(mode.find("crew_")==0&&t/36>=6?0:Maps::Ground::GRASS_START_IMAGE_INDEX,0);
        world.getTile(t).ClearFog(static_cast<PlayerColorsSet>(PlayerColor::BLUE));
    }
    auto & kingdom=world.GetKingdom(PlayerColor::BLUE);Funds funds;funds.gold=50000;funds.wood=funds.ore=funds.mercury=funds.sulfur=funds.crystal=funds.gems=100;
    kingdom.OddFundsResource(kingdom.GetFunds());kingdom.AddFundsResource(funds);
    const auto add=[&](int tile,int race,bool tg) {
        world.addCastle(tile,static_cast<uint8_t>(race),PlayerColor::BLUE);auto * c=world.getCastle({tile%36,tile/36});assert(c);kingdom.GetCastles().push_back(c);
        world.getTile(tile).setMainObjectType(MP2::OBJ_CASTLE);Maps::Map_Format::CastleMetadata meta;meta.customBuildings=true;
        meta.availableToHireMonsterCount.fill(0);meta.builtBuildings={BUILD_CASTLE,BUILD_STATUE,BUILD_MARKETPLACE,BUILD_SHIPYARD,BUILD_WELL,BUILD_TAVERN,
            DWELLING_MONSTER1,DWELLING_MONSTER2,DWELLING_MONSTER3,DWELLING_MONSTER4,tg?BUILD_TRAINING_GROUNDS:BUILD_WEL2};
        c->loadFromResurrectionMap(meta);c->GetArmy().Clean();return c;
    };
    if(mode.find("crew_")==0) {
        const std::map<std::string,std::pair<int,int>> factions{{"crew_knight",{Race::KNGT,Heroes::LORDKILBURN}},
            {"crew_barbarian",{Race::BARB,Heroes::THUNDAX}},{"crew_sorceress",{Race::SORC,Heroes::ARIEL}},
            {"crew_warlock",{Race::WRLK,Heroes::ARIE}},{"crew_wizard",{Race::WZRD,Heroes::MYRINI}},
            {"crew_necromancer",{Race::NECR,Heroes::SANDRO}}};
        const auto it=factions.find(mode);const int race=it==factions.end()?Race::KNGT:it->second.first;
        auto * c=add(150,Race::KNGT,false);auto * hero=world.GetHeroes(it==factions.end()?Heroes::LORDKILBURN:it->second.second);assert(hero->Recruit(*c));
        auto & a=hero->GetArmy();auto & g=c->GetArmy();a.Clean();
        const int low=Monster(race,DWELLING_MONSTER1).GetID();const int base=Monster(race,DWELLING_MONSTER2).GetID();
        const int high=mode=="crew_upgrade"?Monster(base).GetUpgrade().GetID():base;
        assert(a.JoinTroop(low,mode=="crew_short_low"?2:30,false));
        if(mode!="crew_no_high")assert(g.JoinTroop(high,mode=="crew_short_high"?1:8,false));
        if(mode=="crew_full") {
            g.Clean();for(int id:{Monster::GOBLIN,Monster::SPRITE,Monster::CENTAUR,Monster::HALFLING,Monster::ARCHER})assert(g.JoinTroop(id,8,false));
        }
        world.updatePassabilities();KC::Scope scope(kingdom);const auto beforeA=KnightObserver::armyJson(a),beforeG=KnightObserver::armyJson(g);
        const Funds beforeFunds=kingdom.GetFunds();const bool expected=mode!="crew_no_high"&&mode!="crew_short_high"&&mode!="crew_short_low"&&mode!="crew_full";
        assert(KC::prepareOpeningExplorer(*hero,*c)==expected);
        if(!expected)assert(KnightObserver::armyJson(a)==beforeA&&KnightObserver::armyJson(g)==beforeG&&!KC::isOpeningSeaExplorer(*hero));
        else {
            const std::array<int,5> want=race==Race::KNGT||race==Race::BARB?std::array<int,5>{high,low,low,low,high}:std::array<int,5>{low,high,low,high,low};
            const auto verify=[&](){for(size_t n=0;n<5;++n)assert(a.GetTroop(n)->GetID()==want[n]&&a.GetTroop(n)->GetCount()==1);};
            verify();assert(g.GetCountMonsters(low)==27&&g.GetCountMonsters(high)==6);AI::OptimizeTroopsOrder(a);verify();
            hero->SetShipMaster(true);KC::markOpeningEmbarked(*hero);while(world.CountDay()<9)world.NewDay();AI::OptimizeTroopsOrder(a);verify();
            hero->SetShipMaster(false);assert(!KC::isOpeningSeaExplorer(*hero));AI::OptimizeTroopsOrder(a);assert(a.GetOccupiedSlotCount()==2);
        }
        if(mode!="supply_source_reserved"&&mode!="supply_native")assert(beforeFunds==kingdom.GetFunds());
    }
    else {
        while(world.CountDay()<5)world.NewDay(); // Hut empty; next weekly growth is still three days away.
        auto * tg=add(150,Race::KNGT,true);auto * farm=add(258,Race::KNGT,false);farm->GetArmy().JoinTroop(Monster::PEASANT,100,false);
        auto * first=world.GetHeroes(Heroes::LORDKILBURN);assert(first->Recruit(PlayerColor::BLUE,{8,6}));
        auto * second=world.GetHeroes(Heroes::SIRGALLANTH);assert(second->Recruit(PlayerColor::BLUE,{9,6}));
        for(auto * hero:{first,second}) {hero->GetArmy().Clean();assert(hero->GetArmy().JoinTroop(Monster::ARCHER,5,false));hero->setAIRole(Heroes::Role::COURIER);}
        world.updatePassabilities();KC::Scope scope(kingdom);KC::setRoles(kingdom,{151},{});
        assert(tg->RecruitMonster(Troop(Monster::PEASANT,tg->conscriptionDaySnapshot().hut),false));tg->GetArmy().Clean();
        const uint32_t need=KC::populationSupplyNeed(*tg);assert(need>0);
        if(mode=="supply_buffer")tg->GetArmy().JoinTroop(Monster::PEASANT,need,false);
        if(mode=="supply_growth") {auto snap=tg->conscriptionDaySnapshot();(void)snap;Maps::Map_Format::CastleMetadata meta;meta.customBuildings=true;
            meta.builtBuildings={BUILD_CASTLE,BUILD_TRAINING_GROUNDS,DWELLING_MONSTER1,BUILD_WELL};meta.availableToHireMonsterCount[0]=100;tg->loadFromResurrectionMap(meta);assert(KC::populationSupplyNeed(*tg)==0);}
        if(mode=="supply_danger")constructionContext->endangered.insert(150);
        if(mode=="supply_source_danger")constructionContext->endangered.insert(258);
        if(mode=="supply_empty")farm->GetArmy().Clean();
        if(mode=="supply_fighters")for(auto * h:{first,second})h->setAIRole(Heroes::Role::FIGHTER);
        if(mode=="supply_human")Players::Get(PlayerColor::BLUE)->SetControl(CONTROL_HUMAN);
        if(mode=="supply_full")for(int id:{Monster::GOBLIN,Monster::SPRITE,Monster::CENTAUR,Monster::HALFLING,Monster::ARCHER})tg->GetArmy().JoinTroop(id,1,false);
        if(mode=="supply_water")for(int x=0;x<36;++x)world.getTile(216+x).setTerrain(0,0);
        if(mode=="supply_occupied") {auto * blocker=world.GetHeroes(Heroes::ECTOR);assert(blocker->Recruit(*tg));}
        if(mode=="supply_existing") {first->GetArmy().JoinTroop(Monster::PEASANT,need,false);KC::notePopulationRoute(*first,150);}
        world.updatePassabilities();const Funds beforeFunds=kingdom.GetFunds();KC::preparePopulation(kingdom);
        const bool blocked=mode=="supply_buffer"||mode=="supply_growth"||mode=="supply_danger"||mode=="supply_source_danger"||mode=="supply_empty"
            ||mode=="supply_fighters"||mode=="supply_human"||mode=="supply_water"||mode=="supply_occupied"||mode=="supply_existing";
        unsigned pickups=0;Heroes * chosen=nullptr;
        for(auto * h:{first,second})if(KC::populationOrder(*h)==258){++pickups;chosen=h;}
        if(blocked)assert(pickups==0);
        else {
            assert(pickups==1&&chosen&&KC::populationSupplyNeed(*tg)==0);auto * idle=chosen==first?second:first;
            assert(chosen->GetArmy().GetCountMonsters(Monster::PEASANT)==0);assert(AI::KnightPolicyTestAccess::target(*chosen)==258);
            KC::preparePopulation(kingdom);assert(populationRoutes.size()==1);assert(KC::populationOrder(*idle)<0);
            assert(KC::populationAvailableStock(*farm)==100-need);
            if(mode=="supply_source_reserved") {
                auto * other=add(366,Race::KNGT,true);
                assert(other->RecruitMonster(Troop(Monster::PEASANT,other->conscriptionDaySnapshot().hut),false));other->GetArmy().Clean();
                farm->GetArmy().Clean();farm->GetArmy().JoinTroop(Monster::PEASANT,need,false);
                assert(KC::populationAvailableStock(*farm)==0);
                KC::preparePopulation(kingdom);
                assert(populationRoutes.size()==1&&idle->GetArmy().GetCountMonsters(Monster::PEASANT)==0&&farm->GetArmy().GetCountMonsters(Monster::PEASANT)==need);
                assert(KC::populationOrder(*chosen)==258);
            }
            else if(mode=="supply_capture") {kingdom.GetCastles().erase(std::find(kingdom.GetCastles().begin(),kingdom.GetCastles().end(),farm));farm->SetColor(PlayerColor::RED);
                assert(KC::populationOrder(*chosen)<0&&KC::populationSupplyNeed(*tg)==need);}
            else if(mode=="supply_cancel") {KC::cancelPopulationRoute(*chosen,"test_cancel");assert(KC::populationSupplyNeed(*tg)==need);}
            else if(mode=="supply_timeout") {while(world.CountDay()<10)world.NewDay();assert(KC::populationOrder(*chosen)<0);}
            else if(mode=="supply_emergency") {AI::KnightPolicyTestAccess::emergency(chosen->GetIndex());(void)AI::KnightPolicyTestAccess::target(*chosen);assert(populationRoutes.empty());}
            else {
                chosen->Move2Dest(258);if(mode=="supply_native")AI::KnightPolicyTestAccess::reinforce(*farm);else KC::arrangePopulation(*chosen,*farm);
                assert(chosen->GetArmy().GetCountMonsters(Monster::PEASANT)==need&&farm->GetArmy().GetCountMonsters(Monster::PEASANT)==(mode=="supply_native"?112:100)-need);
                assert(KC::populationOrder(*chosen)==150&&KC::populationSupplyNeed(*tg)==0);
                idle->Move2Dest(294);KC::arrangePopulation(*idle,*farm); // outside farm: no transfer
                assert(idle->GetArmy().GetCountMonsters(Monster::PEASANT)==0);
                KC::preparePopulation(kingdom);assert(populationRoutes.size()==1);
                chosen->Move2Dest(150);if(mode=="supply_native")AI::KnightPolicyTestAccess::reinforce(*tg);else KC::arrangePopulation(*chosen,*tg);
                assert(tg->GetArmy().GetCountMonsters(Monster::PEASANT)==need&&chosen->GetArmy().GetCountMonsters(Monster::PEASANT)==0);
                assert(KC::populationOrder(*chosen)<0&&KC::populationSupplyNeed(*tg)==0);
                // A second actual visit must not load the same requested buffer.
                idle->Move2Dest(258);KC::arrangePopulation(*idle,*farm);assert(idle->GetArmy().GetCountMonsters(Monster::PEASANT)==0);
            }
        }
        if(mode!="supply_source_reserved"&&mode!="supply_native")assert(beforeFunds==kingdom.GetFunds());
    }
    std::cout<<"PASS v13 "<<mode<<" day="<<world.CountDay()<<" gold="<<kingdom.GetFunds().gold<<'\n';
    kingdom.GetHeroes().clear();kingdom.GetCastles().clear();
}
