// Observed J1 inventories and Utopeia-distance deliveries through production hooks.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "ai_hero_action.h"
#include "map_format_info.h"
#include "maps_tiles_helper.h"
#include "players.h"
#include "rand.h"
#include "week.h"
#include <iostream>
namespace AI {
struct KnightPolicyTestAccess {
 static Planner & planner() { auto & p=Planner::Get();p._regions.resize(world.getSize());return p; }
 static bool opening(Kingdom & k) { return planner().prepareOpeningTurn(k); }
 static int target(Heroes & h) { double score=0;return planner().getPriorityTarget(h,score); }
 static bool needs(Heroes & h) { return planner().openingPeasantsNeeded(h); }
 static void object(int tile,MP2::MapObjectType type) { planner()._mapActionObjects[tile]=type; }
};
}
int main(int argc,char ** argv) {
 assert(argc==3);const std::string mode=argv[1];Settings::Get().Read(argv[2]);
 Settings::Get().SetGameDifficulty(Difficulty::NORMAL);Settings::Get().SetGameType(Game::TYPE_HOTSEAT);Settings::Get().SetAIMoveSpeed(0);
 Settings::Get().GetPlayers().Init(Color::allPlayerColors());for(const auto c:PlayerColorsVector(Color::allPlayerColors()))Players::Get(c)->SetControl(CONTROL_AI);
 Rand::CurrentThreadRandomDevice()=Rand::PCG32(20261116);
 const bool opening=mode.find("opening_")==0;const bool cox=mode.find("cox")!=std::string::npos;
 const int width=opening&&!cox?108:144;world.generateUninitializedMap(width);world.NewDay();
 const int portTile=cox?9874:640;const int shoreRow=portTile/width+2;
 for(int t=0;t<static_cast<int>(world.getSize());++t) {
  auto & tile=world.getTile(t);tile.setIndex(t);tile.setTerrain(opening&&t/width>=shoreRow?0:Maps::Ground::GRASS_START_IMAGE_INDEX,0);
  tile.ClearFog(static_cast<PlayerColorsSet>(PlayerColor::BLUE));
 }
 auto & k=world.GetKingdom(PlayerColor::BLUE);
 const auto money=[&](int gold){k.OddFundsResource(k.GetFunds());Funds f;f.gold=gold;f.wood=f.ore=20;f.mercury=f.sulfur=f.crystal=f.gems=5;k.AddFundsResource(f);};money(opening?7500:50000);
 const auto add=[&](int tile,std::vector<int32_t> buildings){world.addCastle(tile,Race::KNGT,PlayerColor::BLUE);auto * c=world.getCastle({tile%width,tile/width});assert(c);
  k.GetCastles().push_back(c);world.getTile(tile).setMainObjectType(MP2::OBJ_CASTLE);Maps::Map_Format::CastleMetadata m;m.customBuildings=true;m.builtBuildings.assign(buildings.begin(),buildings.end());c->loadFromResurrectionMap(m);c->GetArmy().Clean();return c;};
 const auto army=[&](Heroes & h,std::initializer_list<std::pair<int,uint32_t>> units){h.GetArmy().Clean();for(const auto & u:units)assert(h.GetArmy().JoinTroop(u.first,u.second,false));};
 if(opening) {
  std::vector<int32_t> buildings{BUILD_CASTLE,BUILD_SHIPYARD};if(cox)buildings.insert(buildings.end(),{BUILD_WEL2,DWELLING_MONSTER1,DWELLING_MONSTER2});
  auto * port=add(portTile,buildings);auto * land=world.GetHeroes(cox?Heroes::ROLAND:Heroes::TYRO);assert(land->Recruit(*port));
  if(cox)army(*land,{{Monster::PEASANT,49},{Monster::ARCHER,4}});else army(*land,{{Monster::PEASANT,36},{Monster::ARCHER,3}});
  auto * knight=world.GetHeroes(Heroes::ECTOR);army(*knight,{{Monster::PEASANT,37},{Monster::ARCHER,5}});
  auto * gem=world.GetHeroes(Heroes::GEM);army(*gem,{{Monster::SPRITE,20}});
  auto * other=world.GetHeroes(Heroes::MYRINI);
  auto & recruits=k.GetCurrentRecruits();recruits.SetHero1(cox?gem:knight);recruits.SetHero2(cox?other:nullptr);
  const int reward=(shoreRow+2)*width+portTile%width;world.getTile(reward).setMainObjectType(MP2::OBJ_FLOTSAM);
  if(mode=="opening_poor")money(3000);
  if(mode=="opening_no_goal")world.getTile(reward).setMainObjectType(MP2::OBJ_NONE);
  if(mode=="opening_late")world.NewDay();
  if(mode=="opening_human")Players::Get(PlayerColor::BLUE)->SetControl(CONTROL_HUMAN);
  if(mode=="opening_cox_no_high") {port->GetArmy().Clean();army(*land,{{Monster::PEASANT,49}});
   Maps::Map_Format::CastleMetadata m;m.customBuildings=true;m.builtBuildings={BUILD_CASTLE,BUILD_SHIPYARD,BUILD_WEL2};port->loadFromResurrectionMap(m);}
  world.updatePassabilities();KC::Scope scope(k);
  if(mode=="opening_danger")constructionContext->endangered.insert(portTile);
  const auto before=k.GetFunds();const int beforeTile=land->GetIndex();const uint32_t beforeMoves=land->GetMovePoints();
  const bool blocked=mode=="opening_poor"||mode=="opening_no_goal"||mode=="opening_late"||mode=="opening_human"||mode=="opening_danger";
  const bool result=AI::KnightPolicyTestAccess::opening(k);assert(result==!blocked);
  if(blocked)assert(k.GetHeroes().size()==1&&k.GetFunds()==before&&land->GetIndex()==beforeTile);
  else if(mode=="opening_cox_no_high") {assert(k.GetHeroes().size()==2);assert(KC::prepareNaval(k).empty());assert(!KC::isOpeningSeaExplorer(*gem));}
  else {
   Heroes * sailor=cox?gem:knight;assert(k.GetHeroes().size()==2&&port->isBuild(BUILD_STATUE));
   assert(land->GetIndex()!=beforeTile&&land->GetMovePoints()<beforeMoves&&land->GetMovePoints()>0);
   assert(KC::isOpeningSeaExplorer(*sailor));
   const int low=cox?Monster::SPRITE:Monster::PEASANT;const std::array<int,5> formation{Monster::ARCHER,low,low,low,Monster::ARCHER};
   for(size_t n=0;n<5;++n)assert(sailor->GetArmy().GetTroop(n)->GetID()==formation[n]&&sailor->GetArmy().GetTroop(n)->GetCount()==1);
   if(cox) {assert(land->GetArmy().GetCountMonsters(Monster::ARCHER)==10&&land->GetArmy().GetCountMonsters(Monster::SPRITE)==17);
    assert(land->GetArmy().GetCountMonsters(Monster::PEASANT)==0&&port->GetArmy().GetCountMonsters(Monster::PEASANT)==61);assert(k.GetFunds().gold==1310);}
   else {assert(land->GetArmy().GetCountMonsters(Monster::PEASANT)==70&&land->GetArmy().GetCountMonsters(Monster::ARCHER)==6);assert(!port->GetArmy().isValid());assert(k.GetFunds().gold==2750);}
   assert(k.GetFunds().wood==10&&k.GetFunds().ore==15);
   const int target=AI::KnightPolicyTestAccess::target(*sailor);assert(target>=0);
   AIWorldPathfinder path;path.reset();path.reEvaluateIfNeeded(*sailor);sailor->GetPath().setPath(path.buildPath(target,true));sailor->SetMove(true);
   for(int n=0;n<12&&!sailor->isShipMaster()&&sailor->GetMovePoints()&&sailor->GetPath().isValidForMovement();++n)(void)sailor->Move(true);
   sailor->SetMove(false);assert(sailor->isShipMaster()&&sailor->GetMovePoints()==0&&world.CountDay()==1);
   AI::OptimizeTroopsOrder(sailor->GetArmy());for(size_t n=0;n<5;++n)assert(sailor->GetArmy().GetTroop(n)->GetID()==formation[n]);
   world.NewDay();assert(KC::navalTarget(*sailor)==target);assert(!AI::KnightPolicyTestAccess::opening(k));
  }
 }
 else if(mode.find("peasants_")==0) {
  auto * port=add(5678,{BUILD_CASTLE,BUILD_WEL2});auto * h=world.GetHeroes(Heroes::LORDKILBURN);assert(h->Recruit(*port));h->Move2Dest(5679);
  army(*h,{{Monster::ARCHER,1},{Monster::PEASANT,70}});
  world.updatePassabilities();KC::Scope scope(k);
  if(mode=="peasants_weak") {
   const int enemy=5824;world.getTile(enemy).setMainObjectType(MP2::OBJ_MONSTER);Maps::setMonsterOnTile(world.getTile(enemy),Monster::PEASANT,40);
   world.updatePassabilities();AI::KnightPolicyTestAccess::object(enemy,MP2::OBJ_MONSTER);
  }
  if(mode=="peasants_sufficient")army(*h,{{Monster::ARCHER,10},{Monster::SPRITE,17},{Monster::PEASANT,61}});
  const bool need=AI::KnightPolicyTestAccess::needs(*h);assert(need==(mode=="peasants_weak"));KC::settleOpeningTaxpayers(*h,*port,need);
  assert(h->GetArmy().GetCountMonsters(Monster::PEASANT)==(need?70:0));assert(port->GetArmy().GetCountMonsters(Monster::PEASANT)==(need?0:61));
  if(need) {
   h->Move2Dest(port->GetIndex());KC::arrangePopulation(*h,*port);AI::transferSlowestTroopsToGarrison(h,port);
   assert(h->GetArmy().GetCountMonsters(Monster::PEASANT)==70);world.NewDay();assert(!KC::keepOpeningLandPeasants(*h));
  }
 }
 else if(mode=="growth_plague") {
  auto * c=add(5678,{BUILD_CASTLE,BUILD_WELL,BUILD_WEL2,DWELLING_MONSTER1,DWELLING_MONSTER2});
  while(world.CountDay()<28) { world.NewDay(); }
  world.NewDay();
  const_cast<Week &>(world.GetWeekType())=Week(WeekName::PLAGUE);const auto before=c->getMonstersInDwelling(DWELLING_MONSTER2);
  c->ActionNewWeek();assert(c->getMonstersInDwelling(DWELLING_MONSTER2)==before/2);
 }
 else {
  const bool observed=mode=="supply_utopia_j192";
  while(world.CountDay()<(observed?192U:4U))world.NewDay();
  auto * farm=add(5678,{BUILD_CASTLE,BUILD_STATUE,BUILD_WEL2});auto * tg=add(5841,{BUILD_CASTLE,BUILD_STATUE,BUILD_TRAINING_GROUNDS,DWELLING_MONSTER1,BUILD_WELL});
  assert(tg->RecruitMonster(Troop(Monster::PEASANT,observed?5:9),false));tg->GetArmy().Clean();
  farm->GetArmy().JoinTroop(Monster::PEASANT,66,false);
  for(int id:{Monster::RANGER,Monster::VETERAN_PIKEMAN,Monster::MASTER_SWORDSMAN,Monster::CAVALRY,Monster::PALADIN})assert(tg->GetArmy().JoinTroop(id,10,false));
  auto * h=world.GetHeroes(Heroes::DARLANA);assert(h->Recruit(*farm));army(*h,{{Monster::ARCHER,5}});h->setAIRole(Heroes::Role::HUNTER);
  auto * other=world.GetHeroes(Heroes::SIRGALLANTH);assert(other->Recruit(PlayerColor::BLUE,{farm->GetIndex()%width-1,farm->GetIndex()/width+1}));army(*other,{{Monster::ARCHER,5}});other->setAIRole(Heroes::Role::HUNTER);
  if(observed) {
   farm->GetArmy().Clean();for(const auto & u:std::vector<std::pair<int,uint32_t>>{{45,4},{7,36},{5,42},{1,132}})assert(farm->GetArmy().JoinTroop(u.first,u.second,false));
   tg->GetArmy().Clean();for(const auto & u:std::vector<std::pair<int,uint32_t>>{{3,104},{9,32},{11,10},{5,97},{7,59}})assert(tg->GetArmy().JoinTroop(u.first,u.second,false));
   army(*h,{{3,86},{11,12},{56,13},{9,30},{15,43}});h->setAIRole(Heroes::Role::COURIER);
  }
  if(mode=="supply_peasant_pickup") {army(*h,{{Monster::PEASANT,1}});h->setAIRole(Heroes::Role::COURIER);h->Move2Dest(5677);}
  if(mode=="supply_sleep")h->SetModes(Heroes::SLEEPER);
  if(mode=="supply_fighter") {h->setAIRole(Heroes::Role::FIGHTER);other->setAIRole(Heroes::Role::FIGHTER);}
  world.updatePassabilities();KC::Scope scope(k);KC::setRoles(k,{5842},{});
  constructionContext->economic.erase(farm->GetIndex()); // Farm role need not already be rear/economic.
  if(mode=="supply_unsafe")constructionContext->endangered.insert(tg->GetIndex());
  if(mode=="supply_occupied") {auto * visitor=world.GetHeroes(Heroes::ECTOR);assert(visitor->Recruit(*tg));}
  const auto initial=KC::populationSupplyNeed(*tg);const auto cash=k.GetFunds();KC::preparePopulation(k);
  const bool blocked=mode=="supply_unsafe"||mode=="supply_occupied"||mode=="supply_fighter";
  if(blocked)assert(populationRoutes.empty());
  else {
   if(mode=="supply_peasant_pickup") {
    assert(populationRoutes.size()==1&&KC::populationOrder(*h)==5678&&KC::populationSupplyNeed(*tg)==0);
    KC::preparePopulation(k);assert(populationRoutes.size()==1);
    (void)AI::KnightPolicyTestAccess::planner();
    AIWorldPathfinder pickup;pickup.reset();pickup.reEvaluateIfNeeded(*h);h->GetPath().setPath(pickup.buildPath(5678,true));h->SetMove(true);(void)h->Move(true);h->SetMove(false);
    KC::arrangePopulation(*h,*farm);assert(h->GetArmy().GetCountMonsters(Monster::PEASANT)==initial+1);
   }
   assert(initial>0&&populationRoutes.size()==1&&KC::populationOrder(*h)==5841&&h->GetArmy().GetCountMonsters(Monster::PEASANT)==initial+(mode=="supply_peasant_pickup"?1:0));
   if(observed)assert(initial==6&&farm->GetArmy().GetCountMonsters(Monster::PEASANT)==126&&farm->GetArmy().GetCountMonsters(56)==13);
   assert(!h->Modes(Heroes::SLEEPER)&&KC::populationSupplyNeed(*tg)==0);KC::preparePopulation(k);assert(populationRoutes.size()==1);
   assert(KC::canReceivePopulation(*h,*tg));assert(AI::KnightPolicyTestAccess::target(*h)==5841);
   if(mode=="supply_direct_full") {h->Move2Dest(5841);KC::arrangePopulation(*h,*tg);}
   else {
    const uint32_t firstDay=world.CountDay();
    for(int pass=0;pass<3&&h->GetIndex()!=5841;++pass) {
     AIWorldPathfinder path;path.reset();path.reEvaluateIfNeeded(*h);h->GetPath().setPath(path.buildPath(5841,false));h->SetMove(true);
     for(int step=0;step<40&&h->GetMovePoints()&&h->GetPath().isValidForMovement()&&h->GetIndex()!=5841;++step)(void)h->Move(true);
     h->SetMove(false);if(h->GetIndex()!=5841)world.NewDay();
    }
    assert(h->GetIndex()==5841&&world.CountDay()<=firstDay+2);KC::arrangePopulation(*h,*tg);
   }
   assert(tg->GetArmy().HasMonster(Monster::PEASANT)&&h->GetArmy().GetCountMonsters(Monster::PEASANT)==0&&KC::populationOrder(*h)<0);
   assert(tg->GetArmy().GetOccupiedSlotCount()==5);assert(h->GetArmy().isValid());
  }
  if(mode=="supply_direct_full"||blocked)assert(k.GetFunds()==cash);
 }
 std::cout<<"PASS v14 "<<mode<<" day="<<world.CountDay()<<" gold="<<k.GetFunds().gold<<'\n';k.GetHeroes().clear();k.GetCastles().clear();
}
