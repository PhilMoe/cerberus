//----------------------------------------------------------------------------------------------------------------------
// Ted, a simple text editor/IDE.
//
// Copyright 2012, Blitz Research Ltd.
//
// See LICENSE.TXT for licensing terms.
//
//  NOTE: This version is not backwards compatible with versions earlier than Qt 5.9.0
//----------------------------------------------------------------------------------------------------------------------
// TODO: Add comments.
#include "debugtreemodel.h"
#include "process.h"

#include <QStack>

//----------------------------------------------------------------------------------------------------------------------
//  DebugTreeModel: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
DebugTreeModel::DebugTreeModel(Process *proc, QObject *parent)
    : QStandardItemModel(parent), _proc(proc), _stopped(false) {}

//----------------------------------------------------------------------------------------------------------------------
//  DebugTreeModel: PUBLIC MEMBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
void DebugTreeModel::stop()
{
    if(_stopped)
        return;

    _stopped = true;

    QStandardItem *root = invisibleRootItem();

    // build call stack...
    DebugItem *func = nullptr;
    int n_funcs = 0, n_vars = 0;

    QStack<DebugItem *> objs;

    for(;;) {
        QString text = _proc->readLine(1);
        if(text.isEmpty())
            break;

        if(text.startsWith("+")) {
            if(func)
                func->setRowCount(n_vars);
            func = dynamic_cast<DebugItem *>(root->child(n_funcs++));
            if(!func) {
                func = new DebugItem;
                root->appendRow(func);
            }
            func->update(text);
            n_vars = 0;
        } else if(func) {
            DebugItem *item = dynamic_cast<DebugItem *>(func->child(n_vars++));
            if(!item) {
                item = new DebugItem;
                func->appendRow(item);
            }
            item->update(text);

            if(item->type() == 3 && item->address() != nullptr && item->expanded())
                objs.push(item);

            else
                item->setRowCount(0);
        }
    }

    if(func)
        func->setRowCount(n_vars);

    root->setRowCount(n_funcs);

    while(!objs.isEmpty()) {

        DebugItem *item = objs.pop();

        _proc->writeLine(QString("@") + QString::number((qulonglong)item->address(), 16));

        int n_vars = 0;

        for(;;) {
            QString text = _proc->readLine(1);
            if(text.isEmpty())
                break;

            DebugItem *child = dynamic_cast<DebugItem *>(item->child(n_vars++));
            if(!child) {
                child = new DebugItem;
                item->appendRow(child);
            }

            child->update(text);

            if(child->type() == 2 && child->address() != nullptr && child->expanded())
                objs.push(child);

            else
                child->setRowCount(0);
        }
        item->setRowCount(n_vars);
    }
}

void DebugTreeModel::run()
{
    if(!_stopped)
        return;
    _proc->writeLine("r");
    _stopped = false;
}

void DebugTreeModel::step()
{
    if(!_stopped)
        return;
    _proc->writeLine("s");
    _stopped = false;
}

void DebugTreeModel::stepInto()
{
    if(!_stopped)
        return;
    _proc->writeLine("e");
    _stopped = false;
}

void DebugTreeModel::stepOut()
{
    if(!_stopped)
        return;
    _proc->writeLine("l");
    _stopped = false;
}

void DebugTreeModel::kill()
{
    if(!_stopped)
        return;
    _proc->writeLine("q");
    _stopped = false;
}

bool DebugTreeModel::hasChildren(const QModelIndex &parent) const
{
    if(DebugItem *item = dynamic_cast<DebugItem *>(itemFromIndex(parent)))
        return item->type() > 1;

    return QStandardItemModel::hasChildren(parent);
}

bool DebugTreeModel::canFetchMore(const QModelIndex &parent) const
{
    if(!_stopped)
        return false;

    if(DebugItem *item = dynamic_cast<DebugItem *>(itemFromIndex(parent)))

        return item->type() == 3 && item->address() != nullptr && !item->expanded();

    return false;
}

void DebugTreeModel::fetchMore(const QModelIndex &parent)
{
    if(!_stopped)
        return;

    if(DebugItem *item = dynamic_cast<DebugItem *>(itemFromIndex(parent))) {
        int n_vars = 0;

        _proc->writeLine(QString("@") + QString::number((qulonglong)item->address(), 16));

        for(;;) {

            QString text = _proc->readLine(1);
            if(text.isEmpty())
                break;

            DebugItem *child = dynamic_cast<DebugItem *>(item->child(n_vars++));
            if(!child) {
                child = new DebugItem;
                item->appendRow(child);
            }
            child->update(text);
        }

        item->setRowCount(n_vars);
        item->setExpanded(true);
    }
}

//----------------------------------------------------------------------------------------------------------------------
//  DebugTreeModel: PUBLIC MEMBER SLOTS
//----------------------------------------------------------------------------------------------------------------------
void DebugTreeModel::onClicked(const QModelIndex &index)
{
    DebugItem *item = dynamic_cast<DebugItem *>(itemFromIndex(index));
    if(!item)
        return;

    if(item->type() == 2) {
        QString info = item->info();
        int i = info.lastIndexOf('<');
        if(i != -1 && info.endsWith('>')) {
            QString path = info.left(i);
            int line = info.mid(i + 1, info.length() - i - 2).toInt() - 1;
            emit showCode(path, line);
        }
    }
}
