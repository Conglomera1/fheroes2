// Real recruitment API characterization. No production guard or receipt authority.
#include "castle.h"
#include "kingdom.h"
#include "army_troop.h"
#include "heroes.h"
#include "difficulty.h"
#include "map_format_info.h"
#include "maps_tiles.h"
#include "players.h"
#include "settings.h"
#include "world.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <string>

using Counts = std::map<int, uint64_t>;
Counts counts( const Troops & army )
{
    Counts result;
    for ( size_t i = 0; i < army.Size(); ++i ) {
        const auto * t = army.GetTroop(i);
        if ( t && t->isValid() ) result[t->GetID()] += t->GetCount();
    }
    return result;
}
uint64_t archers( const Counts & army )
{ const auto it=army.find(Monster::ARCHER); return it==army.end() ? 0 : it->second; }
Counts others( Counts army ) { army.erase(Monster::ARCHER); return army; }

int main( int argc, char ** argv )
{
    assert(argc==3); const std::string mode=argv[2];
    const std::set<std::string> modes{"raw_exact","raw_overstock","guarded_overstock","raw_empty","guarded_empty","raw_zero","poor","full","visitor"};
    assert(modes.count(mode));
    Settings::Get().Read(argv[1]); Settings::Get().SetGameDifficulty(Difficulty::NORMAL);
    Settings::Get().GetPlayers().Init(Color::allPlayerColors()); world.generateUninitializedMap(36); world.NewDay();
    auto & kingdom=world.GetKingdom(PlayerColor::BLUE);
    kingdom.AddFundsResource(Funds(100,100,100,100,100,100,100000));
    constexpr int tile=150;
    world.addCastle(tile,static_cast<uint8_t>(Race::KNGT),PlayerColor::BLUE);
    auto * town=world.getCastle({tile%36,tile/36}); assert(town);
    kingdom.GetCastles().push_back(town); world.getTile(tile).setMainObjectType(MP2::OBJ_CASTLE);
    Maps::Map_Format::CastleMetadata meta; meta.customBuildings=true; meta.builtBuildings={BUILD_CASTLE,DWELLING_MONSTER1};
    town->loadFromResurrectionMap(meta); town->SetModes(Castle::ALLOW_TO_BUILD_TODAY);
    const bool seeded=town->BuyBuilding(DWELLING_MONSTER2); assert(seeded); assert(!town->GetHero());
    town->GetArmy().Reset(); const bool defender=town->GetArmy().JoinTroop(Monster::PEASANT,1,false); assert(defender);
    const uint32_t initial=town->getMonstersInDwelling(DWELLING_MONSTER2);
    assert(initial>0 && initial<std::numeric_limits<uint32_t>::max());
    if(mode=="raw_empty" || mode=="guarded_empty") {
        const bool depleted=town->RecruitMonster(Troop(Monster::ARCHER,initial),false); assert(depleted);
        assert(town->getMonstersInDwelling(DWELLING_MONSTER2)==0);
    }
    if(mode=="full" || mode=="visitor") {
        town->GetArmy().Reset();
        for(const int unit : {Monster::PEASANT,Monster::PIKEMAN,Monster::SWORDSMAN,Monster::CAVALRY,Monster::PALADIN}) {
            const bool joined=town->GetArmy().JoinTroop(unit,1,false); assert(joined);
        }
    }
    Heroes * visitor=nullptr;
    if(mode=="visitor") {
        visitor=world.GetHeroes(Heroes::LORDKILBURN); visitor->SetColor(PlayerColor::BLUE); visitor->SetCenter(town->GetCenter());
        visitor->GetArmy().Reset(); const bool joined=visitor->GetArmy().JoinTroop(Monster::PEASANT,1,false); assert(joined);
        kingdom.GetHeroes().push_back(visitor); assert(town->GetHero()==visitor);
    }
    if(mode=="poor") kingdom.OddFundsResource(kingdom.GetFunds());
    const uint32_t stockBefore=town->getMonstersInDwelling(DWELLING_MONSTER2);
    uint32_t requested=stockBefore;
    if(mode=="raw_overstock" || mode=="guarded_overstock" || mode=="raw_empty" || mode=="guarded_empty") ++requested;
    if(mode=="raw_zero") requested=0;
    const Troop request(Monster::ARCHER,requested);
    const auto moneyBefore=kingdom.GetFunds(); const auto gb=counts(town->GetArmy()); const auto vb=visitor?counts(visitor->GetArmy()):Counts{};
    const bool guarded=mode=="guarded_overstock" || mode=="guarded_empty";
    // Test-local stock check only. The production API has NOT been patched.
    const bool invoke=!guarded || (requested>0 && requested<=stockBefore);
    std::cout<<"PROBE_BEGIN"<<std::endl;
    const bool accepted=invoke && town->RecruitMonster(request,false);
    std::cout<<"PROBE_END"<<std::endl;
    const auto moneyAfter=kingdom.GetFunds(); const uint32_t stockAfter=town->getMonstersInDwelling(DWELLING_MONSTER2);
    const auto ga=counts(town->GetArmy()); const auto va=visitor?counts(visitor->GetArmy()):Counts{};
    const int64_t garrisonAdded=static_cast<int64_t>(archers(ga))-static_cast<int64_t>(archers(gb));
    const int64_t visitorAdded=static_cast<int64_t>(archers(va))-static_cast<int64_t>(archers(vb));
    const int64_t added=garrisonAdded+visitorAdded, removed=int64_t{stockBefore}-stockAfter;
    const bool unchanged=moneyAfter==moneyBefore && ga==gb && va==vb && stockAfter==stockBefore && town->GetHero()==visitor;
    const bool exact=added==requested && removed==requested && moneyBefore-moneyAfter==request.GetTotalCost()
        && others(gb)==others(ga) && others(vb)==others(va) && town->GetHero()==visitor;
    const bool consistent=accepted?exact:unchanged;
    std::cout<<"RESULT mode="<<mode<<" invoked="<<invoke<<" accepted="<<accepted<<" requested="<<requested
        <<" stock_before="<<stockBefore<<" stock_after="<<stockAfter<<" units_added="<<added<<" stock_removed="<<removed
        <<" gold_spent="<<int64_t{moneyBefore.gold}-moneyAfter.gold<<" garrison_added="<<garrisonAdded<<" visitor_added="<<visitorAdded
        <<" receipt_consistent="<<consistent<<'\n';
    if(mode=="raw_exact") assert(accepted && exact && garrisonAdded==requested && visitorAdded==0);
    if(mode=="visitor") assert(accepted && exact && visitorAdded==requested && garrisonAdded==0);
    if(guarded) assert(!invoke && !accepted && unchanged);
    if(mode=="poor" || mode=="full" || mode=="raw_zero") assert(!accepted && unchanged);
    kingdom.GetHeroes().clear(); kingdom.GetCastles().clear();
    // Preserve an inconsistent API result as exit 2, never call it a green regression.
    return consistent?0:2;
}
