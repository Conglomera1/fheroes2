#include "../src/fheroes2/ai/economy_budget.h"
#include "../src/fheroes2/kingdom/economy_forecast.h"
#include <cassert>
#include <iostream>

namespace B = AI::EconomyBudget;

B::Step step( const uint64_t id, const uint32_t day, const int64_t gold )
{
    B::Step result;
    result.id = result.project = id;
    result.day = day;
    result.cost[0] = gold;
    return result;
}

B::Morning morning( const uint32_t day, const int64_t gold )
{
    B::Morning result;
    result.id = 10000 + day;
    result.day = day;
    result.income[0] = gold;
    return result;
}

int main()
{
    size_t checks = 0;
    B::Input input;
    input.today = 7; input.throughDay = 9; input.funds[0] = 1000;
    input.steps = { step( 1, 7, 600 ), step( 2, 7, 600 ) };
    auto result = B::evaluate( input );
    assert( result.status == B::Status::Unfunded && result.extraNeeded[0] == 200 );
    assert( result.firstShortfall == 2 && input.funds[0] == 1000 ); ++checks;

    // Positive final cash cannot repair a missed deadline.
    input.mornings = { morning( 8, 500 ) };
    result = B::evaluate( input );
    assert( result.remaining[0] == 300 && !result.covered() && result.firstShortfallDay == 7 ); ++checks;
    input.steps[1].day = 8;
    assert( B::evaluate( input ).covered() ); ++checks;

    // A discretionary purchase must not spend the funding of a later required response.
    input.steps[1].required = true;
    input.steps[1].cost[0] = 1000;
    result = B::evaluate( input );
    assert( !result.covered() && result.firstShortfallRequired && result.firstShortfall == 2 ); ++checks;
    input.steps.erase( input.steps.begin() );
    assert( B::evaluate( input ).covered() ); ++checks;

    input.steps = { step( 1, 7, 0 ) };
    input.steps[0].cost = { 0, 1, 2, 3, 4, 5, 6 };
    result = B::evaluate( input );
    assert( result.extraNeeded == ( B::Resources{ 0, 1, 2, 3, 4, 5, 6 } ) ); ++checks;
    input.funds = input.steps[0].cost;
    input.mornings.clear();
    assert( B::evaluate( input ).covered() ); ++checks;

    // Stock is shared across projects, arrivals are dated, and one claim cannot alias itself.
    input.funds[0] = 1000;
    input.stocks = { { "town:42/pool:archer", 10 } };
    input.steps = { step( 1, 7, 0 ), step( 2, 7, 0 ) };
    input.steps[0].stocks = { { "town:42/pool:archer", 6 } };
    input.steps[1].stocks = { { "town:42/pool:archer", 5 } };
    assert( B::evaluate( input ).reason == "stock_unavailable" ); ++checks;
    input.steps[1].day = 8;
    input.mornings = { morning( 8, 0 ) };
    input.mornings[0].arrivals = { { "town:42/pool:archer", 1 } };
    result = B::evaluate( input );
    assert( result.covered() && result.stocks.at( "town:42/pool:archer" ) == 0 ); ++checks;
    input.steps[0].stocks.push_back( { "town:42/pool:archer", 5 } );
    assert( B::evaluate( input ).reason == "stock_unavailable" ); ++checks;
    input.steps[0].stocks.resize( 1 );

    // Building action: once per town/day. Hero: one inclusive mission interval at a time.
    input.exclusiveAssets = { "town:42/build", "hero:8" };
    input.steps[0].exclusive = { { "town:42/build", 7 }, { "hero:8", 8 } };
    input.steps[1].exclusive = { { "town:42/build", 8 }, { "hero:8", 8 } };
    assert( B::evaluate( input ).reason == "exclusive_conflict" ); ++checks;
    input.steps[1].day = 9;
    input.steps[1].exclusive = { { "town:42/build", 9 }, { "hero:8", 9 } };
    assert( B::evaluate( input ).covered() ); ++checks;
    input.busy = { { "town:42/build", 7, 7 } };
    assert( B::evaluate( input ).reason == "exclusive_conflict" ); ++checks;
    input.busy.clear();

    input.steps[1].day = 10;
    input.steps[1].exclusive.clear();
    result = B::evaluate( input );
    assert( result.status == B::Status::OutsideHorizon && result.pending == ( std::vector<uint64_t>{ 2 } ) ); ++checks;
    input.steps[1].day = 8;
    input.steps[1].id = 1;
    assert( B::evaluate( input ).reason == "invalid_step" ); ++checks;
    input.steps[1].id = input.mornings[0].id;
    assert( B::evaluate( input ).reason == "invalid_step" ); ++checks;
    input.steps.clear();
    input.mornings[0].day = 7;
    assert( B::evaluate( input ).reason == "invalid_morning" ); ++checks;
    input.mornings[0].day = 8;
    input.mornings.push_back( input.mornings[0] );
    input.mornings[1].id++;
    assert( B::evaluate( input ).reason == "invalid_morning" ); ++checks;
    input.mornings.resize( 1 );
    input.mornings[0].income[0] = -1;
    assert( B::evaluate( input ).status == B::Status::Invalid ); ++checks;
    input.mornings.clear();
    input.steps.resize( B::entryLimit + 1 );
    assert( B::evaluate( input ).status == B::Status::Limit ); ++checks;

    B::Input edges;
    edges.today = 7; edges.throughDay = 8;
    edges.exclusiveAssets = { "town:42/build", "town:43/build" };
    edges.steps = { step( 1, 7, 0 ), step( 2, 7, 0 ) };
    edges.steps[0].project = edges.steps[1].project = 50; // One chain may have several distinct steps.
    edges.steps[0].exclusive = edges.steps[1].exclusive = { { "town:42/build", 7 } };
    assert( B::evaluate( edges ).reason == "exclusive_conflict" ); ++checks;
    edges.steps[1].exclusive = { { "town:43/build", 7 } };
    assert( B::evaluate( edges ).covered() ); ++checks;
    edges.steps[1].exclusive = { { "unknown:hero", 7 } };
    assert( B::evaluate( edges ).reason == "invalid_exclusive_use" ); ++checks;
    edges.steps.clear();
    edges.funds[0] = B::resourceMaximum;
    edges.mornings = { morning( 8, B::resourceMaximum ) };
    assert( B::evaluate( edges ).remaining[0] == B::resourceMaximum ); ++checks;
    edges.stocks = { { "stock", std::numeric_limits<uint32_t>::max() } };
    edges.mornings[0].arrivals = { { "stock", 1 } };
    assert( B::evaluate( edges ).reason == "stock_overflow" ); ++checks;
    edges.mornings.clear(); edges.stocks.clear();
    edges.throughDay = edges.today + 29;
    assert( B::evaluate( edges ).status == B::Status::Invalid ); ++checks;
    edges.today = edges.throughDay = std::numeric_limits<uint32_t>::max();
    assert( B::evaluate( edges ).covered() && B::evaluate( edges ).days.size() == 1 ); ++checks;

    // Conscription ceilings follow calendar day, configured prices, activation periods and enabled.
    Conscription::Settings config;
    auto bound = B::conscriptionBound( config, 20, 7, 9, { { 42, 8, 9 } } );
    assert( bound.complete && bound.days[0].second == 240 && bound.days[1].second == 360 ); ++checks;
    config.oddDayLimit = 4; config.evenDayLimit = 1; config.goldPerPeasant = 125;
    bound = B::conscriptionBound( config, 30, 7, 9, { { 42, 9, 9 } } );
    assert( bound.complete && bound.days[0].second == 0 && bound.days[1].second == 155 ); ++checks;
    assert( !B::conscriptionBound( config, 20, 7, 9, { { 42, 8, 9 }, { 42, 9, 9 } } ).complete ); ++checks;
    config.enabled = false;
    assert( B::conscriptionBound( config, 20, 7, 9, { { 42, 8, 9 } } ).days[1].second == 0 ); ++checks;
    config.enabled = true; config.oddDayLimit = std::numeric_limits<uint32_t>::max();
    assert( !B::conscriptionBound( config, 20, 7, 9, { { 42, 8, 9 } } ).complete ); ++checks;

    // Independent brute-force prefix oracle for fixed, uncapped cash flows.
    for ( int64_t cash = 0; cash <= 600; cash += 100 ) {
        for ( int64_t income = 0; income <= 200; income += 100 ) {
            for ( int64_t cost = 0; cost <= 400; cost += 100 ) {
                B::Input fixed;
                fixed.today = 1; fixed.throughDay = 3; fixed.funds[0] = cash;
                fixed.mornings = { morning( 2, income ), morning( 3, income ) };
                fixed.steps = { step( 1, 1, cost ), step( 2, 3, cost ) };
                const int64_t need = std::max<int64_t>( { 0, cost - cash, cost * 2 - cash - income * 2 } );
                const auto actual = B::evaluate( fixed );
                assert( actual.extraNeeded[0] == need && actual.covered() == ( need == 0 ) ); ++checks;
            }
        }
    }

    // Connect the bound to B2's real shared scheduler on copied states, not a fake fee formula.
    size_t certified = 0, conservativeFeasible = 0;
    for ( int64_t cash = 0; cash <= 6000; cash += 200 ) {
        for ( const uint32_t limit : { 0U, 2U, 3U } ) {
            config = {};
            config.oddDayLimit = limit; config.evenDayLimit = limit + 1;
            config.successRate = 850000;
            Economy::Snapshot state;
            state.day = 7; state.gold = cash; state.nonTaxIncome = 300; state.peasantCost = 20;
            Economy::Town town;
            town.training = true; town.troops.garrison = 3; town.troops.hut = 20;
            state.towns = { town, town };
            B::Input budget;
            budget.today = 7; budget.throughDay = 10; budget.funds[0] = cash;
            budget.steps = { step( 1, 7, 100 ), step( 2, 10, 200 ) };
            const auto ceiling = B::conscriptionBound( config, state.peasantCost, 7, 10, { { 42, 8, 10 }, { 43, 8, 10 } } );
            assert( ceiling.complete );
            for ( const auto & charge : ceiling.days ) {
                auto entry = morning( charge.first, 300 );
                entry.automaticBound[0] = charge.second;
                entry.conservative = true;
                budget.mornings.push_back( entry );
            }
            const auto assessment = B::evaluate( budget );
            bool payable = state.gold >= 100;
            if ( payable ) {
                state.gold -= 100;
                for ( uint32_t day = 8; day <= 10; ++day ) {
                    const auto predicted = Economy::forecast( state, config, 1 );
                    assert( predicted.complete );
                    assert( predicted.days[0].fees <= ceiling.days[day - 8].second );
                    state = predicted.end;
                }
                payable = state.gold >= 200;
            }
            if ( assessment.covered() ) { assert( payable ); ++certified; }
            if ( assessment.status == B::Status::ConservativeGap && payable ) ++conservativeFeasible;
            ++checks;
        }
    }
    assert( certified && conservativeFeasible );

    // Non-monotone gold: one extra coin triggers a formation and prevents a later 99-gold payment.
    Economy::Snapshot scarce;
    scarce.day = 7;
    scarce.peasantCost = 20;
    Economy::Town training;
    training.training = true; training.troops.garrison = 1;
    scarce.towns = { training };
    config = {};
    scarce.gold = 99;
    assert( Economy::forecast( scarce, config, 1 ).end.gold == 99 );
    scarce.gold = 100;
    assert( Economy::forecast( scarce, config, 1 ).end.gold == 0 ); ++checks;
    std::cout << "PASS " << checks << " accounting checks; " << certified << " covered TG cases; "
              << conservativeFeasible << " conservative gaps that remain actually payable\n";
}
