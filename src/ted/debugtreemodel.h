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

#include <QModelIndex>
#include <QStandardItem>
#include <QStandardItemModel>

class Process;
//----------------------------------------------------------------------------------------------------------------------
//  DebugItem: IMPLEMENTATION/DECLARATION
//----------------------------------------------------------------------------------------------------------------------
class DebugItem : public QStandardItem
{
public:
    DebugItem() : _type(-1), _addr(nullptr), _expanded(false) {
        setEditable(false);
    }

    int type() const {
        return _type;
    }

    void *address() const {
        return _addr;
    }

    QString info() const {
        return _info;
    }

    void setExpanded(bool expanded) {
        _expanded = expanded;
    }

    bool expanded() const {
        return _expanded;
    }

    void update(const QString &ctext) {
        QString text = ctext;
        _type = 0;
        _addr = nullptr;
        _info = "";
        if(text.startsWith("("))
            _type = 0;

        else if(text.startsWith("+")) {
            _type = 2;
            text = text.mid(1);
            int i = text.indexOf(';');
            if(i != -1) {
                _info = text.mid(i + 1);
                text = text.left(i);
            }
        } else if(text.indexOf("\"") == -1) {
            int i = text.indexOf("=@");
            if(i != -1) {
                _type = 3;
                _addr = reinterpret_cast<void *>(text.mid(i + 2).toULongLong(0, 16));
            }
        }
        setText(text);
    }

private:
    int _type;
    void *_addr;
    QString _info;
    bool _expanded;
};

//----------------------------------------------------------------------------------------------------------------------
//  DebugTreeModel: DECLARATION
//----------------------------------------------------------------------------------------------------------------------
class DebugTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    DebugTreeModel(Process *proc, QObject *parent = nullptr);

    void stop();

    void run();
    void step();
    void stepInto();
    void stepOut();
    void kill();

    bool stopped() {
        return _stopped;
    }

    bool hasChildren(const QModelIndex &parent) const;
    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);

public slots:
    void onClicked(const QModelIndex &index);

signals:
    void showCode(const QString &path, int line);

private:
    Process *_proc;
    bool _stopped;
};
