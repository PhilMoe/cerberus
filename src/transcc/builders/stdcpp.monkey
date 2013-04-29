
Import builder

Class StdcppBuilder Extends Builder

	Method New( tcc:TransCC )
		Super.New( tcc )
	End

	Method Config:String()
		Local config:=New StringStack
		For Local kv:=Eachin _cfgVars
			config.Push "#define CFG_"+kv.Key+" "+kv.Value
		Next
		Return config.Join( "~n" )
	End
	
	Method IsValid:Bool()
		Select HostOS
		Case "winnt"
			If tcc.MINGW_PATH Return True
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
	
		Select ENV_CONFIG
		Case "debug" SetCfgVar "DEBUG","1"
		Case "release" SetCfgVar "RELEASE","1"
		Case "profile" SetCfgVar "PROFILE","1"
		End
		
		Local main:=LoadString( "main.cpp" )

		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )

		SaveString main,"main.cpp"
		
		If tcc.opt_build

			Local out:="main_"+HostOS
			DeleteFile out
			
			Local OPTS:="",LIBS:=""
			
			Select ENV_HOST
			Case "macos"
				OPTS+=" -arch i386 -read_only_relocs suppress -mmacosx-version-min=10.3"
			Case "winnt"
				OPTS+=" -Wno-free-nonheap-object"
				LIBS+=" -lwinmm -lws2_32"
			End
			
			Select ENV_CONFIG
			Case "release"
				OPTS+=" -O3 -DNDEBUG"
			End
			
			Execute "g++"+OPTS+" -o "+out+" main.cpp"+LIBS
			
			If tcc.opt_run
				Execute "~q"+RealPath( out )+"~q"
			Endif
		Endif
	End
	
End

