// Read-only diagnostics. None of these observations feed AI decisions or fog.
#pragma once
#include <string>
class Army;
class Kingdom;
namespace KnightObserver
{
    std::string armyJson( const Army & army );
    void kingdomState( const Kingdom & kingdom, const char * phase );
}
