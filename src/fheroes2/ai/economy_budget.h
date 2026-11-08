// Shared, dated reservations. Pure accounting only: no game mutation or AI choices.
#pragma once

#include "conscription.h"
#include <map>
#include <set>
#include <string>
#include <utility>

namespace AI::EconomyBudget
{
    // Same order everywhere: gold, wood, ore, mercury, sulfur, crystal, gems.
    using Resources = std::array<int64_t, 7>;
    constexpr int64_t resourceMaximum = std::numeric_limits<int32_t>::max();
    constexpr size_t entryLimit = 4096; // Accounting work bound, not a gameplay limit.

    struct StockClaim
    {
        std::string key; // Canonical source, e.g. town:42/pool:2, not a project-specific alias.
        uint32_t count = 0;
    };

    struct ExclusiveUse
    {
        std::string key; // Canonical hero or construction-action identifier.
        uint32_t throughDay = 0; // Inclusive; starts on the step's day.
    };

    struct Step
    {
        uint64_t id = 0, project = 0;
        uint32_t day = 0;
        bool required = false; // Informational: selection/urgency ordering belongs to the caller.
        Resources cost{};
        std::vector<StockClaim> stocks;
        std::vector<ExclusiveUse> exclusive;
    };

    struct Morning
    {
        uint64_t id = 0;
        uint32_t day = 0;
        Resources income{}, automaticBound{};
        bool conservative = false; // Bound may exceed real, funds-dependent TG charges.
        std::vector<StockClaim> arrivals; // Verified supply only, never hoped-for conquest.
    };

    struct Busy
    {
        std::string key;
        uint32_t first = 0, last = 0;
    };

    struct Input
    {
        uint32_t today = 0, throughDay = 0; // Today's morning has already been processed.
        Resources funds{};
        std::map<std::string, uint32_t> stocks;
        std::set<std::string> exclusiveAssets;
        std::vector<Busy> busy;
        std::vector<Morning> mornings;
        std::vector<Step> steps; // Stable supplied order within a day; no hidden reprioritization.
    };

    enum class Status { Covered, Unfunded, ConservativeGap, OutsideHorizon, Invalid, Limit };

    struct Balance
    {
        uint32_t day = 0;
        Resources after{}; // May be negative: diagnostic accounting, NEVER game resources.
    };

    struct Result
    {
        Status status = Status::Invalid;
        std::string reason;
        Resources remaining{}, extraNeeded{}; // Prefix deficits; not funds granted or debt incurred.
        uint64_t firstShortfall = 0;
        uint32_t firstShortfallDay = 0;
        bool firstShortfallRequired = false;
        std::vector<uint64_t> pending;
        std::vector<Balance> days;
        std::map<std::string, uint32_t> stocks;

        bool covered() const { return status == Status::Covered; }
    };

    inline bool validAmounts( const Resources & values )
    {
        return std::all_of( values.begin(), values.end(), []( const int64_t n ) { return n >= 0 && n <= resourceMaximum; } );
    }

    inline Result evaluate( const Input & input )
    {
        Result result;
        result.remaining = input.funds;
        result.stocks = input.stocks;
        auto reject = [&]( const Status status, const char * reason ) { result.status = status; result.reason = reason; return result; };
        if ( !input.today || input.throughDay < input.today || uint64_t{ input.throughDay } - input.today > 28 || !validAmounts( input.funds ) )
            return reject( Status::Invalid, "invalid_window_or_funds" );
        // Count nested claims too, not just top-level projects. Bound all arithmetic below.
        size_t entries = input.steps.size() + input.mornings.size() + input.busy.size() + input.stocks.size() + input.exclusiveAssets.size();
        if ( entries > entryLimit ) return reject( Status::Limit, "accounting_limit" );
        for ( const auto & step : input.steps ) {
            if ( step.stocks.size() > entryLimit - entries ) return reject( Status::Limit, "accounting_limit" );
            entries += step.stocks.size();
            if ( step.exclusive.size() > entryLimit - entries ) return reject( Status::Limit, "accounting_limit" );
            entries += step.exclusive.size();
        }
        for ( const auto & morning : input.mornings ) {
            if ( morning.arrivals.size() > entryLimit - entries ) return reject( Status::Limit, "accounting_limit" );
            entries += morning.arrivals.size();
        }
        for ( const auto & item : input.stocks ) if ( item.first.empty() ) return reject( Status::Invalid, "empty_stock_key" );
        if ( input.exclusiveAssets.count( "" ) ) return reject( Status::Invalid, "empty_asset_key" );
        std::set<uint64_t> ids;
        std::set<uint32_t> morningDays;
        auto busy = input.busy;
        for ( const auto & interval : busy ) {
            if ( !input.exclusiveAssets.count( interval.key ) || interval.first > interval.last )
                return reject( Status::Invalid, "invalid_busy_interval" );
        }
        for ( const auto & morning : input.mornings ) {
            if ( !morning.id || !ids.insert( morning.id ).second || !morningDays.insert( morning.day ).second
                 || morning.day <= input.today || !validAmounts( morning.income ) || !validAmounts( morning.automaticBound ) )
                return reject( Status::Invalid, "invalid_morning" );
            for ( const auto & stock : morning.arrivals )
                if ( !stock.count || !input.stocks.count( stock.key ) ) return reject( Status::Invalid, "invalid_arrival" );
        }
        for ( const auto & step : input.steps ) {
            if ( !step.id || !step.project || !ids.insert( step.id ).second || step.day < input.today || !validAmounts( step.cost ) )
                return reject( Status::Invalid, "invalid_step" );
            for ( const auto & claim : step.stocks )
                if ( !claim.count || !input.stocks.count( claim.key ) ) return reject( Status::Invalid, "invalid_stock_claim" );
            for ( const auto & use : step.exclusive )
                if ( !input.exclusiveAssets.count( use.key ) || use.throughDay < step.day ) return reject( Status::Invalid, "invalid_exclusive_use" );
            if ( step.day > input.throughDay ) result.pending.push_back( step.id );
        }
        bool conservative = false;
        auto debit = [&]( const Resources & amount, const uint64_t id, const uint32_t day, const bool required ) {
            for ( size_t r = 0; r < 7; ++r ) {
                result.remaining[r] -= amount[r];
                result.extraNeeded[r] = std::max( result.extraNeeded[r], -result.remaining[r] );
                if ( result.remaining[r] < 0 && !result.firstShortfall ) {
                    result.firstShortfall = id; result.firstShortfallDay = day; result.firstShortfallRequired = required;
                }
            }
        };
        for ( uint64_t date = input.today; date <= input.throughDay; ++date ) {
            const auto day = static_cast<uint32_t>( date );
            for ( const auto & morning : input.mornings ) {
                if ( morning.day != day ) continue;
                conservative = conservative || morning.conservative;
                for ( size_t r = 0; r < 7; ++r ) result.remaining[r] = std::min( resourceMaximum, result.remaining[r] + morning.income[r] );
                debit( morning.automaticBound, morning.id, day, false );
                for ( const auto & arrival : morning.arrivals ) {
                    auto & stock = result.stocks.at( arrival.key );
                    if ( uint64_t{ stock } + arrival.count > std::numeric_limits<uint32_t>::max() )
                        return reject( Status::Invalid, "stock_overflow" );
                    stock += arrival.count;
                }
            }
            for ( const auto & step : input.steps ) {
                if ( step.day != day ) continue;
                std::map<std::string, uint64_t> claims;
                for ( const auto & claim : step.stocks ) claims[claim.key] += claim.count;
                for ( const auto & claim : claims )
                    if ( claim.second > result.stocks.at( claim.first ) ) return reject( Status::Invalid, "stock_unavailable" );
                for ( const auto & use : step.exclusive ) {
                    if ( std::any_of( busy.begin(), busy.end(), [&]( const Busy & used ) {
                        return used.key == use.key && day <= used.last && used.first <= use.throughDay;
                    } ) ) return reject( Status::Invalid, "exclusive_conflict" );
                    busy.push_back( { use.key, day, use.throughDay } );
                }
                for ( const auto & claim : claims ) result.stocks.at( claim.first ) -= static_cast<uint32_t>( claim.second );
                debit( step.cost, step.id, day, step.required );
            }
            result.days.push_back( { day, result.remaining } );
        }
        if ( result.firstShortfall ) return reject( conservative ? Status::ConservativeGap : Status::Unfunded,
                                                   conservative ? "bound_requires_exact_replay" : "dated_funding_shortfall" );
        if ( !result.pending.empty() ) return reject( Status::OutsideHorizon, "future_commitments_unverified" );
        return reject( Status::Covered, "covered_under_supplied_flows" );
    }

    struct TrainingPeriod
    {
        int32_t town = -1;
        uint32_t firstMorning = 0, lastMorning = 0; // Inclusive; construction afternoon is not a paid morning.
    };

    struct ChargeBound
    {
        bool complete = false;
        std::string reason;
        std::vector<std::pair<uint32_t, int64_t>> days;
    };

    inline ChargeBound conscriptionBound( const Conscription::Settings & config, const uint32_t peasantCost,
                                         const uint32_t today, const uint32_t throughDay, const std::vector<TrainingPeriod> & periods )
    {
        ChargeBound result;
        if ( !today || throughDay < today || uint64_t{ throughDay } - today > 28 || periods.size() > entryLimit ) {
            result.reason = "invalid_window_or_limit"; return result;
        }
        for ( size_t i = 0; i < periods.size(); ++i ) {
            if ( periods[i].town < 0 || periods[i].firstMorning > periods[i].lastMorning ) { result.reason = "invalid_training_period"; return result; }
            for ( size_t j = 0; j < i; ++j ) {
                if ( periods[i].town == periods[j].town && periods[i].firstMorning <= periods[j].lastMorning
                     && periods[j].firstMorning <= periods[i].lastMorning ) { result.reason = "duplicate_training_period"; return result; }
            }
        }
        const uint64_t unitCost = uint64_t{ config.goldPerPeasant } + peasantCost;
        for ( uint64_t date = uint64_t{ today } + 1; date <= throughDay; ++date ) {
            const auto day = static_cast<uint32_t>( date );
            uint64_t charge = 0;
            if ( config.enabled ) {
                const uint64_t limit = Conscription::dailyLimitForDay( config, day );
                for ( const auto & period : periods ) {
                    if ( day < period.firstMorning || day > period.lastMorning ) continue;
                    // Test before multiplication: extreme INI values cannot wrap or become cheap.
                    if ( limit && unitCost > static_cast<uint64_t>( resourceMaximum ) / limit ) {
                        result.reason = "charge_out_of_range"; return result;
                    }
                    const uint64_t cost = limit * unitCost;
                    if ( cost > static_cast<uint64_t>( resourceMaximum ) - charge ) { result.reason = "charge_out_of_range"; return result; }
                    charge += cost;
                }
            }
            result.days.emplace_back( day, static_cast<int64_t>( charge ) );
        }
        result.complete = true;
        result.reason = "maximum_attempts_hut_price_not_actual_debits";
        return result;
    }
}
