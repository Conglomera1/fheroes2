#include "../src/fheroes2/ai/taxpayer_policy.h"
#include <cassert>
#include <iostream>

int main()
{
    // Farm investment uses configurable daily income and the actual building cost.
    static_assert( AI::taxpayerFarmPaysBackQuickly( 180, 1000 ) ); // 90 Peasants at 2 gold: six credits.
    static_assert( !AI::taxpayerFarmPaysBackQuickly( 142, 1000 ) );
    static_assert( AI::taxpayerFarmPaysBackQuickly( 143, 1000 ) ); // Seven-credit boundary.
    static_assert( !AI::taxpayerFarmPaysBackQuickly( 90, 1000 ) ); // Same population at 1 gold.
    static_assert( !AI::taxpayerFarmPaysBackQuickly( 0, 1000 ) );
    static_assert( !AI::taxpayerFarmPaysBackQuickly( 180, 0 ) );
    static_assert( !AI::taxpayerFarmPaysBackQuickly( 180, 2000 ) );
    using AI::TaxpayerDefenseDecision;
    // A doomed defense must not consume an economic population for an insufficient improvement.
    assert( AI::taxpayerDefenseDecision( 0.20, 0.30, false, true ) == TaxpayerDefenseDecision::Preserve );
    assert( AI::taxpayerDefenseDecision( 0.90, 1.09, false, true ) == TaxpayerDefenseDecision::Preserve );
    // A credible change in the outcome permits a militarily useful replacement.
    assert( AI::taxpayerDefenseDecision( 0.90, 1.10, false, true ) == TaxpayerDefenseDecision::PivotalReplacement );
    assert( AI::taxpayerDefenseDecision( 1.09, 1.10, false, true ) == TaxpayerDefenseDecision::Preserve );
    assert( AI::taxpayerDefenseDecision( 1.20, 0.90, false, true ) == TaxpayerDefenseDecision::Preserve );
    // An already adequate defense does not justify replacing taxpayers.
    assert( AI::taxpayerDefenseDecision( 1.20, 1.30, false, true ) == TaxpayerDefenseDecision::Preserve );
    assert( AI::taxpayerDefenseDecision( 0.20, 0.30, true, true ) == TaxpayerDefenseDecision::LastStand );
    // No known threat: preserve regardless of hypothetical strength or last-town status.
    assert( AI::taxpayerDefenseDecision( 0.90, 1.50, false, false ) == TaxpayerDefenseDecision::Preserve );
    assert( AI::taxpayerDefenseDecision( 0.20, 0.30, true, false ) == TaxpayerDefenseDecision::Preserve );
    // Reproduce observed pivotal replacement (five Peasants for twenty Rangers).
    assert( AI::taxpayerDefenseDecision( 0.994648, 1.19682, false, true ) == TaxpayerDefenseDecision::PivotalReplacement );
    // A last town alone is insufficient while another mobile hero exists elsewhere.
    assert( !AI::taxpayerLastStand( 1, true, true ) );
    assert( !AI::taxpayerLastStand( 2, false, true ) );
    assert( !AI::taxpayerLastStand( 1, false, false ) );
    assert( AI::taxpayerLastStand( 1, false, true ) );
    std::cout << "Taxpayer defense policy checks passed\n";
}
