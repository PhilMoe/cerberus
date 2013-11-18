
Import builder

Class IosBuilder Extends Builder
	
	Field _nextFileId:=0
	
	Field _fileRefs:=New StringMap<String>
	Field _buildFiles:=New StringMap<String>

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
		Select HostOS
		Case "macos"
			Return True
		End
		Return False
	End

	Method Begin:Void()
		ENV_LANG="cpp"
		_trans=New CppTranslator
	End
	
	Method FileId:String( path:String,map:StringMap<String> )
		Local id:=map.Get( path )
		If id Return id
		_nextFileId+=1
		id="1ACECAFEBABE"+("0000000000000000"+String(_nextFileId))[-12..]
		map.Set path,id
		Return id
	End

	Method BuildFiles:String()
		Local buf:=New StringStack
		For Local it:=Eachin _buildFiles
			Local path:=it.Key
			Local id:=it.Value
			Local fileRef:=FileId( path,_fileRefs )
			Local dir:=ExtractDir( path )
			Local name:=StripDir( path )
			Select ExtractExt( name )
			Case "a","framework"
				buf.Push "~t~t"+id+" = {isa = PBXBuildFile; fileRef = "+fileRef+"; };"
			End
		Next
		If buf.Length buf.Push ""
		Return buf.Join( "~n" )
	End
	
	Method FileRefs:String()
		Local buf:=New StringStack
		For Local it:=Eachin _fileRefs
			Local path:=it.Key
			Local id:=it.Value
			Local dir:=ExtractDir( path )
			Local name:=StripDir( path )
			Select ExtractExt( name )
			Case "a"
				buf.Push "~t~t"+id+" = {isa = PBXFileReference; lastKnownFileType = archive.ar; path = ~q"+name+"~q; sourceTree = ~q<group>~q; };"
			Case "h"				
				buf.Push "~t~t"+id+" = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.h; path = "+name+"; sourceTree = ~q<group>~q; };"
			Case "framework"
				If dir Die "System frameworks only supported"
				buf.Push "~t~t"+id+" = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = "+name+"; path = System/Library/Frameworks/"+name+"; sourceTree = SDKROOT; };"
			End				
		Next
		If buf.Length buf.Push ""
		Return buf.Join( "~n" )
	End
	
	Method FrameworksBuildPhase:String()
		Local buf:=New StringStack
		For Local it:=Eachin _buildFiles
			Local path:=it.Key
			Local id:=it.Value
			Select ExtractExt( path )
			Case "a","framework"
				buf.Push "~t~t~t~t"+id
			End
		Next
		If buf.Length buf.Push ""
		Return buf.Join( ",~n" )
	End
	
	Method FrameworksGroup:String()
		Local buf:=New StringStack
		For Local it:=Eachin _fileRefs
			Local path:=it.Key
			Local id:=it.Value
			Select ExtractExt( path )
			Case "framework"
				buf.Push "~t~t~t~t"+id
			End
		Next
		If buf.Length buf.Push ""
		Return buf.Join( ",~n" )
	end
	
	Method LibsGroup:String()
		Local buf:=New StringStack
		For Local it:=Eachin _fileRefs
			Local path:=it.Key
			Local id:=it.Value
			Select ExtractExt( path )
			Case "a","h"
				buf.Push "~t~t~t~t"+id
			End
		Next
		If buf.Length buf.Push ""
		Return buf.Join( ",~n" )
	End
	
	Method AddBuildFile:Void( path:String )
		FileId path,_buildFiles
	End
	
	Method FindEol:Int( str:String,substr:String,start:Int=0 )
		Local i:=str.Find( substr,start )
		If i=-1
			Print "Can't find "+substr
			Return -1
		Endif
		i+=substr.Length
		Local eol:=str.Find( "~n",i )+1
		If eol=0 Return str.Length
		Return eol
	End
	
	Method MungProj:String( proj:String )
	
		Local i:=-1
		
		i=FindEol( proj,"/* Begin PBXBuildFile section */" )
		If i=-1 Return ""
		proj=proj[..i]+BuildFiles()+proj[i..]
		
		i=FindEol( proj,"/* Begin PBXFileReference section */" )
		If i=-1 Return ""
		proj=proj[..i]+FileRefs()+proj[i..]
		
		i=FindEol( proj,"/* Begin PBXFrameworksBuildPhase section */" )
		If i<>-1 i=FindEol( proj,"/* Frameworks */ = {",i )
		If i<>-1 i=FindEol( proj,"files = (",i )
		If i=-1 Return ""
		proj=proj[..i]+FrameworksBuildPhase()+proj[i..]
		
		i=FindEol( proj,"/* Begin PBXGroup section */" )
		If i<>-1 i=FindEol( proj,"/* Frameworks */ = {",i )
		If i<>-1 i=FindEol( proj,"children = (",i )
		If i=-1 Return ""
		proj=proj[..i]+FrameworksGroup()+proj[i..]
		
		
		
		i=FindEol( proj,"/* Begin PBXGroup section */" )
		If i<>-1 i=FindEol( proj,"/* libs */ = {",i )
		If i<>-1 i=FindEol( proj,"children = (",i )
		If i=-1 Return ""
		proj=proj[..i]+LibsGroup()+proj[i..]
		
		Return proj
		
	End
	
	Method Backup:Void( path:String )
		Local path2:=path+"_"
		If FileType( path2 )
			CopyFile path2,path
		Else
			CopyFile path,path2
		Endif
	End
	
	Method MungProj:Void()
	
		Local path:="MonkeyGame.xcodeproj/project.pbxproj"
		
		Local proj:=LoadString( path )
		
		'Ok, this ain't pretty...
		Local buf:=New StringStack
		For Local line:=Eachin proj.Split( "~n" )
			If Not line.Trim().StartsWith( "1ACECAFEBABE" ) buf.Push line
		Next
		proj=buf.Join( "~n" )
		
'		Backup path
'		Local proj:=LoadString( path )
		
		proj=MungProj( proj )
		If Not proj Die "Failed to mung XCode project file"
		
		SaveString proj,path
		
	End
	
	Method MakeTarget:Void()
	
		CreateDataDir "data"

		Local main:=LoadString( "main.mm" )
		
		main=ReplaceBlock( main,"TRANSCODE",transCode )
		main=ReplaceBlock( main,"CONFIG",Config() )
		
		SaveString main,"main.mm"
		
		'mung xcode project
		Local libs:=GetConfigVar( "LIBS" )
		If libs
			For Local lib:=Eachin libs.Split( ";" )
				If Not lib Continue
				Select ExtractExt( lib )
				Case "a","h"
					Local path:="libs/"+StripDir( lib )
					CopyFile lib,path
					AddBuildFile path
				Case "framework"
					AddBuildFile lib
				Default
					Die "Unrecognized lib file type:"+lib
				End
			Next
		Endif
		MungProj
		
		If Not tcc.opt_build Return
		
		Execute "xcodebuild -configuration "+casedConfig+" -sdk iphonesimulator"
		
		If Not tcc.opt_run Return

		Local home:=GetEnv( "HOME" )

		Local uuid:="00C69C9A-C9DE-11DF-B3BE-5540E0D72085"
		
		Local src:="build/"+casedConfig+"-iphonesimulator/MonkeyGame.app"
		
		Local sim_path:="/Applications/Xcode.app/Contents/Applications/iPhone Simulator.app"
		If FileType( sim_path )=FILETYPE_NONE sim_path="/Applications/Xcode.app/Contents/Developer/Builders/iPhoneSimulator.platform/Developer/Applications/iPhone Simulator.app"
		
		'New XCode in /Applications?
		If FileType( sim_path )=FILETYPE_DIR
		
			Local dst:=""
			
			For Local f:=Eachin LoadDir( home+"/Library/Application Support/iPhone Simulator" )

				If f.Length>2 And f[0]>48 And f[0]<58 And f[1]=46 And f[2]>=48 And f[2]<58 And Not f.Contains( "-64" ) And f>dst dst=f
				
				'X.Y?
				'If f.Length=3 And f[0]>48 And f[0]<58 And f[1]=46 And f[2]>=48 And f[2]<58 And Float(f)>Float(dst) dst=f
			Next
			If Not dst Die "Can't find iPhone simulator app version dir"
			
			dst=home+"/Library/Application Support/iPhone Simulator/"+dst+"/Applications"

			CreateDir dst
			If FileType( dst )<>FILETYPE_DIR Die "Failed to create dir:"+dst
			
			dst+="/"+uuid
			If Not DeleteDir( dst,True ) Die "Failed to delete dir:"+dst
			If Not CreateDir( dst ) Die "Failed to create dir:"+dst
			
			'Need to use this 'coz it does the permissions thang
			'
			Execute "cp -r ~q"+src+"~q ~q"+dst+"/MonkeyGame.app~q"
			
			'Have to manually create documents dir for monkey://internal/?
			'
			CreateDir dst+"/Documents"

			're-start emulator
			'
			Execute "killall ~qiPhone Simulator~q 2>/dev/null",False
			Execute "open ~q"+sim_path+"~q"
			
			Return
			
		Endif

		'Old xcode in /Developer				
		sim_path="/Developer/Builders/iPhoneSimulator.platform/Developer/Applications/iPhone Simulator.app"
		
		If FileType( sim_path )=FILETYPE_DIR
		
			Local dst:=home+"/Library/Application Support/iPhone Simulator/4.3.2"
			If FileType( dst )=FILETYPE_NONE
				dst=home+"/Library/Application Support/iPhone Simulator/4.3"
				If FileType( dst )=FILETYPE_NONE
					dst=home+"/Library/Application Support/iPhone Simulator/4.2"
				Endif
			Endif
			
			CreateDir dst
			dst+="/Applications"
			CreateDir dst
			dst+="/"+uuid
			If Not DeleteDir( dst,True ) Die "Failed to delete dir:"+dst
			If Not CreateDir( dst ) Die "Failed to create dir:"+dst
			
			'Need to use this 'coz it does the permissions thang
			'
			Execute "cp -r ~q"+src+"~q ~q"+dst+"/MonkeyGame.app~q"

			're-start emulator
			'
			Execute "killall ~qiPhone Simulator~q 2>/dev/null",False
			Execute "open ~q"+sim_path+"~q"
			
			Return
		
		Endif
		
	End
End

