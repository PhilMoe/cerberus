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
#include "codeeditor.h"

#include <QColor>
#include <QIcon>
#include <QObject>
#include <QSet>

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(CodeEditor *editor);
    ~Highlighter();

    CodeEditor *editor() {
        return _editor;
    }

    QColor getLineNumberColor() {
        return _lineNumberColor;
    }

    bool capitalize(const QTextBlock &block, QTextCursor cursor);
    void validateCodeTreeModel();
    QIcon identIcon(const QString &ident);

protected:
    void highlightBlock(const QString &text);

public slots:
    void onPrefsChanged(const QString &name);

private:
    CodeEditor *_editor;
    QWidget *lineNumberArea;
    QSet<BlockData *> _blocks;

    QColor _backgroundColor;
    QColor _console1Color;
    QColor _console2Color;
    QColor _console3Color;
    QColor _console4Color;
    QColor _defaultColor;
    QColor _numbersColor;
    QColor _stringsColor;
    QColor _identifiersColor;

    QColor _keywordsColor;
    QColor _keywords2Color;
    QColor _lineNumberColor;

    QColor _commentsColor;
    QColor _highlightColor;

    bool _blocksDirty;
    static QMap<QString, QString> _keyWords;
    static QMap<QString, QString> _keyWords2;
    static QMap<QString, QString> _keyWords3;

    friend class BlockData;

    const QMap<QString, QString> &keyWords() {
        return _editor->isMonkey2() ? _keyWords2 : _keyWords;
    }
    const QMap<QString, QString> &keyWords3() {
        return _editor->isMonkey2() ? _keyWords2 : _keyWords3;
    }

    void insert(BlockData *data);
    void remove(BlockData *data);

    QString parseToke(QString &text, QColor &color, QString &prevText);
};
