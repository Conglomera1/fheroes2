#include "knight_economy.h"
#include "knight_construction.h"
#include "economy_candidates.h"
#include "economy_military.h"
#include "economy_trace.h"
#include "difficulty.h"
#include "game.h"
#include "heroes.h"
#include "monster.h"
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

    struct InfantryProfile
    {
        std::array<uint32_t, 3> prices{};
        uint64_t weeklyGrowth = 0;
    };

    InfantryProfile infantry( const Castle & castle )
    {
        InfantryProfile result;
        const uint32_t dwellings[] = { DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4 };
        for ( size_t i = 0; i < result.prices.size(); ++i ) {
            const Monster unit( castle.GetRace(), castle.GetActualDwelling( dwellings[i] ) );
            result.prices[i] = unit.GetCost().gold;
            result.weeklyGrowth += unit.GetGrown() + ( castle.isBuild( BUILD_WELL ) ? Castle::GetGrownWell() : 0 );
        }
        return result;
    }

    struct DailyOrder
    {
        uint32_t count = 0;
        uint64_t fees = 0, recruitCost = 0;
        const char * reason = "stock_already_available";
    };

    DailyOrder pickOrder( const Conscription::TownDay & town, const Conscription::Settings & config, const InfantryProfile & infantry, const int64_t budget )
    {
        DailyOrder result;
        const uint64_t stock = std::accumulate( town.pools.begin(), town.pools.end(), uint64_t{} );
        if ( stock >= infantry.weeklyGrowth ) return result;
        if ( !config.successRate ) { result.reason = "training_cannot_succeed"; return result; }
        if ( !town.garrison && !town.hut ) { result.reason = "no_peasants"; return result; }
        const auto hutFee = Monster( Monster::PEASANT ).GetCost().gold;
        const uint32_t limit
            = static_cast<uint32_t>( std::min( { uint64_t{ Conscription::maxDailyOrder }, town.garrison + town.hut, infantry.weeklyGrowth - stock } ) );
        result.reason = "budget_reserved";
        for ( uint32_t count = 1; count <= limit; ++count ) {
            if ( town.state.total > UINT64_MAX - count ) break;
            const auto fees = Conscription::batchCost( config, count, town.garrison, hutFee );
            auto state = town.state;
            const auto recruits = Conscription::train( state, count, config );
            const uint64_t recruitCost = std::inner_product( recruits.begin(), recruits.end(), infantry.prices.begin(), uint64_t{} );
            if ( fees + recruitCost > static_cast<uint64_t>( std::max<int64_t>( 0, budget ) ) ) break;
            result = { count, fees, recruitCost, "funded_training_and_recruitment" };
        }
        return result;
    }

    P::Resources openingFunds( const Economy::Snapshot & today )
    {
        auto balance = P::funds( today );
        balance[0] = std::clamp<int64_t>( balance[0] + today.nonTaxIncome + Economy::taxIncome( today ), 0, INT32_MAX );
        for ( size_t i = 0; i < today.materialIncome.size(); ++i ) {
            balance[i + 1] = std::clamp<int64_t>( balance[i + 1] + today.materialIncome[i], 0, INT32_MAX );
        }
        return balance;
    }

    Conscription::TownDay morningStock( const Economy::Town & town, const bool newWeek )
    {
        auto morning = town.troops;
        if ( newWeek ) {
            morning.hut += town.hutGrowth;
            for ( size_t i = 0; i < morning.pools.size(); ++i ) {
                morning.pools[i] += town.poolGrowth[i];
            }
        }
        return morning;
    }

    // Reserve purchases already possible without conscription. Simulate slots and
    // payments on copies so castles cannot reserve the same gold or resources twice.
    int64_t plannedRecruitmentGold( const P::State & state, P::Resources funds, const bool newWeek )
    {
        const int64_t gold = funds[0];
        for ( size_t city = 0; city < state.towns.size(); ++city ) {
            const auto & town = state.towns[city];
            if ( !town.available ) continue;
            auto garrison = town.garrison;
            auto visitor = town.visitor;
            for ( size_t pool = town.dwellings.size(); pool-- > 1; ) {
                const auto & dwelling = town.dwellings[pool];
                const auto & unit = dwelling.creature( town.mask );
                if ( !( town.mask & dwelling.base ) || unit.id == 0 || !AI::EconomyBudget::validAmounts( unit.price ) ) continue;
                uint64_t count = pool < 4 ? state.economy.towns[city].troops.pools[pool - 1] : town.extraPools[pool - 4];
                if ( newWeek ) {
                    count += unit.growth + ( town.mask & BUILD_WELL ? town.wellBonus : 0 );
                }
                for ( size_t r = 0; r < funds.size(); ++r ) {
                    if ( unit.price[r] > 0 ) count = std::min<uint64_t>( count, funds[r] / unit.price[r] );
                }
                auto & army = G::capacity( garrison, unit.id ) > 0 || town.hero < 0 ? garrison : visitor;
                count = std::min( count, G::capacity( army, unit.id ) );
                if ( count == 0 || !P::join( army, unit.id, static_cast<uint32_t>( count ) ) ) continue;
                for ( size_t r = 0; r < funds.size(); ++r ) {
                    funds[r] -= unit.price[r] * static_cast<int64_t>( count );
                }
            }
        }
        return gold - funds[0];
    }

    int64_t orderBudget( const Kingdom & kingdom )
    {
        const auto development = AI::KnightConstruction::reservedDevelopment( kingdom )[0];
        int64_t conversionReserve = reserveGold;
        for ( const auto * town : kingdom.GetCastles() ) if ( town->hasTrainingGrounds() ) conversionReserve += Conscription::settings().farmGold;
        const int64_t income = kingdom.GetIncome( Kingdom::INCOME_ALL ).gold;
        return std::max<int64_t>( 0, int64_t{ kingdom.GetFunds().gold } + income - conversionReserve - development );
    }

    struct MobilizationPlan { uint32_t seed = 0, until = 0; uint64_t segment = 0; int owner = -1; bool recovering = false; };
    std::map<int, MobilizationPlan> mobilizationPlans;
    MobilizationPlan * activePlan( const Castle & castle )
    {
        const auto it = mobilizationPlans.find( castle.GetIndex() );
        if ( it == mobilizationPlans.end() ) return nullptr;
        const auto & p = it->second;
        if ( p.seed != world.GetMapSeed() || p.segment != Economy::traceState().segment || p.owner != static_cast<int>( castle.GetColor() )
             || world.CountDay() >= p.until || world.CountDay() + horizon < p.until ) {
            mobilizationPlans.erase( it ); return nullptr;
        }
        return &it->second;
    }
    struct Work { P::WorkBudget mechanics{ 512, 100000 }; uint32_t projections = 0; };
    struct Rollout { bool valid = false; double force = 0; int64_t gold = 0; bool sustained = false; };
    int64_t projectedReserve( const P::State & state, const Conscription::Settings & config, const size_t recruiting = SIZE_MAX )
    {
        int64_t reserve = reserveGold;
        for ( size_t i = 0; i < state.towns.size(); ++i ) {
            if ( state.towns[i].race != Race::KNGT || !( state.towns[i].mask & BUILD_TRAINING_GROUNDS ) ) continue;
            std::vector<Conscription::TownDay> days{ state.economy.towns[i].troops };
            constexpr int64_t money = INT64_MAX / 4;
            int64_t gold = money;
            for ( uint32_t offset = 1; offset <= 7; ++offset )
                Conscription::scheduleDay( days, gold, config, Monster( Monster::PEASANT ).GetCost().gold, state.economy.day + offset );
            reserve += config.farmGold + money - gold;
            if ( recruiting != i ) {
                for ( size_t pool = 0; pool < 3; ++pool ) {
                    const auto & dwelling = state.towns[i].dwellings[pool + 1];
                    reserve += dwelling.creature( state.towns[i].mask ).price[0]
                        * static_cast<int64_t>( days[0].pools[pool] - state.economy.towns[i].troops.pools[pool] );
                }
            }
        }
        return reserve;
    }
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
        double sum = 0; bool sustained = true;
        for ( uint64_t date = today; date <= captured.throughDay; ++date ) {
            const auto day = static_cast<uint32_t>( date );
            if ( day != today && !advance( state, captured.config, day, {}, work ) ) return {};
            int64_t recovery = reserveGold;
            for ( const auto & t : state.towns ) if ( t.race == Race::KNGT && ( t.mask & BUILD_TRAINING_GROUNDS ) ) recovery += captured.config.farmGold;
            if ( state.economy.gold < recovery ) sustained = false;
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
                const int64_t floor = projectedReserve( state, captured.config );
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
                    const int64_t floor = projectedReserve( planned, captured.config, city );
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
            if ( captured.config.mode == 2 && day < captured.throughDay ) {
                const bool newWeek = day % 7 == 0;
                auto funds = openingFunds( state.economy );
                funds[0] = std::max<int64_t>( 0, funds[0] - recovery );
                int64_t budget = funds[0] - plannedRecruitmentGold( state, funds, newWeek );
                for ( size_t city = 0; city < state.towns.size(); ++city ) {
                    auto & town = state.economy.towns[city];
                    if ( !town.training ) continue;
                    InfantryProfile units;
                    for ( size_t pool = 0; pool < units.prices.size(); ++pool ) {
                        units.prices[pool] = static_cast<uint32_t>( state.towns[city].dwellings[pool + 1].creature( state.towns[city].mask ).price[0] );
                        units.weeklyGrowth += town.poolGrowth[pool];
                    }
                    const auto order = pickOrder( morningStock( town, newWeek ), captured.config, units, budget );
                    town.troops.order = order.count;
                    budget -= static_cast<int64_t>( order.fees + order.recruitCost );
                }
            }
            const auto force = M::localArmyValue( kingdom, state.towns[townIndex], M::Slot::TownArmy, includeVisitor );
            if ( !force.complete || !std::isfinite( force.force ) ) return {};
            if ( day > today ) sum += force.force;
        }
        return { true, sum / horizon, state.economy.gold, sustained };
    }
}

void AI::KnightEconomy::planDailyOrders( Kingdom & kingdom )
{
    const auto & config = Conscription::settings();
    if ( config.mode != 2 || !kingdom.isControlAI() || fheroes2::taxpayerSimulationDepth ) return;
    const auto & castles = kingdom.GetCastles();
    if ( std::none_of( castles.begin(), castles.end(), []( const Castle * castle ) { return castle->hasTrainingGrounds(); } ) ) return;

    // Book next-day development before deciding what will actually be ordered.
    // The orders left from today must not consume tomorrow's construction budget.
    KnightConstruction::refresh( true );
    const auto day = world.CountDay();
    const bool newWeek = day % 7 == 0;
    const auto state = P::capture( kingdom, day ).start;
    const auto opening = openingFunds( state.economy );
    const auto development = KnightConstruction::reservedDevelopment( kingdom );
    const int64_t operating = KnightConstruction::operatingReserve( kingdom );
    auto funds = opening;
    for ( size_t i = 0; i < funds.size(); ++i ) {
        funds[i] = std::max<int64_t>( 0, funds[i] - development[i] );
    }
    funds[0] = std::max<int64_t>( 0, funds[0] - operating );
    const int64_t recruitment = plannedRecruitmentGold( state, funds, newWeek );
    int64_t budget = funds[0] - recruitment;
    ECONOMY_TRACE( 1, "CONSCRIPTION_ORDER_BUDGET", ",\"player\":" << static_cast<int>( kingdom.GetColor() )
        << ",\"opening_gold\":" << opening[0] << ",\"development\":" << development[0] << ",\"recruitment\":" << recruitment
        << ",\"operating_reserve\":" << operating << ",\"budget\":" << budget );
    size_t city = 0;
    for ( Castle * town : kingdom.GetCastles() ) {
        const auto & economic = state.economy.towns[city++];
        if ( !town->hasTrainingGrounds() ) continue;
        const auto morning = morningStock( economic, newWeek );
        const auto order = pickOrder( morning, config, infantry( *town ), budget );
        ECONOMY_TRACE( 1, "CONSCRIPTION_ORDER_AI", ",\"player\":" << static_cast<int>( kingdom.GetColor() ) << ",\"town\":" << town->GetIndex()
            << ",\"previous\":" << town->getConscriptionOrder() << ",\"requested\":" << order.count << ",\"budget\":" << budget
            << ",\"garrison\":" << morning.garrison << ",\"hut_forecast\":" << morning.hut << ",\"training_and_hut_fee\":" << order.fees
            << ",\"recruitment_cost\":" << order.recruitCost << ",\"reason\":" << Economy::jsonQuote( order.reason ) );
        town->setConscriptionOrder( order.count );
        budget -= static_cast<int64_t>( order.fees + order.recruitCost );
    }
}

uint32_t AI::KnightEconomy::supplyOrder( const Castle & castle )
{
    auto snapshot = castle.conscriptionDaySnapshot();
    const auto units = infantry( castle );
    // A shortage must not erase the demand which triggers a delivery.
    snapshot.garrison = std::max( snapshot.garrison, units.weeklyGrowth );
    return pickOrder( snapshot, Conscription::settings(), units, orderBudget( castle.GetKingdom() ) ).count;
}

AI::KnightEconomy::WeekFunding AI::KnightEconomy::weekFunding( const Castle & castle )
{
    WeekFunding out;
    if ( castle.GetRace() != Race::KNGT || world.CountDay() > UINT32_MAX - 7 ) return out;
    auto config = Conscription::settings();
    // A new TG is evaluated against the fixed schedule, independently of live adjustable orders.
    if ( config.mode == 2 ) config.mode = 1;
    std::vector<Conscription::TownDay> towns{ castle.conscriptionDaySnapshot() };
    const auto pools = towns[0].pools;
    const auto total = towns[0].state.total;
    constexpr int64_t money = INT64_MAX / 4;
    int64_t gold = money;
    for ( uint32_t day = world.CountDay() + 1; day <= world.CountDay() + 7; ++day )
        Conscription::scheduleDay( towns, gold, config, Monster( Monster::PEASANT ).GetCost().gold, day );
    out.fees[0] = money - gold; out.trained = towns[0].state.total - total;
    const std::array<uint32_t, 3> dwellings{ DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4 };
    for ( size_t i = 0; i < dwellings.size(); ++i ) {
        const auto price = Monster( castle.GetRace(), castle.GetActualDwelling( dwellings[i] ) ).GetCost();
        const std::array<int64_t, 7> cost{ price.gold, price.wood, price.ore, price.mercury, price.sulfur, price.crystal, price.gems };
        for ( size_t r = 0; r < cost.size(); ++r ) out.recruits[r] += cost[r] * static_cast<int64_t>( towns[0].pools[i] - pools[i] );
    }
    out.complete = true; return out;
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
    const auto & config = Conscription::settings();
    out.comparisonMode = !training && config.mode == 2 ? 1 : config.mode;
    bool lowCashRecovery = Conscription::settings().mode != 2 && training && int64_t{ castle.GetKingdom().GetFunds().gold } < reserveGold + castle.getBuildingCost( BUILD_WEL2 ).gold;
    if ( lowCashRecovery ) {
        const auto tomorrow = Economy::forecast( castle.GetKingdom().economySnapshot(), Conscription::settings(), 1 );
        const auto required = KnightConstruction::operatingReserve( castle.GetKingdom() );
        const bool fundedTomorrow = tomorrow.complete && tomorrow.end.gold >= required;
        ECONOMY_TRACE( 1, "MOBILIZATION_CASH_REVIEW", ",\"town\":" << castle.GetIndex() << ",\"gold\":" << castle.GetKingdom().GetFunds().gold
            << ",\"tomorrow_gold\":" << ( tomorrow.complete ? tomorrow.end.gold : -1 ) << ",\"required\":" << required
            << ",\"funded_tomorrow\":" << ( fundedTomorrow ? "true" : "false" ) );
        lowCashRecovery = !fundedTomorrow;
    }
    if ( !day || ( ( day - 1 ) % 7 && !training ) || day > UINT32_MAX - horizon ) { out.reason = "weekly_review"; return out; }
    if ( !training && !castle.isBuild( BUILD_WEL2 ) ) { out.reason = "ordinary_farm_development"; return out; }
    const auto building = training ? BUILD_WEL2 : BUILD_TRAINING_GROUNDS;
    const auto & kingdom = castle.GetKingdom();
    if ( kingdom.GetCastles().size() > 64 ) { out.reason = "town_limit"; return out; }
    if ( !Difficulty::allowAIToDevelopCastlesOnDay( Game::getDifficulty(), Game::isCampaign(), day ) ) { out.reason = "difficulty_day_gate"; return out; }
    if ( !Difficulty::allowAIToBuildCastleBuilding( Game::getDifficulty(), Game::isCampaign(), building ) ) { out.reason = "difficulty_building_gate"; return out; }
    if ( !castle.AllowBuyBuilding( building ) ) { out.reason = "building_unavailable"; return out; }
    const auto * currentPlan = activePlan( castle );
    if ( currentPlan ) out.planUntil = currentPlan->until;
    if ( lowCashRecovery ) {
        out.building = BUILD_WEL2; out.reason = "cash_recovery";
        return out;
    }
    if ( currentPlan ) {
        if ( training ) { out.reason = "funded_mobilization_plan"; return out; }
        if ( currentPlan->recovering ) { out.reason = "recovering_mobilization"; return out; }
    }
    if ( !training && KnightConstruction::economicTown( castle ) ) { out.reason = "economic_role"; return out; }
    if ( !training && !KnightConstruction::mobilizationPreservesDevelopment( castle ) ) { out.reason = "development_delayed"; return out; }
    const auto * visitor = castle.GetHero();
    const bool includeVisitor = !visitor || ( visitor->Modes( Heroes::PATROL ) && !visitor->GetPatrolDistance() );
    const auto price = castle.getBuildingCost( building );
    if ( !training && ( int64_t{ kingdom.GetFunds().gold } - price.gold < reserveGold
         || price.gold > kingdom.GetFunds().gold / 4 ) ) { out.reason = "treasury_reserve"; return out; }
    if ( !training ) {
        if ( Conscription::settings().mode == 2 ) {
            const auto troops = castle.conscriptionDaySnapshot();
            const auto taxRate = static_cast<uint64_t>( std::max( 0, Kingdom::GetTaxpayerGoldPerPeasantPerDay() ) );
            const auto cap = Kingdom::GetMaxTaxablePeasantsPerFarm();
            const uint64_t people = troops.garrison + ( visitor ? visitor->GetArmy().GetCountMonsters( Monster::PEASANT ) : 0 );
            out.farmTax = taxRate * std::min( people, cap ? uint64_t{ cap } : people );
        }
        const auto week = weekFunding( castle );
        out.comparisonWeekTrained = week.trained;
        if ( !week.complete || !week.trained ) { out.reason = "no_funded_conscription"; return out; }
        auto development = KnightConstruction::reservedDevelopment( kingdom );
        const std::array<int64_t, 7> entry{ price.gold, price.wood, price.ore, price.mercury, price.sulfur, price.crystal, price.gems };
        const std::array<int64_t, 7> recovery{ Conscription::settings().farmGold, Conscription::settings().farmWood, Conscription::settings().farmOre, 0, 0, 0, 0 };
        for ( size_t r = 0; r < entry.size(); ++r ) out.entryFunding[r] = entry[r] + recovery[r] + week.fees[r] + week.recruits[r] + development[r];
        out.entryFunding[0] += KnightConstruction::operatingReserve( kingdom );
        if ( !KnightConstruction::covers( P::funds( kingdom.economySnapshot() ), out.entryFunding ) ) {
            out.reason = "mobilization_funding"; return out;
        }
    }
    auto input = P::capture( kingdom, day + horizon );
    input.config.mode = out.comparisonMode;
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
            return keep.valid && r.valid && r.sustained && r.gold >= reserveGold
                && r.force - keep.force > margin;
        };
        // A short round trip must also recover its actual cash cost versus staying Farm.
        // Force alone previously admitted Hilltop: only 17 units for two conversions.
        if ( worthwhile( cycle ) && cycle.gold >= keep.gold && ( !worthwhile( change ) || ( cycle.force >= 0.90 * change.force && cycle.gold > change.gold ) ) ) {
            change = cycle; out.temporaryTraining = true;
        }
    }
    out.projections = work.projections;
    out.keepValid = keep.valid; out.switchValid = change.valid;
    out.keepForce = keep.force; out.switchForce = change.force; out.keepGold = keep.gold; out.switchGold = change.gold;
    if ( !keep.valid || !change.valid ) { out.reason = "forecast_incomplete"; return out; }
    if ( !training && !change.sustained ) { out.reason = "unsustainable_mobilization"; return out; }
    // Reuse the prior stability work's 10% margin. Cash breaks only comparisons
    // that do not reduce projected military force; no universal force/gold weight.
    const double margin = 0.10 * std::max( { 1.0, keep.force, change.force } );
    const bool stronger = change.force - keep.force > margin;
    const bool economical = training && change.force >= keep.force && change.gold - keep.gold > std::max<int64_t>( 1, price.gold / 10 );
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
        << ",\"forecast_towns\":" << decision.towns << ",\"daily_overhead\":" << Economy::arrayJson( decision.dailyOverhead )
        << ",\"entry_funding\":" << Economy::arrayJson( decision.entryFunding ) << ",\"plan_until\":" << decision.planUntil
        << ",\"conscription_mode\":" << Conscription::settings().mode << ",\"comparison_mode\":" << decision.comparisonMode
        << ",\"comparison_week_trained\":" << decision.comparisonWeekTrained
        << ",\"farm_tax_at_risk_daily\":" << decision.farmTax );
    if ( !decision.building || !castle.AllowBuyBuilding( decision.building ) || !KnightConstruction::canSpend( castle, decision.building ) ) return false;
    // One immediate, synchronous purchase. Comparisons above issue no future
    // recruitment/transfer orders and do not create persistent commitments.
    if ( !castle.BuyBuilding( decision.building ) ) return false;
    if ( decision.building == BUILD_TRAINING_GROUNDS ) {
        mobilizationPlans[castle.GetIndex()] = { world.GetMapSeed(), world.CountDay() + ( decision.temporaryTraining ? 7 : horizon ), Economy::traceState().segment, static_cast<int>( castle.GetColor() ), false };
    }
    else {
        const auto * old = activePlan( castle );
        const auto until = std::max( old ? old->until : 0U, world.CountDay() + 7 );
        mobilizationPlans[castle.GetIndex()] = { world.GetMapSeed(), until, Economy::traceState().segment, static_cast<int>( castle.GetColor() ), true };
    }
    if ( const auto * plan = activePlan( castle ) ) {
        ECONOMY_TRACE( 1, "MOBILIZATION_PLAN", ",\"town\":" << castle.GetIndex() << ",\"until\":" << plan->until
            << ",\"recovering\":" << ( plan->recovering ? "true" : "false" ) );
    }
    return true;
}

uint32_t AI::KnightEconomy::commitmentUntil( const Castle & castle )
{
    const auto * plan = activePlan( castle );
    return plan && !plan->recovering && castle.hasTrainingGrounds() ? plan->until : 0;
}

void AI::KnightEconomy::prepare( Kingdom & kingdom, const std::vector<int> & safeTowns )
{
    if ( fheroes2::taxpayerSimulationDepth || !kingdom.isControlAI() ) return;
    for ( Castle * castle : kingdom.GetCastles() ) {
        if ( !castle || castle->GetRace() != Race::KNGT || castle->hasTrainingGrounds()
             || std::find( safeTowns.begin(), safeTowns.end(), castle->GetIndex() ) == safeTowns.end() ) continue;
        // Primary development remains ahead of mobilisation. This call executes
        // only a plan chosen by the ordinary comparator; it never books a guess.
        if ( !castle->isBuild( BUILD_WEL2 ) || !castle->Modes( Castle::ALLOW_TO_BUILD_TODAY ) || KnightConstruction::hasPrimaryChoice( *castle ) ) continue;
        ECONOMY_TRACE( 1, "MOBILIZATION_EARLY_REVIEW", ",\"player\":" << static_cast<int>( kingdom.GetColor() )
            << ",\"town\":" << castle->GetIndex() << ",\"gold\":" << kingdom.GetFunds().gold );
        (void)develop( *castle, true );
    }
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
