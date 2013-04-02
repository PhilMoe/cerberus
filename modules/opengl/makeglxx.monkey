
Import os

Function ReplaceBlock$( text$,startTag$,endTag$,repText$ )

	'Find *first* start tag
	Local i=text.Find( startTag )
	If i=-1 Return text
	i+=startTag.Length
	While i<text.Length And text[i-1]<>10
		i+=1
	Wend
	'Find *last* end tag
	Local i2=text.Find( endTag,i )
	If i2=-1 Return text
	While i2>0 And text[i2]<>10
		i2-=1
	Wend
	'replace text!
	Return text[..i]+repText+text[i2..]
End

Function MakeGL11()

	Local kludge_glfw,kludge_android,kludge_ios

	Local const_decls:=New StringStack
	Local glfw_decls:=New StringStack
	Local ios_decls:=New StringStack
	Local android_decls:=New StringStack

	Print "Parsing gles11_src.txt"
		
	Local src:=LoadString( "gles11_src.txt" )
	
	Local lines:=src.Split( "~n" )
	
	For Local line:=Eachin lines
	
		line=line.Trim()
		
		If line.StartsWith( "Const " )
		
			const_decls.Push( line )
			
		Else If line.StartsWith( "Kludge " )
		
			kludge_glfw=False
			kludge_android=False
			kludge_ios=False
		
			If line.Contains( "glfw" ) kludge_glfw=True
			If line.Contains( "android" ) kludge_android=True
			If line.Contains( "ios" ) kludge_ios=True
			
		Else If line.StartsWith( "Function " )
			Local i=line.Find( "(" )
			If i<>-1
				Local id$=line[9..i]
				Local i2=id.Find( ":" )
				If i2<>-1 id=id[..i2]
				
				'GLFW!
				If kludge_glfw
					glfw_decls.Push line+"=~q_"+id+"~q"
				Else
					glfw_decls.Push line
				Endif
				
				'Android!
				If kludge_android
					android_decls.Push line+"=~qbb_opengl_gles11._"+id+"~q"
				Else
					android_decls.Push line+"=~qGLES11."+id+"~q"
				Endif
				
				'ios!
				If kludge_ios
					ios_decls.Push line+"=~q_"+id+"~q"
				Else
					ios_decls.Push line
				Endif
				
			Endif
		Endif
		
	Next
	
	Print "Updating gles11.monkey..."
	
	Local dst:=LoadString( "gles11.monkey" )
	
	dst=ReplaceBlock( dst,"'${CONST_DECLS}","'${END}",const_decls.Join( "~n" ) )
	dst=ReplaceBlock( dst,"'${GLFW_DECLS}","'${END}",glfw_decls.Join( "~n" ) )
	dst=ReplaceBlock( dst,"'${ANDROID_DECLS}","'${END}",android_decls.Join( "~n" ) )
	dst=ReplaceBlock( dst,"'${IOS_DECLS}","'${END}",ios_decls.Join( "~n" ) )

'	Print dst	
	SaveString dst,"gles11.monkey"

	Print "Done!"
End

Function MakeGL20()

	Local kludge_glfw,kludge_android,kludge_ios,kludge_html5

	Local const_decls:=New StringStack
	Local glfw_decls:=New StringStack
	Local android_decls:=New StringStack
	Local ios_decls:=New StringStack
	Local html5_decls:=New StringStack

	Print "Parsing gles20_src.txt"
		
	Local src:=LoadString( "gles20_src.txt" )
	
	Local lines:=src.Split( "~n" )
	
	Local last_id$,rep_id
	
	For Local line:=Eachin lines
	
		line=line.Trim()
		
		If line.StartsWith( "Const " )
		
			const_decls.Push( line )
			
		Else If line.StartsWith( "Kludge " )
		
			kludge_glfw=False
			kludge_android=False
			kludge_ios=False
			kludge_html5=False

			If line.Contains( "all" )
				kludge_glfw=True
				kludge_android=True
				kludge_ios=True
				kludge_html5=True
			Else
				If line.Contains( "glfw" ) kludge_glfw=True
				If line.Contains( "android" ) kludge_android=True
				If line.Contains( "ios" ) kludge_ios=True
				If line.Contains( "html5" ) kludge_html5=True
			Endif

		Else If line.StartsWith( "Function " )
			Local i=line.Find( "(" )
			If i<>-1
				Local id$=line[9..i]
				Local i2=id.Find( ":" )
				If i2<>-1 id=id[..i2]
				
				'GLFW!
				If kludge_glfw
					glfw_decls.Push line+"=~q_"+id+"~q"
				Else
					glfw_decls.Push line
				Endif
				
				'Android!
				If kludge_android
					android_decls.Push line+"=~qbb_opengl_gles20._"+id+"~q"
				Else
					android_decls.Push line+"=~qGLES20."+id+"~q"
				Endif
				
				'ios!
				If kludge_ios
					ios_decls.Push line+"=~q_"+id+"~q"
				Else
					ios_decls.Push line
				Endif
				
				'html5!
				If kludge_html5
					If id=last_id
						rep_id+=1
						html5_decls.Push line+"=~q_"+id+rep_id+"~q"
					Else
						last_id=id
						rep_id=1
						html5_decls.Push line+"=~q_"+id+"~q"
					Endif
				Else
					last_id=""
					html5_decls.Push line+"=~qgl."+id[2..3].ToLower()+id[3..]+"~q"
				Endif
				
			Endif
		Endif
		
	Next
	
	Print "Updating gles20.monkey..."
	
	Local dst:=LoadString( "gles20.monkey" )
	
	dst=ReplaceBlock( dst,"'${CONST_DECLS}","'${END}",const_decls.Join( "~n" ) )
	dst=ReplaceBlock( dst,"'${GLFW_DECLS}","'${END}",glfw_decls.Join( "~n" ) )
	dst=ReplaceBlock( dst,"'${ANDROID_DECLS}","'${END}",android_decls.Join( "~n" ) )
	dst=ReplaceBlock( dst,"'${IOS_DECLS}","'${END}",ios_decls.Join( "~n" ) )
	dst=ReplaceBlock( dst,"'${HTML5_DECLS}","'${END}",html5_decls.Join( "~n" ) )

'	Print dst	
	SaveString dst,"gles20.monkey"

	Print "Done!"
End

Function Toke$( text$ )
	Local i=text.Find( " " )
	If i=-1 Return text
	Return text[..i]
End

Function Bump$( text$ )
	Local i=text.Find( " " )
	If i=-1 Return ""
	Return text[i+1..].Trim()
End

Function MakeGL20Exts()

	Local gl:=LoadString( "GL.h" )
	Local gl2:=LoadString( "gles20.h" )

	Local decls:=New StringStack
	Local inits:=New StringStack
	
	decls.Push "typedef char GLchar;"
	decls.Push "typedef size_t GLintptr;"
	decls.Push "typedef size_t GLsizeiptr;"
	decls.Push "#define INIT_GL_EXTS 1"
		
	For Local line:=Eachin gl2.Split( "~n" )
	
		line=line.Trim()
		If Not line Continue
		
		Local tline:=line
		
		If Toke( line )="#define"
			line=Bump( line )
			Local id:=Toke( line )
			If gl.Find( "#define "+id )=-1
				line=Bump( line )
				Local val:=Toke( line )
				decls.Push "#define "+id+" "+val
			Else
				'Print "//"+tline
			Endif
		Else If Toke( line )="GL_APICALL"
			line=Bump( line )
			Local ret:=Toke( line )
			line=Bump( line )
			If Toke( line )="GL_APIENTRY"
				line=Bump( line )
				Local id:=Toke( line )
				If gl.Find( "APIENTRY "+id )=-1
					line=Bump( line )
					Local args:=line
					
					decls.Push ret+"(__stdcall*"+id+")"+args
					inits.Push "(void*&)"+id+"=(void*)wglGetProcAddress(~q"+id+"~q);"

				Else
					'Print "//"+tline
				Endif
			Endif
		Endif
	Next
	
	Local t:="#if _WIN32~n"+decls.Join("~n")+"~nvoid Init_GL_Exts(){~n~t"+inits.Join( "~n~t" )+"~n}~n#endif~n"
	
	SaveString t,"native/gles20_win32_exts.cpp"
		
End

Function Main()

	ChangeDir "../../"

	Print "MakeGL11"
	MakeGL11
	
	Print "MakeGL20..."
	MakeGL20
	
	Print "MakeGl20Exts..."
	MakeGL20Exts

	Print "Done!"
		
End
