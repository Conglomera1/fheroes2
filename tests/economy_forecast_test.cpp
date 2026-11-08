#include "../src/fheroes2/kingdom/economy_forecast.h"
#include <cassert>
#include <iostream>

int main()
{
    Economy::Snapshot start;
    start.day = 6;
    start.gold = 1000;
    start.nonTaxIncome = 100;
    start.taxRate = 2;
    start.peasantCost = 20;
    Economy::Town farm;
    farm.farm = true;
    farm.troops.garrison = 10;
    farm.visitingPeasants = 5;
    farm.hutGrowth = 20;
    Economy::Town training;
    training.training = true;
    training.troops.garrison = 1;
    training.troops.hut = 1;
    training.hutGrowth = 2;
    training.poolGrowth = { 1, 1, 1 };
    start.towns = { farm, training };
    Conscription::Settings config;
    auto projected = Economy::forecast( start, config, 2 );
    assert( projected.complete && projected.days.size() == 2 );
    assert( projected.days[0].tax == 30 && projected.days[0].fees == 220 && projected.days[0].closingGold == 910 );
    assert( projected.days[1].fees == 240 && projected.days[1].closingGold == 800 );
    assert( projected.end.towns[0].troops.hut == 20 );
    assert( projected.end.towns[1].troops.state.total == 4 );
    assert( projected.end.towns[1].troops.pools == ( std::array<uint64_t, 3>{ 3, 2, 2 } ) );
    assert( start.towns[1].troops.state.total == 0 && start.gold == 1000 );
    start.requireFarms = false;
    start.otherPeasants = 3;
    projected = Economy::forecast( start, config, 2 );
    assert( projected.days[0].tax == 38 && projected.days[1].tax == 36 );
    config.successRate = 0;
    projected = Economy::forecast( start, config, 1 );
    assert( projected.days[0].failed == 2 && projected.days[0].fees == 220 );
    assert( projected.end.towns[1].troops.state.total == 0 );
    config.enabled = false;
    projected = Economy::forecast( start, config, 1 );
    assert( projected.days[0].fees == 0 && projected.days[0].failed == 0 );
    config.enabled = true;
    projected = Economy::forecast( start, config, 1, 0 );
    assert( !projected.complete && projected.days.empty() && projected.end.day == start.day );
    projected = Economy::forecast( start, config, 29 );
    assert( !projected.complete && projected.reason == "horizon_limit" );
    start.day = 0;
    projected = Economy::forecast( start, config, 1 );
    assert( projected.days[0].income == 0 && projected.days[0].fees == 0 );
    start.day = 6;
    start.gold = std::numeric_limits<int32_t>::max();
    projected = Economy::forecast( start, config, 1 );
    assert( !projected.complete && projected.reason == "treasury_out_of_range" );
    Economy::Snapshot poor;
    poor.day = 1;
    poor.gold = 100;
    Economy::Town first;
    first.training = true;
    first.troops.garrison = 1;
    poor.towns = { first, first };
    config.successRate = Conscription::successScale;
    projected = Economy::forecast( poor, config, 1 );
    assert( projected.complete && projected.end.gold == 0 );
    assert( projected.end.towns[0].troops.garrison == 0 && projected.end.towns[1].troops.garrison == 1 );
    std::cout << "Passive economy arithmetic, growth, limits and immutability passed\n";
}
