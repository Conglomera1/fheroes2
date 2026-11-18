#include "../src/fheroes2/kingdom/economy_forecast.h"
#include <cassert>
#include <iostream>
int main() {
    Economy::Snapshot s; s.taxRate=2;
    Economy::Town a; a.farm=true; a.troops.garrison=500; a.visitingPeasants=10;
    Economy::Town b=a; b.troops.garrison=100; b.visitingPeasants=0;
    Economy::Town tg=a; tg.farm=false; tg.training=true;
    s.towns={a,b,tg};
    assert(Economy::taxIncome(s)==1220);
    s.maxTaxablePerFarm=300; assert(Economy::taxIncome(s)==800);
    s.maxTaxablePerFarm=1; assert(Economy::taxIncome(s)==4);
    s.maxTaxablePerFarm=0; assert(Economy::taxIncome(s)==1220);
    s.taxRate=0; assert(Economy::taxIncome(s)==0);
    std::cout<<"PASS public tax: unlimited, per-town cap, stationed visitor, TG excluded, zero rate\n";
}
