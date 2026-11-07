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
#include "../src/engine/serialize.h"
#include <cassert>
#include <iostream>
int main()
{
    using namespace Conscription;
    for ( uint32_t n=0; n<70; ++n ) {
        State before;
        advance(before,n,{8,5,4});
        // Exercise both halves of the serialized lifetime counter.
        before.total += uint64_t{1} << 40;
        before.lossRemainder = 950000;
        RWStreamBuf stream;
        writeState(stream,before);
        State after;
        readState(stream,after);
        assert(!stream.fail());
        assert(before.total == after.total && before.quotas == after.quotas && before.used == after.used && before.next == after.next);
        assert( after.lossRemainder == 950000 );
        assert(advance(before,52,{2,0,7}) == advance(after,52,{2,0,7}));
    }
    // A prototype save has no fractional-loss field. Its following fields must
    // remain at the correct stream position when the new reader loads it.
    {
        State before;
        advance( before, 44, {8,5,4} );
        RWStreamBuf legacy;
        legacy << uint32_t{0} << uint32_t{44};
        for ( const auto value : before.quotas ) legacy << value;
        for ( const auto value : before.used ) legacy << value;
        legacy << before.next << uint32_t{12345};
        State loaded;
        readState( legacy, loaded, false );
        uint32_t following = 0;
        legacy >> following;
        assert( !legacy.fail() && following == 12345 && loaded.total == 44 && loaded.lossRemainder == 0 );
        assert( advance( loaded, 1, {8,5,4} )[1] == 1 );
    }
    for (int broken=0;broken<4;++broken) {
        State bad;
        if(broken==0) bad.next=3;
        if(broken==1) bad.used[0]=9;
        if(broken==2) bad.quotas={};
        if(broken==3) bad.lossRemainder=successScale;
        RWStreamBuf stream;
        writeState(stream,bad);
        State loaded;
        readState(stream,loaded);
        assert(stream.fail());
    }
    std::cout<<"Conscription engine-stream round trips passed\n";
}
