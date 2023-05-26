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
//
//  Changes:
//          2022-12-23: DAWLANE - Fixed an issue with tap stop width/distance and building with Qt 5.9
//          2022-11-02: DAWLANE - Move Highlighter and CodeTreeItem into highlighter.h and highlighter.cpp.
//                      Fixed completer tab and escape key bug.
//                      Updated or removed obsolete Qt member functions and macros.

// TODO: Update where possible to use the function pointer version of connect.

#include "codeeditor.h"
#include "highlighter.h"
#include "macros.h"
#include "mainwindow.h"
#include "prefs.h"

#include <QCoreApplication>
#include <QFile>
#include <QPainter>
#include <QRegularExpression>
#include <QStack>
#include <QTextEdit>
#include <QTextStream>

// A number of static variables and functions for dealing with selected text.
static CodeEditor *extraSelsEditor;
static QList<QTextEdit::ExtraSelection> extraSels;

static void flushExtraSels()
{
    if(!extraSelsEditor)
        return;
    extraSels.clear();
    extraSelsEditor->setExtraSelections(extraSels);
    extraSelsEditor = nullptr;
}

// Insensitive text comparison function.
bool caseInsensitiveLessThan(const QString &a, const QString &b)
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

//----------------------------------------------------------------------------------------------------------------------
//  LineNumber Area: DECLARATION/IMPLEMENTETION
//----------------------------------------------------------------------------------------------------------------------
class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor) {
        codeEditor = editor;
    }

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

//----------------------------------------------------------------------------------------------------------------------
//  CodeEditor: IMPLEMENTETION
//----------------------------------------------------------------------------------------------------------------------
CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent), completedAndSelected(false), _modified(0), _capitalize(false)
{

#ifdef Q_OS_MAC
    QString appPath = extractDir(
                          extractDir(extractDir(QCoreApplication::applicationDirPath())));
#else
    QString appPath = QCoreApplication::applicationDirPath();
#endif

    // Load in the setting from the preferences
    Prefs *prefs = Prefs::prefs();
    QString theme = "";
    theme = prefs->getString("theme");
    imgBookmark.load(appPath + "/themes/" + theme + "/icons/editor/Bookmark.png");
    _highlighter = new Highlighter(this);
    lineNumberArea = new LineNumberArea(this);

    // Create the code view and connect the signals and slots.
    _codeTreeModel = new QStandardItemModel(nullptr);
    _codeTreeModel->setSortRole(Qt::AscendingOrder);

    _codeTreeView = new QTreeView(nullptr);
    _codeTreeView->setHeaderHidden(true);
    _codeTreeView->setModel(_codeTreeModel);
    _codeTreeView->setFocusPolicy(Qt::NoFocus);
    _codeTreeView->setSortingEnabled(true);
    _modSignal = false;

#ifdef Q_OS_WIN
    _codeTreeView->setFrameStyle(QFrame::NoFrame);
    setFrameStyle(QFrame::NoFrame);
#endif

    connect(_codeTreeView, &QTreeView::clicked, this,
            &CodeEditor::onCodeTreeViewClicked);
    connect(this, &CodeEditor::textChanged, this, &CodeEditor::onTextChanged);
    connect(this, &CodeEditor::cursorPositionChanged, this,
            &CodeEditor::onCursorPositionChanged);
    connect(this, &CodeEditor::blockCountChanged, this,
            &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this,
            &CodeEditor::updateLineNumberArea);
    connect(Prefs::prefs(), &Prefs::prefsChanged, this,
            &CodeEditor::onPrefsChanged);

    // Set a number of internal variables for colours, tabs etc.
    setLineWrapMode(QPlainTextEdit::NoWrap);
    doHighlightCurrLine = Prefs::prefs()->getBool("highlightCurrLine");
    doHighlightCurrWord = Prefs::prefs()->getBool("highlightCurrWord");
    doHighlightBrackets = Prefs::prefs()->getBool("highlightBrackets");
    doLineNumbers = Prefs::prefs()->getBool("showLineNumbers");
    doSortCodeBrowser = Prefs::prefs()->getBool("sortCodeBrowser");
    _tabs4spaces = Prefs::prefs()->getBool("tabs4spaces");
    _tabSpaceText = " ";
    _tabSpaceText = _tabSpaceText.repeated(Prefs::prefs()->getInt("tabSize"));
    _capitalizeAPI = Prefs::prefs()->getBool("capitalizeAPI");

    onPrefsChanged("");
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    flushExtraSels();

    // Set up the code completer.
    model = new QStringListModel(this);
    model2 = new CompleterListModel(this);

    completer = new QCompleter(this);
    completer->setWidget(this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setModel(model);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(true);

    // NOTE: clazy analyser complains about this signature not being normalised.
    // A function pointer version of connect cannot be used here unless a new
    // insertCompletion function is implemented.
    connect(completer, SIGNAL(activated(const QString &)), this,
            SLOT(insertCompletion(const QString &)));

#ifdef Q_OS_MACOS
    static_cast<void>(new QShortcut(QKeySequence(Qt::META | Qt::Key_Space), this,
                                    SLOT(performCompletion())));
#else
    static_cast<void>(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Space), this,
                                    SLOT(performCompletion())));
#endif
}

// Clean up when an editor has been removed from the main windows central tab widget.
CodeEditor::~CodeEditor()
{
    flushExtraSels();
    delete _codeTreeView;
    delete _codeTreeModel;
}

//----------------------------------------------------------------------------------------------------------------------
//  CodeEditor: PUBLIC MEMEBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
// Calculate the size of the line number gutter
int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while(max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 0;
    if(doLineNumbers)
        space = 40 + fontMetrics().FONTMETRICS_WIDTH(QLatin1Char('9')) * digits;

    return space;
}

// Open a file into the editor's text widget
bool CodeEditor::open(const QString &path)
{
    QFile file(path);

    if(!file.open(QIODevice::ReadOnly))
        return false;

    evaluatefiletype(path);

    QTextStream stream(&file);
    STREAMS_UTF8 // MACRO: Only used if Qt version is less than 6.0.0
    setPlainText(stream.readAll());
    file.close();

    document()->setModified(false);
    _modified = 0;

    return true;
}

// Save the contents of the text widget to disk
bool CodeEditor::save(const QString &path)
{
    QFile file(path);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    evaluatefiletype(path);

    QTextStream stream(&file);
    STREAMS_UTF8 // MACRO: Only used if Qt version is less than 6.0.0
    stream << toPlainText();

    stream.flush();
    file.close();

    document()->setModified(false);
    _modified = 0;

    // flush all 'modified' marks
    QTextBlock block = document()->firstBlock();
    while(block.isValid()) {
        BlockData *data = dynamic_cast<BlockData *>(block.userData());
        if(data)
            data->setModified(0);
        block = block.next();
    }

    return true;
}

// Evaluate the file is to be text wrapped and the tool chain associated with it.
void CodeEditor::evaluatefiletype(const QString &path)
{
    _path = path;
    _fileType = extractExt(_path).toLower();
    QString t = ';' + _fileType + ';';

    _txt = textFileTypes.contains(t);
    _code = codeFileTypes.contains(t);
    _cerberus = _fileType == "cxs" || _fileType == "monkey";
    _monkey2 = _fileType == "mx2" || _fileType == "monkey2";

    if(_txt)
        setLineWrapMode(QPlainTextEdit::WidgetWidth);
    else
        setLineWrapMode(QPlainTextEdit::NoWrap);
}

// Move the text cursor to the line passed in the parameter.
void CodeEditor::gotoLine(int line)
{
    QTextBlock block = document()->findBlockByNumber(line);
    setTextCursor(QTextCursor(block));
    verticalScrollBar()->setValue(line);
    ensureCursorVisible();
}

// Highlights the current line the text cursor is on.
void CodeEditor::highlightLine(int line)
{
    flushExtraSels();

    QTextBlock block = document()->findBlockByLineNumber(line);
    if(block.isValid()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = Prefs::prefs()->getColor("highlightColor");

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);

        selection.cursor = QTextCursor(block);
        selection.cursor.clearSelection();

        extraSels.append(selection);
        setTextCursor(QTextCursor(block));
    }

    setExtraSelections(extraSels);
    extraSelsEditor = this;
}

//  Toggle's the commenting and removing commenting blocks of code.
void CodeEditor::commentUncommentBlock()
{
    // Get the text cursor position and if there is a selection block, then
    // correct the start and end of the selection. If there is no selection, then
    // exit the member function.
    QTextCursor c = textCursor();
    if(!c.hasSelection())
        return;
    int i1 = c.selectionStart();
    int i2 = c.selectionEnd();
    if(i1 > i2) {  // swap
        int tmp = i1;
        i1 = i2;
        i2 = tmp;
    }

    // Find the start and end of the text to edit.
    QTextBlock b = document()->findBlock(i1);
    QTextBlock end = document()->findBlock(i2);

    c.beginEditBlock();
    while(b.isValid()) {
        QString t = b.text();

        // Check if the line is commented and then remove it, else comment out the
        // line.
        if(t.startsWith('\'')) {
            c.setPosition(b.position());
            c.setPosition(b.position() + 1, QTextCursor::KeepAnchor);
            setTextCursor(c);
            insertPlainText("");
        } else {
            c.setPosition(b.position());
            setTextCursor(c);
            insertPlainText("'");
        }
        if(b == end)
            break;
        b = b.next();
    }
    c.endEditBlock();
}

// Toggle a bookmark on/off
void CodeEditor::bookmarkToggle()
{
    QTextBlock block = textCursor().block();
    if(block.isValid()) {
        BlockData *data = dynamic_cast<BlockData *>(block.userData());
        if(data)
            data->toggleBookmark();
    }
}

// Go back one in the list of bookmarks
void CodeEditor::bookmarkPrev()
{
    bookmarkFind(-1);
}

// Go forward one in the list of bookmarks
void CodeEditor::bookmarkNext()
{
    bookmarkFind(1);
}

// Locate the bookmark in the text editor
void CodeEditor::bookmarkFind(int dir, int start)
{
    int line = -1;
    QTextBlock block;
    if(start == -1)
        block = textCursor().block();
    else
        block = document()->findBlockByLineNumber(start);
    if(!block.isValid())
        return;

    int startFrom = block.blockNumber();
    block = (dir == 1 ? block.next() : block.previous());
    while(block.isValid()) {
        BlockData *data = dynamic_cast<BlockData *>(block.userData());
        if(data && data->isBookmarked()) {
            line = block.blockNumber();
            break;
        }
        block = (dir == 1 ? block.next() : block.previous());
    }

    // do cyclic search
    if(line < 0) {
        int i = (dir == 1 ? 0 : document()->blockCount() - 1);
        block = document()->findBlockByLineNumber(i);
        while(block.isValid() && block.blockNumber() != startFrom) {
            BlockData *data = dynamic_cast<BlockData *>(block.userData());
            if(data && data->isBookmarked()) {
                line = block.blockNumber();
                break;
            }
            block = (dir == 1 ? block.next() : block.previous());
        }
    }
    if(line >= 0)
        highlightLine(line);
}

// Locate a piece of text in the current editor.
bool CodeEditor::findNext(const QString &findText, bool cased, bool wrap)
{
    QTextDocument::FindFlags flags;
    if(cased)
        flags |= QTextDocument::FindCaseSensitively;

    setCenterOnScroll(true);

    bool found = find(findText, flags);
    if(!found && wrap) {
        QTextCursor cursor = textCursor();

        setTextCursor(QTextCursor(document()));

        found = find(findText, flags);
        if(!found)
            setTextCursor(cursor);
    }

    setCenterOnScroll(false);

    return found;
}

// Replace a piece of text in the current editor with a replacement
bool CodeEditor::replace(const QString &findText, const QString &replaceText, bool cased)
{
    Qt::CaseSensitivity cmpFlags =
        cased ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if(textCursor().selectedText().compare(findText, cmpFlags) != 0)
        return false;

    insertPlainText(replaceText);

    return true;
}

// Replace all text in the current editor with the replacement
int CodeEditor::replaceAll(const QString &findText, const QString &replaceText, bool cased, bool wrap)
{
    QTextDocument::FindFlags flags;
    if(cased)
        flags |= QTextDocument::FindCaseSensitively;

    if(wrap) {
        QTextCursor cursor = textCursor();
        setTextCursor(QTextCursor(document()));
        if(!find(findText, flags)) {
            setTextCursor(cursor);
            return 0;
        }
    } else {
        if(!find(findText, flags))
            return 0;
    }

    insertPlainText(replaceText);

    int n = 1;

    while(findNext(findText, cased, false)) {
        insertPlainText(replaceText);
        ++n;
    }

    return n;
}

// Work out the type of character the the text cursor is at.
QString CodeEditor::identAtCursor()
{
    QTextDocument *doc = document();
    int len = doc->characterCount();

    int pos = textCursor().position();

    while(pos >= 0 && pos < len && !isAlpha(doc->characterAt(pos)))
        --pos;
    if(pos < 0)
        return "";

    while(pos > 0 && isAlpha(doc->characterAt(pos - 1)))
        --pos;
    if(pos == len)
        return "";

    int start = pos;
    while(pos < len && isIdent(doc->characterAt(pos)))
        ++pos;
    if(pos == start)
        return "";

    QTextCursor cursor(doc);
    cursor.setPosition(start);
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    QString ident = cursor.selectedText();
    return ident;
}

// Renders the line gutter on the left.
void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if(doLineNumbers) {
        QPainter painter(lineNumberArea);
        painter.fillRect(event->rect(), Qt::NoBrush);

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top =
            (int)blockBoundingGeometry(block).translated(contentOffset()).top();
        int bottom = top + (int)blockBoundingRect(block).height();

        painter.setFont(this->font());
        int bmHeight = imgBookmark.height();
        while(block.isValid() && top <= event->rect().bottom()) {
            if(block.isVisible() && bottom >= event->rect().top()) {
                QString number = QString::number(blockNumber + 1);
                painter.setPen(_highlighter->getLineNumberColor());
                painter.drawText(0, top, lineNumberArea->width() - 20,
                                 fontMetrics().height(), Qt::AlignRight, number);

                // Paint modified markers
                BlockData *data = dynamic_cast<BlockData *>(block.userData());
                if(data) {
                    if(data->getModified() == 1 && _modSignal == true) {
                        painter.fillRect(lineNumberArea->width() - 2, top,
                                         lineNumberArea->width(), fontMetrics().height(),
                                         Qt::red);
                    }
                    bool bkmrk = (data && data->isBookmarked());
                    if(bkmrk)
                        painter.drawImage(lineNumberArea->width() - 15,
                                          top + ((fontMetrics().height() - bmHeight) / 2),
                                          imgBookmark);
                }
            }

            block = block.next();
            top = bottom;
            bottom = top + (int)blockBoundingRect(block).height();
            ++blockNumber;
        }
    }
    QWidget::paintEvent(event);
}

//----------------------------------------------------------------------------------------------------------------------
//  CodeEditor: PUBLIC MEMEBER SLOTS
//----------------------------------------------------------------------------------------------------------------------
// Set the text modified flag if any text has changed.
void CodeEditor::onTextChanged()
{
    if(document()->isModified())
        ++_modified;

    else {
        _modified = 0;

        // flush all 'modified' marks
        QTextBlock block = document()->firstBlock();
        while(block.isValid()) {
            BlockData *data = dynamic_cast<BlockData *>(block.userData());
            if(data)
                data->setModified(0);
            block = block.next();
        }
    }
    _modSignal = true;
    _highlighter->validateCodeTreeModel();
}

// If the text cursor has changes, then update the line to highlight.
void CodeEditor::onCursorPositionChanged()
{
    highlightCurrentLine();
    return;
}

// Any change selected preferences are automatically reflected in the editor.
void CodeEditor::onPrefsChanged(const QString &name)
{
    QString t(name);
    Prefs *prefs = Prefs::prefs();

    // Only update those preferences required.
    if(t == "" || t == "backgroundColor" || t == "highlightColor" ||
            t == "fontFamily" || t == "fontSize" || t == "tabSize" ||
            t == "smoothFonts" || t == "highlightCurrLine" ||
            t == "highlightCurrWord" || t == "highlightBrackets" ||
            t == "showLineNumbers" || t == "sortCodeBrowser" || t == "tabs4spaces" ||
            t == "capitalizeAPI") {

        QColor bg = prefs->getColor("backgroundColor");
        QColor fg(255 - bg.red(), 255 - bg.green(), 255 - bg.blue());

        QString cbg = bg.name();
        QString cfg = fg.name();

        QString s = "background:" + cbg + ";background-color:" + cbg +
                    ";color:" + cfg + ";";
        setStyleSheet(s);

        QFont font;
        font.setFamily(prefs->getString("fontFamily"));
        font.setPixelSize(prefs->getInt("fontSize"));

        if(prefs->getBool("smoothFonts"))
            font.setStyleStrategy(QFont::PreferAntialias);

        else
            font.setStyleStrategy(QFont::NoAntialias);

        doHighlightCurrLine = prefs->getBool("highlightCurrLine");
        doHighlightCurrWord = prefs->getBool("highlightCurrWord");
        doHighlightBrackets = prefs->getBool("highlightBrackets");
        doLineNumbers = prefs->getBool("showLineNumbers");
        doSortCodeBrowser = prefs->getBool("sortCodeBrowser");

        updateLineNumberAreaWidth(0);
        highlightCurrentLine();
        _highlighter->validateCodeTreeModel();

        setFont(font);
        QFontMetrics fm(font);
        SET_TABSTOP_WIDTH(fm.FONTMETRICS_WIDTH('X') * prefs->getInt("tabSize"));

        _tabs4spaces = prefs->getBool("tabs4spaces");
        _tabSpaceText = " ";
        _tabSpaceText = _tabSpaceText.repeated(prefs->getInt("tabSize"));
        _capitalizeAPI = prefs->getBool("capitalizeAPI");
    }
}

// Highlight the current line, including the current words and bracket pairs.
void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if(doHighlightCurrLine) {
        if(!isReadOnly()) {
            QTextEdit::ExtraSelection selection;

            QColor lineColor = Prefs::prefs()->getColor("highlightColor");

            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }
    }

    if(doHighlightCurrWord) {
        if(!isReadOnly()) {

            QTextEdit::ExtraSelection selection2;

            QColor lineColor2 = Prefs::prefs()->getColor("highlightColor");
            lineColor2 = lineColor2.lighter(130);
            selection2.format.setBackground(lineColor2);
            selection2.format.setProperty(QTextFormat::BlockFormat, true);
            selection2.cursor = textCursor();
            selection2.cursor.movePosition(QTextCursor::StartOfWord);
            selection2.cursor.movePosition(QTextCursor::EndOfWord,
                                           QTextCursor::KeepAnchor);
            extraSelections.append(selection2);

            QString _wordunderCursor = "";
            _wordunderCursor = selection2.cursor.selectedText();

            QTextBlock b = firstVisibleBlock();
            float bheight = blockBoundingRect(b).height();
            int areaHeight = rect().height();
            int visibleCount = (areaHeight / bheight) + 2;
            int n = visibleCount;
            int i = 0;

            // try to parse 3-screen height area up
            QTextBlock bbb = b;
            while(bbb.isValid() && i < visibleCount) {
                b = bbb;
                if(bbb.isVisible())
                    ++i;
                bbb = bbb.previous();
            }
            n += i;
            // down
            n += visibleCount;

            i = 0;

            while(b.isValid() && i < n) {
                QString s = b.text();
                int len = s.length();
                for(int k = 0; k < len; /**/) {
                    QChar c = s.at(k);
                    // skip non-Latin chars
                    bool bigc = (c >= 'A' && c <= 'Z');
                    bool smallc = (c >= 'a' && c <= 'z');
                    if(!bigc && !smallc) {
                        ++k;
                        continue;
                    }

                    selection2.cursor.setPosition(b.position() + k);
                    selection2.cursor.select(QTextCursor::WordUnderCursor);
                    QString t = selection2.cursor.selectedText();

                    if(t == _wordunderCursor)
                        extraSelections.append(selection2);
                    k += t.length() + 1;
                }
                if(b.isVisible())
                    ++i;
                b = b.next();
            }
        }
    }

    // Chec of brackets
    if(doHighlightBrackets) {

        if(!isReadOnly()) {
            QTextEdit::ExtraSelection selection3, selection4;
            QColor lineColor3 = Prefs::prefs()->getColor("highlightColor");
            lineColor3 = lineColor3.lighter();

            // Set the colour and how the selection is formatted.
            selection3.format.setBackground(lineColor3);
            selection3.format.setProperty(QTextFormat::BlockFormat, true);
            selection4.format.setBackground(lineColor3);
            selection4.format.setProperty(QTextFormat::BlockFormat, true);

            // Get the current text block and cursor position.
            QString text = textCursor().block().text();
            int cPos = textCursor().positionInBlock();

            // NOTE: Qt 6 doesn't allow out of bound access like previous versions of
            // Qt, so now have to make sure that the string is not empty and that the
            // string is larger than the cursor position returned.
            if(!text.isEmpty()) {

                // The curent position of the cursor should keep track of the character it's currently on and the one
                // behind,
                QChar c, d;
                if(text.size() > cPos) {
                    c = text[cPos];
                    if(cPos > 0)
                        d = text[cPos - 1];
                }
                if(text.size() == cPos)
                    if(cPos > 0)
                        d = text[cPos - 1];

                // Now the index of both the current and previous character needs to be worked out.
                int index_c = -1, index_d = -1;

                // If two brackets are close together, then work out both indexes.
                // The bracket not under the cursor is set to a darker colour shade.
                if((c == '(' || c == '[' || c == '<') && (d == '(' || d == '[' || d == '<')) {
                    index_c = indexOfClosedBracket(text, c, cPos + 1);
                    index_d = indexOfClosedBracket(text, d, index_c + 1);
                    selection4.format.setBackground(lineColor3.darker(120));
                }

                if((c == ')' || c == ']' || c == '>') && (d == ')' || d == ']' || d == '>')) {
                    index_c = indexOfOpenedBracket(text, c, cPos - 1);
                    index_d = indexOfOpenedBracket(text, d, index_c + 1);
                    selection4.format.setBackground(lineColor3.darker(120));
                }

                // Work out the indexes of the current al previous character.
                if(c == '(' || c == '[' || c == '<')
                    index_c = indexOfClosedBracket(text, c, cPos + 1);
                if(c == ')' || c == ']' || c == '>')
                    index_c = indexOfOpenedBracket(text, c, cPos - 1);
                if(d == '(' || d == '[' || d == '<')
                    index_d = indexOfClosedBracket(text, d, cPos);
                if(d == ')' || d == ']' || d == '>')
                    index_d = indexOfOpenedBracket(text, d, cPos - 2);

                // If index c has a non negative value, then process the bracket pair
                if(index_c != -1) {
                    // add bracket under cursor
                    selection3.cursor = textCursor();
                    int blockPos = textCursor().block().position();
                    selection3.cursor.setPosition(blockPos + cPos);
                    selection3.cursor.setPosition(blockPos + cPos + 1,
                                                  QTextCursor::KeepAnchor);
                    extraSelections.append(selection3);

                    // add its pair
                    selection3.cursor.setPosition(blockPos + index_c);
                    selection3.cursor.setPosition(blockPos + index_c + 1,
                                                  QTextCursor::KeepAnchor);
                    extraSelections.append(selection3);

                }

                // If index d has a non negative value, then process the bracket pair
                if(index_d != -1) {  // found a pair of brackets
                    // add bracket under cursor
                    selection4.cursor = textCursor();
                    int blockPos = textCursor().block().position();
                    selection4.cursor.setPosition(blockPos + cPos);
                    selection4.cursor.setPosition(blockPos + cPos - 1,
                                                  QTextCursor::KeepAnchor);
                    extraSelections.append(selection4);

                    // add its pair
                    selection4.cursor.setPosition(blockPos + index_d);
                    selection4.cursor.setPosition(blockPos + index_d + 1,
                                                  QTextCursor::KeepAnchor);
                    extraSelections.append(selection4);

                }
            }
        }
    }
    setExtraSelections(extraSelections);
}

// In the event of the code tree item being selected; then jump to that section.
void CodeEditor::onCodeTreeViewClicked(const QModelIndex &index)
{
    CodeTreeItem *item =
        dynamic_cast<CodeTreeItem *>(_codeTreeModel->itemFromIndex(index));
    if(!item)
        return;

    for(QTextBlock block = document()->firstBlock(); block.isValid();
            block = block.next()) {
        if(block.userData() == item->data()) {
            setCenterOnScroll(true);
            emit showCode(path(), block.blockNumber());
            setCenterOnScroll(false);
            return;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
//  CodeEditor: PROTECTED MEMEBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
// Update the line gutter in the event of a GUI resize.
void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

// Process a key press
void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    if(completedAndSelected && handledCompletedAndSelected(e))
        return;
    completedAndSelected = false;

    // If the completer menu is shown, then process the keyboard input.
    if(completer->popup()->isVisible()) {
        switch(e->key()) {
            case Qt::Key_Up:     // Fall through
            case Qt::Key_Down:   // Fall through
            case Qt::Key_Enter:  // Fall through
            case Qt::Key_Return: // Fall through
            case Qt::Key_Tab:    // Fall through
            case Qt::Key_Escape: {
                e->ignore();
                return;
            }
        }
    }

    flushExtraSels();

    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    bool hasSel = cursor.hasSelection();

    int key = e->key();

    // Set whether text is to be inserted or over written.
    if(key == Qt::Key_Insert) {
        if(overwriteMode() == false)
            setOverwriteMode(true);
        else
            setOverwriteMode(false);
    }

    // Insert tabulation or delete tabulation
    if(key == Qt::Key_Tab || key == Qt::Key_Backtab) {

        // If text has been selected, then do a block tabulation insert or delete.
        if(hasSel) {
            QTextBlock anchor = document()->findBlock(cursor.anchor());
            if(anchor != block) {
                int beg, end;
                if(block < anchor) {
                    beg = block.blockNumber();
                    end = document()->findBlock(cursor.anchor() - 1).blockNumber();
                } else {
                    beg = anchor.blockNumber();
                    end = document()->findBlock(cursor.position() - 1).blockNumber();
                }
                cursor.beginEditBlock();

                if(key == Qt::Key_Backtab || (e->modifiers() & Qt::ShiftModifier)) {
                    for(int i = beg; i <= end; ++i) {
                        QTextBlock block = document()->findBlockByNumber(i);
                        if(!block.length())
                            continue;
                        cursor.setPosition(block.position());
                        if(_tabs4spaces) {
                            cursor.setPosition(block.position() + 1, QTextCursor::KeepAnchor);
                            if(cursor.selectedText() != "\t")
                                continue;
                        } else {
                            cursor.setPosition(block.position() +
                                               Prefs::prefs()->getInt("tabSize"),
                                               QTextCursor::KeepAnchor);
                            if(cursor.selectedText() != _tabSpaceText)
                                continue;
                        }
                        cursor.insertText("");
                    }

                } else {
                    for(int i = beg; i <= end; ++i) {
                        QTextBlock block = document()->findBlockByNumber(i);
                        cursor.setPosition(block.position());
                        if(_tabs4spaces)
                            cursor.insertText("\t");

                        else
                            cursor.insertText(_tabSpaceText);
                    }
                }

                // does editing doc invalidated blocks?
                QTextBlock block0 = document()->findBlockByNumber(beg);
                QTextBlock block1 = document()->findBlockByNumber(end);
                cursor.setPosition(block0.position());
                cursor.setPosition(block1.position() + block1.length(),
                                   QTextCursor::KeepAnchor);
                cursor.endEditBlock();
                setTextCursor(cursor);
                e->accept();
                return;
            }
        }
    } else if(key == Qt::Key_Enter || key == Qt::Key_Return) {
        // auto indent
        if(!hasSel) {
            int i;
            QString text = block.text();
            for(i = 0; i < cursor.positionInBlock() && text[i] <= ' '; ++i) {
            }
            cursor.insertText('\n' + text.left(i));
            ensureCursorVisible();
            e->accept();
            e = nullptr;
        }
    }

    int cursorPos = textCursor().positionInBlock();

    if(e)
        QPlainTextEdit::keyPressEvent(e);

    if(completer->popup()->isVisible())
        performCompletion();

    if(_cerberus && block.userState() == -1) {

        if(key >= 32 && key <= 255) {
            if(!block.text().trimmed().startsWith("'"))
                // TODO: Check why this has an unused variable identifier and if declaration is not needed.
                QString ident = identAtCursor(false);
        }

        if(key >= 32 && key <= 255) {
            if((key >= Qt::Key_A && key <= Qt::Key_Z) ||
                    (key >= Qt::Key_0 && key <= Qt::Key_9) ||
                    (key == Qt::Key_Underscore)) {
                _capitalize = true;
                return;
            }
            if(!_capitalize)
                return;
        } else if(block == textCursor().block()) {
            if(textCursor().positionInBlock() != cursorPos)
                _capitalize = false;
            return;
        }

        _highlighter->capitalize(block, textCursor());

        _capitalize = false;
    }
}

// Returns the identifier at the current cursor position.
QString CodeEditor::identAtCursor(bool fullWord)
{
    QTextCursor c = textCursor();
    QString s;

    if(fullWord) {
        c.select(QTextCursor::WordUnderCursor);
        s = c.selectedText();
    } else {
        QString text = c.block().text();
        int i0 = c.positionInBlock();
        int i = i0 - 1;
        while(i >= 0 && isIdent(text[i]))
            --i;
        ++i;
        s = text.mid(i, i0 - i);
    }

    if(s.isEmpty())
        return "";

    return (isAlpha(s[0]) ? s : "");
}

//----------------------------------------------------------------------------------------------------------------------
//  CodeEditor: PRIVATE MENBER SLOTS
//----------------------------------------------------------------------------------------------------------------------
// The next two member functions deal with the line number gutter.
void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if(dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if(rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

// The next two member functions deal with locating brackets.
int CodeEditor::indexOfClosedBracket(const QString &text, const QChar &sourceBracket, int findFrom)
{
    QChar pairChar;
    if(sourceBracket == '(')
        pairChar = ')';
    else if(sourceBracket == '[')
        pairChar = ']';
    else if(sourceBracket == '<')
        pairChar = '>';
    else
        return -1;

    int len = text.length();
    int counter = 1; // one must be already opened outside this function

    for(int k = findFrom; k < len; ++k) {
        QChar c = text[k];
        if(c == sourceBracket)
            ++counter;

        else if(c == pairChar) {
            --counter;
            if(counter == 0)
                return k;
        }
    }
    return -1;
}

int CodeEditor::indexOfOpenedBracket(const QString &text,
                                     const QChar &sourceBracket, int findFrom)
{
    QChar pairChar;
    if(sourceBracket == ')')
        pairChar = '(';
    else if(sourceBracket == ']')
        pairChar = '[';
    else if(sourceBracket == '>')
        pairChar = '<';
    else
        return -1;

    int counter = 1; // one must be already closed outside this function

    for(int k = findFrom; k >= 0; --k) {
        QChar c = text[k];
        if(c == sourceBracket)
            ++counter;

        else if(c == pairChar) {
            --counter;
            if(counter == 0)
                return k;
        }
    }
    return -1;
}

// TODO: Figure out what this actually is for.
bool CodeEditor::handledCompletedAndSelected(QKeyEvent *event)
{
    completedAndSelected = false;
    QTextCursor cursor = textCursor();
    switch(event->key()) {
        case Qt::Key_Enter: // Fall through
        case Qt::Key_Return:
            cursor.clearSelection();
            break;
        case Qt::Key_Escape:
            cursor.removeSelectedText();
            break;
        default:
            return false;
    }
    setTextCursor(cursor);
    event->accept();
    return true;
}

// The next four member functions deal with code completion, with the first using the current text as a
// starting point
void CodeEditor::performCompletion()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);

    const QString completionPrefix = cursor.selectedText();
    if(!completionPrefix.isEmpty() && completionPrefix.length() >= 3)
        performCompletion(completionPrefix);
}

// Generates the pop up menu items of words matching the current text.
void CodeEditor::performCompletion(const QString &completionPrefix)
{
    populateModel(completionPrefix);

    if(completionPrefix != completer->completionPrefix()) {
        completer->setCompletionPrefix(completionPrefix);
        completer->popup()->setCurrentIndex(
            completer->completionModel()->index(0, 0));
    }

    if(completer->completionCount() == 1)
        insertCompletion(completer->currentCompletion(), true);
    else {
        QRect rect = cursorRect();
        rect.setWidth(completer->popup()->sizeHintForColumn(0) +
                      completer->popup()->verticalScrollBar()->sizeHint().width() +
                      50);
        completer->complete(rect);
    }
}

// Generate the list of words that match the criteria for the first set of characters.
void CodeEditor::populateModel(const QString &completionPrefix)
{
    // NOTE: QRegExp has been retired for version 6, but has been added to the Qt5Compatibility module.
    // QRegularExpression has been available since Qt5, so using that instead.
    static QRegularExpression regexp("\\W+");
    QStringList strings = toPlainText().split(regexp);
    strings.removeAll(completionPrefix);
    if(MAIN_WINDOW != nullptr) {
        for(int i = 0; i < MAIN_WINDOW->CompleteListCount(); ++i) {
            QString command = MAIN_WINDOW->CompleteListItem(i);
            strings << command;
        }
    }
    strings.removeDuplicates();

    // NOTE: I'm sure that all modern C++ compilers now use stl, so the qSort macro is no longer needed.
    std::sort(strings.begin(), strings.end(), caseInsensitiveLessThan);
    model->setStringList(strings);
    model2->setList(strings);
}

// Inserts the word selected into the editor.
void CodeEditor::insertCompletion(const QString &completion, bool singleWord)
{
    QTextCursor cursor = textCursor();
    int insertionPosition = cursor.position();

    cursor.select(QTextCursor::WordUnderCursor);
    cursor.insertText(completion);
    if(singleWord) {
        cursor.setPosition(insertionPosition);
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        completedAndSelected = true;
    }
    setTextCursor(cursor);
}

//----------------------------------------------------------------------------------------------------------------------
//  CompleterListModel: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
// See the QAbstractListModel for a detailed explanation of the overridden member functions.
// But in general the CompleterListModel class uses a QStringList for all keywords.
int CompleterListModel::rowCount(const QModelIndex & /* parent */) const
{
    return _commandList.count();
}

QVariant CompleterListModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= _commandList.size())
        return QVariant();

    if(role == Qt::DisplayRole)
        return _commandList.at(index.row());
    else
        return QVariant();
}

QVariant CompleterListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if(orientation == Qt::Horizontal)
        return QString("Column %1").arg(section);
    else
        return QString("Row %1").arg(section);
}

Qt::ItemFlags CompleterListModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool CompleterListModel::setData(const QModelIndex &index,
                                 const QVariant &value, int role)
{
    if(index.isValid() && role == Qt::EditRole) {
        _commandList.replace(index.row(), value.toString());
        emit dataChanged(index, index);
        return true;
    }

    return false;
}

bool CompleterListModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    beginInsertRows(QModelIndex(), position, position + rows - 1);

    for(int row = 0; row < rows; ++row)
        _commandList.insert(position, "");

    endInsertRows();
    return true;
}

bool CompleterListModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    for(int row = 0; row < rows; ++row)
        _commandList.removeAt(position);

    endRemoveRows();
    return true;
}

void CompleterListModel::setList(QStringList cmdlist)
{
    _commandList << cmdlist;
}

//----------------------------------------------------------------------------------------------------------------------
//  BlockData: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
// NOTE: Only the destructor is here instead of being grouped with the class declaration due to highlighter being
// implemented here.
BlockData::~BlockData()
{
    if(_highlighter)
        _highlighter->remove(this);
}
