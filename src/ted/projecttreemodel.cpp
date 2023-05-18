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

#include "projecttreemodel.h"
#include <QFont>

//----------------------------------------------------------------------------------------------------------------------
//  ProjectTreeModel: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
ProjectTreeModel::ProjectTreeModel(QObject *parent) : QFileSystemModel(parent), _current(-1) {}

//----------------------------------------------------------------------------------------------------------------------
//  ProjectTreeModel: PUBLIC MEMBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
bool ProjectTreeModel::addProject(const QString &dir)
{
    // Return if the directory path is empty
    if(dir.isEmpty())
        return false;

    // Path separator checking
    QString sdir = dir.endsWith('/') ? dir : dir + '/';
    for(int i = 0; i < _dirs.size(); ++i) {
        if(_dirs[i].startsWith(sdir))
            return false;
        QString idir = _dirs[i].endsWith('/') ? _dirs[i] : _dirs[i] + '/';
        if(dir.startsWith(idir))
            return false;
    }

    // Add the directory path and get the model index of the directory path.
    QFileSystemModel::setRootPath("");

    QModelIndex index = QFileSystemModel::index(dir);
    if(!index.isValid())
        return false;

    // Store the directory and the model index to the list of projects.
    QFileSystemModel::beginInsertRows(QModelIndex(), _projs.size(), _projs.size());

    _dirs.push_back(dir);
    _projs.push_back(index);

    QFileSystemModel::endInsertRows();

    return true;
}

void ProjectTreeModel::removeProject(const QString &dir)
{
    // Search for the selected directory and remove it from the stored projects.
    for(int i = 0; i < _dirs.size(); ++i) {
        if(dir == _dirs[i]) {
            QFileSystemModel::beginRemoveRows(QModelIndex(), _dirs.size() - 1, _dirs.size() - 1);
            _dirs.remove(i);
            _projs.remove(i);
            QFileSystemModel::endRemoveRows();
            return;
        }
    }
}

// Check to see if the index passed is in the list of stored directories.
bool ProjectTreeModel::isProject(const QModelIndex &index)
{
    for(int i = 0; i < _projs.size(); ++i) {
        if(index == _projs[i])
            return true;
    }
    return false;
}

// Check to see if the passed model index has any children.
bool ProjectTreeModel::hasChildren(const QModelIndex &parent) const
{
    // If the parent is invalid, then return true if there are projects stored; else return true if the parent model
    //index has children.
    if(!parent.isValid())
        return _projs.size() > 0;

    return QFileSystemModel::hasChildren(parent);
}

// Returns either the total number of project directories, or the number of rows in the model index passed.
int ProjectTreeModel::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return _projs.size();

    return QFileSystemModel::rowCount(parent);
}

// Returns the model index.
// If the parent is not valid, then either return the stored project model, a empty model, or the file system model.
QModelIndex ProjectTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        if(row >= 0 && row < _projs.size())
            return _projs[row];
        return QModelIndex();
    }
    return QFileSystemModel::index(row, column, parent);
}

// Returns data from a model index.
QVariant ProjectTreeModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::FontRole && _current != -1 && index == _projs[_current]) {
        QFont font = QFileSystemModel::data(index, role).value<QFont>();
        font.setBold(true);
        return font;
    }

    return QFileSystemModel::data(index, role);
}

// NOTE: I suspect that there is a bug with either the derived QFileSystemModel or QTreeView.
// Hiding columns in QTreeView with this model always hides the first column, no matter which column is hidden.
// The column count had to be over ridden just to show the first column only.
int ProjectTreeModel::columnCount(const QModelIndex &index) const
{
    return 1;
}
