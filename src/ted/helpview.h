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

#include <QWebEnginePage>
#include <QWebEngineView>

//----------------------------------------------------------------------------------------------------------------------
//  HelpView: DECLARATION
//----------------------------------------------------------------------------------------------------------------------
class HelpView : public QWebEngineView
{
    Q_OBJECT
public: HelpView(QWidget *parent = nullptr) : QWebEngineView(parent) {}

protected:
    void keyPressEvent(QKeyEvent *event);
};

//----------------------------------------------------------------------------------------------------------------------
//  WebEnginePage: DECLARATION
//----------------------------------------------------------------------------------------------------------------------
class WebEnginePage : public QWebEnginePage
{
    Q_OBJECT
public: WebEnginePage(QObject *parent = nullptr) : QWebEnginePage(parent) {}
    bool acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool);

signals:
    void linkClicked(const QUrl &url);
};
