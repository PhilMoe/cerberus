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
// The highlighter serves two purposes:
//                                      Highlights keywords, identifiers etc.
//                                      Keeps track of all declarations in the current file for use in the code browser.
// TODO: Add comments
#include "highlighter.h"
#include "mainwindow.h"
#include "prefs.h"

#include <QApplication>
#include <QMap>
#include <QStack>

#define KW(X) _keyWords.insert(QString(X).toLower(), X)

QMap<QString, QString> Highlighter::_keyWords;
QMap<QString, QString> Highlighter::_keyWords2;
QMap<QString, QString> Highlighter::_keyWords3;

//----------------------------------------------------------------------------------------------------------------------
// HighLighter: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
Highlighter::Highlighter(CodeEditor *editor) : QSyntaxHighlighter(editor->document()), _editor(editor)
{
    // Hard coded list of keywords for Cerberus X/Monkey X and Monkey 2
    if(_keyWords.isEmpty()) {
        const QString &kws =
            "Void;Strict;Public;Private;Protected;Friend;Property;"
            "Bool;Int;Float;String;Array;Object;Mod;Continue;Exit;"
            "Include;Import;Module;Extern;"
            "New;Self;Super;Eachin;True;False;Null;Not;"
            "Extends;Abstract;Final;Select;Case;Default;"
            "Const;Local;Global;Field;Method;Function;Class;Interface;Implements;Enumerate;"
            "And;Or;Shl;Shr;End;If;Then;Else;Elseif;Endif;While;Wend;Repeat;Until;Forever;For;To;Step;Next;Return;Inline;"
            "Try;Catch;Throw;Throwable;"
            "Print;Error;Alias";

        const QString &kws2 =
            "Namespace;Using;Import;Extern;"
            "Public;Private;Protected;Internal;Friend;"
            "Void;Bool;Byte;UByte;Short;UShort;Int;UInt;Long;ULong;Float;Double;String;Object;Continue;Exit;"
            "New;Self;Super;Eachin;True;False;Null;Where;"
            "Alias;Const;Local;Global;Field;Method;Function;Property;Getter;Setter;Operator;Lambda;"
            "Enum;Class;Interface;Struct;Extends;Implements;Virtual;Override;Abstract;Final;Inline;"
            "Var;Varptr;Ptr;"
            "Not;Mod;And;Or;Shl;Shr;End;"
            "If;Then;Else;Elseif;Endif;"
            "While;Wend;"
            "Repeat;Until;Forever;"
            "For;To;Step;Next;"
            "Select;Case;Default;"
            "Try;Catch;Throw;Throwable;"
            "Return;Print;Static;Cast;"
            "Extension;Protocol;Delete";

        QStringList bits = kws.split(";");
        for(int i = 0; i < bits.size(); ++i)
            _keyWords.insert(bits.at(i).toLower(), bits.at(i));

        bits = kws2.split(";");
        for(int i = 0; i < bits.size(); ++i)
            _keyWords2.insert(bits.at(i).toLower(), bits.at(i));
    }

    if(_keyWords3.isEmpty()) {
        for(int i = 0; i < MAIN_WINDOW->CompleteListCount(); ++i)
            _keyWords3.insert(MAIN_WINDOW->CompleteListItem(i).toLower(), MAIN_WINDOW->CompleteListItem(i));
    }

    connect(Prefs::prefs(), &Prefs::prefsChanged, this, &Highlighter::onPrefsChanged);
    onPrefsChanged("");
}

Highlighter::~Highlighter()
{
    QSetIterator<BlockData *> it(_blocks);
    while(it.hasNext())
        it.next()->invalidate();
}

//----------------------------------------------------------------------------------------------------------------------
// HighLighter: PUBLIC MEMEBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
// Capitalize keywords
bool Highlighter::capitalize(const QTextBlock &block, QTextCursor cursor)
{
    QString text = block.text();
    QColor color;
    QString prevToken = "";
    QString prevToken2 = "";
    QString prevToken3 = "";
    QString prevTokenR = "";
    QString prevTokenR2 = "";
    int i = 0, pos = cursor.position();

    cursor.beginEditBlock();

    for(;;) {
        QString t = parseToke(text, color, prevToken);
        if(t.isEmpty())
            break;
        if(((prevToken != "interface") && (prevToken != "class") && (prevToken != "field") && (prevToken != "for") &&
                (prevToken != "local") && (prevToken != "[") && (prevToken != "(") && (prevToken != ",") &&
                (prevTokenR != " ") && (prevTokenR != "\t") && (prevToken != "global") && (prevToken != "import") &&
                (prevToken3 != "import") && (prevToken != "method")) ||
                (!(keyWords().value(t).isEmpty())) || (prevToken == "extends")) {
            QString kw = keyWords().value(t.toLower());
            if((_editor->_capitalizeAPI) && (keyWords().value(t).isEmpty())) {
                QString kw3 = keyWords3().value(t.toLower());

                if(kw.isEmpty())
                    kw = kw3;
            }
            if(!kw.isEmpty() && t != kw) {
                int i0 = block.position() + i;
                int i1 = i0 + t.length();
                cursor.setPosition(i0);
                cursor.setPosition(i1, QTextCursor::KeepAnchor);
                cursor.insertText(kw);
            }
        }

        i += t.length();
        if(t != " " && t != "  " && t != "   " && t != "    " && t != "     ") {
            prevToken3 = prevToken2;
            prevToken2 = prevToken;
            prevToken = t.toLower();
        }
        if(t.length() >= 1)
            prevTokenR = t.right(1);
        else
            prevTokenR = "";
        if(t.length() >= 2)
            prevTokenR2 = t.right(2);
        else
            prevTokenR2 = "";
    }

    cursor.endEditBlock();
    cursor.setPosition(pos);

    return true;
}

// Fill out the code browser with information from the BlockData objects.
void Highlighter::validateCodeTreeModel()
{
    if(!_blocksDirty)
        return;

    QStandardItem *root = _editor->_codeTreeModel->invisibleRootItem();

    int row = 0;
    QStandardItem *parent = root;

    QStack<int> rowStack;
    QStack<QStandardItem *> parentStack;

    int indent = 0x7fff;

    for(QTextBlock block = _editor->document()->firstBlock(); block.isValid(); block = block.next()) {
        BlockData *data = dynamic_cast<BlockData *>(block.userData());
        if(!data)
            continue;

        if(!_blocks.contains(data))
            continue;

        if(data->getCode() == 1)
            continue;

        int level = 0;
        if(data->indent() <= indent) {
            indent = data->indent();
            level = 0;
        } else
            level = 1;
        while(rowStack.size() > level) {
            parent->setRowCount(row);
            parent = parentStack.pop();
            row = rowStack.pop();
        }

        CodeTreeItem *item = dynamic_cast<CodeTreeItem *>(parent->child(row++));
        if(!item) {
            item = new CodeTreeItem;
            parent->appendRow(item);
        }

        item->setData(data);
        item->setText(data->ident());

        QIcon icon = identIcon(data->decl());
        item->setIcon(icon);
        item->setToolTip(data->block().text().trimmed());

        rowStack.push(row);
        parentStack.push(parent);
        row = 0;
        parent = item;
    }

    for(;;) {
        parent->setRowCount(row);
        if(parent == root)
            break;
        parent = parentStack.pop();
        row = rowStack.pop();
    }

    _blocksDirty = false;

    if(_editor->doSortCodeBrowser) {
        _editor->_codeTreeModel->sort(0);
        _editor->_codeTreeView->clearSelection();
    }
}

// Set the icon for each type of major declaration keyword to use in the code browser.
QIcon Highlighter::identIcon(const QString &ident)
{
#ifdef Q_OS_MAC
    QString appPath = extractDir(extractDir(extractDir(QCoreApplication::applicationDirPath())));
#else
    QString appPath = QCoreApplication::applicationDirPath();
#endif
    Prefs *prefs = Prefs::prefs();
    QString theme = "";
    theme = prefs->getString("theme");

    // static
    static QIcon iconst(appPath + "/themes/" + theme + "/icons/editor/const.png");
    static QIcon iglob(appPath + "/themes/" + theme + "/icons/editor/global.png");
    static QIcon ifield(appPath + "/themes/" + theme + "/icons/editor/property.png");
    static QIcon imethod(appPath + "/themes/" + theme + "/icons/editor/method.png");
    static QIcon ifunc(appPath + "/themes/" + theme + "/icons/editor/function.png");
    static QIcon iclass(appPath + "/themes/" + theme + "/icons/editor/class.png");
    static QIcon iinterf(appPath + "/themes/" + theme + "/icons/editor/interface.png");
    static QIcon ienum(appPath + "/themes/" + theme + "/icons/editor/enumerate.png");
    static QIcon iother(appPath + "/themes/" + theme + "/icons/editor/other.png");

    QString s = ident; //.toLower();
    if(s == "const")
        return iconst;
    if(s == "global")
        return iglob;
    if(s == "field")
        return ifield;
    if(s == "method")
        return imethod;
    if(s == "function")
        return ifunc;
    if(s == "class")
        return iclass;
    if(s == "interface")
        return iinterf;
    if(s == "enumerate")
        return ienum;
    return iother;
}

//----------------------------------------------------------------------------------------------------------------------
// HighLighter: PROTECTED MEMEBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
// Highlight the code block.
// NOTE: This has the same issue with #REM/#IF/#END ambiguity as the main Cerberus X/Monkey X parser. e.i. It cannot
// tell if the #END belongs to a #REM or a #IF.
// TODO: See if this can be fixed.
void Highlighter::highlightBlock(const QString &ctext)
{
    QString text = ctext;

    int i = 0, n = text.length();
    while(i < n && text[i] <= ' ')
        ++i;

    if(_editor->isCerberus()) {
        // Handle Cerberus block comments
        int st = previousBlockState();

        if(i < n && text[i] == '#') {
            int i0 = i + 1;
            while(i0 < n && text[i0] <= ' ')
                ++i0;

            int i1 = i0;
            while(i1 < n && isIdent(text[i1]))
                ++i1;

            QString t = text.mid(i0, i1 - i0).toLower();

            if(t == "rem") {
                if(!++st)
                    st = 1;
            } else if(t == "if" && st > 0)
                ++st;

            else if((t == "end" || t == "endif") && st > 0)
                --st;

            else if(!st)
                st = -1;

        } else if(!st)
            st = -1;

        setCurrentBlockState(st);

        if(st != -1) {
            setFormat(0, text.length(), _commentsColor);
            setCurrentBlockUserData(nullptr);
            return;
        }
    }

    if(!_editor->isCode()) {
        setFormat(0, text.length(), _defaultColor);
        setCurrentBlockUserData(nullptr);
        return;
    }

    int indent = i;
    text = text.mid(i);

    int colst = 0;
    QColor curcol = _defaultColor;

    QVector<QString> tokes;
    QString prevToken = "";
    for(;;) {
        QColor col = curcol;

        QString t = parseToke(text, col, prevToken);
        if(t.isEmpty())
            break;

        if(t[0] > ' ')
            tokes.push_back(t);

        if(col != curcol) {
            setFormat(colst, i - colst, curcol);
            curcol = col;
            colst = i;
        }
        if(t != " " && t != "  " && t != "   " && t != "    " && t != "     ")
            prevToken = t.toLower();
        i += t.length();
    }

    if(colst < n)
        setFormat(colst, n - colst, curcol);

    if(_editor->isCerberus()) {
        // Update user block data for code tree.
        BlockData *data = nullptr;

        QString decl = tokes.size() > 0 ? tokes[0].toLower() : "";
        QString ident = tokes.size() > 1 ? tokes[1] : "";

        if((decl == "class" || decl == "interface" || decl == "method" || decl == "function" || decl == "field" ||
                decl == "const" || decl == "global" || decl == "enumerate") &&
                !ident.isEmpty()) {
            QTextBlock block = currentBlock();
            data = dynamic_cast<BlockData *>(currentBlockUserData());
            if(data && data->block() == block && data->decl() == decl && data->ident() == ident &&
                    data->indent() == indent) {
            } else {
                data = new BlockData(this, block, decl, ident, indent);
                setCurrentBlockUserData(data);
                insert(data);
            }
        } else {
            QTextBlock block = currentBlock();
            data = dynamic_cast<BlockData *>(currentBlockUserData());
            if(data && data->block() == block && data->decl() == decl && data->ident() == ident &&
                    data->indent() == indent) {
            } else {
                data = new BlockData(this, block, decl, ident, indent);
                data->setCode(1);
                setCurrentBlockUserData(data);
                insert(data);
            }
        }
        if(data && _editor->_modSignal == true)
            data->setModified(1);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// HighLighter: PUBLIC MEMEBER SLOTS
//----------------------------------------------------------------------------------------------------------------------
// In the event that preferences were changed, then update.
void Highlighter::onPrefsChanged(const QString &name)
{
    QString t(name);

    if(t == "" || t.endsWith("Color")) {
        Prefs *prefs = Prefs::prefs();
        _backgroundColor = prefs->getColor("backgroundColor");
        _lineNumberColor = prefs->getColor("lineNumberColor");
        _console1Color = prefs->getColor("console1Color");
        _console2Color = prefs->getColor("console2Color");
        _console3Color = prefs->getColor("console3Color");
        _console4Color = prefs->getColor("console4Color");
        _defaultColor = prefs->getColor("defaultColor");
        _numbersColor = prefs->getColor("numbersColor");
        _stringsColor = prefs->getColor("stringsColor");
        _identifiersColor = prefs->getColor("identifiersColor");
        _keywordsColor = prefs->getColor("keywordsColor");
        _keywords2Color = prefs->getColor("keywords2Color");
        _commentsColor = prefs->getColor("commentsColor");
        _highlightColor = prefs->getColor("highlightColor");
        rehighlight();
    }
}

//----------------------------------------------------------------------------------------------------------------------
// HighLighter: PRIVATE MEMEBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
void Highlighter::insert(BlockData *data)
{
    _blocks.insert(data);
    _blocksDirty = true;
}

void Highlighter::remove(BlockData *data)
{
    _blocks.remove(data);
    _blocksDirty = true;
}

// Parse tokens to apply colour highlighting.
// TODO: Update to add preprocessor colourisation if possible.
QString Highlighter::parseToke(QString &text, QColor &color, QString &prevText)
{
    if(!text.length())
        return "";

    int i = 0, n = text.length();
    QChar c = text[i++];

    bool cerberusFile = _editor->isCerberus();

    if(c <= ' ') {
        while(i < n && text[i] <= ' ')
            ++i;
    } else if(isAlpha(c)) {
        while(i < n && isIdent(text[i]))
            ++i;
        color = _identifiersColor;
        if((prevText != "class") && (prevText != "field") && (prevText != "global") && (prevText != "local") &&
                (prevText != "method") && (prevText != "(") && (prevText != ",")) {
            if(cerberusFile && keyWords().contains(text.left(i).toLower()))
                color = _keywordsColor;

            else if(cerberusFile && keyWords3().contains(text.left(i).toLower()))
                color = _keywords2Color;
        }

    } else if(c == '0' && !cerberusFile) {
        if(i < n && text[i] == 'x') {
            for(++i; i < n && isHexDigit(text[i]); ++i) {
            }
        } else {
            for(; i < n && isOctDigit(text[i]); ++i) {
            }
        }
        color = _numbersColor;
    } else if(isDigit(c) || (c == '.' && i < n && isDigit(text[i]))) {
        bool flt = (c == '.');
        while(i < n && isDigit(text[i]))
            ++i;
        if(!flt && i < n && text[i] == '.') {
            ++i;
            flt = true;
            while(i < n && isDigit(text[i]))
                ++i;
        }
        if(i < n && (text[i] == 'e' || text[i] == 'E')) {
            flt = true;
            if(i < n && (text[i] == '+' || text[i] == '-'))
                ++i;
            while(i < n && isDigit(text[i]))
                ++i;
        }
        color = _numbersColor;
    } else if(c == '%' && cerberusFile && i < n && isBinDigit(text[i])) {
        for(++i; i < n && isBinDigit(text[i]); ++i) {
        }
        color = _numbersColor;
    } else if(c == '$' && cerberusFile && i < n && isHexDigit(text[i])) {
        for(++i; i < n && isHexDigit(text[i]); ++i) {
        }
        color = _numbersColor;
    } else if(c == '\"') {
        if(cerberusFile) {
            for(; i < n && text[i] != '\"'; ++i) {
            }
        } else {
            for(; i < n && text[i] != '\"'; ++i) {
                if(text[i] == '\\' && i + 1 < n && text[i + 1] == '\"')
                    ++i;
            }
        }
        if(i < n)
            ++i;
        color = _stringsColor;
    } else if(!cerberusFile && c == '/' && i < n && text[i] == '/') {
        for(++i; i < n && text[i] != '\n'; ++i) {
        }
        if(i < n)
            ++i;
        color = _commentsColor;
    } else if(c == '\'') {
        if(cerberusFile) {
            for(; i < n && text[i] != '\n'; ++i) {
            }
            if(i < n)
                ++i;
            color = _commentsColor;
        } else {
            for(; i < n && text[i] != '\''; ++i) {
                if(text[i] == '\\' && i + 1 < n && text[i + 1] == '\'')
                    ++i;
            }
            if(i < n)
                ++i;
            color = _stringsColor;
        }
    } else
        color = _defaultColor;

    QString t = text.left(i);
    text = text.mid(i);
    return t;
}
