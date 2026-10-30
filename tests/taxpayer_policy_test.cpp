#include "../src/fheroes2/ai/taxpayer_policy.h"
#include <cassert>
#include <iostream>

int main()
{
    using AI::TaxpayerDefenseDecision;
    // A doomed defense must not consume an economic population for an insufficient improvement.
    assert( AI::taxpayerDefenseDecision( 0.20, 0.30, false ) == TaxpayerDefenseDecision::Preserve );
    assert( AI::taxpayerDefenseDecision( 0.90, 1.09, false ) == TaxpayerDefenseDecision::Preserve );
    // A credible change in the outcome permits a militarily useful replacement.
    assert( AI::taxpayerDefenseDecision( 0.90, 1.10, false ) == TaxpayerDefenseDecision::PivotalReplacement );
    assert( AI::taxpayerDefenseDecision( 1.09, 1.10, false ) == TaxpayerDefenseDecision::Preserve );
    assert( AI::taxpayerDefenseDecision( 1.20, 0.90, false ) == TaxpayerDefenseDecision::Preserve );
    // Already adequate defenses retain the income-versus-recruitment economic comparison.
    assert( AI::taxpayerDefenseDecision( 1.20, 1.30, false ) == TaxpayerDefenseDecision::EconomicComparison );
    assert( AI::taxpayerDefenseDecision( 0.20, 0.30, true ) == TaxpayerDefenseDecision::LastStand );
    // A last town alone is insufficient while another mobile hero exists elsewhere.
    assert( !AI::taxpayerLastStand( 1, true, true ) );
    assert( !AI::taxpayerLastStand( 2, false, true ) );
    assert( !AI::taxpayerLastStand( 1, false, false ) );
    assert( AI::taxpayerLastStand( 1, false, true ) );
    std::cout << "Taxpayer defense policy checks passed\n";
}
