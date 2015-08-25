
Import builder

Class WinrtBuilder Extends Builder

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
	
	Method Content:String( csharp:Bool )
		Local cont:=New StringStack
		For Local kv:=Eachin dataFiles
			If csharp
				cont.Push "    <Content Include=~qAssets\monkey\"+kv.Value.Replace( "/","\" )+"~q>"
				cont.Push "      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>"
				cont.Push "    </Content>"
			Else
				cont.Push "    <None Include=~qAssets\monkey\"+kv.Value.Replace( "/","\" )+"~q>"
				cont.Push "      <DeploymentContent>true</DeploymentContent>"
				cont.Push "    </None>"
			Endif
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
		If proj
			proj=ReplaceBlock( proj,"CONTENT",Content( False ),"~n    <!-- " )
			SaveString proj,"MonkeyGame.vcxproj"
		Else
			Local proj:=LoadString( "MonkeyGame.csproj" )
			proj=ReplaceBlock( proj,"CONTENT",Content( True ),"~n    <!-- " )
			SaveString proj,"MonkeyGame.csproj"
		Endif
		
		'app code
		Local main:=LoadString( "MonkeyGame.cpp" )
		
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		SaveString main,"MonkeyGame.cpp"

		If tcc.opt_build

			Execute "~q"+tcc.MSBUILD_PATH+"~q /p:Configuration="+casedConfig+" /p:Platform=Win32 MonkeyGame.sln"
			
			If tcc.opt_run
				'Any bright ideas...?
			Endif

		Endif
		
	End
	
End
