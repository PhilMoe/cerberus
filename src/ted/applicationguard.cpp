//----------------------------------------------------------------------------------------------------------------------
// Ted, a simple text editor/IDE.
//
// Copyright 2012, Blitz Research Ltd.
//
// See LICENSE.TXT for licensing terms.
//
//  NOTE: This version is not backwards compatible with versions earlier than Qt 5.9.0
//----------------------------------------------------------------------------------------------------------------------
// The main purpose of this class is to ensure that only one instance of Ted is running at a time.
// CONTRIBUTORS: See contributors.txt
#include "applicationguard.h"
#include "mainwindow.h"

extern MainWindow *MAIN_WINDOW;

//----------------------------------------------------------------------------------------------------------------------
//  ApplicationGuard: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
// Connect the classes own QLocalServer to call the newConnection method.
ApplicationGuard::ApplicationGuard(QObject *parent) : QObject(parent)
{
    connect(&m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

// Close the listening server. This will also close the socket.
ApplicationGuard::~ApplicationGuard()
{
    m_server.close();
}

//----------------------------------------------------------------------------------------------------------------------
//  ApplicationGuard: PUBLIC MEMBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
// Stop listening to the old and start listening to a new connection.
void ApplicationGuard::listen(QString name)
{
    m_server.removeServer(name);
    m_server.listen(name);
}

// Create a socket and when the connection is established, buffer the arguments passed
// Before writing them out to the socket.
bool ApplicationGuard::hasPrevious(QString name, QStringList arguments)
{
    QLocalSocket socket;
    socket.connectToServer(name);
    if(socket.waitForConnected()) {
        QByteArray buffer;
        foreach(QString item, arguments) {
            // buffer.append(item+"\n");
            buffer.append(QString(item + "\n").toUtf8());
        }
        socket.write(buffer);
        socket.waitForBytesWritten();
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
//  ApplicationGuard: PUBLIC MEMBER SLOTS
//----------------------------------------------------------------------------------------------------------------------
// Make a new connection and connect the socket to the readyRead method.
void ApplicationGuard::newConnection()
{
    emit newInstance();
    m_socket = m_server.nextPendingConnection();
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

// Make a temporary socket, read and convert the stream passed before passing it to the MainWindow.
void ApplicationGuard::readyRead()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    QStringList list = QString(socket->readAll()).split("\n");
    MAIN_WINDOW->parseAppArgs(list);
    socket->deleteLater();
}
