
Import builder

Class Win8Builder Extends Builder

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
	
	Method Content:String()
		Local cont:=New StringStack
		For Local kv:=Eachin dataFiles
			cont.Push "    <None Include=~qAssets\monkey\"+kv.Value+"~q>"
			cont.Push "      <DeploymentContent>true</DeploymentContent>"
			cont.Push "    </None>"
		Next
		Return cont.Join( "~n" )
	End
	
	Method IsValid:Bool()
		Select HostOS
		Case "winnt"
			If tcc.MSBUILD_PATH Return true
		End
		Return False
	End
	
	Method Begin:Void()
		ENV_LANG="cpp"
		_trans=New CppTranslator
	End
	
	Method MakeTarget:Void()

		CreateDataDir "Assets/monkey"

		'proj file		
		Local proj:=LoadString( "MonkeyGame.vcxproj" )
		proj=ReplaceBlock( proj,"CONTENT",Content(),"~n    <!-- " )
		SaveString proj,"MonkeyGame.vcxproj"

		'app code
		Local main:=LoadString( "MonkeyGame.cpp" )
		
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		SaveString main,"MonkeyGame.cpp"

		If tcc.opt_build

			Execute tcc.MSBUILD_PATH+" /p:Configuration="+casedConfig+" /p:Platform=Win32 MonkeyGame.sln"
			
			If tcc.opt_run
				'Any bright ideas...?
			Endif

		Endif
		
	End
	
End
