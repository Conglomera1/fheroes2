/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <numeric>
#include <vector>
#include <limits>

namespace Conscription
{
    constexpr uint32_t successScale = 1000000;
    constexpr int32_t defaultMode = 2;

    constexpr int32_t defaultGoldPerPeasant( const int32_t mode )
    {
        return mode == 1 ? 100 : 50;
    }

    using Quotas = std::array<uint32_t, 3>;
    struct State
    {
        uint64_t total = 0;
        Quotas quotas{ 8, 5, 4 };
        Quotas used{};
        uint32_t next = 0;
        uint32_t lossRemainder = 0;
    };

    // Shared by preview and execution. A changed ratio takes effect only at a cycle boundary.
    inline std::array<uint64_t, 3> advance( State & state, uint32_t count, const Quotas & configured )
    {
        std::array<uint64_t, 3> result{};
        const Quotas defaults{ 8, 5, 4 };
        const Quotas & desired = std::accumulate( configured.begin(), configured.end(), uint64_t{ 0 } ) ? configured : defaults;
        state.total += count;
        while ( count ) {
            if ( state.used == Quotas{} ) {
                state.quotas = desired;
                state.next = 0;
                const uint64_t length = std::accumulate( desired.begin(), desired.end(), uint64_t{ 0 } );
                const uint64_t cycles = count / length;
                if ( cycles ) {
                    for ( std::size_t i = 0; i < 3; ++i )
                        result[i] += cycles * desired[i];
                    count -= static_cast<uint32_t>( cycles * length );
                    if ( !count )
                        break;
                }
            }
            while ( state.used[state.next] == state.quotas[state.next] )
                state.next = ( state.next + 1 ) % 3;
            ++state.used[state.next];
            ++result[state.next];
            --count;
            state.next = ( state.next + 1 ) % 3;
            if ( state.used == state.quotas ) {
                state.used = {};
                state.next = 0;
            }
        }
        return result;
    }

    template<class Stream> void writeState( Stream & stream, const State & state )
    {
        stream << static_cast<uint32_t>( state.total >> 32 ) << static_cast<uint32_t>( state.total );
        for ( const auto value : state.quotas ) stream << value;
        for ( const auto value : state.used ) stream << value;
        stream << state.next;
        stream << state.lossRemainder;
    }

    template<class Stream> void readState( Stream & stream, State & state, const bool hasLossRemainder = true )
    {
        uint32_t high = 0, low = 0;
        stream >> high >> low;
        state.total = ( uint64_t{ high } << 32 ) | low;
        for ( auto & value : state.quotas ) stream >> value;
        for ( auto & value : state.used ) stream >> value;
        stream >> state.next;
        state.lossRemainder = 0;
        if ( hasLossRemainder ) stream >> state.lossRemainder;
        bool valid = state.next < 3 && state.lossRemainder < successScale;
        uint64_t remaining = 0;
        for ( std::size_t i = 0; i < 3; ++i ) {
            valid = valid && state.quotas[i] <= 10000 && state.used[i] <= state.quotas[i];
            if ( state.used[i] <= state.quotas[i] ) remaining += state.quotas[i] - state.used[i];
        }
        if ( !valid || !remaining ) stream.setFail();
    }

    struct Settings
    {
        uint32_t goldPerPeasant = 50;
        uint32_t mode = 1;
        double goldIncrement = 1.0;
        double goldIncrementMultiplier = 1.0;
        uint32_t goldIncrementStep = 1;
        bool uniformBatchPrice = false;
        bool showArtwork = true;
        Quotas quotas{ 8, 5, 4 };
        uint32_t weeklyLimit = 0; // Legacy save compatibility; no longer used.
        uint32_t dailyLimit = 3;
        uint32_t oddDayLimit = 2;
        uint32_t evenDayLimit = 3;
        bool enabled = true;
        uint32_t successRate = successScale;
        uint32_t requiredBuildings = 0;
        int32_t trainingGold = 1750, trainingWood = 10, trainingOre = 5;
        int32_t farmGold = 1250, farmWood = 0, farmOre = 0;
    };
    const Settings & settings();

    struct TownDay
    {
        State state;
        uint64_t garrison = 0;
        uint64_t hut = 0;
        std::array<uint64_t, 3> pools{};
        uint32_t fromGarrison = 0, fromHut = 0;
        uint32_t failed = 0;
        uint32_t order = 0;
        uint64_t trainingFees = 0;
        int64_t goldBefore = 0;
    };

    constexpr uint32_t maxDailyOrder = static_cast<uint32_t>( std::numeric_limits<int32_t>::max() );
    constexpr uint64_t feeDisplayLimit = static_cast<uint64_t>( std::numeric_limits<int64_t>::max() / 2 );

    inline std::array<uint64_t, 3> train( State & state, const uint32_t count, const Settings & config )
    {
        const uint64_t losses = state.lossRemainder + uint64_t{ count } * ( successScale - std::min( config.successRate, successScale ) );
        state.lossRemainder = static_cast<uint32_t>( losses % successScale );
        return advance( state, count - static_cast<uint32_t>( losses / successScale ), config.quotas );
    }

    // Combining adjacent blocks avoids work proportional to the order size.
    // Each block carries multiplier^length, the final surcharge, and the sum
    // of the surcharges before each step, normalized to an initial increment of 1.
    inline uint64_t trainingCost( const Settings & config, const uint32_t count )
    {
        if ( count == 0 ) return 0;
        const long double initial = static_cast<long double>( count ) * config.goldPerPeasant;
        if ( config.mode != 2 || config.goldIncrement <= 0 ) return uint64_t{ count } * config.goldPerPeasant;

        // When the first Peasant is free, the ratio is an absolute increment in gold.
        const long double increment = config.goldIncrement * static_cast<long double>( std::max( 1U, config.goldPerPeasant ) );
        const long double normalizedLimit = ( static_cast<long double>( feeDisplayLimit ) / std::max( increment, 1e-12L ) ) + 1;
        struct Block
        {
            uint64_t length;
            long double multiplier;
            long double surcharge;
            long double sum;
        };
        const auto combine = [normalizedLimit]( const Block & first, const Block & second ) {
            const auto cap = [normalizedLimit]( const long double value ) { return std::min( value, normalizedLimit ); };
            return Block{ first.length + second.length, cap( first.multiplier * second.multiplier ),
                          cap( first.surcharge + first.multiplier * second.surcharge ),
                          cap( first.sum + second.length * first.surcharge + first.multiplier * second.sum ) };
        };
        const uint32_t step = std::max( 1U, config.goldIncrementStep );
        const uint32_t lastTier = ( count - 1 ) / step;
        Block result{ 0, 1, 0, 0 };
        Block power{ 1, config.goldIncrementMultiplier, 1, 0 };
        for ( uint32_t tiers = lastTier; tiers; tiers >>= 1 ) {
            if ( tiers & 1U ) result = combine( result, power );
            if ( tiers > 1 ) power = combine( power, power );
        }
        const long double extra = config.uniformBatchPrice ? count * result.surcharge
                                                           : step * result.sum + ( count - uint64_t{ lastTier } * step ) * result.surcharge;
        const long double total = initial + extra * increment;
        if ( total >= static_cast<long double>( feeDisplayLimit ) ) return feeDisplayLimit;
        const long double nearest = std::round( total );
        // Decimal INI values are read as doubles, so an integral price can arrive
        // a few representable values above the integer (e.g. 0.1 * 50).
        const long double tolerance = std::min( 1e-5L, 8 * std::numeric_limits<double>::epsilon() * total );
        return static_cast<uint64_t>( std::abs( total - nearest ) <= tolerance ? nearest : std::ceil( total ) );
    }

    inline uint64_t batchCost( const Settings & config, const uint32_t count, const uint64_t garrison, const uint32_t peasantCost )
    {
        const uint64_t fees = trainingCost( config, count );
        const uint64_t hutPurchase = ( count - std::min<uint64_t>( count, garrison ) ) * peasantCost;
        return fees + std::min( hutPurchase, std::numeric_limits<uint64_t>::max() - fees );
    }

    inline void scheduleOrder( TownDay & town, int64_t & gold, const Settings & config, const uint32_t peasantCost )
    {
        town.goldBefore = gold;
        if ( gold < 0 ) return;
        uint32_t low = 0;
        uint32_t high = static_cast<uint32_t>( std::min<uint64_t>( town.order, town.garrison + town.hut ) );
        const auto canTrain = [&]( const uint32_t count ) {
            if ( batchCost( config, count, town.garrison, peasantCost ) > static_cast<uint64_t>( gold )
                 || town.state.total > std::numeric_limits<uint64_t>::max() - count ) return false;
            auto state = town.state;
            const auto recruits = train( state, count, config );
            for ( size_t i = 0; i < 3; ++i ) {
                if ( town.pools[i] + recruits[i] > std::numeric_limits<uint32_t>::max() ) return false;
            }
            return true;
        };
        while ( low < high ) {
            const uint32_t candidate = low + ( high - low + 1 ) / 2;
            if ( canTrain( candidate ) ) low = candidate;
            else high = candidate - 1;
        }
        const auto recruits = train( town.state, low, config );
        town.failed = low - static_cast<uint32_t>( std::accumulate( recruits.begin(), recruits.end(), uint64_t{ 0 } ) );
        town.fromGarrison = static_cast<uint32_t>( std::min<uint64_t>( low, town.garrison ) );
        town.fromHut = low - town.fromGarrison;
        town.trainingFees = trainingCost( config, low );
        gold -= static_cast<int64_t>( town.trainingFees + uint64_t{ town.fromHut } * peasantCost );
        town.garrison -= town.fromGarrison;
        town.hut -= town.fromHut;
        for ( size_t i = 0; i < 3; ++i ) town.pools[i] += recruits[i];
    }

    // Pure scheduler shared by real turns and forecasts. Input order is the interface order.
    inline uint32_t dailyLimitForDay( const Settings & config, const uint32_t day )
    {
        const uint32_t calendarDay = ( day - 1 ) % 7 + 1;
        return calendarDay % 2 == 0 ? config.evenDayLimit : config.oddDayLimit;
    }

    inline uint64_t scheduleWork( const Settings & config, const TownDay & town, const uint32_t day )
    {
        if ( !config.enabled ) return 0;
        // Adjustable orders are evaluated in bulk, whereas fixed quotas run one attempt at a time.
        return config.mode == 2 ? std::min( town.order, 32U ) : dailyLimitForDay( config, day );
    }

    inline void scheduleDay( std::vector<TownDay> & towns, int64_t & gold, const Settings & config, const uint32_t peasantCost, const uint32_t day = 0 )
    {
        for ( auto & town : towns ) {
            town.fromGarrison = town.fromHut = town.failed = 0;
            town.trainingFees = 0;
            town.goldBefore = gold;
        }
        if ( !config.enabled ) return;
        if ( config.mode == 2 ) {
            for ( auto & town : towns ) scheduleOrder( town, gold, config, peasantCost );
            return;
        }
        const uint32_t limit = day == 0 ? config.dailyLimit : dailyLimitForDay( config, day );
        for ( uint32_t pass = 0; pass < limit; ++pass ) {
            bool progress = false;
            for ( auto & town : towns ) {
                if ( !town.garrison && !town.hut ) continue;
                const uint64_t cost = uint64_t{ config.goldPerPeasant } + ( town.garrison ? 0 : peasantCost );
                if ( gold < 0 || cost > static_cast<uint64_t>( gold ) || town.state.total == std::numeric_limits<uint64_t>::max() ) continue;
                auto next = town.state;
                next.lossRemainder += successScale - std::min( config.successRate, successScale );
                const bool failed = next.lossRemainder >= successScale;
                std::array<uint64_t, 3> recruits{};
                if ( failed ) next.lossRemainder -= successScale;
                else recruits = advance( next, 1, config.quotas );
                bool fits = true;
                for ( size_t i = 0; i < 3; ++i ) fits = fits && town.pools[i] + recruits[i] <= std::numeric_limits<uint32_t>::max();
                if ( !fits ) continue;
                gold -= cost;
                town.trainingFees += config.goldPerPeasant;
                if ( town.garrison ) { --town.garrison; ++town.fromGarrison; }
                else { --town.hut; ++town.fromHut; }
                town.state = next;
                town.failed += failed;
                for ( size_t i = 0; i < 3; ++i ) town.pools[i] += recruits[i];
                progress = true;
            }
            if ( !progress ) break;
        }
    }
}
