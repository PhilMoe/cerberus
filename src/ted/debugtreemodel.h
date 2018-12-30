/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef DEBUGTREEMODEL_H
#define DEBUGTREEMODEL_H

#include "std.h"

class Process;

class DebugTreeModel : public QStandardItemModel{
    Q_OBJECT

public:
    DebugTreeModel( Process *proc,QObject *parent=nullptr );

    void stop();

    void run();
    void step();
    void stepInto();
    void stepOut();
    void kill();

    bool stopped(){ return _stopped; }

    bool hasChildren( const QModelIndex &parent )const;
    bool canFetchMore( const QModelIndex &parent )const;
    void fetchMore( const QModelIndex &parent );

public slots:
    void onClicked( const QModelIndex &index );

signals:
    void showCode( const QString &path,int line );

private:
    Process *_proc;
    bool _stopped;
};

#endif // DEBUGTREEMODEL_H
