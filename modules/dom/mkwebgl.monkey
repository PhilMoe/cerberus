
Import trans.system

Function MonkeyType$( ty$ )

	#rem
	typedef unsigned long  GLenum;
	typedef boolean        GLboolean;
	typedef unsigned long  GLbitfield;
	typedef byte           GLbyte;         /* 'byte' should be a signed 8 bit type. */
	typedef short          GLshort;
	typedef long           GLint;
	typedef long           GLsizei;
	typedef long long      GLintptr;
	typedef long long      GLsizeiptr;
	typedef unsigned byte  GLubyte;        /* 'unsigned byte' should be an unsigned 8 bit type. */
	typedef unsigned short GLushort;
	typedef unsigned long  GLuint;
	typedef float          GLfloat;
	typedef float          GLclampf;  				
	#end
					
	Select ty
	Case "void"
		Return ":void"
	Case "GLenum","GLbitfield",
		"GLbyte","GLshort","GLint","GLsizei",
		"GLintptr","GLsizeiptr",
		"GLubyte","GLushort","GLuint"
		Return ""
	Case "GLboolean"
		Return "?"
	Case "DOMString"
		Return "$"
	Case "GLfloat","GLclampf"
		Return "#"
	Case "int[]","long[]"
		Return "[]"
	Case "float[]"
		Return "#[]"
	Default
		Return ":"+ty
	End
End

Function Main()

	ChangeDir "../../"
	
	Local src:=LoadString( "webgl.txt" )
	src=src.Replace( "[ ]","[]" )
	
	Local lines:=src.Split( "~n" )
	Local protos:=New StringStack
	Local pline$
	
	protos.Push ""
	For Local line:=EachIn lines

		line=line.Trim()
		If Not line Continue
		
		If line.EndsWith( "," )
			pline+=line
			Continue
		Endif
		
		line=pline+line
		pline=""
		
		If line.StartsWith( "/*" )
			protos.Push "~t'"+line
			Continue
		Endif
		
		If line.StartsWith( "const GLenum " )
			'eg:    const GLenum DEPTH_BUFFER_BIT               = 0x00000100;
			Local i=line.Find( "=" ) 
			If i=-1 Error "ERR"
			line=line[13..i].Trim()
			Local proto:="~tField "+line
			If line.EndsWith( "_" ) proto+="=~q"+line[..-1]+"~q"
			protos.Push proto
			Continue
		Endif
		
		Local i=line.Find( " " )
		If i<>-1

			Local ty:=line[..i]
			line=line[i+1..].Trim()
			
			i=line.Find( "(" )
			If i=-1 Error "ERR:"+line
			
			Local id:=line[..i]
			line=line[i+1..].Trim()
			
			i=line.Find( ")" )
			If i=-1 Error "ERR"
			line=line[..i].Trim()

			Local argp$,err

			If line.Length
				Local args:=line.Split( "," )
				
				For Local arg:=Eachin args
					arg=arg.Trim()
					Local i=arg.Find( " " )
					If i=-1 i=0
					Local ty:=arg[..i]
					Local id:=arg[i+1..]
					
					If argp argp+=","
					argp+=id+MonkeyType( ty )
				Next
			Endif
			
			Local proto$="~tMethod "+id+MonkeyType(ty)+"("+argp+")"
			If err proto="'"+proto
			protos.Push proto
			Continue
		Endif
		
		protos.Push "'~t"+line
		
	Next
	protos.Push ""
	
	Local wgl$=LoadString( "webgl.monkey" )
	Local i=wgl.Find( "'*****[WebGLRenderingContext]*****" )
	If i=-1 Error "ERR"
	i=wgl.Find( "~n",i )
	If i=-1 Error "ERR"
	wgl=wgl[..i]+protos.Join( "~n" )+"End~n"
	
	SaveString wgl,"webgl.monkey"
	
End
