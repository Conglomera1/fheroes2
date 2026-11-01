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
        RWStreamBuf stream;
        writeState(stream,before);
        State after;
        readState(stream,after);
        assert(!stream.fail());
        assert(before.total == after.total && before.quotas == after.quotas && before.used == after.used && before.next == after.next);
        assert(advance(before,52,{2,0,7}) == advance(after,52,{2,0,7}));
    }
    for (int broken=0;broken<3;++broken) {
        State bad;
        if(broken==0) bad.next=3;
        if(broken==1) bad.used[0]=9;
        if(broken==2) bad.quotas={};
        RWStreamBuf stream;
        writeState(stream,bad);
        State loaded;
        readState(stream,loaded);
        assert(stream.fail());
    }
    std::cout<<"Conscription engine-stream round trips passed\n";
}
