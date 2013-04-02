/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "prefs.h"

Prefs::Prefs(){
    _settings.beginGroup( "userPrefs" );
}

void Prefs::setValue( const QString &name,const QVariant &value ){
    _settings.setValue( name,value );
    emit prefsChanged( name );
}

Prefs *Prefs::prefs(){
    static Prefs *_prefs;
    if( !_prefs ) _prefs=new Prefs;
    return _prefs;
}
