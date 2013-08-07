
Import transcc
Import reflection.reflector

Class Builder

	Method New( tcc:TransCC )
		Self.tcc=tcc
	End
	
	Method IsValid:Bool() Abstract
	
	Method Begin:Void() Abstract
	
	Method MakeTarget:Void() Abstract
	
	Method Make:Void()
	
		Select tcc.opt_config
		Case "","debug"
			tcc.opt_config="debug"
			casedConfig="Debug"
		Case "release" 
			casedConfig="Release"
		Default
			Die "Invalid config"
		End
	
		If FileType( tcc.opt_srcpath )<>FILETYPE_FILE Die "Invalid source file"
		tcc.opt_srcpath=RealPath( tcc.opt_srcpath )

		If Not tcc.opt_modpath tcc.opt_modpath=tcc.monkeydir+"/modules"

		tcc.opt_modpath=".;"+ExtractDir( tcc.opt_srcpath )+";"+tcc.opt_modpath+";"+tcc.monkeydir+"/targets/"+tcc.target.dir+"/modules"
		
		If Not tcc.opt_check
			tcc.opt_check=True
			tcc.opt_update=True
			tcc.opt_build=True
		Endif
		
		ENV_HOST=HostOS
		ENV_CONFIG=tcc.opt_config
		ENV_SAFEMODE=tcc.opt_safe
		ENV_TARGET=tcc.target.system
			
		Self.Begin

		'***** TRANSLATE *****
		If Not tcc.opt_check Return

		Print "Parsing..."
		
		SetCfgVar "HOST",ENV_HOST
		SetCfgVar "LANG",ENV_LANG
		SetCfgVar "TARGET",ENV_TARGET
		SetCfgVar "CONFIG",ENV_CONFIG
		SetCfgVar "SAFEMODE",ENV_SAFEMODE
		SetCfgVar "MODPATH",tcc.opt_modpath
		
		app=ParseApp( tcc.opt_srcpath )

		Print "Semanting..."
		If GetCfgVar("REFLECTION_FILTER")
			Local r:=New Reflector
			r.Semant app
		Else
			app.Semant
		Endif
		
		Print "Translating..."
		Local transbuf:=New StringStack
		For Local file$=Eachin app.fileImports
			If ExtractExt( file ).ToLower()=ENV_LANG
				transbuf.Push LoadString( file )
			Endif
		Next
		transbuf.Push _trans.TransApp( app )
		
		'***** UPDATE *****
		If Not tcc.opt_update Return
		
		Print "Building..."

		transCode=transbuf.Join("")
		
		Local buildPath:=StripExt( tcc.opt_srcpath )+".build"
		
		Local targetPath:=buildPath+"/"+tcc.target.dir	'ENV_TARGET

		If tcc.opt_clean
			DeleteDir targetPath,True
			If FileType( targetPath )<>FILETYPE_NONE Die "Failed to clean target dir"
		Endif

		If FileType( targetPath )=FILETYPE_NONE
			If FileType( buildPath )=FILETYPE_NONE CreateDir buildPath
			If FileType( buildPath )<>FILETYPE_DIR Die "Failed to create build dir: "+buildPath
			If Not CopyDir( tcc.monkeydir+"/targets/"+tcc.target.dir+"/template",targetPath,True,False ) Die "Failed to copy target dir"
		Endif
		If FileType( targetPath )<>FILETYPE_DIR Die "Failed to create target dir: "+targetPath
		
		Local cfgPath:=targetPath+"/CONFIG.MONKEY"
		If FileType( cfgPath )=FILETYPE_FILE PreProcess cfgPath
		
		TEXT_FILES=GetCfgVar( "TEXT_FILES" )
		IMAGE_FILES=GetCfgVar( "IMAGE_FILES" )
		SOUND_FILES=GetCfgVar( "SOUND_FILES" )
		MUSIC_FILES=GetCfgVar( "MUSIC_FILES" )
		BINARY_FILES=GetCfgVar( "BINARY_FILES" )
		
		DATA_FILES=TEXT_FILES
		If IMAGE_FILES DATA_FILES+="|"+IMAGE_FILES
		If SOUND_FILES DATA_FILES+="|"+SOUND_FILES
		If MUSIC_FILES DATA_FILES+="|"+MUSIC_FILES
		If BINARY_FILES DATA_FILES+="|"+BINARY_FILES
		DATA_FILES=DATA_FILES.Replace( ";","|" )
	
		Local cd:=CurrentDir

		ChangeDir targetPath
		Self.MakeTarget
		ChangeDir cd

	End
	
	Field tcc:TransCC
	Field app:AppDecl
	Field transCode:String
	Field casedConfig:String
	Field dataFiles:=New StringMap<String>	'maps real src path to virtual target path
	Field DATA_FILES$
	Field TEXT_FILES$
	Field IMAGE_FILES$
	Field SOUND_FILES$
	Field MUSIC_FILES$
	Field BINARY_FILES$
	
	Method Execute:Bool( cmd:String,failHard:Bool=True )
		Return tcc.Execute( cmd,failHard )
	End
	
	Method CreateDataDir:Void( dir:String )
		dir=RealPath( dir )
	
		DeleteDir dir,True
		CreateDir dir
		
		Local dataPath:=StripExt( tcc.opt_srcpath )+".data"
		
		If FileType( dataPath )=FILETYPE_DIR
		
			Local srcs:=New StringStack
			srcs.Push dataPath
			
			While Not srcs.IsEmpty()
			
				Local src:=srcs.Pop()
				
				For Local f:=Eachin LoadDir( src )
					If f.StartsWith( "." ) Continue

					Local p:=src+"/"+f
					Local r:=p[dataPath.Length+1..]
					Local t:=dir+"/"+r
					
					Select FileType( p )
					Case FILETYPE_FILE
						If MatchPath( r,DATA_FILES )
							CopyFile p,t
							dataFiles.Set p,r
						Endif
					Case FILETYPE_DIR
						CreateDir t
						srcs.Push p
					End
				Next
			
			Wend
		
		Endif
		
		For Local p:=Eachin app.fileImports
			Local r:=StripDir( p )
			Local t:=dir+"/"+r
			If MatchPath( r,DATA_FILES )
				CopyFile p,t
				dataFiles.Set p,r
			Endif
		Next
		
	End
	
End
