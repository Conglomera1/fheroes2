// Native army regression cases for the first maritime exploration voyage.
#include "../src/fheroes2/ai/ai_planner_castle.cpp"
#include "ai_hero_action.h"
#include "map_format_info.h"
#include "players.h"
#include "rand.h"
#include <iostream>

int main( int argc, char ** argv )
{
    assert( argc == 3 ); const std::string mode = argv[1]; Settings::Get().Read( argv[2] );
    Settings::Get().SetGameDifficulty( Difficulty::NORMAL ); Settings::Get().SetGameType( Game::TYPE_HOTSEAT ); Settings::Get().SetAIMoveSpeed( 0 );
    Settings::Get().GetPlayers().Init( Color::allPlayerColors() );
    for ( const auto color : PlayerColorsVector( Color::allPlayerColors() ) ) Players::Get( color )->SetControl( CONTROL_AI );
    Rand::CurrentThreadRandomDevice() = Rand::PCG32( 20261115 ); world.generateUninitializedMap( 36 ); world.NewDay();
    for ( int tile = 0; tile < static_cast<int>( world.getSize() ); ++tile ) {
        world.getTile( tile ).setIndex( tile );
        world.getTile( tile ).setTerrain( tile / 36 >= 6 ? 0 : Maps::Ground::GRASS_START_IMAGE_INDEX, 0 );
        world.getTile( tile ).ClearFog( static_cast<PlayerColorsSet>( PlayerColor::BLUE ) );
    }
    auto & kingdom = world.GetKingdom( PlayerColor::BLUE );
    const auto money = [&]( int gold ) {
        kingdom.OddFundsResource( kingdom.GetFunds() ); Funds f; f.gold = gold;
        f.wood = f.ore = f.mercury = f.sulfur = f.crystal = f.gems = 100; kingdom.AddFundsResource( f );
    };
    money( 50000 );
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

    const bool ordinary = mode == "ordinary" || mode == "ordinary_embark";
    const bool sorc = mode == "sorceress";
    auto * c = add(150, { BUILD_CASTLE, BUILD_STATUE, BUILD_SHIPYARD }, sorc ? Race::SORC : Race::KNGT);
    auto * hero = world.GetHeroes( Heroes::LORDKILBURN ); assert(hero && hero->Recruit(*c));
    hero->GetArmy().Clean(); auto & a = hero->GetArmy(); auto & g = c->GetArmy();
    const int low = sorc ? Monster::SPRITE : Monster::PEASANT;
    const int high = sorc ? Monster::DWARF : Monster::ARCHER;
    if(mode != "no_t1" && mode != "garrison_t1") assert(a.JoinTroop(low, mode == "short" ? 3 : 40, false));
    assert(a.JoinTroop(high, 8, false));
    if(mode == "garrison_t1") assert(g.JoinTroop(low, 40, false));
    if(mode == "full") for(int m : {Monster::GOBLIN, Monster::SPRITE, Monster::CENTAUR, Monster::SKELETON, Monster::DWARF}) assert(g.JoinTroop(m, 8, false));
    if(mode == "late") while(world.CountDay()<8) world.NewDay();
    if(mode == "human") Players::Get(PlayerColor::BLUE)->SetControl(CONTROL_HUMAN);
    if(mode == "no_shipyard") {
        // Replace metadata with a port that has no shipyard.
        Maps::Map_Format::CastleMetadata meta; meta.customBuildings=true; meta.builtBuildings={BUILD_CASTLE}; c->loadFromResurrectionMap(meta); g.Clean();
    }
    if(mode == "unreachable") world.getTile(1295).setTerrain(0,0);
    world.updatePassabilities();
    KC::Scope scope(kingdom);
    if(mode == "danger") constructionContext->endangered.insert(150);
    const auto beforeA = KnightObserver::armyJson(a), beforeG=KnightObserver::armyJson(g);
    const auto funds=kingdom.GetFunds();
    if(ordinary) {
        KC::arrangePopulation(*hero,*c,-1,mode=="ordinary_embark");
        assert(KnightObserver::armyJson(a)==beforeA && KnightObserver::armyJson(g)==beforeG);
    }
    else {
        const bool expected = mode!="full" && mode!="late" && mode!="human" && mode!="danger" && mode!="no_shipyard" && mode!="unreachable";
        assert(KC::prepareOpeningExplorer(*hero,*c,mode=="unreachable" ? 1295 : -1)==expected);
        if(!expected) assert(KnightObserver::armyJson(a)==beforeA && KnightObserver::armyJson(g)==beforeG);
        else {
            const unsigned wanted=mode=="no_t1" ? 1 : mode=="short" ? 3 : 5;
            const int unit=mode=="no_t1" ? high : low;
            assert(a.GetCountMonsters(unit)==wanted);
            unsigned stacks=0; for(size_t n=0;n<a.Size();++n) if(a.GetTroop(n)->isValid()) {++stacks;assert(a.GetTroop(n)->GetCount()==1);}
            assert(stacks==wanted && KC::isOpeningSeaExplorer(*hero));
            assert(g.GetCountMonsters(high)==(mode=="no_t1" ? 7 : 8));
            AI::OptimizeTroopsOrder(a); assert(a.GetOccupiedSlotCount()==wanted);
            if(mode=="recover") {
                hero->Move2Dest(151); auto * main=world.GetHeroes(Heroes::SIRGALLANTH); assert(main && main->Recruit(*c));
                main->GetArmy().Clean(); main->GetArmy().JoinStrongestFromArmy(g);
                assert(main->GetArmy().GetCountMonsters(low)==35 && main->GetArmy().GetCountMonsters(high)==8);
            }
            if(mode=="same_player") {
                hero->Move2Dest(151); auto * main=world.GetHeroes(Heroes::SIRGALLANTH); assert(main && main->Recruit(*c));
                main->GetArmy().Clean(); assert(main->GetArmy().JoinTroop(low,40,false));
                KC::arrangePopulation(*main,*c); assert(main->GetArmy().GetCountMonsters(low)==40);
                assert(!KC::prepareOpeningExplorer(*main,*c));
            }
            if(mode=="native_merge") {
                hero->SetShipMaster(true); KC::markOpeningEmbarked(*hero); hero->SetShipMaster(false);
                assert(!KC::isOpeningSeaExplorer(*hero)); AI::OptimizeTroopsOrder(a); assert(a.GetOccupiedSlotCount()==1);
            }
            if(mode=="first_only") {
                hero->SetShipMaster(true); KC::markOpeningEmbarked(*hero); hero->SetShipMaster(false);
                assert(!KC::isOpeningSeaExplorer(*hero)); assert(!KC::prepareOpeningExplorer(*hero,*c));
            }
            if(mode=="voyage") {
                hero->SetShipMaster(true); KC::markOpeningEmbarked(*hero);
                while(world.CountDay()<9) world.NewDay();
                assert(KC::isOpeningSeaExplorer(*hero)); AI::OptimizeTroopsOrder(a); assert(a.GetOccupiedSlotCount()==5);
                hero->SetShipMaster(false); assert(!KC::isOpeningSeaExplorer(*hero));
                assert(!KC::prepareOpeningExplorer(*hero,*c));
            }
        }
    }
    assert(kingdom.GetFunds()==funds);
    std::cout << "PASS v11 " << mode << " day=" << world.CountDay() << '\n';
    kingdom.GetHeroes().clear(); kingdom.GetCastles().clear();
}
