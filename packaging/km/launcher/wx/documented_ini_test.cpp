// SPDX-License-Identifier: GPL-2.0-or-later
#include "documented_ini.h"

#include <cassert>

int main()
{
    auto values = km::defaults();
    const auto generated = km::updateDocumentedIni( {}, values );
    std::string unixText = generated;
    unixText.erase( std::remove( unixText.begin(), unixText.end(), '\r' ), unixText.end() );
    assert( unixText == gameDefaultIni );

    km::selectMode( values, 1 );
    values[km::index( "uniform" )] = 1;
    const auto old = km::updateIni( "# Knight Mod settings\r\n", values );
    const auto restored = km::updateDocumentedIni( old, values );
    assert( restored == km::updateDocumentedIni( {}, values ) );
    assert( restored.find( "conscription.gold_per_peasant = 100" ) != restored.npos );
    assert( restored.find( "conscription.uniform_batch_price = 1" ) != restored.npos );
    assert( restored.find( "conscription.gold_increment = 1.0" ) != restored.npos );

    const auto edited = km::updateDocumentedIni( "\xef\xbb\xbf# Personal note\n  CONSCRIPTION.MODE  =  2  # mode comment\n"
                                               "external.setting = 47\nconscription.gold_increment_multiplier =  ; empty\n", values );
    assert( edited.starts_with( "\xef\xbb\xbf# Personal note\n  CONSCRIPTION.MODE  =  1  # mode comment\nexternal.setting = 47\n" ) );
    assert( edited.find( "//" ) == edited.npos );
    assert( edited.find( "conscription.gold_increment_multiplier =  1.0; empty" ) != edited.npos );
    auto loaded = km::defaults();
    assert( km::load( restored, loaded, false ) && loaded == values );
}
