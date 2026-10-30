// Pure decisions for Taxpayer investment and population protection.
#pragma once

#include <cstdint>

namespace AI
{
    // Promote an existing population's Farm only if it recovers its cost within seven daily credits.
    constexpr bool taxpayerFarmPaysBackQuickly( const uint64_t dailyIncome, const uint32_t farmCost )
    {
        return farmCost > 0 && dailyIncome >= ( static_cast<uint64_t>( farmCost ) + 6 ) / 7;
    }

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
