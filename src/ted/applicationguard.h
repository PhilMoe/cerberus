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
#pragma once

#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>

//----------------------------------------------------------------------------------------------------------------------
//  ApplicationGuard: DECLARATION
//----------------------------------------------------------------------------------------------------------------------
class ApplicationGuard : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationGuard(QObject *parent = 0);
    ~ApplicationGuard();

    void listen(QString name);
    bool hasPrevious(QString name, QStringList arguments);
signals:
    void newInstance();

public slots:
    void newConnection();
    void readyRead();

private:
    QLocalServer m_server;
    QLocalSocket *m_socket;
};
