
/*
Ted, simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "codeeditor.h"
#include "prefs.h"
#include <QtGui>

static CodeEditor *extraSelsEditor;
static QList<QTextEdit::ExtraSelection> extraSels;

static void flushExtraSels(){
    if( !extraSelsEditor ) return;
    extraSels.clear();
    extraSelsEditor->setExtraSelections( extraSels );
    extraSelsEditor=nullptr;
}


bool caseInsensitiveLessThan(const QString &a, const QString &b)
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}


//***** CodeTreeItem *****

class CodeTreeItem : public QStandardItem{
public:
    CodeTreeItem():_data(nullptr){
        setEditable( false );
    }

    void setData( BlockData *data ){
        _data=data;
    }

    BlockData *data(){
        return _data;
    }

private:
    BlockData *_data;
};

//***** LineNumber Area *****
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

//***** CodeEditor *****

CodeEditor::CodeEditor( QWidget *parent, MainWindow *wnd ):QPlainTextEdit( parent ), completedAndSelected(false) ,_modified( 0 ),_capitalize( false ), _mainWnd( wnd ) {
    QString appPath=QCoreApplication::applicationDirPath();
    #ifdef Q_OS_MAC
        appPath = extractDir(extractDir(extractDir(appPath)));
    #endif
    //QSettings settings;

    Prefs *prefs=Prefs::prefs();
    QString theme = "";
    theme = prefs->getString( "theme" );

    imgBookmark.load(appPath+"/themes/"+theme+"/icons/editor/Bookmark.png");

    _highlighter=new Highlighter( this );

    lineNumberArea = new LineNumberArea(this);

    _codeTreeModel=new QStandardItemModel( nullptr );//this );
    _codeTreeModel->setSortRole(Qt::AscendingOrder);

    _codeTreeView=new QTreeView( nullptr );
    _codeTreeView->setHeaderHidden( true );
    _codeTreeView->setModel( _codeTreeModel );
    _codeTreeView->setFocusPolicy( Qt::NoFocus );
    _codeTreeView->setSortingEnabled(true);

    _modSignal = false;

#ifdef Q_OS_WIN
    _codeTreeView->setFrameStyle( QFrame::NoFrame );
    setFrameStyle( QFrame::NoFrame );
#endif

    connect( _codeTreeView,SIGNAL(clicked(QModelIndex)),SLOT(onCodeTreeViewClicked(QModelIndex)) );

    connect( this,SIGNAL(textChanged()),SLOT(onTextChanged()) );
    connect( this,SIGNAL(cursorPositionChanged()),SLOT(onCursorPositionChanged()) );

    connect( Prefs::prefs(),SIGNAL(prefsChanged(const QString&)),SLOT(onPrefsChanged(const QString&)) );

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));

    setLineWrapMode( QPlainTextEdit::NoWrap );

    doHighlightCurrLine = Prefs::prefs()->getBool( "highlightCurrLine" );
    doHighlightCurrWord = Prefs::prefs()->getBool( "highlightCurrWord" );
    doHighlightBrackets = Prefs::prefs()->getBool( "highlightBrackets" );
    doLineNumbers = Prefs::prefs()->getBool( "showLineNumbers" );
    doSortCodeBrowser = Prefs::prefs()->getBool( "sortCodeBrowser" );
    _tabs4spaces = Prefs::prefs()->getBool( "tabs4spaces" );
    _tabSpaceText = " ";
    _tabSpaceText = _tabSpaceText.repeated(Prefs::prefs()->getInt( "tabSize" ));
    _capitalizeAPI = Prefs::prefs()->getBool( "capitalizeAPI" );

    onPrefsChanged( "" );
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    flushExtraSels();

    // Setting the completer
    model = new QStringListModel(this);
    model2 = new CompleterListModel(this);

    completer = new QCompleter(this);
    completer->setWidget(this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setModel(model);
    completer->setModelSorting(
            QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(true);

    connect(completer, SIGNAL(activated(const QString&)),
            this, SLOT(insertCompletion(const QString&)));
    (void) new QShortcut(QKeySequence(tr("Ctrl+Space", "Complete")),
                         this, SLOT(performCompletion()));

}

CodeEditor::~CodeEditor(){

    flushExtraSels();

    delete _codeTreeView;

    delete _codeTreeModel;

    //delete _highlighter;
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 0;
    if ( doLineNumbers ) {
        space = 40 + fontMetrics().width(QLatin1Char('9')) * digits;
    }

    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    //if ( doLineNumbers ) {
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    //}
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    //if ( doLineNumbers ) {
        if (dy)
            lineNumberArea->scroll(0, dy);
        else
            lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

        if (rect.contains(viewport()->rect()))
            updateLineNumberAreaWidth(0);
    //}
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    //if ( doLineNumbers ) {
        lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    //} else {
    //    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), 20, cr.height()));
    //}
}

void CodeEditor::bookmarkToggle() {
    QTextBlock block = textCursor().block();
    if( block.isValid() ) {
        //BlockData *data = BlockData::data(block, true);
        BlockData *data = dynamic_cast<BlockData*>(block.userData());
        if (data) {
            data->toggleBookmark();
        }
    }
}

void CodeEditor::bookmarkPrev() {
    bookmarkFind(-1);
}

void CodeEditor::bookmarkNext() {
    bookmarkFind(1);
}

void CodeEditor::bookmarkFind(int dir , int start) {
    int line = -1;
    QTextBlock block;
    if(start == -1)
        block = textCursor().block();
    else
        block = document()->findBlockByLineNumber(start);
    if( !block.isValid() )
        return;
    int startFrom = block.blockNumber();
    block = (dir == 1 ? block.next() : block.previous());
    while( block.isValid() ) {
        //BlockData *data = BlockData::data(block);
        BlockData *data = dynamic_cast<BlockData*>(block.userData());
        if( data && data->isBookmarked() ) {
            line = block.blockNumber();
            break;
        }
        block = (dir == 1 ? block.next() : block.previous());
    }
    //do cyclic search
    if(line < 0) {
        int i = (dir == 1 ? 0 : document()->blockCount()-1);
        block = document()->findBlockByLineNumber(i);
        while( block.isValid() && block.blockNumber() != startFrom ) {
            BlockData *data = dynamic_cast<BlockData*>(block.userData());
            if( data && data->isBookmarked() ) {
                line = block.blockNumber();
                break;
            }
            block = (dir == 1 ? block.next() : block.previous());
        }
    }
    if(line >= 0) {
        /*
        QTextBlock b = document()->findBlockByNumber( line );
        CodeItem *i = CodeAnalyzer::scopeAt(b);
        if(i) {
            unfoldBlock(i->block());
            if(i->parent())
                unfoldBlock(i->parent()->block());
        }
        */
        highlightLine(line);
        //highlightLine(line, HlCaretRowCentered);
    }
}


void CodeEditor::commentUncommentBlock() {
    QTextCursor c = textCursor();
    if(!c.hasSelection())
        return;
    int i1 = c.selectionStart();
    int i2 = c.selectionEnd();
    if(i1 > i2) {//swap
        int tmp = i1;
        i1 = i2;
        i2 = tmp;
    }
    QTextBlock b = document()->findBlock(i1);
    QTextBlock end = document()->findBlock(i2);
    c.beginEditBlock();
    while(b.isValid()) {
        QString t = b.text();
        if(t.startsWith('\'')) {
            c.setPosition(b.position());
            c.setPosition(b.position()+1,QTextCursor::KeepAnchor);
            setTextCursor(c);
            insertPlainText("");
        }
        else {
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


void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{

    if ( doLineNumbers ) {
        QPainter painter(lineNumberArea);
        painter.fillRect(event->rect(), Qt::NoBrush);

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
        int bottom = top + (int) blockBoundingRect(block).height();

        painter.setFont(this->font());
        int bmHeight = imgBookmark.height();
        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                QString number = QString::number(blockNumber + 1);
                painter.setPen(_highlighter->_lineNumberColor);
                painter.drawText(0, top, lineNumberArea->width()-20, fontMetrics().height(), Qt::AlignRight, number);
                // Paint modified markers
                BlockData *data = dynamic_cast<BlockData*>(block.userData());
                //if (data) {
                //    painter.drawText(0, top, lineNumberArea->width()-20, fontMetrics().height(), Qt::AlignRight,  QString::number(data->indent()));
                //}
                if (data) {
                    if (data->_modified == 1 && _modSignal==true) {
                        painter.fillRect(lineNumberArea->width()-2, top, lineNumberArea->width(), fontMetrics().height(), Qt::red ); //(document()->isModified()==1 ? Qt::red : Qt::green) );
                    }
                    bool bkmrk = (data && data->isBookmarked());
                    if( bkmrk ) {
                        painter.drawImage(lineNumberArea->width()-15, top+((fontMetrics().height()-bmHeight)/2), imgBookmark);
                    }                }
            }

            block = block.next();
            top = bottom;
            bottom = top + (int) blockBoundingRect(block).height();
            ++blockNumber;

        }
    }
    QWidget::paintEvent(event);
}


bool CodeEditor::open( const QString &path ){

    QFile file( path );

    if( !file.open( QIODevice::ReadOnly ) ) return false;

    rename( path );

    QTextStream stream( &file );

    stream.setCodec( "UTF-8" );

    setPlainText( stream.readAll() );

    file.close();

    document()->setModified( false );
    _modified=0;

    return true;
}

bool CodeEditor::save( const QString &path ){

    QFile file( path );

    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) return false;

    rename( path );

    QTextStream stream( &file );

    stream.setCodec( "UTF-8" );

    stream<<toPlainText();

    stream.flush();
    file.close();

    document()->setModified( false );
    _modified=0;

    //flush all 'modified' marks
    QTextBlock block = document()->firstBlock();
    while( block.isValid() ) {
        //BlockData *data = BlockData::data(block);
        BlockData *data = dynamic_cast<BlockData*>(block.userData());
        if(data) {
            //data->_modified = false;
            data->setModified(0);
        }
        block = block.next();
    }

    return true;
}

void CodeEditor::rename( const QString &path ){

    _path=path;

    _fileType=extractExt( _path ).toLower();
    QString t=';'+_fileType+';';

    _txt=textFileTypes.contains( t );
    _code=codeFileTypes.contains( t );
    _cerberus=_fileType=="cxs" || _fileType=="monkey" || _fileType=="mx2" || _fileType=="monkey2";
    _monkey2=_fileType=="mx2" || _fileType=="monkey2";

    if( _txt ){
        setLineWrapMode( QPlainTextEdit::WidgetWidth );
    }else{
        setLineWrapMode( QPlainTextEdit::NoWrap );
    }
}

void CodeEditor::gotoLine( int line ){

    QTextBlock block=document()->findBlockByNumber( line );

    setTextCursor( QTextCursor( block ) );

//    verticalScrollBar()->setValue( line );

    ensureCursorVisible();
}

void CodeEditor::highlightCurrentLine(){
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (doHighlightCurrLine) {
        if (!isReadOnly()) {
            QTextEdit::ExtraSelection selection;

            QColor lineColor=Prefs::prefs()->getColor( "highlightColor" );

            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }
    }

    if (doHighlightCurrWord) {
        if (!isReadOnly()) {

            QTextEdit::ExtraSelection selection2;

            QColor lineColor2=Prefs::prefs()->getColor( "highlightColor" );
            lineColor2 = lineColor2.lighter(130);
            selection2.format.setBackground(lineColor2);
            selection2.format.setProperty(QTextFormat::BlockFormat, true);
            selection2.cursor = textCursor();
            selection2.cursor.movePosition(QTextCursor::StartOfWord);
            selection2.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            extraSelections.append(selection2);

            QString _wordunderCursor ="";
            _wordunderCursor = selection2.cursor.selectedText();

            QTextBlock b = firstVisibleBlock();
            float bheight = blockBoundingRect(b).height();
            int areaHeight = rect().height();
            int visibleCount = (areaHeight/bheight)+2;
            int n = visibleCount;
            int i = 0;

            // try to parse 3-screen height area
            // up
            QTextBlock bbb = b;
            while (bbb.isValid() && i < visibleCount) {
                b = bbb;
                if (bbb.isVisible())
                    ++i;
                bbb = bbb.previous();
            }
            n += i;
            // down
            n += visibleCount;

            i = 0;

            //int len = _wordunderCursor.length();

            while (b.isValid() && i < n) {
                QString s = b.text();
                int len = s.length();
                for (int k = 0; k < len; /**/) {
                    QChar c = s.at(k);
                    // skip non-latin chars
                    bool bigc = (c >= 'A' && c <= 'Z');
                    bool smallc = (c >= 'a' && c <= 'z');
                    if (!bigc && !smallc) {
                        ++k;
                        continue;
                    }
                    QTextCursor cursor = QTextCursor(b);
                     selection2.cursor.setPosition(b.position()+k);
                     selection2.cursor.select(QTextCursor::WordUnderCursor);
                    QString t =  selection2.cursor.selectedText();

                    if (t == _wordunderCursor) {
                        extraSelections.append(selection2);
                    }
                    k += t.length()+1;
                }
                if (b.isVisible())
                    ++i;
                b = b.next();
            }
        }
    }

    if ( doHighlightBrackets ) {
        if (!isReadOnly()) {
            QTextEdit::ExtraSelection selection3;
            QColor lineColor3=Prefs::prefs()->getColor( "highlightColor" );
            lineColor3 = lineColor3.lighter();
            selection3.format.setBackground(lineColor3);
            selection3.format.setProperty(QTextFormat::BlockFormat, true);

            // check for brackets
            QString text = textCursor().block().text();
            int cPos = textCursor().positionInBlock();
            QChar c = text[cPos];
            int index = -1;
            if (c == '(' || c == '[' || c == '<')
                index = indexOfClosedBracket(text, c, cPos+1);
            else if (c == ')' || c == ']' || c == '>')
                index = indexOfOpenedBracket(text, c, cPos-1);
            if (index != -1) {// found a pair of brackets
                // add bracket under cursor
                selection3.cursor = textCursor();
                int blockPos = textCursor().block().position();
                selection3.cursor.setPosition(blockPos+cPos);
                selection3.cursor.setPosition(blockPos+cPos+1, QTextCursor::KeepAnchor);
                extraSelections.append(selection3);
                // add its pair
                selection3.cursor.setPosition(blockPos+index);
                selection3.cursor.setPosition(blockPos+index+1, QTextCursor::KeepAnchor);
                extraSelections.append(selection3);
            }
        }
    }
    setExtraSelections(extraSelections);

}

void CodeEditor::highlightLine( int line ){
    flushExtraSels();

    QTextBlock block=document()->findBlockByLineNumber( line );

    if( block.isValid() ){

        QTextEdit::ExtraSelection selection;

        QColor lineColor=Prefs::prefs()->getColor( "highlightColor" );

        selection.format.setBackground( lineColor );
        selection.format.setProperty( QTextFormat::FullWidthSelection,true );

        selection.cursor=QTextCursor( block );
        selection.cursor.clearSelection();

        extraSels.append( selection );
        setTextCursor( QTextCursor( block ) );
    }

    setExtraSelections( extraSels );
    extraSelsEditor=this;
}

int CodeEditor::indexOfClosedBracket(const QString &text, const QChar &sourceBracket, int findFrom)
{
    QChar pairChar;
    if (sourceBracket == '(')
        pairChar = ')';
    else if (sourceBracket == '[')
        pairChar = ']';
    else if (sourceBracket == '<')
        pairChar = '>';
    else
        return -1;

    int len = text.length();
    int counter = 1;// one must be already opened outside this func

    for (int k = findFrom; k < len; ++k) {
        QChar c = text[k];
        if (c == sourceBracket) {
            ++counter;
        } else if (c == pairChar) {
            --counter;
            if (counter == 0) {
                return k;
            }
        }
    }
    return -1;
}

int CodeEditor::indexOfOpenedBracket(const QString &text, const QChar &sourceBracket, int findFrom)
{
    QChar pairChar;
    if (sourceBracket == ')')
        pairChar = '(';
    else if (sourceBracket == ']')
        pairChar = '[';
    else if (sourceBracket == '>')
        pairChar = '<';
    else
        return -1;

    int counter = 1;// one must be already closed outside this func

    for (int k = findFrom; k >= 0; --k) {
        QChar c = text[k];
        if (c == sourceBracket) {
            ++counter;
        } else if (c == pairChar) {
            --counter;
            if (counter == 0) {
                return k;
            }
        }
    }
    return -1;
}


void CodeEditor::onPrefsChanged( const QString &name ){

    QString t( name );

    Prefs *prefs=Prefs::prefs();

    if( t=="" || t=="backgroundColor" || t=="highlightColor" || t=="fontFamily" || t=="fontSize" || t=="tabSize" || t=="smoothFonts"
              || t=="highlightCurrLine" || t=="highlightCurrWord" || t=="highlightBrackets" || t=="showLineNumbers" || t=="sortCodeBrowser" || t=="tabs4spaces" || t=="capitalizeAPI" ){

        QColor bg=prefs->getColor( "backgroundColor" );
        QColor fg( 255-bg.red(),255-bg.green(),255-bg.blue() );

        /*
        QPalette p=palette();
        p.setColor( QPalette::Base,bg );
        p.setColor( QPalette::Text,fg );
        setPalette( p );
        */
        QString cbg = bg.name();
        QString cfg = fg.name();

        //if ( isCerberus()){
            QString s = "background:"+cbg+";background-color:"+cbg+";color:"+cfg+";";
            setStyleSheet(s);
        //}



        QFont font;
        font.setFamily( prefs->getString( "fontFamily" ) );
        font.setPixelSize( prefs->getInt( "fontSize" ) );

        if( prefs->getBool( "smoothFonts" ) ){
            font.setStyleStrategy( QFont::PreferAntialias );
        }else{
            font.setStyleStrategy( QFont::NoAntialias );
        }

        doHighlightCurrLine = prefs->getBool("highlightCurrLine");
        doHighlightCurrWord = prefs->getBool("highlightCurrWord");
        doHighlightBrackets = prefs->getBool("highlightBrackets");
        doLineNumbers = prefs->getBool("showLineNumbers");
        doSortCodeBrowser = prefs->getBool("sortCodeBrowser");

        updateLineNumberAreaWidth(0);
        highlightCurrentLine();
        _highlighter->validateCodeTreeModel();


        setFont( font );

        QFontMetrics fm( font );
        setTabStopWidth( fm.width( 'X' )* prefs->getInt( "tabSize" ) );
        _tabs4spaces = prefs->getBool("tabs4spaces");
        _tabSpaceText = " ";
        _tabSpaceText = _tabSpaceText.repeated(prefs->getInt( "tabSize" ));
        _capitalizeAPI = prefs->getBool("capitalizeAPI");
    }
}

void CodeEditor::onCursorPositionChanged(){
    highlightCurrentLine();
    return;
}

void CodeEditor::onTextChanged(){
    if( document()->isModified() ){
        ++_modified;
    }else{
        _modified=0;

        //flush all 'modified' marks
        QTextBlock block = document()->firstBlock();
        while( block.isValid() ) {
            //BlockData *data = BlockData::data(block);
            BlockData *data = dynamic_cast<BlockData*>(block.userData());
            if(data) {
                data->setModified(0);
            }
            block = block.next();
        }
    }
    _modSignal = true;
    _highlighter->validateCodeTreeModel();
}

void CodeEditor::onCodeTreeViewClicked( const QModelIndex &index ){
    CodeTreeItem *item=dynamic_cast<CodeTreeItem*>( _codeTreeModel->itemFromIndex( index ) );
    if( !item ) return;

    for( QTextBlock block=document()->firstBlock();block.isValid();block=block.next() ){
        if( block.userData()==item->data() ){
            setCenterOnScroll( true );
            emit showCode( path(),block.blockNumber() );
            setCenterOnScroll( false );
            return;
        }
    }
}

QString CodeEditor::identAtCursor(bool fullWord) {
    QTextCursor c = textCursor();
    QString s;
    if(fullWord) {
        c.select(QTextCursor::WordUnderCursor);
        s = c.selectedText();
    }
    else {
        QString text = c.block().text();
        int i0 = c.positionInBlock();
        int i = i0-1;
        while( i >= 0 && isIdent(text[i])) {
            --i;
        }
        ++i;
        s = text.mid(i, i0-i);
    }

    return (isAlpha(s[0]) ? s : "");
}

void CodeEditor::keyPressEvent( QKeyEvent *e ){
    if (completedAndSelected && handledCompletedAndSelected(e))
        return;
    completedAndSelected = false;

    if (completer->popup()->isVisible()) {
        switch (e->key()) {
            case Qt::Key_Up:      // Fallthrough
            case Qt::Key_Down:    // Fallthrough
            case Qt::Key_Enter:   // Fallthrough
            case Qt::Key_Return:  // Fallthrough
            case Qt::Key_Escape: e->ignore(); return;
           // default: completer->popup()->hide(); break;
        }
    }

    flushExtraSels();

    QTextCursor cursor=textCursor();
    QTextBlock block=cursor.block();
    bool hasSel=cursor.hasSelection();
    //BlockData *data = dynamic_cast<BlockData*>(block.userData());

    int key=e->key();

    if(key==Qt::Key_Insert){
        if (overwriteMode()==false){
            setOverwriteMode(true);
        }
        else {
            setOverwriteMode(false);
        }
    }

    if( key==Qt::Key_Tab || key==Qt::Key_Backtab ){
        //block tab/untab
        if( hasSel ){
            QTextBlock anchor=document()->findBlock( cursor.anchor() );
            if( anchor!=block ){
                int beg,end;
                if( block<anchor ){
                    beg=block.blockNumber();
                    end=document()->findBlock( cursor.anchor()-1 ).blockNumber();
                }else{
                    beg=anchor.blockNumber();
                    end=document()->findBlock( cursor.position()-1 ).blockNumber();
                }
                cursor.beginEditBlock();
                if( key==Qt::Key_Backtab || (e->modifiers() & Qt::ShiftModifier) ){
                    for( int i=beg;i<=end;++i ){
                        QTextBlock block=document()->findBlockByNumber( i );
                        if( !block.length() ) continue;
                        cursor.setPosition( block.position() );
                        if ( _tabs4spaces ){
                            cursor.setPosition( block.position()+1,QTextCursor::KeepAnchor );
                            if( cursor.selectedText()!="\t" ) continue;
                        } else {
                            cursor.setPosition( block.position()+Prefs::prefs()->getInt( "tabSize" ),QTextCursor::KeepAnchor );
                            if( cursor.selectedText()!=_tabSpaceText ) continue;
                        }
                        cursor.insertText( "" );
                    }

                }else{
                    for( int i=beg;i<=end;++i ){
                        QTextBlock block=document()->findBlockByNumber( i );
                        cursor.setPosition( block.position() );
                        if ( _tabs4spaces ){
                            cursor.insertText( "\t" );
                        } else {
                            cursor.insertText( _tabSpaceText );
                        }

                    }
                }
                //does editing doc invalidated blocks?
                QTextBlock block0=document()->findBlockByNumber( beg );
                QTextBlock block1=document()->findBlockByNumber( end );
                cursor.setPosition( block0.position() );
                cursor.setPosition( block1.position()+block1.length(),QTextCursor::KeepAnchor );
                cursor.endEditBlock();
                setTextCursor( cursor );
                e->accept();
                return;
            }
        }
    }else if( key==Qt::Key_Enter || key==Qt::Key_Return ){
        //auto indent
        if( !hasSel ){
            int i;
            QString text=block.text();
            for( i=0;i<cursor.positionInBlock() && text[i]<=' ';++i ){}
            cursor.insertText( '\n'+text.left(  i ) );
            ensureCursorVisible();
            e->accept();
            e=nullptr;
        }
    }

    int cursorPos=textCursor().positionInBlock();

    if( e ) QPlainTextEdit::keyPressEvent( e );

    if (completer->popup()->isVisible()) performCompletion();

    if( _cerberus && block.userState()==-1 ){

        if( key>=32 && key<=255 ){
            if (!block.text().trimmed().startsWith("'")) {
                QString ident = identAtCursor(false);
                if (ident.length() >= 3 || key == 56) {
                    //performCompletion();
                }
            }
        }

        if( key>=32 && key<=255 ){
            if( (key>=Qt::Key_A && key<=Qt::Key_Z) || (key>=Qt::Key_0 && key<=Qt::Key_9) || (key==Qt::Key_Underscore) ){
                _capitalize=true;
                return;
            }
            if( !_capitalize ) return;
        }else if( block==textCursor().block() ){
            if( textCursor().positionInBlock()!=cursorPos ) _capitalize=false;
            return;
        }

        _highlighter->capitalize( block,textCursor() );

        _capitalize=false;
        /*

        //auto capitalize
        if( key>=32 && key<=255 ){
            if( (key>=Qt::Key_A && key<=Qt::Key_Z) || (key>=Qt::Key_0 && key<=Qt::Key_9) || (key==Qt::Key_Underscore) ){
                _lastChar=true;
                return;
            }
            if( !lastChar ) return;
        }else{
            if( block==textCursor().block() ) return;
        }

        _highlighter->capitalize( block,textCursor() );
        */

    }


}

bool CodeEditor::handledCompletedAndSelected(QKeyEvent *event)
{
    completedAndSelected = false;
    QTextCursor cursor = textCursor();
    switch (event->key()) {
        case Qt::Key_Enter:  // Fallthrough
        case Qt::Key_Return: cursor.clearSelection(); break;
        case Qt::Key_Escape: cursor.removeSelectedText(); break;
        default: return false;
    }
    setTextCursor(cursor);
    event->accept();
    return true;
}

void CodeEditor::performCompletion()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);

    const QString completionPrefix = cursor.selectedText();

    if (!completionPrefix.isEmpty() && completionPrefix.length() >= 3 ) {
        performCompletion(completionPrefix);
    } else if (completionPrefix.isEmpty()) {
        //performCompletion("");
    }
}


void CodeEditor::performCompletion(const QString &completionPrefix)
{
    populateModel(completionPrefix);

    if (completionPrefix != completer->completionPrefix()) {
        completer->setCompletionPrefix(completionPrefix);
        completer->popup()->setCurrentIndex(
                completer->completionModel()->index(0, 0));
    }

    if (completer->completionCount() == 1)
        insertCompletion(completer->currentCompletion(), true);
    else {
        QRect rect = cursorRect();
        rect.setWidth(completer->popup()->sizeHintForColumn(0) +
                completer->popup()->verticalScrollBar()->
                sizeHint().width()+50);
        completer->complete(rect);
    }
}

void CodeEditor::populateModel(const QString &completionPrefix)
{
    QStringList strings = toPlainText().split(QRegExp("\\W+"));
    strings.removeAll(completionPrefix);
    if ( _mainWnd != nullptr) {
        for( int i=0;i < _mainWnd->_completeList.count();++i ){

            QString command=_mainWnd->_completeList.at( i );
            strings << command;
        }
    }
    strings.removeDuplicates();
    qSort(strings.begin(), strings.end(), caseInsensitiveLessThan);
    model->setStringList(strings);
    model2->setList(strings);
}


void CodeEditor::insertCompletion(const QString &completion,
                               bool singleWord)
{
    QTextCursor cursor = textCursor();
    //int numberOfCharsToComplete = completion.length() -
    //        completer->completionPrefix().length();
    int insertionPosition = cursor.position();
    //cursor.insertText(completion.right(numberOfCharsToComplete));
    cursor.select(QTextCursor::WordUnderCursor);
    cursor.insertText(completion);
    if (singleWord) {
        cursor.setPosition(insertionPosition);
        cursor.movePosition(QTextCursor::EndOfWord,
                            QTextCursor::KeepAnchor);
        completedAndSelected = true;
    }
    setTextCursor(cursor);
}


bool CodeEditor::findNext( const QString &findText,bool cased,bool wrap ){

    QTextDocument::FindFlags flags=nullptr;
    if( cased ) flags|=QTextDocument::FindCaseSensitively;

    setCenterOnScroll( true );

    bool found=find( findText,flags );

    if( !found && wrap ){

        QTextCursor cursor=textCursor();

        setTextCursor( QTextCursor( document() ) );

        found=find( findText,flags );

        if( !found ) setTextCursor( cursor );
    }

    setCenterOnScroll( false );

    return found;
}

bool CodeEditor::replace( const QString &findText,const QString &replaceText,bool cased ){

    Qt::CaseSensitivity cmpFlags=cased ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if( textCursor().selectedText().compare( findText,cmpFlags )!=0 ) return false;

    insertPlainText( replaceText );

    return true;
}

int CodeEditor::replaceAll( const QString &findText,const QString &replaceText,bool cased,bool wrap ){

    QTextDocument::FindFlags flags=nullptr;
    if( cased ) flags|=QTextDocument::FindCaseSensitively;

    if( wrap ){
        QTextCursor cursor=textCursor();
        setTextCursor( QTextCursor( document() ) );
        if( !find( findText,flags ) ){
            setTextCursor( cursor );
            return 0;
        }
    }else{
        if( !find( findText,flags ) ) return 0;
    }

    insertPlainText( replaceText );

    int n=1;

    while( findNext( findText,cased,false ) ){
        insertPlainText( replaceText );
        ++n;
    }

    return n;
}

QString CodeEditor::identAtCursor(){

    QTextDocument *doc=document();
    int len=doc->characterCount();

    int pos=textCursor().position();

    while( pos>=0 && pos<len && !isAlpha( doc->characterAt(pos) ) ){
        --pos;
    }
    if( pos<0 ) return "";

    while( pos>0 && isAlpha( doc->characterAt(pos-1) ) ){
        --pos;
    }
    if( pos==len ) return "";

    int start=pos;
    while( pos<len && isIdent( doc->characterAt(pos) ) ){
        ++pos;
    }
    if( pos==start ) return "";

    QTextCursor cursor( doc );
    cursor.setPosition( start );
    cursor.setPosition( pos,QTextCursor::KeepAnchor );
    QString ident=cursor.selectedText();
    return ident;
}

//***** Highlighter *****

#define KW(X) _keyWords.insert( QString(X).toLower(),X )

QMap<QString,QString> Highlighter::_keyWords;
QMap<QString,QString> Highlighter::_keyWords2;
QMap<QString,QString> Highlighter::_keyWords3;

Highlighter::Highlighter( CodeEditor *editor ):QSyntaxHighlighter( editor->document() ),_editor( editor ){

    if( _keyWords.isEmpty() ){
        const QString &kws=
            "Void;Strict;Public;Private;Protected;Friend;Property;"
            "Bool;Int;Float;String;Array;Object;Mod;Continue;Exit;"
            "Include;Import;Module;Extern;"
            "New;Self;Super;Eachin;True;False;Null;Not;"
            "Extends;Abstract;Final;Native;Select;Case;Default;"
            "Const;Local;Global;Field;Method;Function;Class;Interface;Implements;Enumerate;"
            "And;Or;Shl;Shr;End;If;Then;Else;Elseif;Endif;While;Wend;Repeat;Until;Forever;For;To;Step;Next;Return;Inline;"
            "Try;Catch;Throw;Throwable;"
            "Print;Error;Alias";

        const QString &kws2=
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

        QStringList bits=kws.split( ";" );
        for( int i=0;i<bits.size();++i ){
            _keyWords.insert( bits.at(i).toLower(),bits.at(i) );
        }

        bits=kws2.split( ";" );
        for( int i=0;i<bits.size();++i ){
            _keyWords2.insert( bits.at(i).toLower(),bits.at(i) );
        }

    }

    if( _keyWords3.isEmpty() ){
        for( int i=0;i<_editor->_mainWnd->_completeList.count();++i ){
            _keyWords3.insert( _editor->_mainWnd->_completeList.at(i).toLower(),_editor->_mainWnd->_completeList.at(i) );
        }
    }

    connect( Prefs::prefs(),SIGNAL(prefsChanged(const QString&)),SLOT(onPrefsChanged(const QString&)) );

    onPrefsChanged( "" );
}

Highlighter::~Highlighter(){
    QSetIterator<BlockData*> it( _blocks );
    while( it.hasNext() ){
        it.next()->invalidate();
    }
}

QIcon Highlighter::identIcon( const QString &ident ) {
    QString appPath=QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    appPath = extractDir(extractDir(extractDir(appPath)));
#endif
    //QSettings settings;

    Prefs *prefs=Prefs::prefs();
    QString theme = "";
    theme = prefs->getString( "theme" );

    //static
    static QIcon iconst( appPath+"/themes/"+theme+"/icons/editor/const.png" );
    static QIcon iglob( appPath+"/themes/"+theme+"/icons/editor/global.png" );
    static QIcon ifield( appPath+"/themes/"+theme+"/icons/editor/property.png" );
    static QIcon imethod( appPath+"/themes/"+theme+"/icons/editor/method.png" );
    static QIcon ifunc( appPath+"/themes/"+theme+"/icons/editor/function.png" );
    static QIcon iclass(appPath+"/themes/"+theme+"/icons/editor/class.png" );
    static QIcon iinterf( appPath+"/themes/"+theme+"/icons/editor/interface.png" );
    static QIcon ienum( appPath+"/themes/"+theme+"/icons/editor/enumerate.png" );
    static QIcon iother( appPath+"/themes/"+theme+"/icons/editor/other.png" );


    QString s = ident;//.toLower();
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


void Highlighter::insert( BlockData *data ){
    _blocks.insert( data );
    _blocksDirty=true;
}

void Highlighter::remove( BlockData *data ){
    _blocks.remove( data );
    _blocksDirty=true;
}

void Highlighter::validateCodeTreeModel(){
    if( !_blocksDirty ) return;

    QStandardItem *root=_editor->_codeTreeModel->invisibleRootItem();

    int row=0;
    QStandardItem *parent=root;

    QStack<int> rowStack;
    QStack<QStandardItem*> parentStack;

    int indent=0x7fff;

    for( QTextBlock block=_editor->document()->firstBlock();block.isValid();block=block.next() ){

        BlockData *data=dynamic_cast<BlockData*>( block.userData() );
        if( !data ) continue;

        if( !_blocks.contains( data ) ){
            continue;
        }

        if (data->_code == 1) {
            continue;
        }

        int level=0;
        if( data->indent()<=indent ){
            indent=data->indent();
            level=0;
        }else{
            level=1;
        }
        while( rowStack.size()>level ){
            parent->setRowCount( row );
            parent=parentStack.pop();
            row=rowStack.pop();
        }

        CodeTreeItem *item=dynamic_cast<CodeTreeItem*>( parent->child( row++ ) );
        if( !item ){
            item=new CodeTreeItem;
            parent->appendRow( item );
        }

        item->setData( data );
        item->setText( data->ident() );
        //+QString::number(data->indent())

        QIcon icon = identIcon(data->decl());
        item->setIcon(icon);
        //item->setToolTip(data->decl()+" <b>"+data->ident()+"</b> "+data->block().text());
        item->setToolTip(data->block().text().trimmed());

        rowStack.push( row );
        parentStack.push( parent );
        row=0;
        parent=item;
    }

    for(;;){
        parent->setRowCount( row );
        if( parent==root ) break;
        parent=parentStack.pop();
        row=rowStack.pop();
    }

    _blocksDirty=false;

    if ( _editor->doSortCodeBrowser ) {
        _editor->_codeTreeModel->sort(0);
        _editor->_codeTreeView->clearSelection();
    }

}

void Highlighter::onPrefsChanged( const QString &name ){
    QString t(name);

    if( t=="" || t.endsWith( "Color" ) ){
        Prefs *prefs=Prefs::prefs();
        _backgroundColor=prefs->getColor( "backgroundColor" );
        _lineNumberColor=prefs->getColor("lineNumberColor");
        _console1Color=prefs->getColor("console1Color");
        _console2Color=prefs->getColor("console2Color");
        _console3Color=prefs->getColor("console3Color");
        _console4Color=prefs->getColor("console4Color");
        _defaultColor=prefs->getColor("defaultColor");
        _numbersColor=prefs->getColor("numbersColor");
        _stringsColor=prefs->getColor("stringsColor");
        _identifiersColor=prefs->getColor("identifiersColor");
        _keywordsColor=prefs->getColor("keywordsColor");
        _keywords2Color=prefs->getColor("keywords2Color");
        _commentsColor=prefs->getColor("commentsColor");
        _highlightColor=prefs->getColor("highlightColor");
        rehighlight();
    }
}

QString Highlighter::parseToke( QString &text,QColor &color, QString &prevText ){
    if( !text.length() ) return "";
//qDebug() << text;
    int i=0,n=text.length();
    QChar c=text[i++];

    bool cerberusFile=_editor->isCerberus();

    if( c<=' ' ){
        while( i<n && text[i]<=' ' ) ++i;
    }else if( isAlpha(c) ){
        while( i<n && isIdent(text[i]) ) ++i;
        color=_identifiersColor;
        if ((prevText != "class") && (prevText != "field") && (prevText != "global") && (prevText != "local") && (prevText != "(") && (prevText != ",")) {
            if( cerberusFile &&  keyWords().contains( text.left(i).toLower() ) ) {
                color=_keywordsColor;
            } else if( cerberusFile &&  keyWords3().contains( text.left(i).toLower() ) ) color=_keywords2Color;
        }

    }else if( c=='0' && !cerberusFile ){
        if( i<n && text[i]=='x' ){
            for( ++i;i<n && isHexDigit( text[i] );++i ){}
        }else{
            for( ;i<n && isOctDigit( text[i] );++i ){}
        }
        color=_numbersColor;
    }else if( isDigit(c) || (c=='.' && i<n && isDigit(text[i])) ){
        bool flt=(c=='.');
        while( i<n && isDigit(text[i]) ) ++i;
        if( !flt && i<n && text[i]=='.' ){
            ++i;
            flt=true;
            while( i<n && isDigit(text[i]) ) ++i;
        }
        if( i<n && (text[i]=='e' || text[i]=='E') ){
            flt=true;
            if( i<n && (text[i]=='+' || text[i]=='-') ) ++i;
            while( i<n && isDigit(text[i]) ) ++i;
        }
        color=_numbersColor;
    }else if( c=='%' && cerberusFile && i<n && isBinDigit( text[i] ) ){
        for( ++i;i<n && isBinDigit( text[i] );++i ){}
        color=_numbersColor;
    }else if( c=='$' && cerberusFile && i<n && isHexDigit( text[i] ) ){
        for( ++i;i<n && isHexDigit( text[i] );++i ){}
        color=_numbersColor;
    }else if( c=='\"' ){
        if( cerberusFile ){
            for( ;i<n && text[i]!='\"';++i ){}
        }else{
            for( ;i<n && text[i]!='\"';++i ){
                if( text[i]=='\\' && i+1<n && text[i+1]=='\"' ) ++i;
            }
        }
        if( i<n ) ++i;
        color=_stringsColor;
    }else if( !cerberusFile && c=='/' && i<n && text[i]=='/' ){
        for( ++i;i<n && text[i]!='\n';++i ){}
        if( i<n ) ++i;
        color=_commentsColor;
    }else if( c=='\'' ){
        if( cerberusFile ){
            for( ;i<n && text[i]!='\n';++i ){}
            if( i<n ) ++i;
            color=_commentsColor;
        }else{
            for( ;i<n && text[i]!='\'';++i ){
                if( text[i]=='\\' && i+1<n && text[i+1]=='\'' ) ++i;
            }
            if( i<n ) ++i;
            color=_stringsColor;
        }
    }else{
        color=_defaultColor;
    }
    QString t=text.left(i);
    text=text.mid(i);
    return t;
}

bool Highlighter::capitalize( const QTextBlock &block,QTextCursor cursor ){

    QString text=block.text();
    QColor color;
    QString prevToken = "";
    int i=0,pos=cursor.position();

    cursor.beginEditBlock();

    for(;;){
        QString t=parseToke( text,color, prevToken );
        if( t.isEmpty() ) break;
        if ((prevToken != "class") && (prevToken != "field") && (prevToken != "local") && (prevToken != "global") && (prevToken != "(") && (prevToken != ",")) {
            //qDebug() << lastToken << ":" << t;
            QString kw=keyWords().value( t.toLower() );
            if ((_editor->_capitalizeAPI) && (keyWords().value( t ).isEmpty())){
                QString kw3=keyWords3().value( t.toLower() );

                if ( kw.isEmpty() ) kw = kw3;
            }
            if( !kw.isEmpty() && t!=kw ){
                int i0=block.position()+i;
                int i1=i0+t.length();
                cursor.setPosition( i0 );
                cursor.setPosition( i1,QTextCursor::KeepAnchor );
                cursor.insertText( kw );
            }
        }

        i+=t.length();
        if (t != " ")
            prevToken = t.toLower();
    }

    cursor.endEditBlock();

    cursor.setPosition( pos );

    return true;
}

void Highlighter::highlightBlock( const QString &ctext ){
    QString text=ctext;

    int i=0,n=text.length();
    while( i<n && text[i]<=' ' ) ++i;

    if( _editor->isCerberus() ){

        //Handle Cerberus block comments

        int st=previousBlockState();

        if( i<n && text[i]=='#' ){

            int i0=i+1;
            while( i0<n && text[i0]<=' ' ) ++i0;

            int i1=i0;
            while( i1<n && isIdent(text[i1]) ) ++i1;

            QString t=text.mid( i0,i1-i0 ).toLower();

            if( t=="rem" ){
                if( !++st ) st=1;
            }else if( t=="if" && st>0 ){
                ++st;
            }else if( (t=="end" || t=="endif") && st>0 ){
                --st;
            }else if( !st ){
                st=-1;
            }
        }else if( !st ){
            st=-1;
        }

        setCurrentBlockState( st );

        if( st!=-1 ){
            setFormat( 0,text.length(),_commentsColor );
            setCurrentBlockUserData( nullptr );
            return;
        }
    }

    if( !_editor->isCode() ){
        setFormat( 0,text.length(),_defaultColor );
        setCurrentBlockUserData( nullptr );
        return;
    }

    int indent=i;
    text=text.mid(i);

    int colst=0;
    QColor curcol=_defaultColor;

    QVector<QString> tokes;
    QString prevToken = "";
    for(;;){

        QColor col=curcol;

        QString t=parseToke( text,col, prevToken );
        if( t.isEmpty() ) break;

        if( t[0]>' ' ) tokes.push_back( t );

        if( col!=curcol ){
            setFormat( colst,i-colst,curcol );
            curcol=col;
            colst=i;
        }
        if (t != " ")
            prevToken = t.toLower();
        i+=t.length();
    }

    if( colst<n ) setFormat( colst,n-colst,curcol );

    if( _editor->isCerberus() ){
        //
        //Update user block data for code tree.
        //
        BlockData *data=nullptr;

        QString decl=tokes.size()>0 ? tokes[0].toLower() : "";
        QString ident=tokes.size()>1 ? tokes[1] : "";

        if( (decl=="field" || decl=="method") ){
            //if (indent==0) ++indent;
        }

        if( (decl=="global" || decl=="class" || decl=="interface") ){
            //indent = 0;
        }

        if( (decl=="class" || decl=="interface" || decl=="method" || decl=="function" || decl=="field" || decl=="const" || decl=="global" || decl=="enumerate") && !ident.isEmpty() ){
            QTextBlock block=currentBlock();
            data=dynamic_cast<BlockData*>( currentBlockUserData() );
            if( data && data->block()==block && data->decl()==decl && data->ident()==ident && data->indent()==indent ){
            }else{
                data=new BlockData( this,block,decl,ident,indent );
                setCurrentBlockUserData( data );
                insert( data );
            }
        }else{
            //setCurrentBlockUserData( 0 );

            QTextBlock block=currentBlock();
            data=dynamic_cast<BlockData*>( currentBlockUserData() );
            if( data && data->block()==block && data->decl()==decl && data->ident()==ident && data->indent()==indent ){
            }else{
                data=new BlockData( this,block,decl,ident,indent );
                data->_code = 1;
                setCurrentBlockUserData( data );
                insert( data );
            }
        }
        if ( data && _editor->_modSignal == true ) {
            data->setModified(1);
        }
    }
}

//***** CompleterListModel *****

int CompleterListModel::rowCount(const QModelIndex & /* parent */) const
{
    return _commandList.count();
}

QVariant CompleterListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= _commandList.size())
        return QVariant();

    if (role == Qt::DisplayRole)
        return _commandList.at(index.row());
    else
        return QVariant();
}

QVariant CompleterListModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return QString("Column %1").arg(section);
    else
        return QString("Row %1").arg(section);
}

Qt::ItemFlags CompleterListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool CompleterListModel::setData(const QModelIndex &index,
                              const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {

        _commandList.replace(index.row(), value.toString());
        emit dataChanged(index, index);
        return true;
    }

    return false;
}

bool CompleterListModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row = 0; row < rows; ++row) {
        _commandList.insert(position, "");
    }

    endInsertRows();
    return true;

}

bool CompleterListModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row = 0; row < rows; ++row) {
        _commandList.removeAt(position);
    }

    endRemoveRows();
    return true;

}

void CompleterListModel::setList(QStringList cmdlist){
    _commandList << cmdlist;
}


//***** BlockUserData *****

BlockData::BlockData( Highlighter *highlighter,const QTextBlock &block,const QString &decl,const QString &ident,int indent )
:_highlighter( highlighter ),_block( block ),_decl( decl ),_ident( ident ),_indent(indent){
    _modified = 0;
    _marked = false;
    _code = 0;
    /*
    QMessageBox msgBox;
    //msgBox.setText("current: "+QDir::currentPath());
    //msgBox.exec();
    msgBox.setText("xxxx");
    msgBox.exec();
    */
}

void BlockData::invalidate(){
    _highlighter=nullptr;
}

BlockData::~BlockData(){
    if( _highlighter ) _highlighter->remove( this );
}
