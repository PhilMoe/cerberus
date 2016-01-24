
Import builder

Class GlfwBuilder Extends Builder

	Method New( tcc:TransCC )
		Super.New( tcc )
	End
	
	Method Config:String()
		Local config:=New StringStack
		For Local kv:=Eachin GetConfigVars()
			config.Push "#define CFG_"+kv.Key+" "+kv.Value
		Next
		Return config.Join( "~n" )
	End
	
	'***** GCC *****
	Method MakeGcc:Void()
	
		Local msize:=GetConfigVar( "GLFW_GCC_MSIZE_"+HostOS.ToUpper() )
		
		Local tconfig:=casedConfig+msize
	
		Local dst:="gcc_"+HostOS
		
		CreateDir dst+"/"+tconfig
		CreateDir dst+"/"+tconfig+"/internal"
		CreateDir dst+"/"+tconfig+"/external"
		
		CreateDataDir dst+"/"+tconfig+"/data"
		
		Local main:=LoadString( "main.cpp" )
		
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		SaveString main,"main.cpp"
		
		If tcc.opt_build

			ChangeDir dst
			CreateDir "build"
			CreateDir "build/"+tconfig
			
			Local ccopts:="",ldopts:=""

			If msize ccopts+=" -m"+msize;ldopts+=" -m"+msize
			
			ccopts+=" "+GetConfigVar( "GLFW_GCC_CC_OPTS" ).Replace( ";"," " )
			ldopts+=" "+GetConfigVar( "GLFW_GCC_LD_OPTS" ).Replace( ";"," " )
			
			Select ENV_CONFIG
			Case "debug"
				ccopts+=" -O0"
			Case "release"
				ccopts+=" -O3 -DNDEBUG"
			End
			
			Local cmd:="make"
			If HostOS="winnt" And FileType( tcc.MINGW_PATH+"/bin/mingw32-make.exe" ) cmd="mingw32-make"
			
			Execute cmd+" CCOPTS=~q"+ccopts+"~q LDOPTS=~q"+ldopts+"~q OUT=~q"+tconfig+"/MonkeyGame~q"
			
			If tcc.opt_run

				ChangeDir tconfig

				If HostOS="winnt"
					Execute "MonkeyGame"
				Else
					Execute "./MonkeyGame"
				Endif
			Endif
		Endif
			
	End
	
	'***** Vc2010 *****
	Method MakeVc2010:Void()
	
		CreateDir "vc2010/"+casedConfig
		CreateDir "vc2010/"+casedConfig+"/internal"
		CreateDir "vc2010/"+casedConfig+"/external"
		
		CreateDataDir "vc2010/"+casedConfig+"/data"
		
		Local main:=LoadString( "main.cpp" )
		
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		SaveString main,"main.cpp"
		
		If tcc.opt_build

			ChangeDir "vc2010"

			Execute "~q"+tcc.MSBUILD_PATH+"~q /p:Configuration="+casedConfig+" /p:Platform=Win32 MonkeyGame.sln"
			
			If tcc.opt_run
			
				ChangeDir casedConfig

				Execute "MonkeyGame"
				
			Endif
		Endif
	End

	'***** Msvc *****
	Method MakeMsvc:Void()
	
		CreateDir "msvc/"+casedConfig
		CreateDir "msvc/"+casedConfig+"/internal"
		CreateDir "msvc/"+casedConfig+"/external"
		
		CreateDataDir "msvc/"+casedConfig+"/data"
		
		Local main:=LoadString( "main.cpp" )
		
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		SaveString main,"main.cpp"
		
		If tcc.opt_build

			ChangeDir "msvc"

			Execute "~q"+tcc.MSBUILD_PATH+"~q /p:Configuration="+casedConfig'+" /p:Platform=Win32 MonkeyGame.sln"
			
			If tcc.opt_run
			
				ChangeDir casedConfig

				Execute "MonkeyGame"
				
			Endif
		Endif
	End

	'***** Xcode *****	
	Method MakeXcode:Void()

		CreateDataDir "xcode/data"

		Local main:=LoadString( "main.cpp" )
		
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		SaveString main,"main.cpp"
		
		If tcc.opt_build
		
			ChangeDir "xcode"
			
'			Execute "set -o pipefail && xcodebuild -configuration "+casedConfig+" | egrep -A 5 ~q(error|warning):~q"
			Execute "xcodebuild -configuration "+casedConfig
			
			If tcc.opt_run
			
				ChangeDir "build/"+casedConfig
				ChangeDir "MonkeyGame.app/Contents/MacOS"
				
				Execute "./MonkeyGame"
			Endif
		Endif
	End
	
	'***** Builder *****	
	Method IsValid:Bool()
		Select HostOS
		Case "winnt"
			If tcc.MINGW_PATH Or tcc.MSBUILD_PATH Return True
		Default
			Return True
		End
		Return False
	End
	
	Method Begin:Void()
		ENV_LANG="cpp"
		_trans=New CppTranslator
	End
	
	Method MakeTarget:Void()
		Select HostOS
		Case "winnt"
			If GetConfigVar( "GLFW_USE_MINGW" )="1" And tcc.MINGW_PATH
				MakeGcc
			Else If FileType( "vc2010" )=FILETYPE_DIR
				MakeVc2010
			Else If FileType( "msvc" )=FILETYPE_DIR
				MakeMsvc
			Else If tcc.MINGW_PATH
				MakeGcc
			Endif
		Case "macos"
			MakeXcode
		Case "linux"
			MakeGcc
		End
	End
	
End
