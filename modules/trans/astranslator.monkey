
' Module trans.astranslator
'
' Placed into the public domain 24/02/2011.
' No warranty implied; use at your own risk.

Import trans

Class AsTranslator Extends CTranslator

	Method TransType$( ty:Type )
		If VoidType( ty ) Return "void"
		If BoolType( ty ) Return "Boolean"
		If IntType( ty ) Return "int"
		If FloatType( ty ) Return "Number"
		If StringType( ty ) Return "String"
		If ArrayType( ty ) Return "Array"
		If ObjectType( ty ) Return ObjectType( ty ).classDecl.munged
		InternalErr
	End
	
	Method TransValue$( ty:Type,value$ )
		If value
			If IntType( ty ) And value.StartsWith( "$" ) Return "0x"+value[1..]
			If BoolType( ty ) Return "true"
			If NumericType( ty ) Return value
			If StringType( ty ) Return Enquote( value )
		Else
			If BoolType( ty ) Return "false"
			If NumericType( ty ) Return "0"
			If StringType( ty ) Return "~q~q"
			If ArrayType( ty ) Return "[]"
			If ObjectType( ty ) Return "null"
		Endif
		InternalErr
	End

	Method TransValDecl$( decl:ValDecl )
		Return decl.munged+":"+TransType( decl.type )
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
		Return "var "+munged+":"+TransType( init.exprType )+"="+init.Trans()
	End

	Method EmitEnter( func:FuncDecl )
		Emit "pushErr();"
	End
	
	Method EmitSetErr( info$ )
		Emit "_errInfo=~q"+info.Replace( "\","/" )+"~q;"
	End
	
	Method EmitLeave()
		Emit "popErr();"
	End

	'***** Declarations *****

	Method TransStatic$( decl:Decl )
		If decl.IsExtern() And ModuleDecl( decl.scope )
			Return decl.munged
		Else If _env And decl.scope And decl.scope=_env.ClassScope
			Return decl.munged
		Else If ClassDecl( decl.scope )
			Return decl.scope.munged+"."+decl.munged
		Else If ModuleDecl( decl.scope )
			Return decl.munged
		Endif
		InternalErr
	End

	Method TransGlobal$( decl:GlobalDecl )
		Return TransStatic( decl )
	End
	
	Method TransField$( decl:FieldDecl,lhs:Expr )
'		Local t_lhs$="this"
		If lhs
			Local t_lhs$=TransSubExpr( lhs )
			If ENV_CONFIG="debug" t_lhs="dbg_object"+Bra(t_lhs)
			Return t_lhs+"."+decl.munged
		Endif
		Return decl.munged
'		Return t_lhs+"."+decl.munged
	End
		
	Method TransFunc$( decl:FuncDecl,args:Expr[],lhs:Expr )
		If decl.IsMethod()
			Local t_lhs$="this"
			If lhs
				t_lhs=TransSubExpr( lhs )
'				If ENV_CONFIG="debug" t_lhs="dbg_object"+Bra(t_lhs)
			Endif
			Return t_lhs+"."+decl.munged+TransArgs( args )
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
		Local t$="(new "+expr.classDecl.munged+")"
		If expr.ctor t+="."+expr.ctor.munged+TransArgs( expr.args )
		Return t
	End
	
	Method TransNewArrayExpr$( expr:NewArrayExpr )
		Local texpr$=expr.expr.Trans()
		Local ty:=expr.ty
		If BoolType( ty ) Return "new_bool_array("+texpr+")"
		If NumericType( ty ) Return "new_number_array("+texpr+")"
		If StringType( ty ) Return "new_string_array("+texpr+")"
		If ObjectType( ty ) Return "new_object_array("+texpr+")"
		If ArrayType( ty ) Return "new_array_array("+texpr+")"
		InternalErr
	End
	
	Method TransSelfExpr$( expr:SelfExpr )
		Return "this"
	End
	
	Method TransCastExpr$( expr:CastExpr )
		Local dst:=expr.exprType
		Local src:=expr.expr.exprType
		Local texpr$=Bra( expr.expr.Trans() )
		
		If BoolType( dst )
			If BoolType( src ) Return texpr
			If IntType( src ) Return Bra( texpr+"!=0" )
			If FloatType( src ) Return Bra( texpr+"!=0.0" )
			If StringType( src ) Return Bra( texpr+".length!=0" )
			If ArrayType( src ) Return Bra( texpr+".length!=0" )
			If ObjectType( src ) Return Bra( texpr+"!=null" )
		Else If IntType( dst )
			If BoolType( src ) Return Bra( texpr+"?1:0" )
			If IntType( src ) Return texpr
			If FloatType( src ) Return Bra( texpr+"|0" )
			If StringType( src ) Return "parseInt"+Bra( texpr+",10" )
		Else If FloatType( dst )
			If NumericType( src ) Return texpr
			If StringType( src ) 	Return "parseFloat"+texpr
		Else If StringType( dst )
			If NumericType( src ) Return "String"+texpr
			If StringType( src )  Return texpr
		Else If ObjectType( dst ) And ObjectType( src )
			If src.GetClass().ExtendsClass( dst.GetClass() )
				'upcast
				Return texpr
			Else
				'downcast
				Return Bra( texpr + " as "+TransType( dst ) )
			Endif
		Endif
		Err "AS translator can't convert "+src.ToString()+" to "+dst.ToString()
	End
	
	Method TransUnaryExpr$( expr:UnaryExpr )
		Local pri=ExprPri( expr )
		Local t_expr$=TransSubExpr( expr.expr,pri )
		Return TransUnaryOp( expr.op )+t_expr
	End
	
	Method TransBinaryExpr$( expr:BinaryExpr )
		Local pri=ExprPri( expr )
		Local t_lhs$=TransSubExpr( expr.lhs,pri )
		Local t_rhs$=TransSubExpr( expr.rhs,pri-1 )
		Local t_expr$=t_lhs+TransBinaryOp( expr.op,t_rhs )+t_rhs
		If expr.op="/" And IntType( expr.exprType ) t_expr=Bra( Bra(t_expr)+"|0" )
		Return t_expr
	End
	
	Method TransIndexExpr$( expr:IndexExpr )
		Local t_expr:=TransSubExpr( expr.expr )
		If StringType( expr.expr.exprType ) 
			Local t_index:=expr.index.Trans()
			If ENV_CONFIG="debug" Return "dbg_charCodeAt("+t_expr+","+t_index+")"
			Return t_expr+".charCodeAt("+t_index+")"
		Else If ENV_CONFIG="debug"
			Local t_index:=expr.index.Trans()
			Return "dbg_array("+t_expr+","+t_index+")[dbg_index]"
		Else
			Local t_index:=expr.index.Trans()
			Return t_expr+"["+t_index+"]"
		Endif
	End
	
	Method TransSliceExpr$( expr:SliceExpr )
		Local t_expr$=TransSubExpr( expr.expr )
		Local t_args$="0"
		If expr.from t_args=expr.from.Trans()
		If expr.term t_args+=","+expr.term.Trans()
		Return t_expr+".slice("+t_args+")"
	End
	
	Method TransArrayExpr$( expr:ArrayExpr )
		Local t$
		For Local elem:=Eachin expr.exprs
			If t t+=","
			t+=elem.Trans()
		Next
		Return "["+t+"]"
	End
	
	Method TransIntrinsicExpr$( decl:Decl,expr:Expr,args:Expr[] )
	
		Local texpr$,arg0$,arg1$,arg2$
		
		If expr texpr=TransSubExpr( expr )
		
		If args.Length>0 And args[0] arg0=args[0].Trans()
		If args.Length>1 And args[1] arg1=args[1].Trans()
		If args.Length>2 And args[2] arg2=args[2].Trans()
		
		Local id$=decl.munged[1..]
		
		Select id

		'global functions
		Case "print" Return "print"+Bra( arg0 )
		Case "error" Return "error"+Bra( arg0 )
		Case "debuglog" Return "debugLog"+Bra( arg0 )
		Case "debugstop" Return "debugStop()"
		
		'string/array methods
		Case "length" Return texpr+".length"
		
		'array methods
		Case "resize"
			Local ty:=ArrayType( expr.exprType ).elemType
			If BoolType( ty ) Return "resize_bool_array"+Bra( texpr+","+arg0 )
			If NumericType( ty ) Return "resize_number_array"+Bra( texpr+","+arg0 )
			If StringType( ty ) Return "resize_string_array"+Bra( texpr+","+arg0 )
			If ArrayType( ty ) Return "resize_array_array"+Bra( texpr+","+arg0 )
			If ObjectType( ty ) Return "resize_object_array"+Bra( texpr+","+arg0 )
			InternalErr

		'string methods
		Case "compare" Return "string_compare"+Bra( texpr+","+arg0 )
		Case "find" Return texpr+".indexOf"+Bra( arg0+","+arg1 )
		Case "findlast" Return texpr+".lastIndexOf"+Bra( arg0 )
		Case "findlast2" Return texpr+".lastIndexOf"+Bra( arg0+","+arg1 )
		Case "trim" Return "string_trim"+Bra( texpr )
		Case "join" Return arg0+".join"+Bra( texpr )
		Case "split" Return texpr+".split"+Bra( arg0 )
		Case "replace" Return "string_replace"+Bra(texpr+","+arg0+","+arg1)
		Case "tolower" Return texpr+".toLowerCase()"
		Case "toupper" Return texpr+".toUpperCase()"
		Case "contains" Return Bra( texpr+".indexOf"+Bra( arg0 )+"!=-1" )
		Case "startswith" Return "string_startswith"+Bra( texpr+","+arg0 )
		Case "endswith" Return "string_endswith"+Bra( texpr+","+arg0 )
		Case "tochars" Return "string_tochars"+Bra( texpr )

		'string functions
		Case "fromchar" Return "String.fromCharCode"+Bra( arg0 )
		Case "fromchars" Return "string_fromchars"+Bra( arg0 )

		'trig functions - degrees
		Case "sin","cos","tan" Return "Math."+id+Bra( Bra( arg0 )+"*D2R" )
		Case "asin","acos","atan" Return Bra( "Math."+id+Bra( arg0 )+"*R2D" )
		Case "atan2" Return Bra( "Math."+id+Bra( arg0+","+arg1 )+"*R2D" )
		
		'trig functions - radians
		Case "sinr","cosr","tanr" Return "Math."+id[..-1]+Bra( arg0 )
		Case "asinr","acosr","atanr" Return "Math."+id[..-1]+Bra( arg0 )
		Case "atan2r" Return "Math."+id[..-1]+Bra( arg0+","+arg1 )

		'misc math functions
		Case "sqrt","floor","ceil","log","exp" Return "Math."+id+Bra( arg0 )
		Case "pow" Return "Math."+id+Bra( arg0+","+arg1 )

		End Select

		InternalErr
	End
	
	'***** Statements *****
	
	Method TransTryStmt$( stmt:TryStmt )
		Emit "try{"
		Local unr:=EmitBlock( stmt.block )
		For Local c:=Eachin stmt.catches
			MungDecl c.init
			Emit "}catch("+c.init.munged+":"+TransType( c.init.type )+"){"
			Local unr:=EmitBlock( c.block )
		Next
		Emit "}"
	End

	'***** Declarations *****
	
	Method EmitFuncDecl( decl:FuncDecl )
		BeginLocalScope

		Local args$
		For Local arg:=Eachin decl.argDecls
			MungDecl arg
			If args args+=","
			args+=TransValDecl( arg )
		Next

		Local t$="function "+decl.munged+Bra( args )+":"+TransType( decl.retType )
		
		Local cdecl:=decl.ClassScope()
		If cdecl And cdecl.IsInterface()
			Emit t+";"
		Else
			Local q$="internal "
			If cdecl
				q="public "
				If decl.IsStatic() q+="static "
				If decl.overrides q+="override "
			Endif
			
			Emit q+t+"{"
			If decl.IsAbstract()
				If VoidType( decl.retType )
					Emit "return;"
				Else
					Emit "return "+TransValue( decl.retType,"" )+";"
				Endif
			Else
				EmitBlock decl
			Endif
			Emit "}"
		Endif
		
		EndLocalScope
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
		
		Emit "class "+classid+" extends "+superid+bases+"{"
		
		'members...
		For Local decl:=Eachin classDecl.Semanted
			Local tdecl:=FieldDecl( decl )
			If tdecl
				Emit "internal var "+TransValDecl( tdecl )+"="+tdecl.init.Trans()+";"
				Continue
			Endif
			
			Local gdecl:=GlobalDecl( decl )
			If gdecl
				Emit "internal static var "+TransValDecl( gdecl )+";"
				Continue
			Endif
			
			Local fdecl:=FuncDecl( decl )
			If fdecl
				EmitFuncDecl fdecl
				Continue
			Endif
		Next
		
		Emit "}"
	End
	
	Method TransApp$( app:AppDecl )
		
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
	
		'decls
		'
		For Local decl:=Eachin app.Semanted

			Local gdecl:=GlobalDecl( decl )
			If gdecl
				Emit "var "+TransValDecl( gdecl )+";"
				Continue
			Endif
			
			Local fdecl:=FuncDecl( decl )
			If fdecl
				EmitFuncDecl fdecl
				Continue
			Endif
			
			Local cdecl:=ClassDecl( decl )
			If cdecl
				EmitClassDecl cdecl
				Continue
			Endif
		Next
		
		BeginLocalScope
		Emit "function bbInit():void{"
		For Local decl:=Eachin app.semantedGlobals
			Emit TransGlobal( decl )+"="+decl.init.Trans()+";"
		Next
		Emit "}"
		EndLocalScope
		
		Return JoinLines()
	End
	
End

