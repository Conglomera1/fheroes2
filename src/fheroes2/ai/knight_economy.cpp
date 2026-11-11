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

namespace
{
    namespace P = AI::EconomyProject;
    namespace G = AI::EconomyCandidates;
    namespace M = AI::EconomyMilitary;
    constexpr int64_t reserveGold = 2000; // Existing taxpayer investment reserve.
    constexpr uint32_t horizon = 14;
    struct Work { P::WorkBudget mechanics{ 64, 4096 }; uint32_t projections = 0; };
    struct Rollout { bool valid = false; double force = 0; int64_t gold = 0; };
    bool advance( P::State & state, const Conscription::Settings & config, uint32_t day, std::vector<P::Action> actions, Work & work )
    {
        if ( work.projections >= 384 ) return false;
        ++work.projections;
        P::Input input; input.start = state; input.config = config; input.throughDay = day; input.actions = std::move( actions );
        const auto result = P::project( input, work.mechanics );
        if ( !result.covered() ) return false;
        state = result.end; return true;
    }
    Rollout rollout( const Kingdom & kingdom, const P::Input & captured, size_t townIndex, uint32_t building, Work & work, const bool returnToFarm = false, const bool includeVisitor = true )
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
            // Identical ordinary military outlet in both comparisons. Presort by
            // native unit strength/gold; Project checks all seven actual resources,
            // stocks and army slots. No gold-to-force score or virtual funds.
            std::array<size_t, 5> order{ 1, 2, 3, 4, 5 };
            const auto & town = state.towns[townIndex];
            std::stable_sort( order.begin(), order.end(), [&]( size_t a, size_t b ) {
                const auto & x = town.dwellings[a].creature( town.mask ); const auto & y = town.dwellings[b].creature( town.mask );
                return x.quickStrength / std::max<int64_t>( 1, x.price[0] ) > y.quickStrength / std::max<int64_t>( 1, y.price[0] );
            } );
            for ( const size_t pool : order ) {
                const auto & t = state.towns[townIndex]; const auto & dwelling = t.dwellings[pool];
                const auto & creature = dwelling.creature( t.mask );
                if ( !( t.mask & dwelling.base ) || !creature.id || !AI::EconomyBudget::validAmounts( creature.price ) ) continue;
                auto funds = P::funds( state.economy ); funds[0] = std::max<int64_t>( 0, funds[0] - reserveGold );
                uint64_t count = P::stock( state, townIndex, pool );
                for ( size_t r = 0; r < 7; ++r ) if ( creature.price[r] > 0 ) count = std::min<uint64_t>( count, funds[r] / creature.price[r] );
                const auto capacity = G::capacity( t.garrison, creature.id );
                count = std::min<uint64_t>( count, capacity ? capacity : includeVisitor && t.hero >= 0 ? G::capacity( t.visitor, creature.id ) : 0 );
                if ( !count ) continue;
                P::Action a = G::recruit( t.tile, pool, static_cast<uint32_t>( count ), day ); a.id = a.project = 1;
                if ( !advance( state, captured.config, day, { a }, work ) ) return {};
            }
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
    // One weekly role review limits oscillation without a new saved policy state.
    const bool training = castle.hasTrainingGrounds();
    const bool lowCashRecovery = training && int64_t{ castle.GetKingdom().GetFunds().gold } < reserveGold + castle.getBuildingCost( BUILD_WEL2 ).gold;
    if ( !day || ( ( day - 1 ) % 7 && !lowCashRecovery ) || day > UINT32_MAX - horizon ) { out.reason = "weekly_review"; return out; }
    if ( !training && !castle.isBuild( BUILD_WEL2 ) ) { out.reason = "ordinary_farm_development"; return out; }
    const auto building = training ? BUILD_WEL2 : BUILD_TRAINING_GROUNDS;
    const auto & kingdom = castle.GetKingdom();
    if ( kingdom.GetCastles().size() > 64 ) { out.reason = "town_limit"; return out; }
    if ( !Difficulty::allowAIToDevelopCastlesOnDay( Game::getDifficulty(), Game::isCampaign(), day ) ) { out.reason = "difficulty_day_gate"; return out; }
    if ( !Difficulty::allowAIToBuildCastleBuilding( Game::getDifficulty(), Game::isCampaign(), building ) ) { out.reason = "difficulty_building_gate"; return out; }
    if ( !castle.AllowBuyBuilding( building ) ) { out.reason = "building_unavailable"; return out; }
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
    const auto keep = rollout( kingdom, input, index, 0, work, false, includeVisitor );
    auto change = rollout( kingdom, input, index, building, work, false, includeVisitor );
    if ( !training ) {
        const auto cycle = rollout( kingdom, input, index, building, work, true, includeVisitor );
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
    if ( !keep.valid || !change.valid ) { out.reason = "forecast_incomplete"; return out; }
    out.keepForce = keep.force; out.switchForce = change.force; out.keepGold = keep.gold; out.switchGold = change.gold;
    // Reuse the prior stability work's 10% margin. Cash breaks only comparisons
    // that do not reduce projected military force; no universal force/gold weight.
    const double margin = 0.10 * std::max( { 1.0, keep.force, change.force } );
    const bool stronger = change.force - keep.force > margin;
    const bool economical = change.force >= keep.force && change.gold - keep.gold > std::max<int64_t>( 1, price.gold / 10 );
    const bool recovery = training && ( keep.gold < reserveGold || lowCashRecovery ) && change.gold > keep.gold && change.force >= 0.90 * keep.force;
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
        << ",\"cycle_valid\":" << ( decision.cycleValid ? "true" : "false" )
        << ",\"cycle_force\":" << decision.cycleForce << ",\"cycle_gold\":" << decision.cycleGold
        << ",\"temporary_training\":" << ( decision.temporaryTraining ? "true" : "false" )
        << ",\"keep_force\":" << decision.keepForce << ",\"switch_force\":" << decision.switchForce
        << ",\"keep_gold\":" << decision.keepGold << ",\"switch_gold\":" << decision.switchGold << ",\"projections\":" << decision.projections );
    if ( !decision.building || !castle.AllowBuyBuilding( decision.building ) || !KnightConstruction::canSpend( castle, decision.building ) ) return false;
    // One immediate, synchronous purchase. Comparisons above issue no future
    // recruitment/transfer orders and do not create persistent commitments.
    return castle.BuyBuilding( decision.building );
}
