/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef FILETREE_H
#define FILETREE_H

#include "std.h"

class ProjectTreeModel : public QFileSystemModel{
    Q_OBJECT

public:

    ProjectTreeModel( QObject *parent=nullptr );

    bool addProject( const QString &dir );
    void removeProject( const QString &dir );
    bool isProject( const QModelIndex &index );

    QVector<QString> projects(){ return _dirs; }

    QString currentProject(){ return _current!=-1 ? _dirs[_current] : ""; }

    virtual bool hasChildren( const QModelIndex &parent=QModelIndex() )const;
    virtual int	rowCount( const QModelIndex &parent=QModelIndex() )const;
    virtual QModelIndex	index( int row, int column, const QModelIndex &parent=QModelIndex() )const;
    QVariant data( const QModelIndex &index,int role )const;

private:

    int _current;
    QVector<QString> _dirs;
    QVector<QPersistentModelIndex> _projs;
};

#endif
