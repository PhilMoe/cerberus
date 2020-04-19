
'Deprecating!
'
'Use brl.filesystem and brl.filepath instead!
'
#If Not BRL_OS_IMPLEMENTED
#If TARGET="stdcpp" Or TARGET="glfw"
#BRL_OS_IMPLEMENTED=True
Import "native/os.cpp"
#Endif
#Endif

#If Not BRL_OS_IMPLEMENTED
#Error "Native OS Module not implemented"
#Endif

Extern

Function HostOS$()
Function AppPath$()
Function AppArgs$[]()
Function RealPath$( path$ )
Function FileType( path$ )
Function FileSize( path$ )
Function FileTime( path$ )
Function CopyFile( src$,dst$ )
Function DeleteFile( path$ )
Function LoadString$( path$ )
Function SaveString( str$,path$ )
Function LoadDir$[]( path$ )
Function CreateDir( path$ )
Function DeleteDir( path$ )
Function ChangeDir( path$ )
Function CurrentDir$()
Function SetEnv( name$,value$ )
Function GetEnv$( name$ )
Function Execute( cmd$ )
Function ExitApp( retcode )

Public

Const FILETYPE_NONE=0
Const FILETYPE_FILE=1
Const FILETYPE_DIR=2

Function LoadDir$[]( path$,recursive?,hidden?=False )

	Local dirs:=New StringList,files:=New StringList
	
	dirs.AddLast ""
	
	While Not dirs.IsEmpty()

		Local dir$=dirs.RemoveFirst()

		For Local f$=Eachin LoadDir( path+"/"+dir )
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

Function CopyDir( srcpath$,dstpath$,recursive?=True,hidden?=False )

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
	
	For Local f$=Eachin files
		If Not hidden And f.StartsWith(".") Continue
		
		Local srcp$=srcpath+"/"+f
		Local dstp$=dstpath+"/"+f
		
		Select FileType( srcp )
		Case FILETYPE_FILE
			If Not CopyFile( srcp,dstp ) Return False
		Case FILETYPE_DIR
			If recursive And Not CopyDir( srcp,dstp,recursive,hidden ) Return False
		End
	Next
	
	Return True
End

Function DeleteDir( path$,recursive? )

	If Not recursive Return DeleteDir( path )
	
	Select FileType( path )
	Case FILETYPE_NONE Return True
	Case FILETYPE_FILE Return False
	End Select
	
	For Local f$=Eachin LoadDir( path )
		If f="." Or f=".." Continue

		Local fpath$=path+"/"+f

		If FileType( fpath )=FILETYPE_DIR
			If Not DeleteDir( fpath,True ) Return False
		Else
			If Not DeleteFile( fpath ) Return False
		Endif
	Next

	Return DeleteDir( path )
End

Function StripDir$( path$ )
	Local i=path.FindLast( "/" )
	If i=-1 i=path.FindLast( "\" )
	If i<>-1 Return path[i+1..]
	Return path
End

Function ExtractDir$( path$ )
	Local i=path.FindLast( "/" )
	If i=-1 i=path.FindLast( "\" )
	If i<>-1 Return path[..i]
End

Function StripExt$( path$ )
	Local i=path.FindLast( "." )
	If i<>-1 And path.Find( "/",i+1 )=-1 And path.Find( "\",i+1 )=-1 Return path[..i]
	Return path
End

Function ExtractExt$( path$ )
	Local i=path.FindLast( "." )
	If i<>-1 And path.Find( "/",i+1 )=-1 And path.Find( "\",i+1 )=-1 Return path[i+1..]
	Return ""
End

Function StripAll$( path$ )
	Return StripDir( StripExt( path ) )
End
