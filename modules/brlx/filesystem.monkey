
#If LANG="cpp" Or LANG="java"

#If LANG="cpp"
Import "native/filesystem.cpp"
#Elseif LANG="java"
Import "native/filesystem.java"
#Endif

Const FILETYPE_NONE:=0
Const FILETYPE_FILE:=1
Const FILETYPE_DIR:=2

Extern Private

Class BBFileSystemDriver

	Method HostOS:String()

	Method AppPath:String()
	Method AppArgs:String[]()
	Method Execute:Int( cmd:String )
	
	Method SetEnv:Int( name:String,value:String )
	Method GetEnv:String( name:String)
	
	Method FileType:Int( path:String )
	Method FileSize:Int( path:String )
	Method FileTime:Int( path:String )
	
	Method CreateFile:Int( path:String )
	Method DeleteFile:Int( path:String )
	Method CopyFile:Int( srcPath:String,dstPath:String )
	
	Method CreateDir:Int( path:String )
	Method DeleteDir:Int( path:String )
	Method LoadDir:String[]( path:String )
	
	Method ChangeDir:Int( path:String )
	Method CurrentDir:String()

End

Private

Global Driver:=New BBFileSystemDriver

Public

Function AppPath:String()
	Return Driver.AppPath()
End

Function AppArgs:String[]()
	Return Driver.AppArgs()
End

Function SetEnv:Int( name:String,value:String )
	Return Driver.SetEnv( name,value )
End

Function GetEnv:String( name:String )
	Return Driver.GetEnv( name )
End

Function Execute:Int( cmd:String )
	Return Driver.Execute( cmd )
End

Function FileType:Int( path:String )
	Return Driver.FileType( path )
End

Function FileSize:Int( path:String )
	Return Driver.FileSize( path )
End

Function FileTime:Int( path:String )
	Return Driver.FileTime( path )
End

Function CreateFile:Int( path:String )
	Return Driver.CreateFile( path )
End

Function DeleteFile:Int( path:String )
	Return Driver.DeleteFile( path )
End

Function CopyFile:Int( src:String,dst:String )
	Return Driver.CopyFile( path )
End

Function CreateDir:Int( path:String )
	Return Driver.CreateDir( path )
End

Function DeleteDir:Int( path:String )
	Return Driver.DeleteDir( path )
End

Function LoadDir:String[]( path:String )
	Return Driver.LoadDir( path )
End

Function ChangeDir:Int( path:String )
	Return Driver.ChangeDir( path )
End

Function CurrentDir:String()
	Return Driver.CurrentDir()
End

Function LoadDir:String[]( path:String,recursive?,hidden?=False )

	Local dirs:=New StringList,files:=New StringList
	
	dirs.AddLast ""
	
	While Not dirs.IsEmpty()

		Local dir:=dirs.RemoveFirst()

		For Local f:=Eachin LoadDir( path+"/"+dir )
			If Not hidden And f.StartsWith(".") Continue
		
			If dir f=dir+"/"+f
			
			Select FileType( path+"/"+f )
			Case FILETYPE_FILE
				files.AddLast f
			Case FILETYPE_DIR
				If recursive
					dirs.AddLast f
				Else
					files.AddLast f
				Endif
			End
		Next
	Wend

	Return files.ToArray()
End

Function CopyDir( srcpath:String,dstpath:String,recursive?=True,hidden?=False )

	If FileType( srcpath )<>FILETYPE_DIR Return False

	'do this before create of destdir to allow a dir to be copied into itself!
	'
	Local files:=LoadDir( srcpath )
	
	Select FileType( dstpath )
	Case FILETYPE_NONE
		If Not CreateDir( dstpath ) Return False
	Case FILETYPE_FILE 
		Return False
	End
	
	For Local f:=Eachin files
		If Not hidden And f.StartsWith(".") Continue
		
		Local srcp:=srcpath+"/"+f
		Local dstp:=dstpath+"/"+f
		
		Select FileType( srcp )
		Case FILETYPE_FILE
			If Not CopyFile( srcp,dstp ) Return False
		Case FILETYPE_DIR
			If recursive And Not CopyDir( srcp,dstp,recursive,hidden ) Return False
		End
	Next
	
	Return True
End

Function DeleteDir( path:String,recursive? )

	If Not recursive Return DeleteDir( path )
	
	Select FileType( path )
	Case FILETYPE_NONE Return True
	Case FILETYPE_FILE Return False
	End Select
	
	For Local f:=Eachin LoadDir( path )
		If f="." Or f=".." Continue

		Local fpath:=path+"/"+f

		If FileType( fpath )=FILETYPE_DIR
			If Not DeleteDir( fpath,True ) Return False
		Else
			If Not DeleteFile( fpath ) Return False
		Endif
	Next

	Return DeleteDir( path )
End

#Endif

Function StripDir:String( path:String )
	Local i=path.FindLast( "/" )
	If i=-1 i=path.FindLast( "\" )
	If i<>-1 Return path[i+1..]
	Return path
End

Function ExtractDir:String( path:String )
	Local i=path.FindLast( "/" )
	If i=-1 i=path.FindLast( "\" )
	If i<>-1 Return path[..i]
End

Function StripExt:String( path:String )
	Local i=path.FindLast( "." )
	If i<>-1 And path.Find( "/",i+1 )=-1 And path.Find( "\",i+1 )=-1 Return path[..i]
	Return path
End

Function ExtractExt:String( path:String )
	Local i=path.FindLast( "." )
	If i<>-1 And path.Find( "/",i+1 )=-1 And path.Find( "\",i+1 )=-1 Return path[i+1..]
	Return ""
End

Function StripAll:String( path:String )
	Return StripDir( StripExt( path ) )
End
