#include "../src/fheroes2/ai/taxpayer_policy.h"
#include <cassert>
#include <iostream>

int main()
{
    static_assert( AI::knightFarmBeforeCaptain( false, false, false ) );
    static_assert( !AI::knightFarmBeforeCaptain( true, false, false ) );
    static_assert( !AI::knightFarmBeforeCaptain( false, true, false ) );
    static_assert( !AI::knightFarmBeforeCaptain( false, false, true ) );
    // Fountainhead/Avalon: all-faction castle income, not the total 1000.
    static_assert( AI::taxpayerFarmPaysBackQuickly( 1000 - 250, 5000 ) );
    static_assert( AI::taxpayerDefensiveFarmAffordable( 16516, 5000 ) );
    static_assert( !AI::taxpayerDefensiveFarmAffordable( 6999, 5000 ) );
    static_assert( !AI::taxpayerFarmPaysBackQuickly( 250, 5000 ) );
    // Fountainhead J208: 328 residents, 22328 gold, actual Farm cost 1000.
    static_assert( AI::taxpayerFarmPaysBackQuickly( 328 * 2, 1000 ) );
    static_assert( AI::taxpayerDefensiveFarmAffordable( 22328, 1000 ) );
    static_assert( !AI::taxpayerDefensiveFarmAffordable( 2999, 1000 ) );
    static_assert( AI::taxpayerDefensiveFarmAffordable( 3000, 1000 ) );
    // Dragontooth J94: protect TG population during routine reinforcement,
    // release it for an explicit defense task; disabled TG has no reservation.
    static_assert( AI::taxpayerKeepTrainingPopulation( true, true, false ) );
    static_assert( !AI::taxpayerKeepTrainingPopulation( true, true, true ) );
    static_assert( !AI::taxpayerKeepTrainingPopulation( true, false, false ) );
    static_assert( !AI::taxpayerKeepTrainingPopulation( false, true, false ) );
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
