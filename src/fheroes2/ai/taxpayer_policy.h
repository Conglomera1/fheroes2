// Pure, testable decisions for sacrificing a taxable population.
#pragma once

namespace AI
{
    enum class TaxpayerDefenseDecision { EconomicComparison, Preserve, PivotalReplacement, LastStand };

    // Ratios use the worst known, reachable defense threat. No threat means ordinary economic evaluation.
    constexpr TaxpayerDefenseDecision taxpayerDefenseDecision( const double currentRatio, const double replacementRatio, const bool lastStand )
    {
        constexpr double viableDefenseRatio = 1.10;
        if ( currentRatio >= viableDefenseRatio ) {
            return replacementRatio >= viableDefenseRatio ? TaxpayerDefenseDecision::EconomicComparison : TaxpayerDefenseDecision::Preserve;
        }
        if ( replacementRatio >= viableDefenseRatio && replacementRatio >= currentRatio * 1.10 ) {
            return TaxpayerDefenseDecision::PivotalReplacement;
        }
        return lastStand ? TaxpayerDefenseDecision::LastStand : TaxpayerDefenseDecision::Preserve;
    }

    constexpr bool taxpayerLastStand( const unsigned townCount, const bool hasOutsideHero, const bool hasKnownDefenseThreat )
    {
        return townCount == 1 && !hasOutsideHero && hasKnownDefenseThreat;
    }
}
