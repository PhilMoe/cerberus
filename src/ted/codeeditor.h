/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include "std.h"

class CodeDocument;
class CodeEditor;
class Highlighter;
class Prefs;
class BlockData;

//***** CodeEditor *****

class CodeEditor : public QPlainTextEdit{
    Q_OBJECT

public:
    CodeEditor( QWidget *parent=0 );
    ~CodeEditor();

    //return true if successful and path updated
    bool open( const QString &path );
    bool save( const QString &path );
    void rename( const QString &path );
    const QString &path(){ return _path; }
    int modified(){ return _modified; }

    QString fileType(){ return _fileType; }

    bool isTxt(){ return _txt; }
    bool isCode(){ return _code; }
    bool isMonkey(){ return _monkey; }

    void gotoLine( int line );
    void highlightLine( int line );

    bool findNext( const QString &findText,bool cased,bool wrap );
    bool replace( const QString &findText,const QString &replaceText,bool cased );
    int  replaceAll( const QString &findText,const QString &replaceText,bool cased,bool wrap );

    QString identAtCursor();

    Highlighter *highlighter(){ return _highlighter; }
    QTreeView *codeTreeView(){ return _codeTreeView; }

public slots:

    void onTextChanged();
    void onCursorPositionChanged();
    void onPrefsChanged( const QString &name );

    void onCodeTreeViewClicked( const QModelIndex &index );

signals:

    void showCode( const QString &file,int line );

protected:

    void keyPressEvent( QKeyEvent *e );

private:
    Highlighter *_highlighter;
    QStandardItemModel *_codeTreeModel;
    QTreeView *_codeTreeView;

    QString _path;
    QString _fileType;
    bool _txt;
    bool _code;
    bool _monkey;

    int _modified;

    friend class Highlighter;
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

public slots:

    void onPrefsChanged( const QString &name );

protected:

    void highlightBlock( const QString &text );

private:
    CodeEditor *_editor;

    QColor _backgroundColor;
    QColor _defaultColor;
    QColor _numbersColor;
    QColor _stringsColor;
    QColor _identifiersColor;
    QColor _keywordsColor;
    QColor _commentsColor;
    QColor _highlightColor;

    QSet<BlockData*> _blocks;
    bool _blocksDirty;

    void insert( BlockData *data );
    void remove( BlockData *data );

    QString parseToke( QString &text,QColor &color );

    static QMap<QString,QString> _keyWords;

    friend class BlockData;
};

#endif // CODEEDITOR_H
