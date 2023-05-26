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

#pragma once

#include <QString>
#include <QTabWidget>
#include <QWidget>

const QString textFileTypes = ";txt;cerberusdoc;monkeydoc;";
const QString codeFileTypes = ";cxs;monkey;bmx;h;cpp;java;js;as;cs;py;mx2;monkey2;";

// Inline functions for testing the character range of certain types of identifiers.
inline bool isDigit(QChar ch)
{
    return (ch >= '0' && ch <= '9');
}

inline bool isBinDigit(QChar ch)
{
    return ch == '0' || ch == '1';
}

inline bool isOctDigit(QChar ch)
{
    return (ch >= '0' && ch <= '7');
}

inline bool isHexDigit(QChar ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
}

inline bool isAlpha(QChar ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

inline bool isIdent(QChar ch)
{
    return isAlpha(ch) || isDigit(ch);
}

QString stripDir(const QString &path);
QString extractDir(const QString &path);
QString extractExt(const QString &path);
QString fixPath(QString path);
bool removeDir(const QString &path);
void replaceTabWidgetWidget(QTabWidget *tabWidget, int index, QWidget *widget);
bool isUrl(const QString &path);
bool isImageFile(const QString &path);
bool isAudioFile(const QString &path);
bool isDocFile(const QString &path);
