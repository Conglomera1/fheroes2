// Observation-only dated projects. All mutations are confined to copied states.
#pragma once
#include "castle.h"
#include "economy_budget.h"
#include "economy_forecast.h"
#include <map>
#include <functional>
#include <set>

class Kingdom;

namespace AI::EconomyProject
{
    using Resources = EconomyBudget::Resources;
    struct Stack { int monster = 0; uint32_t count = 0; };
    using Army = std::array<Stack, 5>;
    struct Creature { int id = 0; uint32_t growth = 0; Resources price{}; double quickStrength = 0; }; // No commander bonuses; presort only.
    struct Dwelling
    {
        uint32_t base = 0, upgrade = 0, secondUpgrade = 0;
        Creature basic, improved, second;
        const Creature & creature( const uint32_t mask ) const
        { return secondUpgrade && ( mask & secondUpgrade ) ? second : upgrade && ( mask & upgrade ) ? improved : basic; }
    };
    struct Building
    {
        uint32_t id = 0, prerequisites = 0;
        Resources price{}, income{};
        int newPool = -1; // A newly built BASE dwelling gives its initial base growth.
    };
    struct Town
    {
        int32_t tile = -1;
        int race = 0, hero = -1;
        uint32_t mask = 0, disabled = 0, wellBonus = 0, farmBonus = 0;
        bool buildAvailable = true;
        Army garrison{}, visitor{};
        std::array<Dwelling, 6> dwellings;
        std::array<uint64_t, 2> extraPools{}; // Tiers 5/6; first four live in Economy::Snapshot.
        std::map<uint32_t, Building> catalog; // Only operations whose effects are supported.
    };
    struct State { Economy::Snapshot economy; std::vector<Town> towns; };
    enum class Kind { Build, Recruit, ToGarrison, ToVisitor };
    struct Action
    {
        uint64_t id = 0, project = 0;
        uint32_t day = 0, building = 0, count = 0;
        int32_t town = -1;
        Kind kind = Kind::Build;
        size_t pool = 0;
        bool required = false;
    };
    struct Input
    {
        State start;
        Conscription::Settings config;
        uint32_t throughDay = 0;
        std::vector<Action> actions;
        std::vector<Economy::ScheduledEvent> events; // Explicit known inputs only.
    };
    struct WorkBudget { uint64_t days = 24576, attempts = 200000; }; // Shared across candidate evaluations.
    enum class Status { Covered, Unfunded, Invalid, Incomplete, Limit };
    struct Purchase { uint32_t day; int32_t town; int monster; uint32_t count; bool visitor; };
    struct Day { Economy::Day morning; Resources afterActions{}; };
    struct Result
    {
        Status status = Status::Invalid;
        std::string reason;
        uint64_t failedAction = 0;
        uint32_t failedDay = 0;
        State end;
        EconomyBudget::Result budget;
        std::vector<Day> days;
        std::vector<Purchase> purchases;
        bool covered() const { return status == Status::Covered; }
    };

    // Engine adapter captures effective INI costs, actual stocks, armies and construction availability.
    Input capture( const Kingdom & kingdom, uint32_t throughDay );

    inline Resources funds( const Economy::Snapshot & state )
    { return { state.gold, state.materials[0], state.materials[1], state.materials[2], state.materials[3], state.materials[4], state.materials[5] }; }
    inline void setFunds( Economy::Snapshot & state, const Resources & value )
    { state.gold = value[0]; for ( size_t i = 0; i < 6; ++i ) state.materials[i] = value[i + 1]; }
    inline uint64_t count( const Army & army, const int monster = Monster::PEASANT )
    { uint64_t n = 0; for ( const auto & stack : army ) if ( stack.monster == monster ) n += stack.count; return n; }
    inline uint64_t population( const Army & army )
    { uint64_t n = 0; for ( const auto & stack : army ) n += stack.count; return n; }
    inline bool join( Army & army, const int monster, const uint32_t quantity )
    {
        for ( auto & stack : army ) if ( stack.count && stack.monster == monster ) {
            if ( uint64_t{ stack.count } + quantity > std::numeric_limits<uint32_t>::max() ) return false;
            stack.count += quantity; return true;
        }
        for ( auto & stack : army ) if ( !stack.count ) { stack = { monster, quantity }; return true; }
        return false;
    }
    inline void removePeasants( Army & army, uint64_t quantity )
    {
        for ( auto & stack : army ) if ( stack.monster == Monster::PEASANT ) {
            const uint64_t taken = std::min<uint64_t>( quantity, stack.count );
            stack.count -= static_cast<uint32_t>( taken ); quantity -= taken;
            if ( !stack.count ) stack.monster = 0;
        }
    }
    inline uint64_t & stock( State & state, const size_t town, const size_t pool )
    {
        auto & troops = state.economy.towns[town].troops;
        if ( !pool ) return troops.hut;
        if ( pool < 4 ) return troops.pools[pool - 1];
        return state.towns[town].extraPools[pool - 4];
    }
    inline bool refresh( State & state )
    {
        for ( size_t i = 0; i < state.towns.size(); ++i ) {
            const auto & town = state.towns[i];
            auto & economic = state.economy.towns[i];
            economic.training = town.race == Race::KNGT && ( town.mask & BUILD_TRAINING_GROUNDS );
            economic.farm = town.race == Race::KNGT && ( town.mask & BUILD_WEL2 );
            economic.troops.garrison = count( town.garrison );
            economic.visitingPeasants = count( town.visitor );
            for ( size_t pool = 0; pool < 4; ++pool ) {
                const auto & dwelling = town.dwellings[pool];
                const uint64_t growth = !( town.mask & dwelling.base ) ? 0 : uint64_t{ dwelling.creature( town.mask ).growth }
                    + ( town.mask & BUILD_WELL ? town.wellBonus : 0 ) + ( pool == 0 && ( town.mask & BUILD_WEL2 ) ? town.farmBonus : 0 );
                if ( growth > std::numeric_limits<uint32_t>::max() ) return false;
                if ( !pool ) economic.hutGrowth = static_cast<uint32_t>( growth );
                else economic.poolGrowth[pool - 1] = static_cast<uint32_t>( growth );
            }
        }
        return true;
    }

    using DayObserver = std::function<void( const State & )>;
    inline Result project( const Input & input, WorkBudget & work, const DayObserver & observer = {} )
    {
        Result result;
        result.end = input.start;
        State state = input.start;
        EconomyBudget::Input ledger;
        ledger.today = state.economy.day; ledger.throughDay = input.throughDay; ledger.funds = funds( state.economy );
        auto stop = [&]( const Status status, const std::string & reason, const uint64_t action = 0, const uint32_t day = 0 ) {
            result.status = status; result.reason = reason; result.failedAction = action; result.failedDay = day;
            result.budget = EconomyBudget::evaluate( ledger ); return result;
        };
        if ( !ledger.today || ledger.throughDay < ledger.today || uint64_t{ ledger.throughDay } - ledger.today > 28
             || state.towns.size() != state.economy.towns.size() || !EconomyBudget::validAmounts( ledger.funds ) )
            return stop( Status::Invalid, "invalid_initial_state" );
        if ( state.towns.size() > 256 || input.actions.size() > 1024 || input.events.size() > 1024 ) return stop( Status::Limit, "input_limit" );
        std::set<int32_t> tiles;
        std::set<int> heroes;
        std::set<uint64_t> ids;
        for ( size_t i = 0; i < state.towns.size(); ++i ) {
            const auto & town = state.towns[i];
            const auto & economic = state.economy.towns[i];
            if ( town.tile < 0 || economic.tile != town.tile || !tiles.insert( town.tile ).second
                 || ( town.hero >= 0 && !heroes.insert( town.hero ).second ) || ( town.hero < 0 && population( town.visitor ) )
                 || economic.troops.garrison != count( town.garrison ) || economic.visitingPeasants != count( town.visitor )
                 || ( town.race == Race::KNGT && ( town.mask & BUILD_WEL2 ) && ( town.mask & BUILD_TRAINING_GROUNDS ) ) )
                return stop( Status::Invalid, "inconsistent_town" );
            const auto key = "town:" + std::to_string( town.tile ) + "/build";
            for ( size_t pool = 0; pool < 6; ++pool ) {
                if ( stock( state, i, pool ) > std::numeric_limits<uint32_t>::max() ) return stop( Status::Invalid, "invalid_stock_range" );
                if ( ( town.mask & town.dwellings[pool].base ) && !town.dwellings[pool].creature( town.mask ).id )
                    return stop( Status::Incomplete, "unsupported_existing_dwelling" );
            }
            if ( town.catalog.size() > 32 ) return stop( Status::Limit, "catalog_limit" );
            for ( const auto & entry : town.catalog ) {
                const auto & building = entry.second;
                if ( !entry.first || ( entry.first & ( entry.first - 1 ) ) || entry.first != building.id
                     || building.newPool < -1 || building.newPool >= 6 || !EconomyBudget::validAmounts( building.price )
                     || !EconomyBudget::validAmounts( building.income ) ) return stop( Status::Invalid, "invalid_catalog" );
            }
            ledger.exclusiveAssets.insert( key );
            if ( !town.buildAvailable ) ledger.busy.push_back( { key, ledger.today, ledger.today } );
        }
        for ( const auto & action : input.actions ) {
            if ( !action.id || action.id >= ( uint64_t{ 1 } << 63 ) || !action.project || action.day < ledger.today || !ids.insert( action.id ).second
                 || !tiles.count( action.town ) ) return stop( Status::Invalid, "invalid_action", action.id, action.day );
        }
        if ( !refresh( state ) || !Economy::forecast( state.economy, input.config, 0, 200000, input.events ).complete )
            return stop( Status::Invalid, "invalid_economic_range" );
        for ( uint64_t date = ledger.today; date <= ledger.throughDay; ++date ) {
            const auto day = static_cast<uint32_t>( date );
            Day record;
            record.morning.day = day;
            if ( date > ledger.today ) {
                size_t training = 0;
                for ( const auto & town : state.economy.towns ) if ( town.training ) ++training;
                const uint64_t attempts = input.config.enabled ? uint64_t{ Conscription::dailyLimitForDay( input.config, day ) } * training : 0;
                if ( !work.days || attempts > work.attempts ) return stop( Status::Limit, "shared_projection_limit", 0, day );
                --work.days; work.attempts -= attempts;
                const Resources before = funds( state.economy );
                if ( ( day - 1 ) % 7 == 0 ) {
                    for ( size_t i = 0; i < state.towns.size(); ++i ) for ( size_t pool = 4; pool < 6; ++pool ) {
                        const auto & town = state.towns[i];
                        const auto & dwelling = town.dwellings[pool];
                        const uint64_t growth = !( town.mask & dwelling.base ) ? 0 : uint64_t{ dwelling.creature( town.mask ).growth }
                            + ( town.mask & BUILD_WELL ? town.wellBonus : 0 );
                        if ( stock( state, i, pool ) + growth > std::numeric_limits<uint32_t>::max() ) return stop( Status::Incomplete, "pool_overflow", 0, day );
                        stock( state, i, pool ) += growth;
                    }
                }
                const auto next = Economy::forecast( state.economy, input.config, 1, attempts, input.events );
                if ( !next.complete ) return stop( Status::Incomplete, next.reason, 0, day );
                for ( size_t i = 0; i < state.towns.size(); ++i )
                    removePeasants( state.towns[i].garrison, state.economy.towns[i].troops.garrison - next.end.towns[i].troops.garrison );
                state.economy = next.end;
                record.morning = next.days[0];
                for ( auto & town : state.towns ) town.buildAvailable = true;
                EconomyBudget::Morning morning;
                morning.id = ( uint64_t{ 1 } << 63 ) + day; morning.day = day;
                const auto after = funds( state.economy );
                // Audit the APPLIED net morning flow, including event clamps; don't charge TG twice.
                for ( size_t r = 0; r < 7; ++r ) {
                    morning.income[r] = std::max<int64_t>( 0, after[r] - before[r] );
                    morning.automaticBound[r] = std::max<int64_t>( 0, before[r] - after[r] );
                }
                ledger.mornings.push_back( morning );
            }
            for ( const auto & action : input.actions ) {
                if ( action.day != day ) continue;
                const size_t i = static_cast<size_t>( std::find_if( state.towns.begin(), state.towns.end(), [&]( const Town & t ) { return t.tile == action.town; } ) - state.towns.begin() );
                auto & town = state.towns[i];
                Resources price{};
                EconomyBudget::Step commitment;
                commitment.id = action.id; commitment.project = action.project; commitment.day = day; commitment.required = action.required;
                const Building * building = nullptr;
                const Creature * creature = nullptr;
                if ( action.kind == Kind::Build ) {
                    const auto found = town.catalog.find( action.building );
                    if ( found == town.catalog.end() ) return stop( Status::Incomplete, "unsupported_building", action.id, day );
                    building = &found->second;
                    const bool knight = town.race == Race::KNGT;
                    const uint32_t required = knight && action.building == BUILD_TRAINING_GROUNDS ? input.config.requiredBuildings : building->prerequisites;
                    if ( !town.buildAvailable || !( town.mask & BUILD_CASTLE ) || ( town.mask & action.building )
                         || ( town.disabled & action.building ) || ( ( town.mask & required ) != required )
                         || ( knight && action.building == BUILD_TRAINING_GROUNDS && ( !input.config.enabled || ( town.disabled & BUILD_WEL2 ) ) ) )
                        return stop( Status::Invalid, "building_unavailable", action.id, day );
                    price = building->price;
                    if ( knight && action.building == BUILD_TRAINING_GROUNDS ) price = { input.config.trainingGold, input.config.trainingWood, input.config.trainingOre, 0, 0, 0, 0 };
                    else if ( knight && action.building == BUILD_WEL2 && ( town.mask & BUILD_TRAINING_GROUNDS ) )
                        price = { input.config.farmGold, input.config.farmWood, input.config.farmOre, 0, 0, 0, 0 };
                    commitment.exclusive = { { "town:" + std::to_string( town.tile ) + "/build", day } };
                }
                else if ( action.kind == Kind::Recruit ) {
                    if ( action.pool >= 6 || !action.count || !( town.mask & town.dwellings[action.pool].base )
                         || stock( state, i, action.pool ) < action.count ) return stop( Status::Invalid, "recruit_stock_unavailable", action.id, day );
                    creature = &town.dwellings[action.pool].creature( town.mask );
                    if ( !creature->id || !EconomyBudget::validAmounts( creature->price ) ) return stop( Status::Incomplete, "unsupported_creature", action.id, day );
                    for ( size_t r = 0; r < 7; ++r ) {
                        if ( creature->price[r] > EconomyBudget::resourceMaximum / action.count ) return stop( Status::Incomplete, "price_overflow", action.id, day );
                        price[r] = creature->price[r] * action.count;
                    }
                }
                else if ( action.kind != Kind::ToGarrison && action.kind != Kind::ToVisitor ) return stop( Status::Incomplete, "unsupported_action", action.id, day );
                if ( !EconomyBudget::validAmounts( price ) ) return stop( Status::Incomplete, "invalid_price", action.id, day );
                commitment.cost = price;
                ledger.steps.push_back( commitment );
                auto balance = funds( state.economy );
                for ( size_t r = 0; r < 7; ++r ) if ( price[r] > balance[r] ) return stop( Status::Unfunded, "payment_unavailable", action.id, day );
                if ( building ) {
                    if ( building->newPool >= 0 && building->newPool < 6 ) stock( state, i, static_cast<size_t>( building->newPool ) ) = town.dwellings[static_cast<size_t>( building->newPool )].basic.growth;
                    if ( town.race == Race::KNGT && action.building == BUILD_TRAINING_GROUNDS ) town.mask &= ~BUILD_WEL2;
                    else if ( town.race == Race::KNGT && action.building == BUILD_WEL2 ) town.mask &= ~BUILD_TRAINING_GROUNDS;
                    town.mask |= action.building; town.buildAvailable = false;
                    const int64_t income = state.economy.nonTaxIncome + building->income[0];
                    if ( income > EconomyBudget::resourceMaximum ) return stop( Status::Incomplete, "income_overflow", action.id, day );
                    state.economy.nonTaxIncome = income;
                    for ( size_t r = 0; r < 6; ++r ) {
                        state.economy.materialIncome[r] += building->income[r + 1];
                        if ( state.economy.materialIncome[r] > EconomyBudget::resourceMaximum ) return stop( Status::Incomplete, "income_overflow", action.id, day );
                    }
                }
                else if ( creature ) {
                    bool visitor = false;
                    if ( !join( town.garrison, creature->id, action.count ) ) {
                        if ( town.hero < 0 || !join( town.visitor, creature->id, action.count ) ) return stop( Status::Invalid, "army_slots_unavailable", action.id, day );
                        visitor = true;
                    }
                    stock( state, i, action.pool ) -= action.count;
                    result.purchases.push_back( { day, town.tile, creature->id, action.count, visitor } );
                }
                else {
                    if ( town.hero < 0 || !action.count ) return stop( Status::Invalid, "visitor_unavailable", action.id, day );
                    Army & source = action.kind == Kind::ToGarrison ? town.visitor : town.garrison;
                    Army & destination = action.kind == Kind::ToGarrison ? town.garrison : town.visitor;
                    if ( count( source ) < action.count || ( action.kind == Kind::ToGarrison && population( source ) <= action.count )
                         || !join( destination, Monster::PEASANT, action.count ) ) return stop( Status::Invalid, "peasant_transfer_unavailable", action.id, day );
                    removePeasants( source, action.count );
                }
                for ( size_t r = 0; r < 7; ++r ) balance[r] -= price[r];
                setFunds( state.economy, balance );
                if ( !refresh( state ) ) return stop( Status::Incomplete, "growth_overflow", action.id, day );
            }
            record.afterActions = funds( state.economy );
            result.days.push_back( record ); result.end = state;
            if ( observer ) observer( state );
        }
        for ( const auto & action : input.actions ) if ( action.day > ledger.throughDay ) {
            EconomyBudget::Step pending;
            pending.id = action.id; pending.project = action.project; pending.day = action.day; pending.required = action.required;
            ledger.steps.push_back( pending ); // Cost not certified outside H; keep identity and date.
        }
        result.budget = EconomyBudget::evaluate( ledger );
        if ( result.budget.status == EconomyBudget::Status::OutsideHorizon ) { result.status = Status::Incomplete; result.reason = "future_actions_unverified"; return result; }
        if ( !result.budget.covered() || result.budget.remaining != funds( state.economy ) ) return stop( Status::Incomplete, "budget_reconciliation_failed" );
        result.status = Status::Covered; result.reason = "copied_actions_normal_growth_explicit_events";
        return result;
    }
}
