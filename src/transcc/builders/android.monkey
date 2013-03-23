
Import builder

Class AndroidBuilder Extends Builder

	Method New( tcc:TransCC )
		Super.New( tcc )
	End
	
	Method Config:String()
		Local config:=New StringStack
		For Local kv:=Eachin _cfgVars
			config.Push "static final String "+kv.Key+"="+Enquote( kv.Value,"java" )+";"
		Next
		Return config.Join( "~n" )
	End

	Method IsValid:Bool()
		Return tcc.ANDROID_PATH<>""
	End
	
	Method Begin:Void()
		ENV_LANG="java"
		_trans=New JavaTranslator
	End
	
	Method MakeTarget:Void()
		
		SetCfgVar "ANDROID_MAINFEST_MAIN",GetCfgVar( "ANDROID_MANIFEST_MAIN" ).Replace( ";","~n" )+"~n"

		SetCfgVar "ANDROID_MAINFEST_APPLICATION",GetCfgVar( "ANDROID_MANIFEST_APPLICATION" ).Replace( ";","~n" )+"~n"
	
		'create data dir
		CreateDataDir "assets/monkey"

		Local app_label:=GetCfgVar( "ANDROID_APP_LABEL" )
		Local app_package:=GetCfgVar( "ANDROID_APP_PACKAGE" )
		
		SetCfgVar "ANDROID_SDK_DIR",tcc.ANDROID_PATH.Replace( "\","\\" )
		
		'create package
		Local jpath:="src"
		DeleteDir jpath,True
		CreateDir jpath
		For Local t:=Eachin app_package.Split(".")
			jpath+="/"+t
			CreateDir jpath
		Next
		jpath+="/MonkeyGame.java"
		
		'template files
		For Local file:=Eachin LoadDir( "templates",True )
			Select ExtractExt( file ).ToLower()
			Case "xml","properties","java"
				Local str:=LoadString( "templates/"+file )
				str=ReplaceEnv( str )
				SaveString str,file
			Default
				CopyFile "templates/"+file,file
			End
		Next
		
		'create main source file
		Local main:=LoadString( "MonkeyGame.java" )
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		'extract all imports
		Local imps:=New StringStack
		Local done:=New StringSet
		Local out:=New StringStack
		For Local line:=Eachin main.Split( "~n" )
			If line.StartsWith( "import " )
				Local i:=line.Find( ";",7 )
				If i<>-1
					Local id:=line[7..i+1]
					If Not done.Contains( id )
						done.Insert id
						imps.Push "import "+id
					Endif
				Endif
			Else
				out.Push line
			Endif
		End
		main=out.Join( "~n" )

		main=ReplaceBlock( main,"IMPORTS",imps.Join( "~n" ) )
		main=ReplaceBlock( main,"PACKAGE","package "+app_package+";" )
		
		SaveString main,jpath

		'create 'libs' dir		
		For Local lib:=Eachin GetCfgVar( "LIBS" ).Split( ";" )
			Select ExtractExt( lib )
			Case "jar","so"
				CopyFile lib,"libs/"+StripDir( lib )
			End
		Next
		
		If GetCfgVar( "ANDROID_NATIVE_GL_ENABLED" )="1"
			CopyDir "nativegl/libs","libs",True
			CreateDir "src/com"
			CreateDir "src/com/monkey"
			CopyFile "nativegl/NativeGL.java","src/com/monkey/NativeGL.java"
		Endif
		
		If tcc.opt_build
		
			Local r:=Execute( "ant clean",False ) And Execute( "ant debug install",False )
			
			If Not r
				Die "Android build failed."
			Else If tcc.opt_run
			
				Execute "adb logcat -c",False
				Execute "adb shell am start -n "+app_package+"/"+app_package+".MonkeyGame",False
				Execute "adb logcat [Monkey]:I *:E",False
				'
				'NOTE: This leaves ADB server running which can LOCK the .build dir making it undeletable...
				'
			Endif
		Endif
	End
End
