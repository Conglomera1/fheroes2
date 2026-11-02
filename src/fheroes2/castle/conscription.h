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
#include <cstdint>
#include <cstddef>
#include <numeric>
#include <vector>
#include <limits>

namespace Conscription
{
    using Quotas = std::array<uint32_t, 3>;
    struct State
    {
        uint64_t total = 0;
        Quotas quotas{ 8, 5, 4 };
        Quotas used{};
        uint32_t next = 0;
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
    }

    template<class Stream> void readState( Stream & stream, State & state )
    {
        uint32_t high = 0, low = 0;
        stream >> high >> low;
        state.total = ( uint64_t{ high } << 32 ) | low;
        for ( auto & value : state.quotas ) stream >> value;
        for ( auto & value : state.used ) stream >> value;
        stream >> state.next;
        bool valid = state.next < 3;
        uint64_t remaining = 0;
        for ( std::size_t i = 0; i < 3; ++i ) {
            valid = valid && state.quotas[i] <= 10000 && state.used[i] <= state.quotas[i];
            if ( state.used[i] <= state.quotas[i] ) remaining += state.quotas[i] - state.used[i];
        }
        if ( !valid || !remaining ) stream.setFail();
    }

    struct Settings
    {
        uint32_t goldPerPeasant = 100;
        Quotas quotas{ 8, 5, 4 };
        uint32_t weeklyLimit = 0; // Legacy save compatibility; no longer used.
        uint32_t dailyLimit = 3;
        bool enabled = true;
        int32_t trainingGold = 2000, trainingWood = 10, trainingOre = 10;
        int32_t farmGold = 1500, farmWood = 0, farmOre = 0;
    };
    const Settings & settings();

    struct TownDay
    {
        State state;
        uint64_t garrison = 0;
        uint64_t hut = 0;
        std::array<uint64_t, 3> pools{};
        uint32_t fromGarrison = 0, fromHut = 0;
    };

    // Pure scheduler shared by real turns and forecasts. Input order is the interface order.
    inline void scheduleDay( std::vector<TownDay> & towns, int64_t & gold, const Settings & config, const uint32_t peasantCost )
    {
        if ( !config.enabled ) return;
        for ( uint32_t pass = 0; pass < config.dailyLimit; ++pass ) {
            bool progress = false;
            for ( auto & town : towns ) {
                if ( !town.garrison && !town.hut ) continue;
                const uint64_t cost = uint64_t{ config.goldPerPeasant } + ( town.garrison ? 0 : peasantCost );
                if ( gold < 0 || cost > static_cast<uint64_t>( gold ) || town.state.total == std::numeric_limits<uint64_t>::max() ) continue;
                auto next = town.state;
                const auto recruits = advance( next, 1, config.quotas );
                bool fits = true;
                for ( size_t i = 0; i < 3; ++i ) fits = fits && town.pools[i] + recruits[i] <= std::numeric_limits<uint32_t>::max();
                if ( !fits ) continue;
                gold -= cost;
                if ( town.garrison ) { --town.garrison; ++town.fromGarrison; }
                else { --town.hut; ++town.fromHut; }
                town.state = next;
                for ( size_t i = 0; i < 3; ++i ) town.pools[i] += recruits[i];
                progress = true;
            }
            if ( !progress ) break;
        }
    }
}
