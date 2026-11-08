// Passive forecasts: copies only, normal weekly growth, no purchases or events.
#pragma once

#include "conscription.h"
#include <string>
#include <utility>

namespace Economy
{
    struct Town
    {
        int32_t tile = -1;
        bool training = false;
        bool farm = false; // An eligible Knight Farm, not another race's growth building.
        uint64_t visitingPeasants = 0;
        Conscription::TownDay troops;
        uint32_t hutGrowth = 0;
        std::array<uint32_t, 3> poolGrowth{};
    };

    struct Snapshot
    {
        uint32_t day = 0; // Today's income and conscription have already been applied.
        int64_t gold = 0;
        int64_t nonTaxIncome = 0;
        uint32_t taxRate = 0;
        bool requireFarms = true;
        uint64_t otherPeasants = 0;
        uint32_t peasantCost = 0;
        std::vector<Town> towns; // Preserve kingdom/interface order.
    };

    inline int64_t taxIncome( const Snapshot & snapshot )
    {
        const uint64_t maximum = std::numeric_limits<int32_t>::max();
        if ( snapshot.taxRate == 0 ) return 0;
        // Saturate the count before multiplication, just as the credited tax is capped.
        uint64_t count = snapshot.requireFarms ? 0 : std::min( maximum, snapshot.otherPeasants );
        for ( const Town & town : snapshot.towns ) {
            if ( !snapshot.requireFarms || town.farm ) {
                count = std::min( maximum, count + std::min( maximum, town.troops.garrison ) );
                count = std::min( maximum, count + std::min( maximum, town.visitingPeasants ) );
            }
        }
        return static_cast<int64_t>( std::min( maximum, count * snapshot.taxRate ) );
    }

    struct Day
    {
        uint32_t day = 0;
        int64_t openingGold = 0, income = 0, tax = 0, fees = 0, closingGold = 0;
        uint64_t fromGarrison = 0, fromHut = 0, failed = 0;
        std::array<uint64_t, 3> recruits{};
    };

    struct Forecast
    {
        Snapshot end;
        std::vector<Day> days;
        bool complete = false;
        std::string reason;
    };

    inline Forecast forecast( const Snapshot & start, const Conscription::Settings & config, const uint32_t days,
                              const uint64_t workLimit = 200000 )
    {
        Forecast result;
        result.end = start;
        if ( start.gold < 0 || start.gold > std::numeric_limits<int32_t>::max()
             || start.nonTaxIncome < 0 || start.nonTaxIncome > std::numeric_limits<int32_t>::max() ) {
            result.reason = "unsupported_input_range"; return result;
        }
        if ( days > 28 ) { result.reason = "horizon_limit"; return result; }
        uint64_t work = 0;
        for ( uint32_t n = 0; n < days; ++n ) {
            Snapshot next = result.end; // Never publish a partially projected day.
            if ( next.day == std::numeric_limits<uint32_t>::max() ) { result.reason = "day_overflow"; return result; }
            ++next.day;
            Day record;
            record.day = next.day;
            record.openingGold = next.gold;
            for ( Town & town : next.towns ) {
                town.troops.fromGarrison = town.troops.fromHut = town.troops.failed = 0;
                if ( next.day > 1 && ( next.day - 1 ) % 7 == 0 ) {
                    if ( town.troops.hut + town.hutGrowth > std::numeric_limits<uint32_t>::max() ) {
                        result.reason = "population_overflow"; return result;
                    }
                    town.troops.hut += town.hutGrowth;
                    for ( size_t i = 0; i < 3; ++i ) {
                        if ( town.troops.pools[i] + town.poolGrowth[i] > std::numeric_limits<uint32_t>::max() ) {
                            result.reason = "pool_overflow"; return result;
                        }
                        town.troops.pools[i] += town.poolGrowth[i];
                    }
                }
            }
            record.tax = next.day > 1 ? taxIncome( next ) : 0;
            record.income = next.day > 1 ? next.nonTaxIncome + record.tax : 0;
            const int64_t credited = next.gold + record.income;
            if ( credited > std::numeric_limits<int32_t>::max() || credited < 0 ) {
                result.reason = "treasury_out_of_range"; return result;
            }
            next.gold = credited;
            std::vector<Conscription::TownDay> training;
            for ( const Town & town : next.towns ) if ( town.training ) training.push_back( town.troops );
            const uint64_t dayWork = next.day > 1 && config.enabled
                                         ? uint64_t{ Conscription::dailyLimitForDay( config, next.day ) } * training.size() : 0;
            if ( dayWork > workLimit - work ) { result.reason = "work_limit"; return result; }
            work += dayWork;
            if ( next.day > 1 ) Conscription::scheduleDay( training, next.gold, config, next.peasantCost, next.day );
            size_t index = 0;
            for ( Town & town : next.towns ) {
                if ( !town.training ) continue;
                const auto & after = training[index++];
                record.fromGarrison += after.fromGarrison;
                record.fromHut += after.fromHut;
                record.failed += after.failed;
                for ( size_t i = 0; i < 3; ++i ) record.recruits[i] += after.pools[i] - town.troops.pools[i];
                town.troops = after;
            }
            record.fees = credited - next.gold;
            record.closingGold = next.gold;
            result.end = std::move( next );
            result.days.push_back( record );
        }
        result.complete = true;
        result.reason = "normal_growth_no_actions_no_events";
        return result;
    }
}
