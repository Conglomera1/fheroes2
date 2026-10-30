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

    // Keep existing log IDs for preservation and the two military exceptions.
    enum class TaxpayerDefenseDecision { Preserve = 1, PivotalReplacement = 2, LastStand = 3 };

    // Ratios use the worst known, reachable defense threat. Preserve taxpayers by default;
    // recruitment price alone never justifies discarding a productive population.
    constexpr TaxpayerDefenseDecision taxpayerDefenseDecision( const double currentRatio, const double replacementRatio, const bool lastStand,
                                                              const bool hasKnownDefenseThreat )
    {
        constexpr double viableDefenseRatio = 1.10;
        if ( !hasKnownDefenseThreat || currentRatio >= viableDefenseRatio ) {
            return TaxpayerDefenseDecision::Preserve;
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
