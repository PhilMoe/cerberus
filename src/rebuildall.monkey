
Import brl.process
Import brl.filepath
Import brl.filesystem

Function RebuildAll:Void()
	RebuildTranscc		'not available on windows - use rebuildall.bmx
'	RebuildMakedocs
'	RebuildMServer
'	RebuildLauncher		'not available on window/macos - use rebuildall.bmx
End

#If HOST<>"linux" And HOST<>"macos"
#Error "Host<>linux/macos"
#Endif

#If TARGET<>"stdcpp"
#Error "Target<>stdcpp"
#Endif

#If HOST="winnt"
Const bin:="..\bin\"
Const ext:="_winnt.exe"
#Else If HOST="macos"
Const bin:="../bin/"
Const ext:="_macos"
#Else
Const bin:="../bin/"
Const ext:="_linux"
#Endif

Const trans:=bin+"transcc"+ext

Function Main()

	'change to root monkey dir...
	Local dir:=CurrentDir()
	While Not dir.EndsWith( "/src" )
		dir=ExtractDir( dir )
	Wend
	
	ChangeDir dir

	RebuildAll	
End

Function Update:Void( src:String,dst:String )

	Select FileType( src )
	Case FILETYPE_FILE
	
		DeleteFile dst
		If FileType( dst )<>FILETYPE_NONE Error "Failed to delete:"+dst
	
		CopyFile src,dst
		If FileType( dst )<>FILETYPE_FILE Error "Failed to copy:"+src+","+dst

		#If HOST<>"winnt"
			Execute "chmod +x "+dst
		#Endif	

	Case FILETYPE_DIR
	
		DeleteDir dst,True
		If FileType( dst )<>FILETYPE_NONE Error "Failed to delete:"+dst
	
		CopyDir src,dst,True
		If FileType( dst )<>FILETYPE_DIR Error "Failed to copy:"+src+","+dst
		
	 Default
	 
	 	Error "no source file:"+src
	 	
	End
	
End

#If HOST<>"winnt"

Function RebuildTranscc:Void()

	Print "~nRebuildall: rebuilding transcc..."
	
	Local opts:=""
	opts+=" -target=C++_Tool -builddir=transcc.build"
	opts+=" -clean -config=release +CPP_DOUBLE_PRECISION_FLOATS=1 +CPP_GC_MODE=0"
	
	Local make:=trans+" "+opts+" transcc/transcc.monkey"
	Print make
	If Execute( make ) Error "Failed to build transcc"

	Update "transcc/transcc.build/cpptool/main"+ext,"../bin/transcc"+ext
	
	Print "transcc built OK!"

End

#Endif

Function RebuildMakedocs:Void()

	Print "~nRebuild all: rebuilding makedocs..."
	
	Local opts:=""
	opts+=" -target=C++_Tool -builddir=makedocs.build"
	opts+=" -clean -config=release +CPP_GC_MODE=0"
	
	Local make:=trans+" "+opts+" makedocs/makedocs.monkey"
	Print make
	If Execute( make ) Error "Failed to build makedocs"
	
	Update "makedocs/makedocs.build/cpptool/main"+ext,"../bin/makedocs"+ext
	
	Print "makedocs built OK!"
	
End

Function RebuildMServer:Void()

	Print "~nRebuild all: rebuilding mserver..."
	
	Local opts:=""
	opts+=" ~q-target=Desktop_Game_(Glfw3)~q -builddir=mserver.build"
	opts+=" -clean -config=release +CPP_GC_MODE=1"
	
	Local make:=trans+" "+opts+" mserver/mserver.monkey"
	Print make
	If Execute( make ) Error "Failed to build mserver"
	
	Local out:=""

#If HOST="linux"
	Update "mserver/mserver.build/glfw3/gcc_linux/Release/MonkeyGame","../bin/mserver_linux"
#Else If HOST="macos"
	Update "mserver/mserver.build/glfw3/xcode/build/Release/MonkeyGame.app","../bin/mserver_macos.app"
#Endif
	
	Print "mserver built OK!"
End

#If HOST<>"winnt" And HOST<>"macos"

'Doesn't work on windows/macos yet
Function RebuildLauncher:Void()

	Print "~nRebuild all: rebuilding launcher..."
	
	Local opts:=""
	opts+=" -target=C++_Tool -builddir=launcher.build"
	opts+=" -clean -config=release +CPP_GC_MODE=0"
	
	Local make:=trans+" "+opts+" launcher/launcher.monkey"
	Print make
	If Execute( make ) Error "Failed to build launcher"
	
	Update "launcher/launcher.build/cpptool/main"+ext,"../Monkey"
	
	Print "launcher built OK!"
	
End

#Endif

