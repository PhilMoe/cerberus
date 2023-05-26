///----------------------------------------------------------------------------------------------------------------------
// Ted, a simple text editor/IDE.
//
// Copyright 2012, Blitz Research Ltd.
//
// See LICENSE.TXT for licensing terms.
//
//  NOTE: This version is not backwards compatible with versions earlier than Qt 5.9.0
//----------------------------------------------------------------------------------------------------------------------
// CONTRIBUTORS: See contributors.txt
// NOTE: To avoid lots of preprocessor directives in the main code files. Important changes to Qt versions should use
//       macros for code replacement.

#pragma once
#include <QtGlobal>

// NOTE: Documentation for Qt6 states that QTextStrean uses UTF-8 as standard.
// As this could be used in a number of places, then define it as a macro.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    #define STREAMS_UTF8 stream.setCodec("UTF-8");
#else
    #define STREAMS_UTF8
#endif

// NOTE: There has been some changes to QFontMetric from Qt5.11, so define a macro for replacements
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
    #define FONTMETRICS_WIDTH(x) width(x)
#else
    #define FONTMETRICS_WIDTH(x) horizontalAdvance(x)
#endif

// NOTE: tabStopDistance was introduced in Qt 5.10. Prior to that it was tabStopWidth
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    #define SET_TABSTOP_WIDTH(x) setTabStopWidth(x)
#else
    #define SET_TABSTOP_WIDTH(x) setTabStopDistance(x)
#endif
