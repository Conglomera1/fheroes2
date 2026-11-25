#pragma once

namespace AI
{
    // Hypothetical combat must not update the live kingdom budget or its missions.
    inline thread_local unsigned simulationDepth = 0;

    inline bool isSimulating()
    {
        return simulationDepth != 0;
    }

    class SimulationScope
    {
    public:
        SimulationScope()
        {
            ++simulationDepth;
        }

        ~SimulationScope()
        {
            --simulationDepth;
        }

        SimulationScope( const SimulationScope & ) = delete;
        SimulationScope & operator=( const SimulationScope & ) = delete;
    };
}
