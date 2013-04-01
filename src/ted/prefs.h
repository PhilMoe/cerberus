/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef PREFS_H
#define PREFS_H

#include "std.h"

class Prefs : public QObject{
    Q_OBJECT

public:

    void setValue( const QString &name,const QVariant &value );

    bool getBool( const QString &name ){ return _settings.value( name ).toBool(); }
    int getInt( const QString &name ){ return _settings.value( name ).toInt(); }
    QString getString( const QString &name){ return _settings.value( name ).toString(); }
    QFont getFont( const QString &name ){ return _settings.value( name ).value<QFont>(); }
    QColor getColor( const QString &name ){ return _settings.value( name ).value<QColor>(); }

    static Prefs *prefs();

signals:

    void prefsChanged( const QString &name );

private:
    QSettings _settings;

    Prefs();
};

#endif // PREFS_H
