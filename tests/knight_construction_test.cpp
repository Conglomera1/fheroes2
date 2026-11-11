#include "../src/fheroes2/ai/knight_construction.h"
#include <cassert>
#include <iostream>
using namespace AI::KnightConstruction;
int main()
{
    size_t checks = 0;
    // Dwelling: 1500 gold + 5 wood; 900/4 now, 800/2 tomorrow.
    Ledger l;
    assert( l.book( 1, Promise{ 10, 1, 0, { 1500, 5 } }, { 1700, 6 } ) );
    assert( !l.allows( 1, 20, { 950, 6 } ) ); ++checks;
    // Same present moat purchase, larger income: both builds are possible.
    assert( l.allows( 1, 20, { 1750, 6 } ) ); ++checks;
    // Wood still insufficient tomorrow: no promise and no idle gold lock.
    Ledger blocked;
    assert( !blocked.book( 1, Promise{ 10, 1, 0, { 1500, 5 } }, { 1700, 3 } ) );
    assert( blocked.allows( 1, 20, { 950, 3 } ) ); ++checks;
    // A Farm paid now must not consume tomorrow's promise in another city.
    assert( !l.allows( 2, 30, { 1000, 6 } ) ); ++checks;
    // 2500 gold tomorrow cannot promise two 1500-gold dwellings.
    Ledger shared;
    assert( shared.book( 1, Promise{ 10, 1, 0, { 1500, 5 } }, { 2500, 10 } ) );
    assert( !shared.book( 2, Promise{ 10, 1, 1, { 1500, 5 } }, { 2500, 10 } ) ); ++checks;
    // Constructing one's promised building pays it exactly once, not twice.
    assert( shared.allows( 1, 10, { 1000, 5 } ) );
    shared.release( 1, 10 ); assert( shared.promises.empty() ); ++checks;
    // An urgent expenditure invalidates weaker funding first; no invented money.
    Ledger urgent;
    assert( urgent.book( 1, Promise{ 10, 0, 0, { 1000 } }, { 3000 } ) );
    assert( urgent.book( 2, Promise{ 20, 8, 1, { 1500 } }, { 3000 } ) );
    const auto dropped = urgent.reconcile( { 1200 } );
    assert( dropped == std::vector<int>{ 2 } && urgent.reserved()[0] == 1000 ); ++checks;
    // Crystal is just another shared resource: no special Cathedral rule.
    Ledger crystal;
    assert( crystal.book( 1, Promise{ 40, 8, 0, { 5000, 20, 0, 0, 0, 20 } }, { 7000, 25, 5, 5, 5, 20, 5 } ) );
    assert( !crystal.allows( 2, 50, { 6000, 20, 0, 1, 1, 16, 1 } ) );
    assert( crystal.allows( 2, 50, { 6000, 20, 0, 1, 1, 20, 1 } ) ); ++checks;
    std::cout << "PASS " << checks << " shared construction budget scenarios\n";
}
