
' Module trans.cstranslator
'
' Placed into the public domain 24/02/2011.
' No warranty implied; use at your own risk.

Import trans

Class CsTranslator Extends CTranslator

	Method TransType$( ty:Type )
		If VoidType( ty ) Return "void"
		If BoolType( ty ) Return "bool"
		If IntType( ty ) Return "int"
		If FloatType( ty ) Return "float"
		If StringType( ty ) Return "String"
		If ArrayType( ty ) Return TransType( ArrayType(ty).elemType )+"[]"
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
		Local vdecl:=ValDecl( decl )
		If vdecl Return TransType( vdecl.type )+" "+decl.munged
		InternalErr
	End
	
	Method TransArgs$( args:Expr[] )
		Local t$
		For Local arg:=Eachin args
			If t t+=","
			t+=arg.Trans()
		Next
		Return Bra( t )
	End
	
	'***** Utility *****
	
	Method TransLocalDecl$( munged$,init:Expr )
		Return TransType( init.exprType )+" "+munged+"="+init.Trans()
	End
	
	Method EmitEnter( func:FuncDecl )
		Emit "bb_std_lang.pushErr();"
	End
	
	Method EmitSetErr( info$ )
		Emit "bb_std_lang.errInfo=~q"+info.Replace( "\","/" )+"~q;"
	End
	
	Method EmitLeave()
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
		Return "base."+decl.munged+TransArgs( args )
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
	
		Local dst:=expr.exprType
		Local src:=expr.expr.exprType
		Local uexpr$=expr.expr.Trans()
		Local texpr$=Bra( uexpr )
		
		If BoolType( dst )
			If BoolType( src ) Return texpr
			If IntType( src ) Return Bra( texpr+"!=0" )
			If FloatType( src ) Return Bra( texpr+"!=0.0f" )
			If StringType( src ) Return Bra( texpr+".Length!=0" )
			If ArrayType( src ) Return Bra( texpr+".Length!=0" )
			If ObjectType( src ) Return Bra( texpr+"!=null" )
		Else If IntType( dst )
			If BoolType( src ) Return Bra( texpr+"?1:0" )
			If IntType( src ) Return texpr
			If FloatType( src ) Return "(int)"+texpr
			If StringType( src ) Return "int.Parse"+texpr
		Else If FloatType( dst )
			If IntType( src ) Return "(float)"+texpr
			If FloatType( src ) Return texpr
			If StringType( src ) 
				If ENV_TARGET="xna" Return "float.Parse"+Bra(uexpr+",CultureInfo.InvariantCulture")
				Return "float.Parse"+Bra(uexpr)
			Endif
		Else If StringType( dst )
			If IntType( src ) Return texpr+".ToString()"
			If FloatType( src ) 
				If ENV_TARGET="xna" Return texpr+".ToString(CultureInfo.InvariantCulture)"
				Return texpr+".ToString()"
			Endif
			If StringType( src ) Return texpr
		Else If ObjectType( dst ) And ObjectType( src )
			If src.GetClass().ExtendsClass( dst.GetClass() )
				'upcast
				Return texpr
			Else 
				'downcast
				Return "("+texpr+" as "+TransType(dst)+")"
				'Local tmp:=New LocalDecl( "",0,src,Null )
				'MungDecl tmp
				'Emit TransType( src )+" "+tmp.munged+"="+uexpr+";"
				'Return "($t is $c ? ($c)$t : null)".Replace( "$t",tmp.munged ).Replace( "$c",TransType(dst) )
			Endif
		Endif
		Err "CS translator can't convert "+src.ToString()+" to "+dst.ToString()
	End
	
	Method TransUnaryExpr$( expr:UnaryExpr )
		Local pri=ExprPri( expr )
		Local t_expr$=TransSubExpr( expr.expr,pri )
		Return TransUnaryOp( expr.op )+t_expr
	End
	
	Method TransBinaryExpr$( expr:BinaryExpr )
		If BinaryCompareExpr( expr ) And StringType( expr.lhs.exprType ) And StringType( expr.rhs.exprType )
			Return Bra( TransSubExpr( expr.lhs )+".CompareTo("+expr.rhs.Trans()+")"+TransBinaryOp( expr.op,"" )+"0" )
		Endif
		Local pri=ExprPri( expr )
		Local t_lhs$=TransSubExpr( expr.lhs,pri )
		Local t_rhs$=TransSubExpr( expr.rhs,pri-1 )
		Return t_lhs+TransBinaryOp( expr.op,t_rhs )+t_rhs

	End
	
	Method TransIndexExpr$( expr:IndexExpr )
		Local t_expr:=TransSubExpr( expr.expr )
		Local t_index:=expr.index.Trans()
		If StringType( expr.expr.exprType ) Return "(int)"+t_expr+"["+t_index+"]"
		Return t_expr+"["+t_index+"]"
	End
	
	Method TransSliceExpr$( expr:SliceExpr )
		Local t_expr:=expr.expr.Trans()
		Local t_args:="0"
		If expr.from t_args=expr.from.Trans()
		If expr.term t_args+=","+expr.term.Trans()
		Return "(("+TransType( expr.exprType )+")bb_std_lang.slice("+t_expr+","+t_args+"))"
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
		Local id2$=id[..1].ToUpper()+id[1..]

		Select id

		'global functions
		Case "print" Return "bb_std_lang.Print"+Bra( arg0 )
		Case "error" Return "bb_std_lang.Error"+Bra( arg0 )
		Case "debuglog" Return "bb_std_lang.DebugLog"+Bra( arg0 )
		Case "debugstop" Return "bb_std_lang.DebugStop()"

		'string/array methods
		Case "length"
			If StringType( expr.exprType ) Return texpr+".Length"
			Return "bb_std_lang.length"+Bra( texpr )
		
		'array methods
		Case "resize" 
			Local ty:=ArrayType( expr.exprType ).elemType
			If StringType( ty ) Return "bb_std_lang.resize("+texpr+","+arg0+")"
			Local ety:=TransType( ty )
			Return "("+ety+"[])bb_std_lang.resize("+texpr+","+arg0+",typeof("+ety+"))"
#rem			
			Local fn$="resizeArray"
			Local ty:=ArrayType( expr.exprType ).elemType
			If StringType( ty )
				fn="resizeStringArray"
			Else If ArrayType( ty )
				fn="resizeArrayArray"
			Endif
			Return "("+TransType( expr.exprType )+")bb_std_lang."+fn+Bra( texpr+","+arg0 )
#end
		'string methods
		Case "compare" Return texpr+".CompareTo"+Bra( arg0 )
		Case "find" Return texpr+".IndexOf"+Bra( arg0+","+arg1 )
		Case "findlast" Return texpr+".LastIndexOf"+Bra( arg0 )
		Case "findlast2" Return texpr+".LastIndexOf"+Bra( arg0+","+arg1 )
		Case "trim" Return texpr+".Trim()"
		Case "join" Return "String.Join"+Bra( texpr+","+arg0 )
		Case "split" Return "bb_std_lang.split"+Bra( texpr+","+arg0 )
		Case "replace" Return texpr+".Replace"+Bra( arg0+","+arg1 )
		Case "tolower" Return texpr+".ToLower()"
		Case "toupper" Return texpr+".ToUpper()"
		Case "contains" Return Bra( texpr+".IndexOf"+Bra( arg0 )+"!=-1" )
		Case "startswith" Return texpr+".StartsWith"+Bra( arg0 )
		Case "endswith" Return texpr+".EndsWith"+Bra( arg0 )
		Case "tochars" Return "bb_std_lang.toChars"+Bra( texpr );

		'string functions
		Case "fromchar" Return "new String"+Bra("(char)"+Bra( arg0 )+",1")
		Case "fromchars" Return "bb_std_lang.fromChars"+Bra( arg0 )

		'trig functions - degrees
		Case "sin","cos","tan" Return "(float)Math."+id2+Bra( Bra(arg0)+"*bb_std_lang.D2R" )
		Case "asin","acos","atan" Return "(float)"+Bra( "Math."+id2+Bra(arg0)+"*bb_std_lang.R2D" )
		Case "atan2" Return "(float)"+Bra( "Math."+id2+Bra(arg0+","+arg1)+"*bb_std_lang.R2D" )

		'trig functions - radians
		Case "sinr","cosr","tanr" Return "(float)Math."+id2[..-1]+Bra( arg0 )
		Case "asinr","acosr","atanr" Return "(float)Math."+id2[..-1]+Bra( arg0 )
		Case "atan2r" Return "(float)Math."+id2[..-1]+Bra( arg0+","+arg1 )

		'misc math functions
		Case "sqrt","floor","log","exp" Return "(float)Math."+id2+Bra(arg0)
		Case "ceil" Return "(float)Math.Ceiling"+Bra(arg0)
		Case "pow" Return "(float)Math."+id2+Bra( arg0+","+arg1 )

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
		BeginLocalScope
		
		Local args$
		For Local arg:=Eachin decl.argDecls
			MungDecl arg
			If args args+=","
			args+=TransType( arg.type )+" "+arg.munged
		Next
		
		Local t$=TransType( decl.retType )+" "+decl.munged+Bra( args )

		If decl.ClassScope() And decl.ClassScope().IsInterface()
			Emit t+";"
		Else If decl.IsAbstract()
			If decl.overrides
				Emit "public abstract override "+t+";"
			Else
				Emit "public abstract "+t+";"
			Endif
		Else
			Local q$="public "
			If decl.IsStatic()
				q+="static "
			Else If decl.overrides
				q+="override "
				If Not decl.IsVirtual() q+="sealed "
			Else If decl.IsVirtual()
				q+="virtual "
			Endif
			
			Emit q+t+"{"
			EmitBlock decl
			Emit "}"
		Endif
		
		EndLocalScope
	End
	
	Method EmitClassDecl( classDecl:ClassDecl )
	
		Local classid:=classDecl.munged
		
		If classDecl.IsInterface() 
		
			Local bases$
			For Local iface:=Eachin classDecl.implments
				If bases bases+="," Else bases=" : "
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
	
		Local superid:=classDecl.superClass.munged

		Local bases$=" : "+superid
		
		For Local iface:=Eachin classDecl.implments
			bases+=","+iface.munged
		Next
		
		Local q$
		If classDecl.IsAbstract() q+="abstract " Else If classDecl.IsFinal() q+="sealed "
		
		Emit q+"class "+classid+bases+"{"
		
		For Local decl:=Eachin classDecl.Semanted
			Local tdecl:=FieldDecl( decl )
			If tdecl
				Emit "public "+TransDecl( tdecl )+"="+tdecl.init.Trans()+";"
				Continue
			Endif
			
			Local fdecl:=FuncDecl( decl )
			If fdecl
				EmitFuncDecl fdecl
				Continue
			Endif
			
			Local gdecl:=GlobalDecl( decl )
			If gdecl
				Emit "public static "+TransDecl( gdecl )+";"
				Continue
			Endif
		Next
		
		Emit "}"
	End
	
	Method TransApp$( app:AppDecl )
	
		app.mainModule.munged="bb_"
		app.mainFunc.munged="bbMain"
		
		For Local decl:=Eachin app.imported.Values
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
		
		'classes		
		For Local decl:=Eachin app.Semanted
			
			Local cdecl:=ClassDecl( decl )
			If cdecl
				EmitClassDecl cdecl
				Continue
			Endif
		Next
		
		'Translate globals
		For Local mdecl:=Eachin app.imported.Values()

			Emit "class "+mdecl.munged+"{"

			For Local decl:=Eachin mdecl.Semanted
				If decl.IsExtern() Or decl.scope.ClassScope() Continue
			
				Local gdecl:=GlobalDecl( decl )
				If gdecl
					Emit "public static "+TransDecl( gdecl )+";"
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
	
End
