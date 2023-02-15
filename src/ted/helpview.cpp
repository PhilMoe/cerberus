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
#include "helpview.h"
#include "mainwindow.h"
#include "std.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QKeyEvent>

//----------------------------------------------------------------------------------------------------------------------
//  HelpView: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//  HelpView: PROTECTED MEMBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
// Capture the F1 key to show context help
void HelpView::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_F1)
        MAIN_WINDOW->onShowHelpView();
}

//----------------------------------------------------------------------------------------------------------------------
//  WebEnginePage: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//  WebEnginePage: PUBLIC MEMBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
// Override the QWebEnginePage's acceptNavigationRequest for the applications own purposes.
bool WebEnginePage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
    // Capture links that have been clicked only in the main frame. Embedded elements will be child frames within a
    // QWebEnginePage.
    if((type == QWebEnginePage::NavigationTypeLinkClicked) && (isMainFrame)) {
        // Check if the URL passed is a local file. If not then try to open the OS's default web browser.
        if(url.isLocalFile()) {
            // Set the parameter to pass to the openFile functions as one to open the help view.
            QString param = url.toString();

            // The file is a local, so check to see what the file extension is. If the extension matches
            // one of the pre-defined, then set param as a normal file path.
            QString ext = ";" + extractExt(url.fileName()) + ";";
            if(textFileTypes.contains(ext.toLower()) || codeFileTypes.contains(ext.toLower()))
                param = url.toLocalFile();

            MAIN_WINDOW->openFile(param, false);
            return false;
        }

        // Try to open with the default web browser.
        QDesktopServices::openUrl(url);
        return false;
    }
    return true;
}
