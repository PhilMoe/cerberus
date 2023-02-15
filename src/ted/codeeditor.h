//----------------------------------------------------------------------------------------------------------------------
// Ted, a simple text editor/IDE.
//
// Copyright 2012, Blitz Research Ltd.
//
// See LICENSE.TXT for licensing terms.
//
//  NOTE: This version is not backwards compatible with versions earlier than
//  Qt 5.9.0
//----------------------------------------------------------------------------------------------------------------------
// CONTRIBUTORS: See contributors.txt
#pragma once

#include "std.h"

#include <QAbstractListModel>
#include <QCompleter>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QShortcut>
#include <QStandardItemModel>
#include <QString>
#include <QStringListModel>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTreeView>
#include <QVector>

class CodeDocument;
class CodeEditor;
class Highlighter;
class Prefs;
class BlockData;

//----------------------------------------------------------------------------------------------------------------------
//  CodeTreeItem: DECLARATION/IMPLEMENTETION
//----------------------------------------------------------------------------------------------------------------------
class CodeTreeItem : public QStandardItem
{
public:
    CodeTreeItem() : _data(nullptr) {
        setEditable(false);
    }

    void setData(BlockData *data) {
        _data = data;
    }

    BlockData *data() {
        return _data;
    }

private:
    BlockData *_data;
};

//----------------------------------------------------------------------------------------------------------------------
//  CompleterListModel: DECLARATION
//----------------------------------------------------------------------------------------------------------------------
class CompleterListModel : public QAbstractListModel
{
    Q_OBJECT

public: CompleterListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {};

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());
    void setList(QStringList cmdlist);

private:
    QStringList _commandList;
};

//----------------------------------------------------------------------------------------------------------------------
//  BlockData: DECLARATION/IMPLEMENTETION
//----------------------------------------------------------------------------------------------------------------------
// NOTE: Added additional information to text blocks, such as if the text is an identifier, declaration etc.
class BlockData : public QTextBlockUserData
{
public:
    BlockData(Highlighter *highlighter, const QTextBlock &block,
              const QString &decl, const QString &ident, int indent)
        : _highlighter(highlighter), _block(block), _decl(decl), _ident(ident),
          _indent(indent) {
        _modified = 0;
        _marked = false;
        _code = 0;
    }

    ~BlockData();

    QTextBlock block() {
        return _block;
    }

    const QString &decl() {
        return _decl;
    }

    const QString &ident() {
        return _ident;
    }

    int indent() {
        return _indent;
    }

    bool isBookmarked() {
        return _marked;
    }

    void setBookmark(bool mark) {
        _marked = mark;
    }

    void toggleBookmark() {
        _marked = !_marked;
    }

    int getModified() {
        return _modified;
    }

    void setModified(int mod) {
        _modified = mod;
    }

    int getCode() {
        return _code;
    }

    void setCode(int code) {
        _code = code;
    }

    void invalidate() {
        _highlighter = nullptr;
    };

private:
    Highlighter *_highlighter;
    QTextBlock _block;
    QString _decl;
    QString _ident;
    int _indent;

    int _modified;
    bool _marked;
    int _code;
};

//----------------------------------------------------------------------------------------------------------------------
//  CodeEditor: DECLARATION
//----------------------------------------------------------------------------------------------------------------------
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);
    ~CodeEditor();

    const QString &path() {
        return _path;
    }

    int modified() {
        return _modified;
    }

    QString fileType() {
        return _fileType;
    }

    bool isTxt() {
        return _txt;
    }

    bool isCode() {
        return _code;
    }

    bool isCerberus() {
        return _cerberus;
    }

    bool isMonkey2() {
        return _monkey2;
    }

    Highlighter *highlighter() {
        return _highlighter;
    }

    QTreeView *codeTreeView() {
        return _codeTreeView;
    }

    bool isCompleterVisible() {
        return completer->popup()->isVisible();
    }

    void closeCompleter() {
        completer->popup()->close();
        completer->activated("");
    }

    int lineNumberAreaWidth();
    bool open(const QString &path);
    bool save(const QString &path);
    void evaluatefiletype(const QString &path);
    void gotoLine(int line);
    void highlightLine(int line);
    void commentUncommentBlock();
    void bookmarkToggle();
    void bookmarkNext();
    void bookmarkPrev();
    void bookmarkFind(int dir, int start = -1);

    bool findNext(const QString &findText, bool cased, bool wrap);
    bool replace(const QString &findText, const QString &replaceText, bool cased);
    int replaceAll(const QString &findText, const QString &replaceText,
                   bool cased, bool wrap);
    QString identAtCursor();
    void lineNumberAreaPaintEvent(QPaintEvent *event);

public slots:
    void onTextChanged();
    void onCursorPositionChanged();
    void onPrefsChanged(const QString &name);
    void highlightCurrentLine();
    void onCodeTreeViewClicked(const QModelIndex &index);

signals:
    void showCode(const QString &file, int line);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    QString identAtCursor(bool fullWord);

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &, int);
    void insertCompletion(const QString &completion, bool singleWord = false);
    void performCompletion();

private:
    Highlighter *_highlighter;
    QStandardItemModel *_codeTreeModel;
    QTreeView *_codeTreeView;
    QCompleter *completer;
    QStringListModel *model;
    CompleterListModel *model2;
    QWidget *lineNumberArea;

    QString _path;
    QString _fileType;
    QImage imgBookmark;
    QString _tabSpaceText;

    bool completedAndSelected;
    bool _txt;
    bool _code;
    bool _cerberus;
    bool _monkey2;

    int _modified;
    bool _capitalize;

    bool doHighlightCurrLine;
    bool doHighlightCurrWord;
    bool doHighlightBrackets;
    bool doLineNumbers;
    bool doSortCodeBrowser;
    bool _modSignal;
    bool _tabs4spaces;
    bool _capitalizeAPI;

    friend class Highlighter;

    int indexOfClosedBracket(const QString &text, const QChar &sourceBracket,
                             int findFrom);
    int indexOfOpenedBracket(const QString &text, const QChar &sourceBracket,
                             int findFrom);

    bool handledCompletedAndSelected(QKeyEvent *event);
    void performCompletion(const QString &completionPrefix);
    void populateModel(const QString &completionPrefix);
};
