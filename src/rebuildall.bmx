
RebuildTranscc
'RebuildMakedocs
'RebuildMServer
'RebuildLauncher	'Note: On Windows, kill process Monkey.exe before rebuilding launcher!

End

?Win32
Const bin$="..\bin\"
Const ext$="_winnt.exe"
?MacOS
Const bin$="../bin/"
Const ext$="_macos"
?Linux
Const bin$="../bin/"
Const ext$="_linux"
?

Const trans$=bin+"transcc"+ext

Const makedocs$=bin+"makedocs"+ext

Function Error( msg$ )
	Print "***** ERROR ***** "+msg
	exit_ -1
'	End
End Function

Function Execute( cmd$,fail=True )
	Return system_( cmd )
End Function

Function Update( src$,dst$ )

	DeleteFile dst
	If FileType( dst ) Error "Failed to delete file:"+dst
	
	CopyFile src,dst
	If FileType( dst )<>FILETYPE_FILE Error "Failed to copy "+src+","+dst

?Not Win32
	Execute "chmod +x "+dst
?

End Function

Function RebuildTranscc()

	Print "~nRebuildall: rebuilding transcc..."
	
	Local opts$=""
	opts:+" -target=C++_Tool -builddir=transcc.build"
'	opts:+" -clean -config=debug +CPP_DOUBLE_PRECISION_FLOATS=1 +CPP_GC_MODE=0"
	opts:+" -clean -config=release +CPP_DOUBLE_PRECISION_FLOATS=1 +CPP_GC_MODE=0"
	
	Local make$=trans+" "+opts+" transcc/transcc.monkey"
	Print make
	If Execute( make ) Error "Failed to build transcc"

	Update "transcc/transcc.build/cpptool/main"+ext,"../bin/transcc"+ext
	
	Print "transcc built OK!"

End Function

Function RebuildMakedocs()

	Print "~nRebuild all: rebuilding makedocs..."
	
	Local opts$=""
	opts:+" -target=C++_Tool -builddir=makedocs.build"
	opts:+" -clean -config=release +CPP_GC_MODE=0"
	
	Local make$=trans+" "+opts+" makedocs/makedocs.monkey"
	Print make
	If Execute( make ) Error "Failed to build makedocs"
	
	Update "makedocs/makedocs.build/cpptool/main"+ext,"../bin/makedocs"+ext
	
	Print "makedocs built OK!"

End Function

Function RebuildMServer()

	Print "~nRebuild all: rebuilding mserver..."

	If Execute( "~q"+BlitzMaxPath()+"/bin/bmk~q makeapp -h -t gui -a -r -o "+bin+"mserver"+ext+" mserver/mserver.bmx" ) Error "Failed to build mserver"
	
	Print "mserver built OK!"
	
End Function

Function RebuildLauncher()

	Print "~nRebuild all: rebuilding launcher..."

?Win32
	Execute "windres launcher/resource.rc launcher/resource.o"
?
	Execute "~q"+BlitzMaxPath()+"/bin/bmk~q makeapp -t gui -a -r -o ../Monkey launcher/launcher.bmx"
?MacOS
	Execute "cp launcher/info.plist ../Monkey.app/Contents"
	Execute "rm ../Monkey.app/Contents/Resources/monkey.icns"
	Execute "cp launcher/monkey.icns ../Monkey.app/Contents/Resources"
?	
	Print "launcher built OK!"
	
End Function
