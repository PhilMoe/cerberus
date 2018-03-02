
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
    extraSelsEditor=0;
}




//***** CodeTreeItem *****

class CodeTreeItem : public QStandardItem{
public:
    CodeTreeItem():_data(0){
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

CodeEditor::CodeEditor( QWidget *parent ):QPlainTextEdit( parent ),_modified( 0 ),_capitalize( false ){
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

    _codeTreeModel=new QStandardItemModel( 0 );//this );
    _codeTreeModel->setSortRole(Qt::AscendingOrder);

    _codeTreeView=new QTreeView( 0 );
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
    doLineNumbers = Prefs::prefs()->getBool( "showLineNumbers" );
    doSortCodeBrowser = Prefs::prefs()->getBool( "sortCodeBrowser" );

    onPrefsChanged( "" );
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    flushExtraSels();
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

        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                QString number = QString::number(blockNumber + 1);
                painter.setPen(_highlighter->_keywordsColor);
                painter.drawText(0, top, lineNumberArea->width()-20, fontMetrics().height(),
                    Qt::AlignRight, number);
                // Paint modified markers
                BlockData *data = dynamic_cast<BlockData*>(block.userData());
                if (data) {
                    if (data->_modified == 1 && _modSignal==true) {
                        painter.fillRect(lineNumberArea->width()-2, top, lineNumberArea->width(), fontMetrics().height(), Qt::red ); //(document()->isModified()==1 ? Qt::red : Qt::green) );
                    }
                    bool bkmrk = (data && data->isBookmarked());
                    if( bkmrk ) {
                        painter.drawImage(lineNumberArea->width()-15, top, imgBookmark);
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

            //QColor lineColor = QColor(Qt::gray).lighter(40);
            QColor lineColor=Prefs::prefs()->getColor( "highlightColor" );

            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
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

void CodeEditor::onPrefsChanged( const QString &name ){

    QString t( name );

    Prefs *prefs=Prefs::prefs();

    if( t=="" || t=="backgroundColor" || t=="highlightColor" || t=="fontFamily" || t=="fontSize" || t=="tabSize" || t=="smoothFonts" || t=="highlightCurrLine" || t=="showLineNumbers" || t=="sortCodeBrowser" ){

        QColor bg=prefs->getColor( "backgroundColor" );
        QColor fg( 255-bg.red(),255-bg.green(),255-bg.blue() );

        QPalette p=palette();
        p.setColor( QPalette::Base,bg );
        p.setColor( QPalette::Text,fg );
        setPalette( p );

        QFont font;
        font.setFamily( prefs->getString( "fontFamily" ) );
        font.setPixelSize( prefs->getInt( "fontSize" ) );

        if( prefs->getBool( "smoothFonts" ) ){
            font.setStyleStrategy( QFont::PreferAntialias );
        }else{
            font.setStyleStrategy( QFont::NoAntialias );
        }

        doHighlightCurrLine = prefs->getBool("highlightCurrLine");
        doLineNumbers = prefs->getBool("showLineNumbers");
        doSortCodeBrowser = prefs->getBool("sortCodeBrowser");

        updateLineNumberAreaWidth(0);
        highlightCurrentLine();
        _highlighter->validateCodeTreeModel();


        setFont( font );

        QFontMetrics fm( font );
        setTabStopWidth( fm.width( 'X' )* prefs->getInt( "tabSize" ) );
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

void CodeEditor::keyPressEvent( QKeyEvent *e ){
    flushExtraSels();

    QTextCursor cursor=textCursor();
    QTextBlock block=cursor.block();
    bool hasSel=cursor.hasSelection();
    //BlockData *data = dynamic_cast<BlockData*>(block.userData());

    int key=e->key();


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
                        cursor.setPosition( block.position()+1,QTextCursor::KeepAnchor );
                        if( cursor.selectedText()!="\t" ) continue;
                        cursor.insertText( "" );
                    }

                }else{
                    for( int i=beg;i<=end;++i ){
                        QTextBlock block=document()->findBlockByNumber( i );
                        cursor.setPosition( block.position() );
                        cursor.insertText( "\t" );
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
            e=0;
        }
    }

    int cursorPos=textCursor().positionInBlock();

    if( e ) QPlainTextEdit::keyPressEvent( e );

    if( _cerberus && block.userState()==-1 ){

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

bool CodeEditor::findNext( const QString &findText,bool cased,bool wrap ){
    QTextDocument::FindFlags flags=0;
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

    QTextDocument::FindFlags flags=0;
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
    QIcon iconst( appPath+"/themes/"+theme+"/icons/editor/const.png" );
    QIcon iglob( appPath+"/themes/"+theme+"/icons/editor/global.png" );
    QIcon ifield( appPath+"/themes/"+theme+"/icons/editor/property.png" );
    QIcon imethod( appPath+"/themes/"+theme+"/icons/editor/method.png" );
    QIcon ifunc( appPath+"/themes/"+theme+"/icons/editor/function.png" );
    QIcon iclass(appPath+"/themes/"+theme+"/icons/editor/class.png" );
    QIcon iinterf( appPath+"/themes/"+theme+"/icons/editor/interface.png" );
    QIcon ienum( appPath+"/themes/"+theme+"/icons/editor/enumerate.png" );
    QIcon iother( appPath+"/themes/"+theme+"/icons/editor/other.png" );


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
            qDebug()<<"Highlighter::validateCodeTreeModel Block error!";
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
        item->setText(data->ident());
        //+QString::number(data->indent())

        QIcon icon = identIcon(data->decl());
        item->setIcon(icon);
        //item->setToolTip(data->decl()+" <b>"+data->ident()+"</b> "+data->block().text());
        item->setToolTip(data->block().text().trimmed()+" ("+QString::number(data->indent())+")");

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
        _console1Color=prefs->getColor("console1Color");
        _console2Color=prefs->getColor("console2Color");
        _console3Color=prefs->getColor("console3Color");
        _defaultColor=prefs->getColor("defaultColor");
        _numbersColor=prefs->getColor("numbersColor");
        _stringsColor=prefs->getColor("stringsColor");
        _identifiersColor=prefs->getColor("identifiersColor");
        _keywordsColor=prefs->getColor("keywordsColor");
        _commentsColor=prefs->getColor("commentsColor");
        _highlightColor=prefs->getColor("highlightColor");
        rehighlight();
    }
}

QString Highlighter::parseToke( QString &text,QColor &color ){
    if( !text.length() ) return "";

    int i=0,n=text.length();
    QChar c=text[i++];

    bool cerberusFile=_editor->isCerberus();

    if( c<=' ' ){
        while( i<n && text[i]<=' ' ) ++i;
    }else if( isAlpha(c) ){
        while( i<n && isIdent(text[i]) ) ++i;
        color=_identifiersColor;
        if( cerberusFile && keyWords().contains( text.left(i).toLower()  ) ) color=_keywordsColor;
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

    int i=0,pos=cursor.position();

    cursor.beginEditBlock();

    for(;;){
        QString t=parseToke( text,color );
        if( t.isEmpty() ) break;

        QString kw=keyWords().value( t.toLower() );

        if( !kw.isEmpty() && t!=kw ){
            int i0=block.position()+i;
            int i1=i0+t.length();
            cursor.setPosition( i0 );
            cursor.setPosition( i1,QTextCursor::KeepAnchor );
            cursor.insertText( kw );
        }

        i+=t.length();
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
            setCurrentBlockUserData( 0 );
            return;
        }
    }

    if( !_editor->isCode() ){
        setFormat( 0,text.length(),_defaultColor );
        setCurrentBlockUserData( 0 );
        return;
    }

    int indent=i;
    text=text.mid(i);

    int colst=0;
    QColor curcol=_defaultColor;

    QVector<QString> tokes;

    for(;;){

        QColor col=curcol;

        QString t=parseToke( text,col );
        if( t.isEmpty() ) break;

        if( t[0]>' ' ) tokes.push_back( t );

        if( col!=curcol ){
            setFormat( colst,i-colst,curcol );
            curcol=col;
            colst=i;
        }

        i+=t.length();
    }

    if( colst<n ) setFormat( colst,n-colst,curcol );

    if( _editor->isCerberus() ){
        //
        //Update user block data for code tree.
        //
        BlockData *data=0;

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
    _highlighter=0;
}

BlockData::~BlockData(){
    if( _highlighter ) _highlighter->remove( this );
}
