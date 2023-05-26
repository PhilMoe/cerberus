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

#include <QFileSystemModel>

//----------------------------------------------------------------------------------------------------------------------
//  ProjectTreeModel: DECLARATION
//----------------------------------------------------------------------------------------------------------------------
class ProjectTreeModel : public QFileSystemModel
{
    Q_OBJECT

public:
    ProjectTreeModel(QObject *parent = nullptr);

    bool addProject(const QString &dir);
    void removeProject(const QString &dir);
    bool isProject(const QModelIndex &index);

    QVector<QString> projects() {
        return _dirs;
    }

    QString currentProject() {
        return _current != -1 ? _dirs[_current] : "";
    }

    virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    virtual int columnCount(const QModelIndex &index) const;

private:
    int _current;
    QVector<QString> _dirs;
    QVector<QPersistentModelIndex> _projs;
};
