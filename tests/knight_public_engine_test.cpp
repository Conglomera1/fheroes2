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
 assert(argc==3);const uint32_t cap=static_cast<uint32_t>(std::stoul(argv[1]));Settings::Get().Read(argv[2]);
 Settings::Get().SetGameDifficulty(Difficulty::NORMAL);Settings::Get().SetGameType(Game::TYPE_HOTSEAT);Settings::Get().SetAIMoveSpeed(0);
 Settings::Get().GetPlayers().Init(Color::allPlayerColors());for(const auto c:PlayerColorsVector(Color::allPlayerColors()))Players::Get(c)->SetControl(CONTROL_AI);
 Rand::CurrentThreadRandomDevice()=Rand::PCG32(20261117);world.generateUninitializedMap(144);
 for(int t=0;t<static_cast<int>(world.getSize());++t){auto & x=world.getTile(t);x.setIndex(t);x.setTerrain(Maps::Ground::GRASS_START_IMAGE_INDEX,0);x.ClearFog(static_cast<PlayerColorsSet>(PlayerColor::BLUE));}
 while(world.CountDay()<237)world.NewDay();
 auto & k=world.GetKingdom(PlayerColor::BLUE);k.OddFundsResource(k.GetFunds());Funds funds;funds.gold=50000;funds.wood=funds.ore=funds.mercury=funds.sulfur=funds.crystal=funds.gems=100;k.AddFundsResource(funds);
 [[maybe_unused]] const auto totalPopulation=[&]() {
  uint64_t total=0;for(const auto * hero:k.GetHeroes())total+=hero->GetArmy().GetCountMonsters(Monster::PEASANT);
  for(const auto * city:k.GetCastles()){total+=city->GetArmy().GetCountMonsters(Monster::PEASANT);if(city->GetRace()==Race::KNGT)total+=city->conscriptionDaySnapshot().hut;}
  return total;
 };
 const auto add=[&](int tile,int race,bool tg){world.addCastle(tile,static_cast<uint8_t>(race),PlayerColor::BLUE);auto * c=world.getCastle({tile%144,tile/144});assert(c);k.AddCastle(c);world.getTile(tile).setMainObjectType(MP2::OBJ_CASTLE);
  Maps::Map_Format::CastleMetadata m;m.customBuildings=true;m.builtBuildings={BUILD_CASTLE,BUILD_STATUE,BUILD_WELL,DWELLING_MONSTER1,DWELLING_MONSTER2,DWELLING_MONSTER3,DWELLING_MONSTER4,DWELLING_MONSTER5,DWELLING_MONSTER6,tg?BUILD_TRAINING_GROUNDS:BUILD_WEL2};c->loadFromResurrectionMap(m);c->GetArmy().Clean();c->SetModes(Castle::ALLOW_TO_BUILD_TODAY);return c;};
 auto * farm=add(5678,Race::KNGT,false);auto * second=add(2232,Race::KNGT,false);auto * tg=add(9727,Race::KNGT,true);
 auto * h=world.GetHeroes(Heroes::ECTOR);assert(h->Recruit(*farm));h->GetArmy().Clean();assert(h->GetArmy().JoinTroop(Monster::PEASANT,10,false));
 assert(farm->GetArmy().JoinTroop(Monster::PEASANT,500,false));assert(second->GetArmy().JoinTroop(Monster::PEASANT,100,false));assert(tg->GetArmy().JoinTroop(Monster::PEASANT,200,false));
 assert(Kingdom::TaxpayerRequiresFarms());assert(Kingdom::GetMaxTaxablePeasantsPerFarm()==cap);
 assert(!Settings::Get().isKnightObserverBuild()&&!Settings::Get().isTaxpayerTestMapRevealEnabled()&&Settings::Get().taxpayerTestDiagnostics()==0&&Settings::Get().taxpayerTraceDetail()==0);
 const auto snapshot=k.economySnapshot();const int64_t expected=cap?2*(std::min<uint32_t>(510,cap)+std::min<uint32_t>(100,cap)):1220;
 assert(Economy::taxIncome(snapshot)==expected);
 const auto forecast=Economy::forecast(snapshot,Conscription::settings(),1);assert(forecast.complete&&forecast.days.front().tax==expected);
 const auto income=k.GetIncome(Kingdom::INCOME_ALL);assert(income.gold-snapshot.nonTaxIncome==expected);
 std::cout<<"PASS public engine tax cap="<<cap<<" income="<<expected<<" legacy/debug switches ignored\n";
}
