//----------------------------------------------------------------------------------------------------------------------
// Ted, a simple text editor/IDE.
//
// Copyright 2012, Blitz Research Ltd.
//
// See LICENSE.TXT for licensing terms.
//
//  NOTE: This version is not backwards compatible with versions earlier than Qt 5.9.0
//----------------------------------------------------------------------------------------------------------------------
// CONTRIBUTORS: See contributors.txt
#include "prefs.h"

//----------------------------------------------------------------------------------------------------------------------
//  Prefs: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//  Prefs: PUBLIC MEMBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
Prefs::Prefs()
{
    _settings.beginGroup("userPrefs");
}

void Prefs::setValue(const QString &name, const QVariant &value)
{
    _settings.setValue(name, value);
    emit prefsChanged(name);
}

// Note this is a static member function to update preference value directly.
Prefs *Prefs::prefs()
{
    static Prefs *_prefs;
    if(!_prefs)
        _prefs = new Prefs;
    return _prefs;
}
