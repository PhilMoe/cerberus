
Import builder

Class AndroidBuilder Extends Builder

	Method New( tcc:TransCC )
		Super.New( tcc )
	End
	
	Method Config:String()
		Local config:=New StringStack
		For Local kv:=Eachin GetConfigVars()
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
	
	Method CreateDirRecursive:Bool( path:String )
		Local i:=0
		Repeat
			i=path.Find( "/",i )
			If i=-1
				CreateDir( path )
				Return FileType( path )=FILETYPE_DIR
			Endif
			Local t:=path[..i]
			CreateDir( t )
			If FileType( t )<>FILETYPE_DIR Return False
			i+=1
		Forever
	End
	
	Method MakeTarget:Void()

		'create data dir
		CreateDataDir "assets/monkey"

		Local app_label:=GetConfigVar( "ANDROID_APP_LABEL" )
		Local app_package:=GetConfigVar( "ANDROID_APP_PACKAGE" )
		
		SetEnv "ANDROID_SDK_DIR",tcc.ANDROID_PATH.Replace( "\","\\" )
		
		SetConfigVar "ANDROID_MAINFEST_MAIN",GetConfigVar( "ANDROID_MANIFEST_MAIN" ).Replace( ";","~n" )+"~n"
		SetConfigVar "ANDROID_MAINFEST_APPLICATION", GetConfigVar("ANDROID_MANIFEST_APPLICATION").Replace(";", "~n") + "~n"
		SetConfigVar "ANDROID_MAINFEST_ACTIVITY", GetConfigVar("ANDROID_MAINFEST_ACTIVITY").Replace(";", "~n") + "~n"
		
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
		For Local lib:=Eachin GetConfigVar( "LIBS" ).Split( ";" )
			Select ExtractExt( lib )
			Case "jar","so"
				CopyFile lib,"libs/"+StripDir( lib )
			End
		Next

		'copy src files
		For Local src:=Eachin GetConfigVar( "SRCS" ).Split( ";" )
			Select ExtractExt( src )
			Case "java","aidl"
				Local i:=src.FindLast( "/src/" )
				If i<>-1
					Local dst:=src[i+1..]
					If CreateDirRecursive( ExtractDir( dst ) )
						CopyFile src,dst
					Endif
				Endif
			End
		Next
		
		If GetConfigVar( "ANDROID_NATIVE_GL_ENABLED" )="1"
			CopyDir "nativegl/libs","libs",True
			CreateDir "src/com"
			CreateDir "src/com/monkey"
			CopyFile "nativegl/NativeGL.java","src/com/monkey/NativeGL.java"
		Endif

		If tcc.opt_build
		
			Local antcfg:="debug"
			
			If GetConfigVar( "ANDROID_SIGN_APP" )="1" antcfg="release"

			If Not (Execute( "ant clean",False ) And Execute( "ant "+antcfg+" install",False ))

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
