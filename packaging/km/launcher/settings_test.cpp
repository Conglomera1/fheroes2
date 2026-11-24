// SPDX-License-Identifier: GPL-2.0-or-later
#include "settings.h"

#include <cassert>
#include <iostream>

int main()
{
    using namespace km;
    auto values = defaults();
    assert( values[modeIndex] == 2 && values[baseIndex] == 50 );
    auto changed = values;
    assert( selectMode( changed, 1 ) );
    assert( changed[modeIndex] == 1 && changed[baseIndex] == 100 );
    assert( selectMode( changed, 2 ) );
    assert( changed[modeIndex] == 2 && changed[baseIndex] == 50 );
    changed[baseIndex] = 75;
    assert( !selectMode( changed, 2 ) && changed[baseIndex] == 75 );
    assert( selectMode( changed, 0 ) && changed[baseIndex] == 75 );
    assert( load( "conscription.mode = 1\n", changed, true ) );
    assert( changed[modeIndex] == 1 && changed[baseIndex] == 100 );
    assert( load( "conscription.mode = 1\nCONSCRIPTION.GOLD_PER_PEASANT = 137\n", changed, false ) );
    assert( changed[modeIndex] == 1 && changed[baseIndex] == 137 );
    assert( quote( values, 3 ) == 300 );
    values[index( "uniform" )] = 1;
    assert( quote( values, 3 ) == 450 );
    values[index( "increment" )] = 0.75;
    values[index( "success" )] = 87.5;
    values[index( "artwork" )] = 0;
    const std::string contents = updateIni( "# personal comment\r\nforeign.option = untouched\r\nCONSCRIPTION.MODE = 1\r\nconscription.mode = 0\r\n", values );
    assert( contents.starts_with( "# personal comment\r\nforeign.option = untouched\r\nconscription.mode = 2\r\n" ) );
    assert( contents.find( "conscription.mode = 0" ) == contents.npos );
    auto other = defaults();
    assert( load( contents, other, true ) );
    assert( values == other );
    assert( !load( "training_grounds.show_artwork = 1\nconscription.gold_increment_multiplier = 0", other, true ) );
    assert( values == other );
    assert( !load( "foreign = ok", other, true ) );
    assert( load( "conscription.enabled = 0\n", other, false ) );
    assert( other[index( "mode" )] == 0 );
    assert( load( "conscription.enabled = 1\n", other, false ) );
    assert( other[modeIndex] == 2 && other[baseIndex] == 50 );
    assert( parse( "0,75", settings[index( "increment" )] ) == 0.75 );
    assert( !parse( "1.5", settings[index( "step" )] ) );
    std::cout << "Launcher settings, profiles and pricing passed\n";
}
