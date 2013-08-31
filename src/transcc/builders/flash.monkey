
Import builder

Class FlashBuilder Extends Builder

	Method New( tcc:TransCC )
		Super.New( tcc )
	End
	
	Method Config:String()
		Local config:=New StringStack
		For Local kv:=Eachin GetConfigVars()
			config.Push "internal static var "+kv.Key+":String="+Enquote( kv.Value,"as" )
		Next
		Return config.Join( "~n" )
	End
	
	Method Assets:String()
		Local assets:=New StringStack
		For Local kv:=Eachin dataFiles
			
			Local ext:=ExtractExt( kv.Value )
			
			Local munged:="_"
			For Local q:=Eachin StripExt( kv.Value ).Split( "/" )
				For Local i=0 Until q.Length
					If IsAlpha( q[i] ) Or IsDigit( q[i] ) Or q[i]=95 Continue
					Die "Invalid character in flash filename: "+kv.Value+"."
				Next
				munged+=q.Length+q
			Next
			munged+=ext.Length+ext
			
			Select ext.ToLower()
			Case "png","jpg","mp3"
				assets.Push "[Embed(source=~qdata/"+kv.Value+"~q)]"
				assets.Push "public static var "+munged+":Class;"
			Default
				assets.Push "[Embed(source=~qdata/"+kv.Value+"~q,mimeType=~qapplication/octet-stream~q)]"
				assets.Push "public static var "+munged+":Class;"
			End
			
		Next
		Return assets.Join( "~n" )
	End
	
	Method IsValid:Bool()
		Return tcc.FLEX_PATH<>""
	End
	
	Method Begin:Void()
		ENV_LANG="as"
		_trans=New AsTranslator
	End
	
	Method MakeTarget:Void()

		CreateDataDir "data"
		
		'app code
		Local main:=LoadString( "MonkeyGame.as" )

		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"ASSETS",Assets() )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		SaveString main,"MonkeyGame.as"
		
		If tcc.opt_build
		
			Local cc_opts:=" -static-link-runtime-shared-libraries=true"
			
			If ENV_CONFIG="debug" cc_opts+=" -debug=true"

			DeleteFile "main.swf"

			Execute "mxmlc"+cc_opts+" MonkeyGame.as"
			
			If tcc.opt_run
				If tcc.FLASH_PLAYER
					Execute tcc.FLASH_PLAYER+" ~q"+RealPath( "MonkeyGame.swf" )+"~q",False
				Else If tcc.HTML_PLAYER
					Execute tcc.HTML_PLAYER+" ~q"+RealPath( "MonkeyGame.html" )+"~q",False
				Endif
			Endif
		Endif
	End
End
