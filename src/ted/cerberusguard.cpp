/*
Class to allow only a single instance of a running version of Ted.
*/
#include "cerberusguard.h"

// Connet the classes own QLocalServer to call the newConnection method.
CerberusGuard::CerberusGuard(QObject *parent) : QObject(parent)
{
    connect(&m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

// Close the lisening server. This will also close the socket.
CerberusGuard::~CerberusGuard()
{
    m_server.close();
}

// Stop listening to the old and start lisening to a new connection.
void CerberusGuard::listen(QString name)
{
    m_server.removeServer(name);
    m_server.listen(name);
}

// Create a socket and when the connection is established, buffer the arguments pased
// Before writting them out to the socket.
bool CerberusGuard::hasPrevious(QString name, QStringList arguments)
{
    QLocalSocket socket;
    socket.connectToServer(name);
    if(socket.waitForConnected())
    {
        QByteArray buffer;
        foreach(QString item, arguments)
        {
            buffer.append(item+"\n");
        }
        socket.write(buffer);
        socket.waitForBytesWritten();
        return true;
    }
    return false;
}

// Make a new connection and connet the socket to the readyRead method.
void CerberusGuard::newConnection()
{
    emit newInstance();
    m_socket=m_server.nextPendingConnection();
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

// Make a temporary socket, read and convert the stream passed before passing it to the MainWindow.
void CerberusGuard::readyRead()
{   
    QLocalSocket *socket = qobject_cast<QLocalSocket*>(sender());
    QStringList list = QString(socket->readAll()).split("\n");
    CerberusApplication::mainWindow->parseAppArgs(list);
    socket->deleteLater();
}
