// Local rolling Farm/TG decision. Future purchases are comparisons, never orders.
#pragma once
#include <cstdint>
#include <string>
#include <array>
#include <vector>
class Kingdom;
struct Funds;
class Castle;
namespace AI::KnightEconomy
{
    struct Decision
    {
        uint32_t building = 0;
        double keepForce = 0, switchForce = 0;
        int64_t keepGold = 0, switchGold = 0;
        uint32_t projections = 0;
        bool keepValid = false, switchValid = false, cycleValid = false, temporaryTraining = false;
        double cycleForce = 0;
        int64_t cycleGold = 0;
        std::string reason;
        uint32_t towns = 0;
        uint32_t planUntil = 0;
        uint32_t comparisonOrder = 0;
        uint64_t farmTax = 0;
        std::array<int64_t, 7> dailyOverhead{};
        std::array<int64_t, 7> entryFunding{};
    };
    struct WeekFunding { bool complete = false; std::array<int64_t, 7> fees{}, recruits{}; uint64_t trained = 0; };
    // The end-of-turn order is the one the actual scheduler will execute.
    void planDailyOrders( Kingdom & kingdom );
    uint32_t supplyOrder( const Castle & castle );
    WeekFunding weekFunding( const Castle & castle );
    Decision choose( const Castle & castle, bool safe );
    bool develop( Castle & castle, bool safe );
    uint32_t commitmentUntil( const Castle & castle );
    // Evaluate before optional reinforcement spends the common treasury.
    void prepare( Kingdom & kingdom, const std::vector<int> & safeTowns );
    void recordDebit( const Kingdom & kingdom, const Funds & before, const Funds & after, const char * cause );
    std::array<int64_t, 7> overhead( const Kingdom & kingdom );
}
