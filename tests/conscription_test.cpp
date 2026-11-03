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

#include "../src/fheroes2/castle/conscription.h"
#include <cassert>
#include <iostream>
#include <limits>
int main()
{
    using namespace Conscription;
    const Quotas quotas{ 8, 5, 4 };
    State state;
    const int expected[] = { 0,1,2,0,1,2,0,1,2,0,1,2,0,1,0,0,0 };
    for ( const int type : expected ) {
        const auto got = advance( state, 1, quotas );
        assert( got[type] == 1 && got[0]+got[1]+got[2] == 1 );
    }
    state = {};
    assert( (advance( state, 52, quotas ) == std::array<uint64_t,3>{25,15,12}) );
    assert( state.total == 52 );
    assert( advance( state, 1, quotas )[1] == 1 );
    state = {};
    advance( state, 44, quotas );
    assert( advance( state, 1, quotas )[1] == 1 );
    // Partition invariance, including incomplete cycles and zero-category ratios.
    for ( const Quotas ratio : { quotas, Quotas{0,2,1}, Quotas{1,0,0}, Quotas{1,2,9} } ) {
        for ( uint32_t start = 0; start < 40; ++start ) {
            State initial;
            advance( initial, start, ratio );
            auto whole = initial, split = initial;
            const auto total = advance( whole, 75, ratio );
            std::array<uint64_t,3> sums{};
            for ( int n = 0; n < 75; ++n ) {
                const auto part = advance( split, 1, ratio );
                for ( size_t i=0;i<3;++i ) sums[i] += part[i];
            }
            assert( sums == total && split.total == whole.total && split.used == whole.used && split.next == whole.next );
        }
    }
    // A preview operates on a copy. A new INI ratio waits until the old cycle is finished.
    state = {};
    advance( state, 14, quotas );
    auto preview = state;
    assert( (advance( preview, 5, Quotas{0,0,2} ) == std::array<uint64_t,3>{3,0,2}) );
    assert( state.total == 14 && state.used == Quotas({5,5,4}) );
    assert( (advance( state, 5, Quotas{0,0,2} ) == std::array<uint64_t,3>{3,0,2}) );
    state = {};
    const auto big = advance( state, std::numeric_limits<uint32_t>::max(), quotas );
    assert( big[0]+big[1]+big[2] == std::numeric_limits<uint32_t>::max() );
    state = {};
    assert( (advance( state,17,Quotas{} ) == std::array<uint64_t,3>{8,5,4}) );
    Settings config;
    assert( dailyLimitForDay( config, 1 ) == 2 );
    assert( dailyLimitForDay( config, 2 ) == 3 );
    assert( 4 * dailyLimitForDay( config, 1 ) + 3 * dailyLimitForDay( config, 2 ) == 17 );
    std::vector<TownDay> alternating( 1 );
    alternating[0].garrison = 20;
    int64_t alternatingGold = 5000;
    scheduleDay( alternating, alternatingGold, config, 20, 7 );
    assert( alternating[0].state.total == 2 );
    scheduleDay( alternating, alternatingGold, config, 20, 8 );
    assert( alternating[0].state.total == 5 );
    std::vector<TownDay> towns( 3 );
    for ( auto & town : towns ) town.garrison = 20;
    int64_t gold = 400;
    scheduleDay( towns, gold, config, 20 );
    assert( gold == 0 && towns[0].fromGarrison == 2 && towns[1].fromGarrison == 1 && towns[2].fromGarrison == 1 );
    towns = std::vector<TownDay>( 1 );
    towns[0].garrison = 1;
    towns[0].hut = 10;
    gold = 340;
    scheduleDay( towns, gold, config, 20 );
    assert( gold == 0 && towns[0].garrison == 0 && towns[0].hut == 8 );
    assert( towns[0].fromGarrison == 1 && towns[0].fromHut == 2 && towns[0].state.total == 3 );
    assert( towns[0].pools == ( std::array<uint64_t,3>{ 1,1,1 } ) );
    // An unaffordable hut must not stop a later city's cheaper garrison formation.
    towns = std::vector<TownDay>( 2 );
    towns[0].hut = 5;
    towns[1].garrison = 5;
    gold = 110;
    scheduleDay( towns, gold, config, 20 );
    assert( gold == 10 && towns[0].state.total == 0 && towns[1].state.total == 1 );
    config.dailyLimit = 0;
    gold = 1000;
    const auto total = towns[1].state.total;
    scheduleDay( towns, gold, config, 20 );
    assert( gold == 1000 && towns[1].state.total == total );
    config.dailyLimit = 3;
    config.goldPerPeasant = 0;
    towns = std::vector<TownDay>( 1 );
    towns[0].garrison = 8;
    gold = 0;
    scheduleDay( towns, gold, config, 20 );
    assert( towns[0].state.total == 3 && gold == 0 );
    towns = std::vector<TownDay>( 1 );
    towns[0].garrison = 8;
    towns[0].pools[0] = std::numeric_limits<uint32_t>::max();
    scheduleDay( towns, gold, config, 20 );
    assert( towns[0].state.total == 0 && towns[0].garrison == 8 );
    std::cout << "Conscription cycle and daily scheduler checks passed\n";
}
