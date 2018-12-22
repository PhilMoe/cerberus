
#if _WIN32

#define _usew 1

static HWND focHwnd;

static void beginPanel(){
	focHwnd=GetFocus();
}

static void endPanel(){
	SetFocus( focHwnd );
}

static int panel( String title,String text,int flags ){
	beginPanel();
	int n=MessageBoxW( GetActiveWindow(),text.ToCString<WCHAR>(),title.ToCString<WCHAR>(),flags );
	endPanel();
	return n;
}

static WCHAR *tmpWString( String str ){
	WCHAR *p=(WCHAR*)malloc( str.Length()*2+2 );
	memcpy( p,str.Data(),str.Length()*2 );
	p[str.Length()]=0;
	return p;
}

void bbNotify( String title,String text,int serious ){
	int flags=(serious ? MB_ICONWARNING : MB_ICONINFORMATION)|MB_OK|MB_APPLMODAL|MB_TOPMOST;
	panel( title,text,flags );
}

int bbConfirm( String title,String text,int serious ){
	int flags=(serious ? MB_ICONWARNING : MB_ICONINFORMATION)|MB_OKCANCEL|MB_APPLMODAL|MB_TOPMOST;
	int n=panel( title,text,flags );
	if( n==IDOK ) return 1;
	return 0;
}

int bbProceed( String title,String text,int serious ){
	int flags=(serious ? MB_ICONWARNING : MB_ICONINFORMATION)|MB_YESNOCANCEL|MB_APPLMODAL|MB_TOPMOST;
	int n=panel( title,text,flags );
	if( n==IDYES ) return 1;
	if( n==IDNO ) return 0;
	return -1;
}

int bbRequestColor( void ){
	CHOOSECOLOR cc; // common dialog box structure
	static COLORREF acrCustClr[16]; // array of custom colors
	HWND hwnd; // owner window
	HBRUSH hbrush; // brush handle
	static DWORD rgbCurrent; // initial color selection
	// Initialize CHOOSECOLOR
	ZeroMemory(&cc, sizeof(CHOOSECOLOR));
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = GetActiveWindow();
	cc.lpCustColors = (LPDWORD) acrCustClr;
	cc.rgbResult = rgbCurrent;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	if (ChooseColor(&cc)==TRUE) {
		//hbrush = CreateSolidBrush(cc.rgbResult);
		rgbCurrent = cc.rgbResult;
		return rgbCurrent;
	}
	return -1;
}

String bbRequestFile( String title,String exts,int save,String path ){

	String file,dir;
	path=path.Replace( "/","\\" );
		
	int i=path.FindLast( "\\" );
	if( i!=-1 ){
		dir=path.Slice( 0,i );
		file=path.Slice( i+1 );
	}else{
		file=path;
	}

	if( file.Length()>MAX_PATH ) return "";
	
	if( exts.Length() ){
		if( exts.Find( ":" )==-1 ){
			exts=String( "Files\0*." )+exts;
		}else{
			exts=exts.Replace( ":",String( "\0*.",3 ) );
		}
		exts=exts.Replace( ";",String( "\0",1 ) );
		exts=exts.Replace( ",",";*." )+String( "\0",1 );
	}

	WCHAR buf[MAX_PATH+1],*p;
	memcpy( buf,file.Data(),file.Length()*2 );
	buf[file.Length()]=0;

	OPENFILENAMEW of={sizeof(of)};
	
	of.hwndOwner=GetActiveWindow();
	of.lpstrTitle=tmpWString( title );
	of.lpstrFilter=tmpWString( exts );
	of.lpstrFile=buf;
	of.lpstrInitialDir=dir.Length() ? tmpWString( dir ) : 0;
	of.nMaxFile=MAX_PATH;
	of.Flags=OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
		
	beginPanel();
	
	String str;
	
	if( save ){
		of.lpstrDefExt=L"";
		of.Flags|=OFN_OVERWRITEPROMPT;
		if( GetSaveFileNameW( &of ) ){
			str=String( buf );
		}
	}else{
		of.Flags|=OFN_FILEMUSTEXIST;
		if( GetOpenFileNameW( &of ) ){
			str=String( buf );
		}
	}
	
	endPanel();
	
	free( (void*)of.lpstrTitle );
	free( (void*)of.lpstrFilter );
	free( (void*)of.lpstrInitialDir );
	
	return str;
}

static int CALLBACK BrowseForFolderCallbackW( HWND hwnd,UINT uMsg,LPARAM lp,LPARAM pData ){
	wchar_t szPath[MAX_PATH];
	switch( uMsg ){
	case BFFM_INITIALIZED:
		SendMessageW( hwnd,BFFM_SETSELECTIONW,TRUE,pData );
		break;
	case BFFM_SELCHANGED: 
		if( SHGetPathFromIDListW( (LPITEMIDLIST)lp,szPath ) ){
			SendMessageW( hwnd,BFFM_SETSTATUSTEXTW,0,(LPARAM)szPath );
		}
		break;
	}
	return 0;
}

static int CALLBACK BrowseForFolderCallbackA( HWND hwnd,UINT uMsg,LPARAM lp,LPARAM pData ){
	char szPath[MAX_PATH];
	switch( uMsg ){
	case BFFM_INITIALIZED:
		SendMessageA( hwnd,BFFM_SETSELECTIONA,TRUE,pData );
		break;
	case BFFM_SELCHANGED: 
		if( SHGetPathFromIDListA( (LPITEMIDLIST)lp,szPath ) ){
			SendMessageA( hwnd,BFFM_SETSTATUSTEXTA,0,(LPARAM)szPath );
		}
		break;
	}
	return 0;
}

String bbRequestDir( String title,String dir ){

	dir=dir.Replace( "/","\\" );

	LPMALLOC shm;
	BROWSEINFOW bi={0};
	
	WCHAR buf[MAX_PATH],*p;
	GetFullPathNameW( dir.ToCString<WCHAR>(),MAX_PATH,buf,&p );
	
	bi.hwndOwner=GetActiveWindow();
	bi.lpszTitle=tmpWString( title );
	bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
	bi.lpfn=BrowseForFolderCallbackW;
	bi.lParam=(LPARAM)buf;
	
	beginPanel();

	String str;	
	if( ITEMIDLIST *idlist=SHBrowseForFolderW( &bi ) ){
		SHGetPathFromIDListW( idlist,buf );
		str=String( buf );
		//SHFree( idlist );	//?!?
	}
	
	endPanel();
	
	free( (void*)bi.lpszTitle );

	return str;
}

#elif __APPLE__ && __OBJC__

typedef int (*AlertPanel)( 
	NSString *title,
	NSString *msg,
	NSString *defaultButton,
	NSString *alternateButton,
	NSString *otherButton );

static NSWindow *keyWin;

static void beginPanel(){
	keyWin=[NSApp keyWindow];
	if( !keyWin ) [NSApp activateIgnoringOtherApps:YES];
}

static void endPanel(){
	if( keyWin ) [keyWin makeKeyWindow];
}

void bbNotify( String title,String text,int serious ){

	AlertPanel panel=(AlertPanel) ( serious ? (void*)NSRunCriticalAlertPanel : (void*)NSRunAlertPanel );
	
	beginPanel();
	
	panel( title.ToNSString(),text.ToNSString(),@"OK",0,0 );
	
	endPanel();
}

int bbConfirm( String title,String text,int serious ){

	AlertPanel panel=(AlertPanel) ( serious ? (void*)NSRunCriticalAlertPanel : (void*)NSRunAlertPanel );
	
	beginPanel();
	
	int n=panel( title.ToNSString(),text.ToNSString(),@"OK",@"Cancel",0 );

	endPanel();
	
	switch( n ){
	case NSAlertDefaultReturn:return 1;
	}
	return 0;
}

int bbProceed( String title,String text,int serious ){

	AlertPanel panel=(AlertPanel) ( serious ? (void*)NSRunCriticalAlertPanel : (void*)NSRunAlertPanel );
	
	beginPanel();
	
	int n=panel( title.ToNSString(),text.ToNSString(),@"Yes",@"No",@"Cancel" );
	
	endPanel();
	
	switch( n ){
	case NSAlertDefaultReturn:return 1;
	case NSAlertAlternateReturn:return 0;
	}
	return -1;
}

String bbRequestFile( String title,String filter,int save,String path ){

	String file,dir;
	int i=path.FindLast( "\\" );
	if( i!=-1 ){
		dir=path.Slice( 0,i );
		file=path.Slice( i+1 );
	}else{
		file=path;
	}
	
	NSMutableArray *nsfilter=0;
	bool allowOthers=true;

	if( filter.Length() ){
	
		allowOthers=false;
	
		nsfilter=[NSMutableArray arrayWithCapacity:10];
		
		int i0=0;
		while( i0<filter.Length() ){
		
			int i1=filter.Find( ":",i0 )+1;
			if( !i1 ) break;
			
			int i2=filter.Find( ";",i1 );
			if( i2==-1 ) i2=filter.Length();
			
			while( i1<i2 ){
			
				int i3=filter.Find( ",",i1 );
				if( i3==-1 ) i3=i2;
				
				String ext=filter.Slice( i1,i3 );
				if( ext=="*" ){
					allowOthers=true;
				}else{
					[nsfilter addObject:ext.ToNSString()];
				}
				i1=i3+1;
			}
			i0=i2+1;
		}
	}

	NSString *nsdir=0;
	NSString *nsfile=0;
	NSString *nstitle=0;
	NSMutableArray *nsexts=0;

	if( dir.Length() ) nsdir=dir.ToNSString();
	if( file.Length() ) nsfile=file.ToNSString();
	if( title.Length() ) nstitle=title.ToNSString();

	beginPanel();
	
	String str;

	if( save ){
		NSSavePanel *panel=[NSSavePanel savePanel];
		
		if( nstitle ) [panel setTitle:nstitle];
		
		if( nsfilter ){
			[panel setAllowedFileTypes:nsfilter];
			[panel setAllowsOtherFileTypes:allowOthers];
		}
		
		if( [panel runModalForDirectory:nsdir file:nsfile]==NSFileHandlingPanelOKButton ){
			str=String( [panel filename] );
		}

	}else{
		NSOpenPanel *panel=[NSOpenPanel openPanel];

		if( nstitle ) [panel setTitle:nstitle];
		
		if( allowOthers ) nsfilter=0;
		
		if( [panel runModalForDirectory:nsdir file:nsfile types:nsfilter]==NSFileHandlingPanelOKButton ){
			str=String( [panel filename] );
		}
	}
	endPanel();

	return str;
}

String bbRequestDir( String title,String dir ){

	NSString *nsdir=0;
	NSString *nstitle=0;
	NSOpenPanel *panel;
	
	if( dir.Length() ) nsdir=dir.ToNSString();
	
	if( title.Length() ) nstitle=title.ToNSString();

	panel=[NSOpenPanel openPanel];
	
	[panel setCanChooseFiles:NO];
	[panel setCanChooseDirectories:YES];
	[panel setCanCreateDirectories:YES];
	
	if( nstitle ) [panel setTitle:nstitle];

	beginPanel();
	
	String str;
	
	if( [panel runModalForDirectory:nsdir file:0 types:0]==NSFileHandlingPanelOKButton ){
	
		str=String( [panel filename] );
	}

	endPanel();
	
	return str;
}

#endif
