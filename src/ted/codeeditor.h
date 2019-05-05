/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include "std.h"
#include "mainwindow.h"

#include <QSyntaxHighlighter>
#include <QVector>
#include <QScrollBar>
#include <QShortcut>

#include <QCompleter>
#include <QStringListModel>
#include <QAbstractListModel>

class CodeDocument;
class CodeEditor;
class Highlighter;
class Prefs;
class BlockData;
class QString;
class QTextDocument;
class QCompleter;
class QStringListModel;
class QAbstractListModel;
class CompleterListModel;
//class QStringList;

//***** CompleterListModel *****
class CompleterListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    CompleterListModel( QObject *parent=nullptr ): QAbstractListModel(parent){};

    int	rowCount( const QModelIndex &parent=QModelIndex() )const ;

    QVariant data( const QModelIndex &index,int role )const ;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());
    void setList(QStringList cmdlist);
private:
    QStringList _commandList;
};

//***** BlockData *****
class BlockData : public QTextBlockUserData{
public:
    BlockData( Highlighter *highlighter,const QTextBlock &block,const QString &decl,const QString &ident,int indent );
    ~BlockData();

    QTextBlock block(){ return _block; }
    const QString &decl(){ return _decl; }
    const QString &ident(){ return _ident; }
    int indent(){ return _indent; }

    bool isBookmarked(){ return _marked; }
    void setBookmark(bool mark) { _marked = mark; }
    void toggleBookmark() { _marked = !_marked; }
    void setModified(int mod) { _modified = mod; }
    void invalidate();

    int _modified;
    bool _marked;
    int _code;

private:
    Highlighter *_highlighter;
    QTextBlock _block;
    QString _decl;
    QString _ident;
    int _indent;

};


//***** CodeEditor *****

class CodeEditor : public QPlainTextEdit{
    Q_OBJECT

public:
    CodeEditor( QWidget *parent=nullptr, MainWindow *wnd=nullptr );
    ~CodeEditor();

    //return true if successful and path updated
    bool open( const QString &path );
    bool save( const QString &path );
    void rename( const QString &path );
    const QString &path(){ return _path; }
    int modified(){ return _modified; }
    bool doHighlightCurrLine;
    bool doHighlightCurrWord;
    bool doHighlightBrackets;
    bool doLineNumbers;
    bool doSortCodeBrowser;
    bool _modSignal;
    bool _tabs4spaces;
    bool _capitalizeAPI;
    QString _tabSpaceText;
    MainWindow *_mainWnd;

    QString fileType(){ return _fileType; }

    bool isTxt(){ return _txt; }
    bool isCode(){ return _code; }
    bool isCerberus(){ return _cerberus; }
    bool isMonkey2(){ return _monkey2; }

    void gotoLine( int line );
    void highlightLine( int line );
    void commentUncommentBlock();
    void bookmarkToggle();
    void bookmarkNext();
    void bookmarkPrev();
    void bookmarkFind( int dir, int start=-1 );

    bool findNext( const QString &findText,bool cased,bool wrap );
    bool replace( const QString &findText,const QString &replaceText,bool cased );
    int  replaceAll( const QString &findText,const QString &replaceText,bool cased,bool wrap );

    QString identAtCursor();

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    Highlighter *highlighter(){ return _highlighter; }
    QTreeView *codeTreeView(){ return _codeTreeView; }

    QImage imgBookmark;

public slots:

    void onTextChanged();
    void onCursorPositionChanged();
    void onPrefsChanged( const QString &name );
    void highlightCurrentLine();
    void onCodeTreeViewClicked( const QModelIndex &index );

signals:

    void showCode( const QString &file,int line );

protected:

    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent( QKeyEvent *e );
    QString identAtCursor(bool fullWord);

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    //void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);
    void insertCompletion(const QString &completion,
                          bool singleWord=false);
    void performCompletion();
private:
    Highlighter *_highlighter;
    QStandardItemModel *_codeTreeModel;
    QTreeView *_codeTreeView;

    QString _path;
    QString _fileType;
    bool _txt;
    bool _code;
    bool _cerberus;
    bool _monkey2;

    int _modified;
    bool _capitalize;

    friend class Highlighter;

    QWidget *lineNumberArea;
    int indexOfClosedBracket(const QString &text, const QChar &sourceBracket, int findFrom);
    int indexOfOpenedBracket(const QString &text, const QChar &sourceBracket, int findFrom);

    void performCompletion(const QString &completionPrefix);
    bool handledCompletedAndSelected(QKeyEvent *event);
    void populateModel(const QString &completionPrefix);
    bool completedAndSelected;
    QCompleter *completer;
    QStringListModel *model;
    CompleterListModel *model2;
};

//***** Highlighter *****

class Highlighter : public QSyntaxHighlighter{
    Q_OBJECT

public:
    Highlighter( CodeEditor *editor );
    ~Highlighter();

    CodeEditor *editor(){ return _editor; }

    bool capitalize( const QTextBlock &block,QTextCursor cursor );

    void validateCodeTreeModel();
    QIcon identIcon( const QString &ident );
    QColor _keywordsColor;
    QColor _keywords2Color;
    QColor _lineNumberColor;

protected:
    void highlightBlock( const QString &text );


public slots:

    void onPrefsChanged( const QString &name );




private:
    CodeEditor *_editor;
    QWidget *lineNumberArea;

    QColor _backgroundColor;
    QColor _console1Color;
    QColor _console2Color;
    QColor _console3Color;
    QColor _console4Color;
    QColor _defaultColor;
    QColor _numbersColor;
    QColor _stringsColor;
    QColor _identifiersColor;

    QColor _commentsColor;
    QColor _highlightColor;

    QSet<BlockData*> _blocks;
    bool _blocksDirty;

    void insert( BlockData *data );
    void remove( BlockData *data );

    QString parseToke( QString &text,QColor &color, QString &prevText );

    const QMap<QString,QString>&keyWords(){ return _editor->isMonkey2() ? _keyWords2 : _keyWords; }
    const QMap<QString,QString>&keyWords3(){ return _editor->isMonkey2() ? _keyWords2 : _keyWords3; }

    static QMap<QString,QString> _keyWords;
    static QMap<QString,QString> _keyWords2;
    static QMap<QString,QString> _keyWords3;

    friend class BlockData;
};

#endif // CODEEDITOR_H
