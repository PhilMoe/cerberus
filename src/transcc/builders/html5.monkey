
Import builder

Import "makemeta.cpp"

Extern

Global info_width
Global info_height

Function get_info_png( path:String )
Function get_info_jpg( path:String )
Function get_info_gif( path:String )

Public

Class Html5Builder Extends Builder

	Method New( tcc:TransCC )
		Super.New( tcc )
	End
	
	Method Config:String()
		Local config:=New StringStack
		For Local kv:=Eachin GetConfigVars()
			config.Push "CFG_"+kv.Key+"="+Enquote( kv.Value,"js" )+";"
		Next
		Return config.Join( "~n" )
	End
	
	Method MetaData:String()
		Local meta:=New StringStack
		For Local kv:=Eachin dataFiles
			Local src:=kv.Key
			Local ext:=ExtractExt( src ).ToLower()
			Select ext
			Case "png","jpg","gif"
				info_width=0
				info_height=0
				Select ext
				Case "png" get_info_png( src )
				Case "jpg" get_info_jpg( src )
				Case "gif" get_info_gif( src )
				End
				If info_width=0 Or info_height=0 Die "Unable to load image file '"+src+"'."
				meta.Push "["+kv.Value+"];type=image/"+ext+";"
				meta.Push "width="+info_width+";"
				meta.Push "height="+info_height+";"
				meta.Push "\n"
			End
		Next
		Return meta.Join("")
	End
	
	Method IsValid:Bool()
		Return True
	End
	
	Method Begin:Void()
		ENV_LANG="js"
		_trans=New JsTranslator
	End
	
	Method MakeTarget:Void()

		CreateDataDir "data"

		Local meta:="var META_DATA=~q"+MetaData()+"~q;~n"
		
		Local main:=LoadString( "main.js" )
		
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"METADATA",meta )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		SaveString main,"main.js"
		
		If tcc.opt_run
			Local p:=RealPath( "MonkeyGame.html" )
			Local t:=tcc.HTML_PLAYER+" ~q"+p+"~q"
			Execute t,False
		Endif
	End
	
End