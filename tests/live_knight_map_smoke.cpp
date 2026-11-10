// Bounded real-map smoke, derived from the earlier balance runner.
// All-AI control, one seeded Knight town; normal gameplay fog, not an unchanged campaign.
#include "ai_planner.h"
#include "map_format_info.h"
#include "interface_gamearea.h"
#include "game_interface.h"
#include "kingdom.h"
#include "game_over.h"
#include "interface_gamearea.h"
#include "game_delays.h"
#include "game_auto_playtest.h"
#include "race.h"
#include "color.h"
#include "game_init.h"
#include "game.h"
#include "settings.h"
#include "maps_fileinfo.h"
#include "world.h"
#include "castle.h"
#include "game_language.h"
#include "screen.h"
#include "image_tool.h"
#include <iostream>
#include <csignal>
#include <execinfo.h>
#include <unistd.h>
void crash(int n){void* b[80];int k=backtrace(b,80);backtrace_symbols_fd(b,k,2);_exit(128+n);}
int main(int argc,char**argv){
 signal(SIGSEGV,crash);signal(SIGABRT,crash);signal(SIGFPE,crash);
 auto hardware=Game::createHardwareComponent();
 Game::initLogging();Game::initDataDir();Game::initConfigDir(argv[0]);
 auto core=Game::createCoreComponent();
 auto display=Game::createDisplayComponent();
 auto data=Game::createDataComponent();
 Game::initPalette();
 Game::initTranslations();Game::initEventHandler();Game::initAnimation();Game::initHotKeys();
 auto &conf=Settings::Get();conf.SetGameType(Game::TYPE_STANDARD);
 Maps::FileInfo info;
 if(!info.readResurrectionMap(argv[1],false,fheroes2::SupportedLanguage::English) && !info.readMP2Map(argv[1],false))return 2;
 conf.setCurrentMapInfo(info);
 conf.GetPlayers().Init(info);
 const auto colors=PlayerColorsVector(conf.GetPlayers().GetColors());
 if(colors.size()<2) return 5;
 Players::SetPlayerRace(colors[std::stoi(argv[3])%colors.size()],Race::KNGT);
 conf.GetPlayers().SetStartGame();
 for(auto color:colors) std::cout<<"PLAYER "<<int(color)<<" "<<Players::GetPlayerRace(color)<<std::endl;
 bool ok=info.version==GameVersion::RESURRECTION ? world.loadResurrectionMap(argv[1]) : world.LoadMapMP2(argv[1],info.version==GameVersion::SUCCESSION_WARS);
 if(!ok) return 3;

 // Controlled opportunity on an existing map: starting infrastructure/funds,
 // then real KingdomTurn, movement, recruitment, combat and development.
 auto & seeded=world.GetKingdom(colors[std::stoi(argv[3])%colors.size()]);
 Castle * home=nullptr;
 for(auto * c:seeded.GetCastles()) if(c->GetRace()==Race::KNGT){home=c;break;}
 if(!home) return 6;
 Maps::Map_Format::CastleMetadata meta;meta.customBuildings=true;
 meta.builtBuildings={BUILD_CASTLE,BUILD_CAPTAIN,BUILD_WELL,BUILD_WEL2,DWELLING_MONSTER1,DWELLING_MONSTER2,DWELLING_MONSTER3,DWELLING_MONSTER4};
 home->loadFromResurrectionMap(meta);home->GetArmy().Clean();home->GetArmy().JoinTroop(Monster::PEASANT,20,false);
 seeded.OddFundsResource(seeded.GetFunds());seeded.AddFundsResource(Funds(100,100,100,100,100,100,20000));
 std::cout<<"SEEDED "<<home->GetIndex()<<std::endl;

 auto &play=fheroes2::AutoPlaytest::instance();
 play.reset(conf.GetPlayers().GetColors());
 play.enableAnimation(false);play.enableSounds(false);play.setMaxDaysInPlaythrough(std::stoi(argv[2]));
 conf.SetMusicVolume(0);conf.SetSoundVolume(0);conf.setBattleAutoResolve(true);conf.SetAIMoveSpeed(0);Game::UpdateGameSpeed();conf.SetGameType(Game::TYPE_AUTO_PLAYTEST | Game::TYPE_HOTSEAT);
 
 for(auto color:colors) {
  Players::SetPlayerControl(color, CONTROL_HUMAN);
  Players::Get(color)->setAIAutoControlMode(true);
  world.ClearFog(color); // Normal initial scouting around owned towns/heroes/mines only.
 }
 GameOver::Result::Get().Reset();
 Interface::GameArea::updateMapFogDirections();
 PlayerColor winner=PlayerColor::NONE;
 const int horizon=std::stoi(argv[2]);
 for(int day=1;day<=horizon && winner==PlayerColor::NONE;++day) {
  world.NewDay();
  for(auto color:colors) {
   auto &k=world.GetKingdom(color);
   if(!k.isPlay()) continue;
   conf.SetCurrentColor(color);
   Interface::AdventureMap::Get().getStatusPanel().SetState(Interface::StatusType::STATUS_AITURN);
   k.ActionNewDayResourceUpdate(nullptr);
   k.ActionBeforeTurn();
   AI::Planner::Get().KingdomTurn(k);
   if(world.KingdomIsWins(k, GameOver::WINS_ALL)) {winner=color;break;}
  }
  for(auto color:colors) {
   const auto &k=world.GetKingdom(color);
   int farms=0,tg=0;uint64_t trained=0,peasants=0;
   for(const auto *c:k.GetCastles()) {
    if(c->GetRace()!=Race::KNGT) continue;
    const auto snapshot=c->conscriptionDaySnapshot();
    farms+=c->isBuild(BUILD_WEL2);tg+=c->hasTrainingGrounds();
    trained+=snapshot.state.total;peasants+=snapshot.garrison;
   }
   std::cout<<"DAY "<<day<<" "<<int(color)<<" "<<k.isPlay()<<" "<<k.GetCastles().size()<<" "<<k.GetHeroes().size()<<" "<<k.GetFunds().gold<<" "<<farms<<" "<<tg<<" "<<trained<<" "<<peasants<<std::endl;
  }
 }
 for(auto color:colors)std::cout<<"RESULT "<<int(color)<<" "<<(color==winner?0:!world.GetKingdom(color).isPlay()?1:2)<<" "<<world.CountDay()<<std::endl;
 std::cout<<"END "<<world.CountDay()<<std::endl;
 _exit(0);
}
