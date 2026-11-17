// Native reproduction of logs9 Utopeia J143: observed positions and inventories.
// Original map terrain with controlled objects/fog isolates route policy; no save replay.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "ai_hero_action.h"
#include "map_format_info.h"
#include "players.h"
#include "rand.h"
#include "game_mode.h"
#include "game_over.h"
#include "game_auto_playtest.h"
#include "ui_scrollbar.h"
#include <iostream>
// Drawing/input wrappers only; production planning and movement stay native.
extern "C" void __wrap__ZN9Interface11StatusPanel18drawAITurnProgressEj(void *, uint32_t) {}
fheroes2::Image headlessSlider(const fheroes2::Image &, bool, int32_t, int32_t, int32_t, const fheroes2::Rect &, const fheroes2::Rect &)
 asm("__wrap__ZN8fheroes223generateScrollbarSliderERKNS_5ImageEbiiiRKNS_10RectBase2DIiiEES6_");
fheroes2::Image headlessSlider(const fheroes2::Image &, bool, int32_t, int32_t, int32_t, const fheroes2::Rect &, const fheroes2::Rect &) {return fheroes2::Image(1,1);}
namespace AI {
struct KnightPolicyTestAccess {
 static Planner & planner() {auto & p=Planner::Get();p._regions.resize(world.getSize());return p;}
 static void reinforce(Castle & c) {planner().reinforceCastle(c,true);}
};
}
int main(int argc,char ** argv) {
 assert(argc==3);const std::string mode=argv[1];Settings::Get().Read(argv[2]);
 Settings::Get().SetGameDifficulty(Difficulty::NORMAL);Settings::Get().SetGameType(Game::TYPE_HOTSEAT);Settings::Get().SetAIMoveSpeed(0);
 Settings::Get().GetPlayers().Init(Color::allPlayerColors());for(const auto c:PlayerColorsVector(Color::allPlayerColors()))Players::Get(c)->SetControl(CONTROL_AI);
 Rand::CurrentThreadRandomDevice()=Rand::PCG32(20261117);world.generateUninitializedMap(144);
 for(int t=0;t<static_cast<int>(world.getSize());++t){auto & x=world.getTile(t);x.setIndex(t);x.setTerrain(Maps::Ground::GRASS_START_IMAGE_INDEX,0);x.ClearFog(static_cast<PlayerColorsSet>(PlayerColor::BLUE));}
 while(world.CountDay()<237)world.NewDay();
 auto & k=world.GetKingdom(PlayerColor::BLUE);k.OddFundsResource(k.GetFunds());Funds funds;funds.gold=50000;funds.wood=funds.ore=funds.mercury=funds.sulfur=funds.crystal=funds.gems=100;k.AddFundsResource(funds);
 const auto totalPopulation=[&]() {
  uint64_t total=0;for(const auto * hero:k.GetHeroes())total+=hero->GetArmy().GetCountMonsters(Monster::PEASANT);
  for(const auto * city:k.GetCastles()){total+=city->GetArmy().GetCountMonsters(Monster::PEASANT);if(city->GetRace()==Race::KNGT)total+=city->conscriptionDaySnapshot().hut;}
  return total;
 };
 const auto add=[&](int tile,int race,bool tg){world.addCastle(tile,static_cast<uint8_t>(race),PlayerColor::BLUE);auto * c=world.getCastle({tile%144,tile/144});assert(c);k.AddCastle(c);world.getTile(tile).setMainObjectType(MP2::OBJ_CASTLE);
  Maps::Map_Format::CastleMetadata m;m.customBuildings=true;m.builtBuildings={BUILD_CASTLE,BUILD_STATUE,BUILD_WELL,DWELLING_MONSTER1,DWELLING_MONSTER2,DWELLING_MONSTER3,DWELLING_MONSTER4,DWELLING_MONSTER5,DWELLING_MONSTER6,tg?BUILD_TRAINING_GROUNDS:BUILD_WEL2};c->loadFromResurrectionMap(m);c->GetArmy().Clean();c->SetModes(Castle::ALLOW_TO_BUILD_TODAY);return c;};
 auto * source=add(5678,Race::KNGT,false);auto * target=add(2232,Race::KNGT,mode!="fiscal_unchanged");auto * emergency=add(9727,Race::SORC,false);
 auto * h=world.GetHeroes(Heroes::ECTOR);assert(h->Recruit(*source));h->GetArmy().Clean();
 for(const auto & u:std::vector<std::pair<int,uint32_t>>{{62,28},{7,95},{9,30},{5,145},{1,6}})assert(h->GetArmy().JoinTroop(u.first,u.second,false));
 h->setAIRole(Heroes::Role::HUNTER);world.updatePassabilities();(void)AI::KnightPolicyTestAccess::planner();KC::Scope scope(k);KC::setRoles(k,{2233},{});
 if(mode.find("cargo_")==0) {
  KC::notePopulationRoute(*h,target->GetIndex());
  if(mode!="cargo_arrival")KC::cancelPopulationRoute(*h,"critical_military_order");
  if(mode=="cargo_stale_owner") {h->SetColor(PlayerColor::RED);assert(KC::cancelledCargo(*h)==0);h->SetColor(PlayerColor::BLUE);}
  else if(mode=="cargo_dismiss") {h->Dismiss(1);assert(h->Recruit(*source));h->GetArmy().Clean();assert(h->GetArmy().JoinTroop(Monster::PEASANT,30,false));assert(KC::cancelledCargo(*h)==0);}
  else {
   if(mode=="cargo_only") {h->GetArmy().Clean();assert(h->GetArmy().JoinTroop(Monster::PEASANT,7,false));}
   Castle * depot=mode=="cargo_return"?source:emergency;
   if(mode=="cargo_return")assert(source->GetArmy().JoinTroop(Monster::PEASANT,653,false));
   if(mode=="cargo_full")for(int id:{2,10,20,38,47})assert(depot->GetArmy().JoinTroop(id,10,false));
   h->Move2Dest(depot->GetIndex());constructionContext->endangered.insert(depot->GetIndex());
   const auto beforeTotal=totalPopulation();
   const auto before=depot->GetArmy().GetCountMonsters(Monster::PEASANT)+h->GetArmy().GetCountMonsters(Monster::PEASANT);
   AI::KnightPolicyTestAccess::reinforce(*depot);
   assert(depot->GetArmy().GetCountMonsters(Monster::PEASANT)+h->GetArmy().GetCountMonsters(Monster::PEASANT)>=before);
   assert(depot->GetArmy().GetCountMonsters(Monster::PEASANT)>=(mode=="cargo_return"?659U:6U));
   assert(KC::cancelledCargo(*h)==0&&h->GetArmy().isValid()&&KC::populationDepositProtected(*depot));
   const auto held=depot->GetArmy().GetCountMonsters(Monster::PEASANT);AI::KnightPolicyTestAccess::reinforce(*depot);assert(depot->GetArmy().GetCountMonsters(Monster::PEASANT)>=held);assert(totalPopulation()==beforeTotal);
  }
 }
 else {
  source->GetArmy().JoinTroop(Monster::PEASANT,417,false);h->GetArmy().Clean();assert(h->GetArmy().JoinTroop(Monster::ARCHER,20,false));
  if(mode=="role_pending") {
   populationRoutes[h->GetID()]={world.GetMapSeed(),world.CountDay(),Economy::traceState().segment,static_cast<int>(k.GetColor()),2232,5678,4};populationRoutes[h->GetID()].supply=true;
  }
  else {assert(h->GetArmy().JoinTroop(Monster::PEASANT,4,false));KC::notePopulationRoute(*h,2232);}
  if(mode!="fiscal_unchanged"){assert(target->BuyBuilding(BUILD_WEL2));assert(!target->hasTrainingGrounds());}
  if(mode=="role_near")h->Move2Dest(2231);
  if(mode=="role_enroute")h->Move2Dest(5677);
  if(mode=="role_no_depot") {constructionContext->endangered.insert(5678);constructionContext->endangered.insert(2232);}
  const auto population = [&]() {
   uint64_t total=h->GetArmy().GetCountMonsters(Monster::PEASANT);
   for(const auto * city:k.GetCastles())if(city->GetRace()==Race::KNGT)total+=city->GetArmy().GetCountMonsters(Monster::PEASANT)+city->conscriptionDaySnapshot().hut;
   return total;
  };
  const auto beforePopulation=population();
  const int order=KC::populationOrder(*h);
  if(mode=="role_pending")assert(order<0&&populationRoutes.empty()&&source->GetArmy().GetCountMonsters(Monster::PEASANT)==417);
  else if(mode=="role_no_depot")assert(order<0&&KC::cancelledCargo(*h)==4&&KC::keepPopulationCargo(*h));
  else if(mode=="fiscal_unchanged")assert(order==2232&&!populationRoutes.at(h->GetID()).supply);
  else {
   assert(order==(mode=="role_near"?2232:5678));
   if(h->GetIndex()!=order){AIWorldPathfinder path;path.reset();path.reEvaluateIfNeeded(*h);h->GetPath().setPath(path.buildPath(order,true));h->SetMove(true);for(int step=0;step<5&&h->GetIndex()!=order;++step)(void)h->Move(true);h->SetMove(false);}
   assert(h->GetIndex()==order);auto * city=order==5678?source:target;KC::arrangePopulation(*h,*city);assert(h->GetArmy().GetCountMonsters(Monster::PEASANT)==0&&KC::populationOrder(*h)<0);
   assert(city->GetArmy().GetCountMonsters(Monster::PEASANT)>=(order==5678?421U:4U));
   assert(population()==beforePopulation);
  }
 }
 std::cout<<"PASS v17 "<<mode<<"\n";k.GetHeroes().clear();k.GetCastles().clear();
}
