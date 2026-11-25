#pragma once

#include "economy_project.h"

class Kingdom;

namespace AI::EconomyMilitary
{
    namespace P = EconomyProject;

    enum class Slot
    {
        TownArmy,
        Visitor
    };

    struct Value
    {
        bool complete = false;
        double force = 0;
        std::string reason;
    };

    Value localArmyValue( const Kingdom & kingdom, const P::Town & town, Slot slot, bool includeVisitor = true );
}
