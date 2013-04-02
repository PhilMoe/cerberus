/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef PREFSDIALOG_H
#define PREFSDIALOG_H

#include "std.h"

namespace Ui {
class PrefsDialog;
}

class Prefs;

class PrefsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit PrefsDialog( QWidget *parent=0 );
    ~PrefsDialog();

    void readSettings();
    void writeSettings();

    int exec();

public slots:

    void onFontChanged( const QFont &font );
    void onFontSizeChanged( int size );
    void onTabSizeChanged( int size );
    void onSmoothFontsChanged( bool state );
    void onColorChanged();
    void onBrowseForPath();

private:
    Ui::PrefsDialog *_ui;
    Prefs *_prefs;
    bool _used;
};

#endif // PREFSDIALOG_H
