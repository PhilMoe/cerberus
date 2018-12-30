/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.

Change Log
--------------------------------------------------------------------------------
2018-07-23 - dawlane
		Update Ted to use QtWebEngine and QtWebEnginePage and minor fixes.
		Should now build with later versions of Qt.
*/

#ifndef STD_H
#define STD_H

#include <QtGui/QtGui>
// DAWLANE Qt 5.6+ supported
#if QT_VERSION>0x050501
#include <QtWidgets/QtWidgets>
#include <QWebEngineView>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#else
#if QT_VERSION>=0x050001 || QT_VERSION<=0x050501
#include <QtWidgets/QtWidgets>
#include <QtWebKitWidgets/QtWebKitWidgets>
#else
#include <QtWebKit/QWebView>
#endif
#endif

static QString textFileTypes=";txt;cerberusdoc;monkeydoc;";
static QString codeFileTypes=";cxs;monkey;bmx;h;cpp;java;js;as;cs;py;mx2;monkey2;";

inline bool isDigit( QChar ch ){
    return (ch>='0' && ch<='9');
}

inline bool isBinDigit( QChar ch ){
    return ch=='0' || ch=='1';
}

inline bool isOctDigit( QChar ch ){
    return (ch>='0' && ch<='7');
}

inline bool isHexDigit( QChar ch ){
    return (ch>='0' && ch<='9') || (ch>='A' && ch<='F') || (ch>='a' && ch<='f');
}

inline bool isAlpha( QChar ch ){
    return (ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || ch=='_';
}

inline bool isIdent( QChar ch ){
    return isAlpha(ch) || isDigit(ch);
}

QString fixPath( QString path );

QString stripDir( const QString &path );

QString extractDir( const QString &path );

QString extractExt( const QString &path );

bool removeDir( const QString &path );

void replaceTabWidgetWidget( QTabWidget *tabWidget,int index,QWidget *widget );

bool isUrl( const QString &path );
bool isImageFile(const QString &path);
bool isAudioFile(const QString &path);
bool isDocFile(const QString &path);

#endif // STD_H
