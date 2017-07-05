
' Module trans.javatranslator
'
' Placed into the public domain 24/02/2011.
' No warranty implied; use at your own risk.

Import trans

Class JavaTranslator Extends CTranslator

	Field unsafe
	
	Field langutil:=False

	Method TransType$( ty:Type )
		If VoidType( ty ) Return "void"
		If BoolType( ty ) Return "boolean"
		If IntType( ty ) Return "int"
		If FloatType( ty ) Return "float"
		If StringType( ty ) Return "String"
		If ArrayType( ty ) Return TransType( ArrayType( ty ).elemType )+"[]"
		If ObjectType( ty ) Return ty.GetClass().munged
		InternalErr
	End
	
	Method TransValue$( ty:Type,value$ )
		If value
			If IntType( ty ) And value.StartsWith( "$" ) Return "0x"+value[1..]
			If BoolType( ty ) Return "true"
			If IntType( ty ) Return value
			If FloatType( ty ) Return value+"f"
			If StringType( ty ) Return Enquote( value )
		Else
			If BoolType( ty ) Return "false"
			If NumericType( ty ) Return "0"
			If StringType( ty ) Return "~q~q"
			If ArrayType( ty ) 
				Local elemTy:=ArrayType( ty ).elemType
				If BoolType( elemTy ) Return "bb_std_lang.emptyBoolArray"
				If IntType( elemTy ) Return "bb_std_lang.emptyIntArray"
				If FloatType( elemTy ) Return "bb_std_lang.emptyFloatArray"
				If StringType( elemTy ) Return "bb_std_lang.emptyStringArray"
				Local t$="[0]"
				While ArrayType( elemTy )
					elemTy=ArrayType( elemTy ).elemType
					t+="[]"
				Wend
				Return "new "+TransType( elemTy )+t
			Endif
			If ObjectType( ty ) Return "null"
		Endif
		InternalErr
	End

	Method TransDecl$( decl:Decl )
		Local id$=decl.munged

		Local vdecl:=ValDecl( decl )
		If vdecl Return TransType( vdecl.type )+" "+id
		
		InternalErr
	End
	
	Method TransArgs$( args:Expr[] )
		Local t$
		For Local arg:=Eachin args
			If t t+=","
			t+=arg.Trans()
		Next
		Return Bra(t)
	End
	
	'***** Utility *****
	
	Method TransLocalDecl$( munged$,init:Expr )
		Return TransType( init.exprType )+" "+munged+"="+init.Trans()
	End
	
	Method EmitEnter( func:FuncDecl )

		If unsafe Return
	
		Emit "bb_std_lang.pushErr();"
	End
	
	Method EmitSetErr( info$ )
	
		If unsafe Return
		
		Emit "bb_std_lang.errInfo=~q"+info.Replace( "\","/" )+"~q;"
	End
	
	Method EmitLeave()

		If unsafe Return
	
		Emit "bb_std_lang.popErr();"
	End

	'***** Declarations *****
	
	Method TransStatic$( decl:Decl )
		If decl.IsExtern() And ModuleDecl( decl.scope )
			Return decl.munged
		Else If _env And decl.scope And decl.scope=_env.ClassScope()
			Return decl.munged
		Else If ClassDecl( decl.scope )
			Return decl.scope.munged+"."+decl.munged
		Else If ModuleDecl( decl.scope )
			Return decl.scope.munged+"."+decl.munged
		Endif
		InternalErr
	End
	
	Method TransGlobal$( decl:GlobalDecl )
		Return TransStatic( decl )
	End
	
	Method TransField$( decl:FieldDecl,lhs:Expr )
		If lhs Return TransSubExpr( lhs )+"."+decl.munged
		Return decl.munged
	End
		
	Method TransFunc$( decl:FuncDecl,args:Expr[],lhs:Expr )
		If decl.IsMethod()
			If lhs Return TransSubExpr( lhs )+"."+decl.munged+TransArgs( args )
			Return decl.munged+TransArgs( args )
		Endif
		Return TransStatic( decl )+TransArgs( args )
	End
	
	Method TransSuperFunc$( decl:FuncDecl,args:Expr[] )
		Return "super."+decl.munged+TransArgs( args )
	End

	'***** Expressions *****

	Method TransConstExpr$( expr:ConstExpr )
		Return TransValue( expr.exprType,expr.value )
	End
	
	Method TransNewObjectExpr$( expr:NewObjectExpr )
		Local t$="(new "+expr.classDecl.munged+"())"
		If expr.ctor t+="."+expr.ctor.munged+TransArgs( expr.args )
		Return t
	End
	
	Method TransNewArrayExpr$( expr:NewArrayExpr )
		Local texpr$=expr.expr.Trans()
		Local elemTy:=ArrayType( expr.exprType ).elemType
		'
		If StringType( elemTy ) Return "bb_std_lang.stringArray"+Bra(texpr)
		'		
		Local t$="["+texpr+"]"
		While ArrayType( elemTy )
			elemTy=ArrayType( elemTy ).elemType
			t+="[]"
		Wend
		'
		Return "new "+TransType( elemTy )+t
	End
		
	Method TransSelfExpr$( expr:SelfExpr )
		Return "this"
	End
	
	Method TransCastExpr$( expr:CastExpr )

		Local texpr$=Bra( expr.expr.Trans() )
		
		Local dst:=expr.exprType
		Local src:=expr.expr.exprType

		If BoolType( dst )
			If BoolType( src ) Return texpr
			If IntType( src ) Return Bra( texpr+"!=0" )
			If FloatType( src ) Return Bra( texpr+"!=0.0f" )
			If StringType( src ) Return Bra( texpr+".length()!=0" )
			If ArrayType( src ) Return Bra( texpr+".length!=0" )
			If ObjectType( src ) Return Bra( texpr+"!=null" )
		Else If IntType( dst )
			If BoolType( src ) Return Bra( texpr+"?1:0" )
			If IntType( src ) Return texpr
			If FloatType( src ) Return "(int)"+texpr
			
			If langutil
				If StringType( src ) Return "LangUtil.parseInt("+texpr+".trim())"
			Else
				If StringType( src ) Return "Integer.parseInt("+texpr+".trim())"
			Endif
			
		Else If FloatType( dst )
			If IntType( src ) Return "(float)"+texpr
			If FloatType( src ) Return texpr
			
			If langutil
				If StringType( src ) Return "LangUtil.parseFloat("+texpr+".trim())"
			Else
				If StringType( src ) Return "Float.parseFloat("+texpr+".trim())"
			Endif
			
		Else If StringType( dst )
			If IntType( src ) Return "String.valueOf"+texpr
			If FloatType( src ) Return "String.valueOf"+texpr
			If StringType( src ) Return texpr
		Else If ObjectType( dst ) And ObjectType( src )
			If src.GetClass().ExtendsClass( dst.GetClass() )
				Return texpr
			Else
				Return "bb_std_lang.as("+TransType(dst)+".class,"+texpr+")"
'				Local tmp:=New LocalDecl( "",0,src,Null )
'				MungDecl tmp
'				Emit TransType( src )+" "+tmp.munged+"="+expr.expr.Trans()+";"
'				Return "($t instanceof $c ? ($c)$t : null)".Replace( "$t",tmp.munged ).Replace( "$c",TransType(dst) )
			Endif
		Endif
		Err "Java translator can't convert "+src.ToString()+" to "+dst.ToString()
	End
	
	Method TransUnaryExpr$( expr:UnaryExpr )
		Local texpr$=expr.expr.Trans()
		If ExprPri( expr.expr )>ExprPri( expr ) texpr=Bra( texpr )
		Return TransUnaryOp( expr.op )+texpr
	End
	
	Method TransBinaryExpr$( expr:BinaryExpr )
		Local lhs$=expr.lhs.Trans()
		Local rhs$=expr.rhs.Trans()
		
		'String compare
		If BinaryCompareExpr( expr ) And StringType( expr.lhs.exprType ) And StringType( expr.rhs.exprType )
			If ExprPri( expr.lhs )>2 lhs=Bra( lhs )
			Return Bra( lhs+".compareTo"+Bra(rhs)+TransBinaryOp( expr.op,"" )+"0" )
		Endif
		
		Local pri=ExprPri( expr )
		If ExprPri( expr.lhs )>pri lhs=Bra( lhs )
		If ExprPri( expr.rhs )>=pri rhs=Bra( rhs )
		Return lhs+TransBinaryOp( expr.op,rhs )+rhs
	End
	
	Method TransIndexExpr$( expr:IndexExpr )
		Local texpr$=expr.expr.Trans()
		Local index$=expr.index.Trans()
		If StringType( expr.expr.exprType ) Return "(int)"+texpr+".charAt("+index+")"
		Return texpr+"["+index+"]"
	End
	
	Method TransSliceExpr$( expr:SliceExpr )
		Local texpr$=expr.expr.Trans()
		Local from$=",0",term$
		If expr.from from=","+expr.from.Trans()
		If expr.term term=","+expr.term.Trans()
		If ArrayType( expr.exprType )
			Return "(("+TransType( expr.exprType )+")bb_std_lang.sliceArray"+Bra( texpr+from+term )+")"
		Else If StringType( expr.exprType )
			Return "bb_std_lang.slice("+texpr+from+term+")"
		Endif
		InternalErr
	End
	
	Method TransArrayExpr$( expr:ArrayExpr )
		Local t$
		For Local elem:=Eachin expr.exprs
			If t t+=","
			t+=elem.Trans()
		Next
		Return "new "+TransType( expr.exprType )+"{"+t+"}"
	End

	Method TransIntrinsicExpr$( decl:Decl,expr:Expr,args:Expr[] )
		Local texpr$,arg0$,arg1$,arg2$
		
		If expr texpr=TransSubExpr( expr )
		
		If args.Length>0 And args[0] arg0=args[0].Trans()
		If args.Length>1 And args[1] arg1=args[1].Trans()
		If args.Length>2 And args[2] arg2=args[2].Trans()
		
		Local id$=decl.munged[1..]
		
		Local fmath:="(float)Math."
		
		'Could be slower on devices with FPUs?!?
		'If ENV_TARGET="android" fmath="FloatMath."
		
		Select id

		'global functions
		Case "print" Return "bb_std_lang.print"+Bra( arg0 )
		Case "error" Return "bb_std_lang.error"+Bra( arg0 )
		Case "debuglog" Return "bb_std_lang.debugLog"+Bra( arg0 )
		Case "debugstop" Return "bb_std_lang.debugStop()"
				
		'string/array methods
		Case "length"
			If StringType( expr.exprType ) Return texpr+".length()"
			Return "bb_std_lang.length"+Bra( texpr )
		Case "resize"
			Local ty:=ArrayType( expr.exprType ).elemType
			If StringType( ty ) Return "bb_std_lang.resize("+texpr+","+arg0+")"
			Local ety:=TransType( ty )
			Return "("+ety+"[])bb_std_lang.resize("+texpr+","+arg0+","+ety+".class)"
		'string methods
		Case "compare" Return texpr+".compareTo"+Bra( arg0 )
		Case "find" Return texpr+".indexOf"+Bra( arg0+","+arg1 )
		Case "findlast" Return texpr+".lastIndexOf"+Bra( arg0 )
		Case "findlast2" Return texpr+".lastIndexOf"+Bra( arg0+","+arg1 )
		Case "trim" Return texpr+".trim()"
		Case "join" Return "bb_std_lang.join"+Bra( texpr+","+arg0 )
		Case "split" Return "bb_std_lang.split"+Bra( texpr+","+arg0 )
		Case "replace" Return "bb_std_lang.replace"+Bra( texpr+","+arg0+","+arg1 )
		Case "tolower" Return texpr+".toLowerCase()"
		Case "toupper" Return texpr+".toUpperCase()"
		Case "contains" Return Bra( texpr+".indexOf"+Bra( arg0 )+"!=-1" )
		Case "startswith" Return texpr+".startsWith"+Bra( arg0 )
		Case "endswith" Return texpr+".endsWith"+Bra( arg0 )
		Case "tochars" Return "bb_std_lang.toChars"+Bra( texpr )

		'string functions		
		Case "fromchar" Return "String.valueOf"+Bra("(char)"+Bra( arg0 ) )
		Case "fromchars" Return "bb_std_lang.fromChars"+Bra( arg0 )

		'trig methods - degrees
		Case "sin","cos" Return fmath+id+Bra( Bra(arg0)+"*bb_std_lang.D2R" )
		Case "tan" Return "(float)Math."+id+Bra( Bra(arg0)+"*bb_std_lang.D2R" )
		Case "asin","acos","atan" Return "(float)"+Bra( "Math."+id+Bra(arg0)+"*bb_std_lang.R2D" )
		Case "atan2" Return "(float)"+Bra( "Math."+id+Bra(arg0+","+arg1)+"*bb_std_lang.R2D" )

		'trig methods - radians
		Case "sinr","cosr" Return fmath+id[..-1]+Bra( arg0 )
		Case "tanr" Return "(float)Math."+id[..-1]+Bra( arg0 )
		Case "asinr","acosr","atanr" Return "(float)Math."+id[..-1]+Bra( arg0 )
		Case "atan2r" Return "(float)Math."+id[..-1]+Bra( arg0+","+arg1 )

		'misc math functions
		Case "sqrt","floor","ceil" Return fmath+id+Bra(arg0)
		Case "log","exp" Return "(float)Math."+id+Bra(arg0)
  		Case "pow" Return "(float)Math."+id+Bra( arg0+","+arg1 )

		End Select
		
		InternalErr
	End

	'***** Statements *****
	
	Method TransTryStmt$( stmt:TryStmt )
		Emit "try{"
		Local unr:=EmitBlock( stmt.block )
		For Local c:=Eachin stmt.catches
			MungDecl c.init
			Emit "}catch("+TransType( c.init.type )+" "+c.init.munged+"){"
			Local unr:=EmitBlock( c.block )
		Next
		Emit "}"
	End

	'***** Declarations *****
	
	Method EmitFuncDecl( decl:FuncDecl )
	
		unsafe=decl.ident.EndsWith( "__UNSAFE__" )

		BeginLocalScope
		
		Local args$
		For Local arg:=Eachin decl.argDecls
			MungDecl arg
			If args args+=","
			args+=TransType( arg.type )+" "+arg.munged
		Next
		
		Local t$=TransType( decl.retType )+" "+decl.munged+Bra( args )
		
		If decl.ClassScope() And decl.ClassScope().IsInterface()
			Emit "public "+t+";"
		Else If decl.IsAbstract()
			Emit "public abstract "+t+";"
		Else
			Local q$="public "
			If decl.IsStatic()
				q+="static "
			Else If Not decl.IsVirtual()
				q+="final "
			Endif
			
			Emit q+t+"{"
			EmitBlock decl
			Emit "}"
		Endif
		
		EndLocalScope
		
		unsafe=False
	End
	
	Method EmitClassDecl( classDecl:ClassDecl )
	
		Local classid$=classDecl.munged
		Local superid$=classDecl.superClass.munged

		If classDecl.IsInterface() 

			Local bases$
			For Local iface:=Eachin classDecl.implments
				If bases bases+="," Else bases=" extends "
				bases+=iface.munged
			Next

			Emit "interface "+classid+bases+"{"
			
			For Local decl:=Eachin classDecl.Semanted
				Local fdecl:=FuncDecl( decl )
				If Not fdecl Continue
				EmitFuncDecl fdecl
			Next

			Emit "}"
			Return
		Endif
		
		Local bases$
		For Local iface:=Eachin classDecl.implments
			If bases bases+="," Else bases=" implements "
			bases+=iface.munged
		Next
		
		Local q$
		If classDecl.IsAbstract() q="abstract " Else If classDecl.IsFinal() q="final "
		
		Emit q+"class "+classid+" extends "+superid+bases+"{"
		
		For Local decl:=Eachin classDecl.Semanted

			Local tdecl:=FieldDecl( decl )
			If tdecl
				Emit TransDecl( tdecl )+"="+tdecl.init.Trans()+";"
				Continue
			Endif
			
			Local fdecl:=FuncDecl( decl )
			If fdecl
				EmitFuncDecl fdecl
				Continue
			Endif
			
			Local gdecl:=GlobalDecl( decl )
			If gdecl
				Emit "static "+TransDecl( gdecl )+";"
				Continue
			Endif
		Next
		
		Emit "}"

	End
	
	Method TransApp$( app:AppDecl )
	
		langutil=(GetConfigVar( "ANDROID_LANGUTIL_ENABLED" )="1")

		app.mainModule.munged="bb_"
		app.mainFunc.munged="bbMain"
		
		For Local decl:=Eachin app.imported.Values()
			MungDecl decl
		Next

		For Local decl:=Eachin app.Semanted
		
			MungDecl decl

			Local cdecl:=ClassDecl( decl )
			If Not cdecl Continue
			
			For Local decl:=Eachin cdecl.Semanted
			
				If FuncDecl( decl ) And FuncDecl( decl ).IsCtor()
					decl.ident=cdecl.ident+"_"+decl.ident
				Endif
			
				MungDecl decl
			Next
		Next

		For Local decl:=Eachin app.Semanted
			
			Local cdecl:=ClassDecl( decl )
			If cdecl
				EmitClassDecl cdecl
			Endif
		Next
		
		For Local mdecl:=Eachin app.imported.Values()

			Emit "class "+mdecl.munged+"{"

			For Local decl:=Eachin mdecl.Semanted
				If decl.IsExtern() Or decl.scope.ClassScope() Continue
	
				Local gdecl:=GlobalDecl( decl )
				If gdecl
					Emit "static "+TransDecl( gdecl )+";"
					Continue
				Endif
				
				Local fdecl:=FuncDecl( decl )
				If fdecl
					EmitFuncDecl fdecl
					Continue
				Endif
			Next
			
			If mdecl=app.mainModule
				BeginLocalScope
				Emit "public static int bbInit(){"
				For Local decl:=Eachin app.semantedGlobals
					Emit TransGlobal( decl )+"="+decl.init.Trans()+";"
				Next
				Emit "return 0;"
				Emit "}"
				EndLocalScope
			Endif
			
			Emit "}"
		Next
		
		Return JoinLines()

	End

#rem	
	Method PostProcess$( source$ )
		'
		'move package/imports to top
		'
		Local lines$[]=source.Split( "~n" )
		
		Local pkg$,imps$,code$,imped:=New StringMap<StringObject>
		
		For Local line$=Eachin lines
			'
			line+="~n"
			'
			If line.StartsWith( "package" )
				If pkg Err "Multiple package decls"
				pkg=line
  			Else If line.StartsWith( "import " ) 
				Local i=line.Find( ";" )
				If i=-1 InternalErr
				line=line[..i+1]
				If Not imped.Contains( line )
					imps+=line+"~n"
					imped.Insert line,line
				Endif
			Else
				code+=line
 			Endif
		Next
		Return "~n"+pkg+"~n"+imps+"~n"+code
	End
#end
	
End

