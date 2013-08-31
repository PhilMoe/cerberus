
Import builder

Class AndroidNdkBuilder Extends Builder

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
	
	Method IsValid:Bool()
		Return tcc.ANDROID_PATH<>"" And tcc.ANDROID_NDK_PATH<>""
	End
	
	Method Begin:Void()
		ENV_LANG="cpp"
		_trans=New CppTranslator
	End
	
	Method MakeTarget:Void()
		
		SetConfigVar "ANDROID_MAINFEST_MAIN",GetConfigVar( "ANDROID_MANIFEST_MAIN" ).Replace( ";","~n" )+"~n"
		SetConfigVar "ANDROID_MAINFEST_APPLICATION",GetConfigVar( "ANDROID_MANIFEST_APPLICATION" ).Replace( ";","~n" )+"~n"
	
		'create data dir
		CreateDataDir "assets/monkey"

		Local app_label:=GetConfigVar( "ANDROID_APP_LABEL" )
		Local app_package:=GetConfigVar( "ANDROID_APP_PACKAGE" )
		
		SetConfigVar "ANDROID_SDK_DIR",tcc.ANDROID_PATH.Replace( "\","\\" )
		
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
		
			'Recursive CreateDir...	
			Local i:=0
			Repeat
				i=file.Find( "/",i )
				If i=-1 Exit
				CreateDir file[..i]
				If FileType( file[..i] )<>FILETYPE_DIR
					file=""
					Exit
				Endif
				i+=1
			Forever
			If Not file Continue
		
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
		Local main:=LoadString( "jni/main.cpp" )
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )
		SaveString main,"jni/main.cpp"

		'create 'libs' dir		
		For Local lib:=Eachin GetConfigVar( "LIBS" ).Split( ";" )
			Select ExtractExt( lib )
			Case "jar","so"
				CopyFile lib,"libs/"+StripDir( lib )
			End
		Next
		
		If tcc.opt_build
		
			If Not Execute( tcc.ANDROID_NDK_PATH+"/ndk-build" )
				Die "Failed to build native code"
			Endif
		
			Local r:=Execute( "ant clean",False ) And Execute( "ant debug install",False )
			
			If Not r
				Die "Android build failed."
			Else If tcc.opt_run
			
				Execute "adb logcat -c",False

'				Execute "adb shell am start -n "+app_package+"/"+app_package+".MonkeyGame",False
				Execute "adb shell am start -n "+app_package+"/android.app.NativeActivity",False

'				Execute "adb logcat [Monkey]:I *:E",False	'?!?!?
				Execute "adb logcat",False
				'
				'NOTE: This leaves ADB server running which can LOCK the .build dir making it undeletable...
				'
			Endif
		Endif
	End
End
