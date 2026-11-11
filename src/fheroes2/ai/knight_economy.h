// Local rolling Farm/TG decision. Future purchases are comparisons, never orders.
#pragma once
#include <cstdint>
#include <string>
class Castle;
namespace AI::KnightEconomy
{
    struct Decision
    {
        uint32_t building = 0;
        double keepForce = 0, switchForce = 0;
        int64_t keepGold = 0, switchGold = 0;
        uint32_t projections = 0;
        bool cycleValid = false, temporaryTraining = false;
        double cycleForce = 0;
        int64_t cycleGold = 0;
        std::string reason;
    };
    Decision choose( const Castle & castle, bool safe );
    bool develop( Castle & castle, bool safe );
}
