#include "knight_economy.h"
#include "knight_construction.h"
#include "economy_candidates.h"
#include "economy_military.h"
#include "economy_trace.h"
#include "difficulty.h"
#include "game.h"
#include "heroes.h"
#include "kingdom.h"
#include "world.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <cstring>
#include <map>

namespace
{
    namespace P = AI::EconomyProject;
    namespace G = AI::EconomyCandidates;
    namespace M = AI::EconomyMilitary;
    constexpr int64_t reserveGold = 2000; // Existing taxpayer investment reserve.
    constexpr uint32_t horizon = 14;
    struct Work { P::WorkBudget mechanics{ 512, 100000 }; uint32_t projections = 0; };
    struct Rollout { bool valid = false; double force = 0; int64_t gold = 0; };
    bool advance( P::State & state, const Conscription::Settings & config, uint32_t day, std::vector<P::Action> actions, Work & work )
    {
        if ( work.projections >= 384 ) return false;
        ++work.projections;
        P::Input input; input.start = state; input.config = config; input.throughDay = day; input.actions = std::move( actions );
        const auto result = P::project( input, work.mechanics );
        if ( !result.covered() ) {
            ECONOMY_TRACE( 1, "KNIGHT_FORECAST_REJECT", ",\"reason\":" << Economy::jsonQuote( result.reason )
                << ",\"projected_day\":" << day << ",\"failed_action\":" << result.failedAction );
            return false;
        }
        state = result.end; return true;
    }
    Rollout rollout( const Kingdom & kingdom, const P::Input & captured, size_t townIndex, uint32_t building, Work & work, const bool returnToFarm = false, const bool includeVisitor = true, const P::Resources & overhead = {} )
    {
        auto state = captured.start; const auto today = state.economy.day;
        std::vector<P::Action> initial;
        if ( building ) {
            P::Action a; a.id = a.project = 1; a.day = today; a.town = state.towns[townIndex].tile;
            a.kind = P::Kind::Build; a.building = building; initial.push_back( a );
        }
        if ( !advance( state, captured.config, today, std::move( initial ), work ) ) return {};
        double sum = 0;
        for ( uint64_t date = today; date <= captured.throughDay; ++date ) {
            const auto day = static_cast<uint32_t>( date );
            if ( day != today && !advance( state, captured.config, day, {}, work ) ) return {};
            if ( returnToFarm && day == today + 7 ) {
                P::Action a; a.id = a.project = 1; a.day = day; a.town = state.towns[townIndex].tile;
                a.kind = P::Kind::Build; a.building = BUILD_WEL2;
                if ( !advance( state, captured.config, day, { a }, work ) ) return {};
            }
            // Historical outlays are an OPTIONAL gold allowance, not recurring
            // obligations. Construction/trade materials must never be replayed
            // daily. Scale discretionary spending down before consuming the
            // operating reserve or the money needed to restore Farms.
            if ( day > today ) {
                auto balance = P::funds( state.economy );
                int64_t floor = reserveGold;
                for ( const auto & city : state.towns ) if ( city.race == Race::KNGT && ( city.mask & BUILD_TRAINING_GROUNDS ) ) floor += captured.config.farmGold;
                balance[0] -= std::min( overhead[0], std::max<int64_t>( 0, balance[0] - floor ) );
                P::setFunds( state.economy, balance );
            }
            // All towns compete for ONE treasury, in the same order in each scenario.
            // Stage the batch on a private copy so army slots and funds cannot be spent twice.
            auto planned = state;
            std::vector<P::Action> purchases;
            for ( size_t city = 0; city < planned.towns.size(); ++city ) {
                auto & t = planned.towns[city];
                if ( !t.available ) continue;
                for ( size_t pool = 6; pool-- > 1; ) {
                    const auto & dwelling = t.dwellings[pool];
                    const auto & creature = dwelling.creature( t.mask );
                    if ( !( t.mask & dwelling.base ) || !creature.id || !AI::EconomyBudget::validAmounts( creature.price ) ) continue;
                    auto funds = P::funds( planned.economy );
                    int64_t floor = reserveGold;
                    for ( const auto & cityState : planned.towns ) if ( cityState.race == Race::KNGT && ( cityState.mask & BUILD_TRAINING_GROUNDS ) ) floor += captured.config.farmGold;
                    funds[0] = std::max<int64_t>( 0, funds[0] - floor );
                    uint64_t count = P::stock( planned, city, pool );
                    for ( size_t r = 0; r < funds.size(); ++r ) if ( creature.price[r] > 0 ) count = std::min<uint64_t>( count, funds[r] / creature.price[r] );
                    count = std::min<uint64_t>( count, UINT32_MAX );
                    auto * destination = &t.garrison;
                    if ( !G::capacity( *destination, creature.id ) ) {
                        if ( ( city == townIndex && !includeVisitor ) || t.hero < 0 ) continue;
                        destination = &t.visitor;
                    }
                    count = std::min<uint64_t>( count, G::capacity( *destination, creature.id ) );
                    if ( !count || !P::join( *destination, creature.id, static_cast<uint32_t>( count ) ) ) continue;
                    auto balance = P::funds( planned.economy );
                    for ( size_t r = 0; r < balance.size(); ++r ) balance[r] -= creature.price[r] * static_cast<int64_t>( count );
                    P::setFunds( planned.economy, balance );
                    P::stock( planned, city, pool ) -= count;
                    auto action = G::recruit( t.tile, pool, static_cast<uint32_t>( count ), day );
                    action.id = purchases.size() + 1; action.project = 1; purchases.push_back( action );
                }
            }
            if ( !purchases.empty() && !advance( state, captured.config, day, std::move( purchases ), work ) ) return {};
            const auto force = M::localArmyValue( kingdom, state.towns[townIndex], M::Slot::TownArmy, includeVisitor );
            if ( !force.complete || !std::isfinite( force.force ) ) return {};
            if ( day > today ) sum += force.force;
        }
        return { true, sum / horizon, state.economy.gold };
    }
}

AI::KnightEconomy::Decision AI::KnightEconomy::choose( const Castle & castle, const bool safe )
{
    Decision out;
    if ( castle.GetRace() != Race::KNGT || !safe ) { out.reason = "unsafe_or_other_faction"; return out; }
    const auto day = world.CountDay();
    out.towns = static_cast<uint32_t>( castle.GetKingdom().GetCastles().size() );
    out.dailyOverhead = overhead( castle.GetKingdom() );
    // Starting mobilisation is weekly; financial recovery is checked every safe day.
    const bool training = castle.hasTrainingGrounds();
    const bool lowCashRecovery = training && int64_t{ castle.GetKingdom().GetFunds().gold } < reserveGold + castle.getBuildingCost( BUILD_WEL2 ).gold;
    if ( !day || ( ( day - 1 ) % 7 && !training ) || day > UINT32_MAX - horizon ) { out.reason = "weekly_review"; return out; }
    if ( !training && !castle.isBuild( BUILD_WEL2 ) ) { out.reason = "ordinary_farm_development"; return out; }
    const auto building = training ? BUILD_WEL2 : BUILD_TRAINING_GROUNDS;
    const auto & kingdom = castle.GetKingdom();
    if ( kingdom.GetCastles().size() > 64 ) { out.reason = "town_limit"; return out; }
    if ( !Difficulty::allowAIToDevelopCastlesOnDay( Game::getDifficulty(), Game::isCampaign(), day ) ) { out.reason = "difficulty_day_gate"; return out; }
    if ( !Difficulty::allowAIToBuildCastleBuilding( Game::getDifficulty(), Game::isCampaign(), building ) ) { out.reason = "difficulty_building_gate"; return out; }
    if ( !castle.AllowBuyBuilding( building ) ) { out.reason = "building_unavailable"; return out; }
    if ( training && ( lowCashRecovery || KnightConstruction::economicTown( castle ) ) ) {
        out.building = BUILD_WEL2; out.reason = lowCashRecovery ? "cash_recovery" : "economic_role_recovery";
        return out;
    }
    if ( !training && KnightConstruction::economicTown( castle ) ) { out.reason = "economic_role"; return out; }
    if ( !training && !KnightConstruction::mobilizationPreservesDevelopment( castle ) ) { out.reason = "development_delayed"; return out; }
    const auto * visitor = castle.GetHero();
    const bool includeVisitor = !visitor || ( visitor->Modes( Heroes::PATROL ) && !visitor->GetPatrolDistance() );
    const auto price = castle.getBuildingCost( building );
    if ( !training && ( int64_t{ kingdom.GetFunds().gold } - price.gold < reserveGold
         || price.gold > kingdom.GetFunds().gold / 4 ) ) { out.reason = "treasury_reserve"; return out; }
    auto input = P::capture( kingdom, day + horizon );
    const auto town = std::find_if( input.start.towns.begin(), input.start.towns.end(), [&]( const auto & t ) { return t.tile == castle.GetIndex(); } );
    if ( town == input.start.towns.end() ) { out.reason = "town_missing"; return out; }
    const size_t index = static_cast<size_t>( town - input.start.towns.begin() );
    if ( !includeVisitor ) {
        input.start.towns[index].visitor = {};
        if ( !P::refresh( input.start ) ) { out.reason = "visitor_projection_incomplete"; return out; }
    }
    Work work;
    const auto keep = rollout( kingdom, input, index, 0, work, false, includeVisitor, out.dailyOverhead );
    auto change = rollout( kingdom, input, index, building, work, false, includeVisitor, out.dailyOverhead );
    if ( !training ) {
        const auto cycle = rollout( kingdom, input, index, building, work, true, includeVisitor, out.dailyOverhead );
        out.cycleValid = cycle.valid; out.cycleForce = cycle.force; out.cycleGold = cycle.gold;
        const auto worthwhile = [&]( const Rollout & r ) {
            const double margin = 0.10 * std::max( { 1.0, keep.force, r.force } );
            return keep.valid && r.valid && r.gold >= reserveGold
                && ( r.force - keep.force > margin || ( r.force >= keep.force && r.gold - keep.gold > std::max<int64_t>( 1, price.gold / 10 ) ) );
        };
        if ( worthwhile( cycle ) && ( !worthwhile( change ) || ( cycle.force >= 0.90 * change.force && cycle.gold > change.gold ) ) ) {
            change = cycle; out.temporaryTraining = true;
        }
    }
    out.projections = work.projections;
    out.keepValid = keep.valid; out.switchValid = change.valid;
    out.keepForce = keep.force; out.switchForce = change.force; out.keepGold = keep.gold; out.switchGold = change.gold;
    if ( !keep.valid || !change.valid ) { out.reason = "forecast_incomplete"; return out; }
    // Reuse the prior stability work's 10% margin. Cash breaks only comparisons
    // that do not reduce projected military force; no universal force/gold weight.
    const double margin = 0.10 * std::max( { 1.0, keep.force, change.force } );
    const bool stronger = change.force - keep.force > margin;
    const bool economical = change.force >= keep.force && change.gold - keep.gold > std::max<int64_t>( 1, price.gold / 10 );
    const bool recovery = training && keep.gold < reserveGold && change.gold > keep.gold;
    if ( change.gold >= reserveGold && ( stronger || economical || recovery ) ) {
        out.building = building; out.reason = recovery ? "restore_reserve" : out.temporaryTraining ? "temporary_training" : stronger ? "military_gain" : "economic_gain";
    }
    else out.reason = "keep_current_role";
    return out;
}

bool AI::KnightEconomy::develop( Castle & castle, const bool safe )
{
    const auto decision = choose( castle, safe );
    ECONOMY_TRACE( 1, "KNIGHT_ROLE_REVIEW", ",\"kind\":\"evaluation\",\"town\":" << castle.GetIndex()
        << ",\"reason\":" << Economy::jsonQuote( decision.reason ) << ",\"building\":" << decision.building
        << ",\"farm_status\":" << static_cast<int>( castle.CheckBuyBuilding( BUILD_WEL2 ) )
        << ",\"tg_status\":" << static_cast<int>( castle.CheckBuyBuilding( BUILD_TRAINING_GROUNDS ) )
        << ",\"funds\":" << Economy::fundsJson( castle.GetKingdom().GetFunds() )
        << ",\"keep_valid\":" << ( decision.keepValid ? "true" : "false" ) << ",\"switch_valid\":" << ( decision.switchValid ? "true" : "false" )
        << ",\"cycle_valid\":" << ( decision.cycleValid ? "true" : "false" )
        << ",\"cycle_force\":" << decision.cycleForce << ",\"cycle_gold\":" << decision.cycleGold
        << ",\"temporary_training\":" << ( decision.temporaryTraining ? "true" : "false" )
        << ",\"keep_force\":" << decision.keepForce << ",\"switch_force\":" << decision.switchForce
        << ",\"keep_gold\":" << decision.keepGold << ",\"switch_gold\":" << decision.switchGold << ",\"projections\":" << decision.projections
        << ",\"forecast_towns\":" << decision.towns << ",\"daily_overhead\":" << Economy::arrayJson( decision.dailyOverhead ) );
    if ( !decision.building || !castle.AllowBuyBuilding( decision.building ) || !KnightConstruction::canSpend( castle, decision.building ) ) return false;
    // One immediate, synchronous purchase. Comparisons above issue no future
    // recruitment/transfer orders and do not create persistent commitments.
    return castle.BuyBuilding( decision.building );
}

namespace
{
    struct SpendingHistory { uint64_t segment = 0; uint32_t seed = 0, first = 0, last = 0; std::map<uint32_t, AI::EconomyProject::Resources> days; };
    std::map<int, SpendingHistory> spendingHistory;
    SpendingHistory & history( const Kingdom & kingdom )
    {
        auto & h = spendingHistory[static_cast<int>( kingdom.GetColor() )];
        const uint32_t day = world.CountDay();
        if ( h.segment != Economy::traceState().segment || h.seed != world.GetMapSeed() || day < h.last )
            h = { Economy::traceState().segment, world.GetMapSeed(), day, day, {} };
        h.last = day;
        while ( !h.days.empty() && h.days.begin()->first + 7 < day ) h.days.erase( h.days.begin() );
        return h;
    }
}
void AI::KnightEconomy::recordDebit( const Kingdom & kingdom, const Funds & before, const Funds & after, const char * cause )
{
    if ( fheroes2::taxpayerSimulationDepth || !kingdom.isControlAI() || !KnightConstruction::active( kingdom ) ) return;
    auto & h = history( kingdom );
    if ( !std::strcmp( cause, "troop_purchase" ) || !std::strcmp( cause, "conscription" ) || !std::strcmp( cause, "role_conversion" )
         || !std::strcmp( cause, "building_purchase" ) || !std::strcmp( cause, "market_trade" ) ) return;
    // Other gold outlays (heroes, upgrades, boats, unclassified actions) only
    // inform the bounded discretionary allowance. No material is extrapolated.
    h.days[world.CountDay()][0] += std::max<int64_t>( 0, int64_t{ before.gold } - after.gold );
}
std::array<int64_t, 7> AI::KnightEconomy::overhead( const Kingdom & kingdom )
{
    auto & h = history( kingdom ); AI::EconomyProject::Resources rate{};
    const auto day = world.CountDay();
    const uint32_t count = std::min<uint32_t>( 7, day - h.first );
    if ( !count ) return rate;
    for ( const auto & [date, amount] : h.days ) if ( date < day && date + count >= day )
        for ( size_t r = 0; r < rate.size(); ++r ) rate[r] += amount[r];
    for ( auto & amount : rate ) amount = ( amount + count - 1 ) / count;
    return rate;
}
