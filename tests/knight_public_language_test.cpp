// Exercise the same native catalog loader and description generators as the UI.
#include "translations.h"
#include "castle.h"
#include "castle_building_info.h"
#include "buildinginfo.h"
#include "race.h"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
int main(int argc, char **argv)
{
    assert(argc == 2);
    unsigned count=0;
    Castle town(0, 0, Race::KNGT);
    for (const auto &entry : std::filesystem::directory_iterator(argv[1])) {
        const auto name=entry.path().filename().string();
        if (name.rfind("km_", 0) || entry.path().extension()!=".mo") continue;
        const auto lang=name.substr(3, name.size()-6);
        assert(Translation::setLanguage(lang, entry.path().string()));
        assert(std::string(fheroes2::getBuildingName(Race::KNGT, BUILD_TRAINING_GROUNDS))!="Training Grounds");
        const std::string purchase=_("Convert the Farm into Training Grounds to enable Peasant conscription. The Farm's growth bonus will be lost.");
        assert(purchase.rfind("Convert the Farm",0)!=0);
        const auto farm=BuildingInfo::getBuildingDescription(Race::KNGT, BUILD_WEL2);
        const auto daily=town.dailyConscriptionDescription();
        assert(farm.find("Each Peasant working")==std::string::npos && farm.find("%{")==std::string::npos);
        assert(daily.rfind("Automatically trains",0)!=0 && daily.find("%{")==std::string::npos);
        assert(!farm.empty() && !daily.empty());
        ++count;
    }
    assert(count==23);
    Translation::reset();
    assert(std::string(fheroes2::getBuildingName(Race::KNGT,BUILD_TRAINING_GROUNDS))=="Training Grounds");
    std::cout << "PASS 23 native catalogs: names, purchase text, Farm and daily conscription descriptions; English fallback\n";
}
