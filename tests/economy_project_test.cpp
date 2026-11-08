#include "economy_project.h"
#include <cassert>
#include <iostream>

namespace P = AI::EconomyProject;

P::Action action( const uint64_t id, const uint32_t day, const P::Kind kind, const int32_t town = 42, const uint32_t value = 0 )
{
    P::Action result;
    result.id = result.project = id; result.day = day; result.kind = kind; result.town = town;
    if ( kind == P::Kind::Build ) result.building = value;
    else result.count = value;
    return result;
}

P::Input fixture( const bool training = false, const uint32_t day = 7 )
{
    P::Input input;
    input.throughDay = day + 1;
    input.config.requiredBuildings = BUILD_WEL2 | DWELLING_MONSTER2 | DWELLING_MONSTER3 | DWELLING_MONSTER4 | BUILD_CAPTAIN;
    auto & state = input.start;
    state.economy.day = day; state.economy.gold = 10000; state.economy.materials = { 20, 20, 0, 0, 0, 0 };
    state.economy.nonTaxIncome = 1000; state.economy.taxRate = 2; state.economy.peasantCost = 20;
    P::Town town;
    town.tile = 42; town.race = Race::KNGT; town.farmBonus = 8; town.wellBonus = 2;
    town.mask = BUILD_CASTLE | BUILD_CAPTAIN | DWELLING_MONSTER1 | DWELLING_MONSTER2 | DWELLING_MONSTER3 | DWELLING_MONSTER4
                | ( training ? BUILD_TRAINING_GROUNDS : BUILD_WEL2 );
    town.garrison[0] = { Monster::PEASANT, 100 };
    const uint32_t bases[] = { DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6 };
    const int monsters[] = { Monster::PEASANT, Monster::ARCHER, Monster::PIKEMAN, Monster::SWORDSMAN, Monster::CAVALRY, Monster::PALADIN };
    const uint32_t growth[] = { 12, 8, 5, 4, 3, 2 };
    const int64_t prices[] = { 20, 150, 200, 250, 300, 600 };
    for ( size_t i = 0; i < 6; ++i ) {
        town.dwellings[i].base = bases[i];
        town.dwellings[i].basic = { monsters[i], growth[i], { prices[i], 0, 0, 0, 0, 0, 0 } };
        P::Building building;
        building.id = bases[i]; building.price[0] = 1000; building.newPool = static_cast<int>( i );
        town.catalog.emplace( building.id, building );
    }
    for ( const auto buildingId : { BUILD_WEL2, BUILD_TRAINING_GROUNDS, BUILD_WELL, BUILD_STATUE } ) {
        P::Building building;
        building.id = buildingId; building.price[0] = 1000;
        town.catalog.emplace( building.id, building );
    }
    town.dwellings[1].upgrade = DWELLING_UPGRADE2;
    town.dwellings[1].improved = { Monster::RANGER, 8, { 200, 0, 0, 0, 0, 0, 0 } };
    P::Building upgrade;
    upgrade.id = DWELLING_UPGRADE2; upgrade.prerequisites = DWELLING_MONSTER2; upgrade.price[0] = 1000;
    town.catalog.emplace( upgrade.id, upgrade );
    Economy::Town economic;
    economic.tile = 42; economic.troops.garrison = 100; economic.troops.hut = 20;
    state.economy.towns = { economic }; state.towns = { town };
    P::refresh( state );
    return input;
}

P::Result run( const P::Input & input ) { P::WorkBudget work; return P::project( input, work ); }

int main()
{
    size_t checks = 0;
    auto input = fixture();
    auto result = run( input );
    assert( result.covered() && result.end.economy.gold == 11200 && result.end.economy.towns[0].troops.hut == 40 ); ++checks;
    input.actions = { action( 1, 7, P::Kind::Build, 42, BUILD_TRAINING_GROUNDS ) };
    result = run( input );
    assert( result.covered() && result.end.economy.gold == 8300 );
    assert( result.end.economy.towns[0].troops.garrison == 98 && result.end.economy.towns[0].troops.hut == 32 );
    assert( result.end.economy.towns[0].troops.pools == ( std::array<uint64_t, 3>{ 9, 6, 4 } ) );
    assert( !result.end.economy.towns[0].farm && input.start.economy.gold == 10000 && input.start.towns[0].mask & BUILD_WEL2 ); ++checks;
    input = fixture( true );
    input.actions = { action( 1, 7, P::Kind::Build, 42, BUILD_WEL2 ) };
    result = run( input );
    assert( result.covered() && result.end.economy.gold == 9700 && result.end.economy.towns[0].troops.garrison == 100 ); ++checks;

    input = fixture();
    auto buy = action( 1, 7, P::Kind::Recruit, 42, 10 );
    input.actions = { buy };
    result = run( input );
    assert( result.covered() && result.end.economy.gold == 11020 && result.end.economy.towns[0].troops.garrison == 110 ); ++checks;
    input = fixture( true, 1 );
    buy = action( 1, 2, P::Kind::Recruit, 42, 1 ); buy.pool = 1;
    input.actions = { buy };
    result = run( input );
    assert( result.covered() && result.end.economy.gold == 10550 && result.purchases[0].monster == Monster::ARCHER );
    assert( result.end.economy.towns[0].troops.pools == ( std::array<uint64_t, 3>{ 0, 1, 1 } ) ); ++checks;
    input.config.successRate = 0;
    assert( run( input ).reason == "recruit_stock_unavailable" ); ++checks;

    input = fixture( true );
    input.start.towns[0].garrison[0].count = 1; input.start.economy.towns[0].troops.garrison = 1;
    input.start.economy.nonTaxIncome = 0; input.start.economy.towns[0].troops.hut = 0;
    input.start.towns[0].catalog.at( BUILD_STATUE ).price[0] = 99;
    input.actions = { action( 1, 8, P::Kind::Build, 42, BUILD_STATUE ) };
    input.start.economy.gold = 99;
    assert( run( input ).covered() ); ++checks;
    input.start.economy.gold = 100;
    result = run( input );
    assert( result.status == P::Status::Unfunded && result.failedAction == 1 && result.budget.firstShortfall == 1 ); ++checks;

    input = fixture( true, 1 );
    auto & town = input.start.towns[0];
    town.garrison = {}; input.start.economy.towns[0].troops.garrison = 0;
    town.hero = 5; town.visitor[0] = { Monster::PEASANT, 10 }; town.visitor[1] = { Monster::ARCHER, 1 };
    input.start.economy.towns[0].visitingPeasants = 10;
    const auto unmoved = run( input );
    input.actions = { action( 1, 1, P::Kind::ToGarrison, 42, 10 ) };
    result = run( input );
    assert( result.covered() && result.end.economy.towns[0].troops.garrison == 7 );
    assert( result.end.economy.gold - unmoved.end.economy.gold == 60 && result.end.economy.towns[0].visitingPeasants == 0 ); ++checks;
    input.start.towns[0].visitor[1] = {};
    assert( run( input ).reason == "peasant_transfer_unavailable" ); ++checks;
    input.actions[0].kind = P::Kind::ToVisitor;
    assert( run( input ).reason == "peasant_transfer_unavailable" ); ++checks;

    input = fixture( true, 1 );
    input.start.towns[0].garrison = { P::Stack{ Monster::PEASANT, 100 }, { Monster::GOBLIN, 1 }, { Monster::ORC, 1 }, { Monster::OGRE, 1 }, { Monster::TROLL, 1 } };
    input.start.towns[0].mask |= DWELLING_MONSTER6;
    input.start.towns[0].extraPools[1] = 2;
    buy = action( 1, 1, P::Kind::Recruit, 42, 1 ); buy.pool = 5; input.actions = { buy };
    assert( run( input ).reason == "army_slots_unavailable" ); ++checks;
    input.start.towns[0].hero = 5;
    result = run( input );
    assert( result.covered() && result.purchases[0].visitor && result.end.towns[0].extraPools[1] == 1 ); ++checks;

    input = fixture( false, 1 );
    input.actions = { action( 1, 1, P::Kind::Build, 42, BUILD_WELL ), action( 2, 1, P::Kind::Build, 42, BUILD_STATUE ) };
    assert( run( input ).reason == "building_unavailable" ); ++checks;
    input.actions.resize( 1 ); input.start.towns[0].buildAvailable = false;
    assert( run( input ).reason == "building_unavailable" ); ++checks;
    input.actions[0].day = 2;
    assert( run( input ).covered() ); ++checks;

    input = fixture();
    input.actions = { action( 1, 7, P::Kind::Build, 42, BUILD_TRAINING_GROUNDS ) };
    input.config.requiredBuildings |= DWELLING_MONSTER6;
    assert( run( input ).reason == "building_unavailable" ); ++checks;
    input.config.requiredBuildings &= ~DWELLING_MONSTER6;
    input.start.towns[0].disabled = BUILD_WEL2;
    assert( run( input ).reason == "building_unavailable" ); ++checks;
    input.start.towns[0].disabled = 0; input.config.enabled = false;
    assert( run( input ).reason == "building_unavailable" ); ++checks;
    input = fixture( true ); input.config.enabled = false;
    assert( run( input ).covered() && run( input ).end.economy.towns[0].troops.garrison == 100 && !run( input ).end.economy.towns[0].farm ); ++checks;

    input = fixture( false, 1 );
    input.actions = { action( 1, 1, P::Kind::Build, 42, DWELLING_MONSTER6 ), action( 2, 2, P::Kind::Build, 42, BUILD_TRAINING_GROUNDS ) };
    input.config.requiredBuildings |= DWELLING_MONSTER6;
    result = run( input );
    assert( result.covered() && result.end.towns[0].extraPools[1] == 2 ); ++checks;
    input = fixture( false, 1 );
    input.start.economy.towns[0].troops.pools[0] = 3;
    input.actions = { action( 1, 1, P::Kind::Build, 42, DWELLING_UPGRADE2 ) };
    buy = action( 2, 2, P::Kind::Recruit, 42, 3 ); buy.pool = 1; input.actions.push_back( buy );
    result = run( input );
    assert( result.covered() && result.purchases[0].monster == Monster::RANGER ); ++checks;

    input = fixture();
    input.start.towns.push_back( input.start.towns[0] ); input.start.towns[1].tile = 43;
    input.start.economy.towns.push_back( input.start.economy.towns[0] ); input.start.economy.towns[1].tile = 43;
    input.start.economy.gold = 4000;
    input.actions = { action( 1, 7, P::Kind::Build, 42, BUILD_TRAINING_GROUNDS ), action( 2, 7, P::Kind::Build, 43, BUILD_TRAINING_GROUNDS ) };
    result = run( input );
    assert( result.status == P::Status::Unfunded && result.failedAction == 2 && result.budget.extraNeeded[0] == 1000 ); ++checks;
    input = fixture( false, 1 );
    Economy::ScheduledEvent event;
    event.day = 2; event.gold = -100; event.materials = { 1, 2, 3, 4, 5, 6 }; input.events = { event };
    result = run( input );
    assert( result.covered() && result.end.economy.gold == 11100 && result.end.economy.materials[5] == 6 ); ++checks;
    P::WorkBudget work; work.days = 1;
    assert( P::project( input, work ).covered() && P::project( input, work ).status == P::Status::Limit ); ++checks;
    work = {}; work.attempts = 0; input = fixture( true, 1 );
    assert( P::project( input, work ).status == P::Status::Limit ); ++checks;
    input.actions = { action( 1, 3, P::Kind::Recruit, 42, 1 ) };
    result = run( input );
    assert( result.status == P::Status::Incomplete && result.budget.pending == ( std::vector<uint64_t>{ 1 } ) ); ++checks;
    input.actions[0].day = 2; input.actions.push_back( input.actions[0] );
    assert( run( input ).reason == "invalid_action" ); ++checks;

    input = fixture();
    input.start.towns[0].race = Race::WRLK;
    input.start.towns[0].mask |= DWELLING_MONSTER5 | DWELLING_MONSTER6;
    input.start.towns[0].dwellings[0].basic.id = Monster::CENTAUR;
    result = run( input );
    assert( result.covered() && result.end.economy.gold == 11000 && result.end.towns[0].extraPools == ( std::array<uint64_t, 2>{ 3, 2 } ) ); ++checks;
    buy = action( 1, 8, P::Kind::Recruit, 42, 1 ); buy.pool = 5; input.actions = { buy };
    input.start.towns[0].dwellings[5].basic.price[6] = 1;
    assert( run( input ).status == P::Status::Unfunded ); ++checks;
    input.start.economy.materials[5] = 1;
    assert( run( input ).covered() ); ++checks;
    input = fixture( false, 1 );
    input.start.towns[0].hero = 9;
    input.start.towns[0].visitor = { P::Stack{ Monster::PEASANT, 10 }, { Monster::ARCHER, 1 } };
    input.start.economy.towns[0].visitingPeasants = 10;
    const auto visiting = run( input );
    input.actions = { action( 1, 1, P::Kind::ToGarrison, 42, 10 ) };
    assert( visiting.covered() && run( input ).end.economy.gold == visiting.end.economy.gold ); ++checks;

    input = fixture(); input.start.towns[0].catalog.at( BUILD_WELL ).price[0] = -1;
    assert( run( input ).reason == "invalid_catalog" ); ++checks;
    input = fixture(); input.start.economy.towns[0].troops.hut = uint64_t{ 1 } << 32;
    assert( run( input ).reason == "invalid_stock_range" ); ++checks;
    input = fixture(); input.start.towns[0].farmBonus = std::numeric_limits<uint32_t>::max();
    assert( run( input ).reason == "invalid_economic_range" ); ++checks;
    // No-action trajectories must agree with B2, including scarcity and partial success.
    for ( const bool training : { false, true } ) for ( const uint32_t rate : { 0U, 850000U, 1000000U } )
        for ( const uint32_t limit : { 0U, 2U, 5U } ) for ( const uint32_t horizon : { 1U, 7U, 14U } ) {
            input = fixture( training );
            input.start.economy.gold = 500; input.start.economy.nonTaxIncome = 0;
            input.config.successRate = rate; input.config.oddDayLimit = limit; input.config.evenDayLimit = limit + 1;
            input.throughDay = input.start.economy.day + horizon;
            const auto passive = Economy::forecast( input.start.economy, input.config, horizon );
            result = run( input );
            assert( passive.complete && result.covered() && result.end.economy.gold == passive.end.gold );
            assert( result.end.economy.towns[0].troops.garrison == passive.end.towns[0].troops.garrison );
            assert( result.end.economy.towns[0].troops.hut == passive.end.towns[0].troops.hut );
            assert( result.end.economy.towns[0].troops.pools == passive.end.towns[0].troops.pools );
            ++checks;
        }
    std::cout << "PASS " << checks << " dated-project cases (copies, exact conditional fees, actions and C1 reconciliation)\n";
}
