
Import builder

Class XnaBuilder Extends Builder

	Method New( tcc:TransCC )
		Super.New( tcc )
	End
	
	Method Config:String()
		Local config:=New StringStack
		For Local kv:=Eachin GetConfigVars()
			config.Push "public const String "+kv.Key+"="+Enquote( kv.Value,"cs" )+";"
		Next
		Return config.Join( "~n" )
	End
	
	Method Content:String()
		Local cont:=New StringStack

		For Local kv:=Eachin dataFiles
		
			Local p:=kv.Key
			Local r:=kv.Value
			Local f:=StripDir( r )
			Local t:=("monkey/"+r).Replace( "/","\" )
			
			Local ext:=ExtractExt( r ).ToLower()
			
			If MatchPath( r,TEXT_FILES )
					cont.Push "  <ItemGroup>"
					cont.Push "    <Content Include=~q"+t+"~q>"
					cont.Push "      <Name>"+f+"</Name>"
					cont.Push "      <CopyToOutputDirectory>Always</CopyToOutputDirectory>"
					cont.Push "    </Content>"
					cont.Push "  </ItemGroup>"
			Else If MatchPath( r,IMAGE_FILES )
				Select ext
				Case "bmp","dds","dib","hdr","jpg","pfm","png","ppm","tga"
					cont.Push "  <ItemGroup>"
					cont.Push "    <Compile Include=~q"+t+"~q>"
					cont.Push "      <Name>"+f+"</Name>"
					cont.Push "      <Importer>TextureImporter</Importer>"
					cont.Push "      <Processor>TextureProcessor</Processor>"
					cont.Push "      <ProcessorParameters_ColorKeyEnabled>False</ProcessorParameters_ColorKeyEnabled>"
					cont.Push "      <ProcessorParameters_PremultiplyAlpha>False</ProcessorParameters_PremultiplyAlpha>"
					cont.Push "	   </Compile>"
					cont.Push "  </ItemGroup>"
				Default
					Die "Invalid image file type"
				End
			Else If MatchPath( r,SOUND_FILES )
				Select ext
				Case "wav","mp3","wma"
					Local imp:=ext[..1].ToUpper()+ext[1..]+"Importer"	'eg: wav->WavImporter
					cont.Push "  <ItemGroup>"
					cont.Push "    <Compile Include=~q"+t+"~q>"
					cont.Push "      <Name>"+f+"</Name>"
					cont.Push "      <Importer>"+imp+"</Importer>"
					cont.Push "      <Processor>SoundEffectProcessor</Processor>"
					cont.Push "	   </Compile>"
					cont.Push "  </ItemGroup>"
				Default
					Die "Invalid sound file type"
				End
			Else If MatchPath( r,MUSIC_FILES )
				Select ext
				Case "wav","mp3","wma"
					Local imp:=ext[..1].ToUpper()+ext[1..]+"Importer"	'eg: wav->WavImporter
					cont.Push "  <ItemGroup>"
					cont.Push "    <Compile Include=~q"+t+"~q>"
					cont.Push "      <Name>"+f+"</Name>"
					cont.Push "      <Importer>"+imp+"</Importer>"
					cont.Push "      <Processor>SongProcessor</Processor>"
					cont.Push "	   </Compile>"
					cont.Push "  </ItemGroup>"
				Default
					Die "Invalid music file type"
				End
			Else If MatchPath( r,BINARY_FILES )
					cont.Push "  <ItemGroup>"
					cont.Push "    <Content Include=~q"+t+"~q>"
					cont.Push "      <Name>"+f+"</Name>"
					cont.Push "      <CopyToOutputDirectory>Always</CopyToOutputDirectory>"
					cont.Push "    </Content>"
					cont.Push "  </ItemGroup>"
			Endif

		Next
		
		Return cont.Join( "~n" )
	
	End

	Method IsValid:Bool()
		Select HostOS
		Case "winnt"
			If tcc.MSBUILD_PATH Return True
		End
		Return False
	End

	Method Begin:Void()
		ENV_LANG="cs"
		_trans=New CsTranslator
	End

	Method MakeTarget:Void()
	
		CreateDataDir "MonkeyGame/MonkeyGameContent/monkey"

		'app data
		Local contproj:=LoadString( "MonkeyGame/MonkeyGameContent/MonkeyGameContent.contentproj" )
		contproj=ReplaceBlock( contproj,"CONTENT",Content(),"~n<!-- " )
		SaveString contproj,"MonkeyGame/MonkeyGameContent/MonkeyGameContent.contentproj"
		
		'app code
		Local main:=LoadString( "MonkeyGame/MonkeyGame/Program.cs" )
		
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		SaveString main,"MonkeyGame/MonkeyGame/Program.cs"
			
		If tcc.opt_build
		
			Execute "~q"+tcc.MSBUILD_PATH+"~q /t:MonkeyGame /p:Configuration="+casedConfig+" MonkeyGame.sln"

			If tcc.opt_run
				ChangeDir "MonkeyGame/MonkeyGame/bin/x86/"+casedConfig
				Execute "MonkeyGame",False
			Endif
		Endif
		
	End

End
