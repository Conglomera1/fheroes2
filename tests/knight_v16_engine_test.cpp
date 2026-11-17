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
 static Planner & planner() { auto & p=Planner::Get();p._regions.resize(world.getSize());return p; }
 static int target(Heroes & h) {double score=0;return planner().getPriorityTarget(h,score);}
 static void reinforce(Castle & c) {planner().reinforceCastle(c);}
};
}
int main(int argc,char ** argv) {
 assert(argc==3);const std::string mode=argv[1];Settings::Get().Read(argv[2]);
 Settings::Get().SetGameDifficulty(Difficulty::NORMAL);Settings::Get().SetGameType(Game::TYPE_HOTSEAT);Settings::Get().SetAIMoveSpeed(0);
 Settings::Get().GetPlayers().Init(Color::allPlayerColors());for(const auto c:PlayerColorsVector(Color::allPlayerColors()))Players::Get(c)->SetControl(CONTROL_AI);
 Rand::CurrentThreadRandomDevice()=Rand::PCG32(20261117);Maps::Map_Format::MapFormat observed;assert(Maps::Map_Format::loadMap("maps/Utopeia.fh2m",observed));assert(observed.width==144);world.generateUninitializedMap(144);
 for(int t=0;t<static_cast<int>(world.getSize());++t) {auto & tile=world.getTile(t);tile.setIndex(t);tile.setTerrain(observed.tiles[t].terrainIndex,observed.tiles[t].terrainFlags);tile.ClearFog(static_cast<PlayerColorsSet>(PlayerColor::BLUE));}
 while(world.CountDay()<143)world.NewDay();
 auto & k=world.GetKingdom(PlayerColor::BLUE);k.OddFundsResource(k.GetFunds());Funds money;money.gold=50000;money.wood=money.ore=100;k.AddFundsResource(money);
 const auto add=[&](int tile,bool tg){world.addCastle(tile,Race::KNGT,PlayerColor::BLUE);auto * c=world.getCastle({tile%144,tile/144});assert(c);k.AddCastle(c);
  world.getTile(tile).setMainObjectType(MP2::OBJ_CASTLE);Maps::Map_Format::CastleMetadata meta;meta.customBuildings=true;meta.builtBuildings={BUILD_CASTLE,BUILD_STATUE,BUILD_WELL,DWELLING_MONSTER1,tg?BUILD_TRAINING_GROUNDS:BUILD_WEL2};
  c->loadFromResurrectionMap(meta);c->GetArmy().Clean();return c;};
 auto * farm=add(5841,false);auto * tg=add(5678,true);assert(tg->RecruitMonster(Troop(Monster::PEASANT,5),false));tg->GetArmy().Clean();
 for(const auto & u:std::vector<std::pair<int,uint32_t>>{{44,5},{15,5},{7,12},{1,283}})assert(farm->GetArmy().JoinTroop(u.first,u.second,false));
 for(const auto & u:std::vector<std::pair<int,uint32_t>>{{3,7},{5,13},{13,5},{9,5},{12,21}})assert(tg->GetArmy().JoinTroop(u.first,u.second,false));
 auto * h=world.GetHeroes(Heroes::JEZEBEL);assert(h->Recruit(PlayerColor::BLUE,{5411%144,5411/144}));h->GetArmy().Clean();
 for(const auto & u:std::vector<std::pair<int,uint32_t>>{{3,39},{11,4},{7,50},{20,5},{46,4}})assert(h->GetArmy().JoinTroop(u.first,u.second,false));
 h->setAIRole(Heroes::Role::HUNTER);
 Players::SetPlayerInGame(PlayerColor::BLUE,true);world.updatePassabilities();
 if(mode=="full_turn") {
  Players::Get(PlayerColor::GREEN)->SetControl(CONTROL_HUMAN);
  for(const auto color:PlayerColorsVector(Color::allPlayerColors()))Players::SetPlayerInGame(color,color==PlayerColor::BLUE||color==PlayerColor::GREEN);
  const int enemyTile=144*70+20;world.addCastle(enemyTile,Race::KNGT,PlayerColor::GREEN);
  auto * enemy=world.getCastle({enemyTile%144,enemyTile/144});assert(enemy);world.GetKingdom(PlayerColor::GREEN).AddCastle(enemy);
  world.getTile(enemyTile).setMainObjectType(MP2::OBJ_CASTLE);enemy->GetArmy().Clean();assert(enemy->GetArmy().JoinTroop(Monster::BLACK_DRAGON,1000,false));world.updatePassabilities();
  world.ComputeStaticAnalysis();Settings::Get().SetGameType(Game::TYPE_AUTO_PLAYTEST);Settings::Get().SetCurrentColor(PlayerColor::BLUE);GameOver::Result::Get().Reset();fheroes2::AutoPlaytest::instance().reset(Color::allPlayerColors());
  for(unsigned turn=0;turn<4;++turn) {
   assert(AI::Planner::Get().KingdomTurn(k)==fheroes2::GameMode::END_TURN);
   if(world.CountDay()>143&&h->GetArmy().GetCountMonsters(Monster::PEASANT)==0&&populationRoutes.count(h->GetID())==0&&tg->GetArmy().GetCountMonsters(Monster::PEASANT)>=6)break;
   world.NewDay();k.ActionNewDayResourceUpdate({});
  }
  assert(world.CountDay()<=145&&tg->GetArmy().GetCountMonsters(Monster::PEASANT)>=6);
  while(world.CountDay()<147){world.NewDay();k.ActionNewDayResourceUpdate({});}
  assert(tg->conscriptionDaySnapshot().state.total==10);
  std::cout<<"PASS v16 full_turn conscripted="<<tg->conscriptionDaySnapshot().state.total<<"\n";
  k.GetHeroes().clear();k.GetCastles().clear();return 0;
 }
 KC::Scope scope(k);KC::setRoles(k,{5679},{});
 const uint32_t need=KC::populationSupplyNeed(*tg);assert(need==6);
 AIWorldPathfinder p;p.reset();p.reEvaluateIfNeeded(*h);const auto pickup=p.getDistance(5841);
 AIWorldPathfinder d;d.reset();d.reEvaluateIfNeeded(5841,k.GetColor(),h->GetArmy().GetStrength(),Skill::Level::BASIC);const auto delivery=d.getDistance(5678);
 std::cout<<"GEOMETRY pickup="<<pickup<<" delivery="<<delivery<<" need="<<need<<" load="<<KC::canLoadPopulation(*h,*farm,need)<<" receive="<<KC::canReceivePopulation(*h,*tg)<<std::endl;
 if(mode=="control") {
  while(world.CountDay()<147){world.NewDay();k.ActionNewDayResourceUpdate({});}
  assert(tg->conscriptionDaySnapshot().state.total==7);
  std::cout<<"PASS v16 control conscripted=7 missed=3\n";
  k.GetHeroes().clear();k.GetCastles().clear();return 0;
 }
 if(mode=="danger")constructionContext->endangered.insert(5678);
 if(mode=="military")h->setAIRole(Heroes::Role::FIGHTER);
 if(mode=="loading_blocked") {
  farm->GetArmy().Clean();for(int id:{44,15,56,48,1})assert(farm->GetArmy().JoinTroop(id,100,false));
 }
 KC::preparePopulation(k);
 if(mode=="danger"||mode=="military"||mode=="loading_blocked") {
  assert(populationRoutes.empty()&&h->GetArmy().GetCountMonsters(Monster::PEASANT)==0);
  std::cout<<"PASS v16 "<<mode<<" refused\n";k.GetHeroes().clear();k.GetCastles().clear();return 0;
 }
 Heroes * visitor=nullptr;
 {
  assert(populationRoutes.size()==1&&KC::populationOrder(*h)==5841);
  KC::preparePopulation(k);assert(populationRoutes.size()==1&&KC::populationSupplyNeed(*tg)==0);
  (void)AI::KnightPolicyTestAccess::planner();
  for(unsigned turn=0;turn<5&&h->GetIndex()!=5678;++turn) {
   for(unsigned leg=0;leg<2&&h->GetMovePoints()&&h->GetIndex()!=5678;++leg) {
    const int waypoint=AI::KnightPolicyTestAccess::target(*h);assert(waypoint==5841||waypoint==5678);
    AIWorldPathfinder path;path.reset();path.reEvaluateIfNeeded(*h);h->GetPath().setPath(path.buildPath(waypoint,true));h->SetMove(true);
    for(unsigned step=0;step<100&&h->GetMovePoints()&&h->GetPath().isValidForMovement()&&h->GetIndex()!=waypoint;++step)(void)h->Move(true);
    h->SetMove(false);
    if(h->GetIndex()==5841) {
     AI::KnightPolicyTestAccess::reinforce(*farm);assert(h->GetArmy().GetCountMonsters(Monster::PEASANT)==need);
     if(mode=="far_occupied"&&!visitor){visitor=world.GetHeroes(Heroes::ECTOR);assert(visitor->Recruit(*tg));assert(KC::populationOrder(*h)==5678&&KC::keepPopulationCargo(*h));}
    }
    if(h->GetIndex()==5678)AI::KnightPolicyTestAccess::reinforce(*tg);
   }
   if(h->GetIndex()!=5678){world.NewDay();k.ActionNewDayResourceUpdate({});if(visitor)visitor->Move2Dest(5677);}
  }
  assert(h->GetIndex()==5678&&world.CountDay()<=145);
  assert(tg->GetArmy().GetCountMonsters(Monster::PEASANT)>=need&&h->GetArmy().GetCountMonsters(Monster::PEASANT)==0&&KC::populationOrder(*h)<0);
  const auto arrival=world.CountDay();
  while(world.CountDay()<147){world.NewDay();k.ActionNewDayResourceUpdate({});}
  assert(tg->conscriptionDaySnapshot().state.total==10);
  assert(tg->GetArmy().GetCountMonsters(Monster::PEASANT)+tg->conscriptionDaySnapshot().hut==3);
  std::cout<<"PASS v16 "<<mode<<" delivered arrival="<<arrival<<" conscripted="<<tg->conscriptionDaySnapshot().state.total<<" remaining="<<tg->GetArmy().GetCountMonsters(Monster::PEASANT)<<"\n";
 }
 k.GetHeroes().clear();k.GetCastles().clear();
}
