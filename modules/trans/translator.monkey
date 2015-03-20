
Import trans

Global _trans:Translator

Class Translator

	'***** Expressions *****
	
	Method TransConstExpr$( expr:ConstExpr ) Abstract
	
	Method TransNewObjectExpr$( expr:NewObjectExpr ) Abstract
	
	Method TransNewArrayExpr$( expr:NewArrayExpr ) Abstract
	
	Method TransSelfExpr$( expr:SelfExpr ) Abstract
	
	Method TransCastExpr$( expr:CastExpr ) Abstract
	
	Method TransUnaryExpr$( expr:UnaryExpr ) Abstract
	
	Method TransBinaryExpr$( expr:BinaryExpr ) Abstract
	
	Method TransIndexExpr$( expr:IndexExpr ) Abstract
	
	Method TransSliceExpr$( expr:SliceExpr ) Abstract
	
	Method TransArrayExpr$( expr:ArrayExpr ) Abstract
	
	Method TransStmtExpr$( expr:StmtExpr ) Abstract
	
	Method TransVarExpr$( expr:VarExpr ) Abstract
	
	Method TransMemberVarExpr$( expr:MemberVarExpr ) Abstract
	
	Method TransInvokeExpr$( expr:InvokeExpr ) Abstract
	
	Method TransInvokeMemberExpr$( expr:InvokeMemberExpr ) Abstract
	
	Method TransInvokeSuperExpr$( expr:InvokeSuperExpr ) Abstract
	
	'***** Statements *****
	
	Method TransExprStmt$( stmt:ExprStmt ) Abstract
	
	Method TransAssignStmt$( stmt:AssignStmt ) Abstract
	
	Method TransReturnStmt$( stmt:ReturnStmt ) Abstract
	
	Method TransContinueStmt$( stmt:ContinueStmt ) Abstract
	
	Method TransBreakStmt$( stmt:BreakStmt ) Abstract

	Method TransDeclStmt$( stmt:DeclStmt ) Abstract
	
	Method TransIfStmt$( stmt:IfStmt ) Abstract
	
	Method TransWhileStmt$( stmt:WhileStmt ) Abstract

	Method TransRepeatStmt$( stmt:RepeatStmt ) Abstract

	Method TransForStmt$( stmt:ForStmt ) Abstract
	
	Method TransTryStmt$( stmt:TryStmt ) Abstract

	Method TransThrowStmt$( stmt:ThrowStmt ) Abstract
	
	'***** Decls *****
		
	Method TransBlock$( block:BlockDecl ) Abstract
	
	Method TransApp$( app:AppDecl ) Abstract
	
End

'***** CTranslator for C based languages *****

Class CTranslator Extends Translator

	Field emitDebugInfo?
	Field indent$
	Field lines:=New StringStack
	Field unreachable,broken
	Field mungedScopes:=New StringMap<StringSet>
	Field funcMungs:=New StringMap<FuncDeclList>
	Field mungedFuncs:=New StringMap<FuncDecl>
	
	'***** Utility *****
	
	Method TransValue$( ty:Type,value$ ) Abstract
	
	Method TransCatchVar$( init:LocalDecl )
	End

	Method TransLocalDecl$( munged$,init:Expr ) Abstract
	
	'***** Declarations *****
	
	Method TransGlobal$( decl:GlobalDecl ) Abstract
	
	Method TransField$( decl:FieldDecl,lhs:Expr ) Abstract
	
	Method TransFunc$( decl:FuncDecl,args:Expr[],lhs:Expr ) Abstract
	
	Method TransSuperFunc$( decl:FuncDecl,args:Expr[] ) Abstract
	
	'***** Expressions *****
	
	Method TransIntrinsicExpr$( decl:Decl,expr:Expr,args:Expr[] ) Abstract

	Method BeginLocalScope()
		mungedScopes.Set "$",New StringSet
	End
	
	Method EndLocalScope()
		mungedScopes.Set "$",Null
	End
	
	Method MungMethodDecl( fdecl:FuncDecl )

		If fdecl.munged Return
		
		If fdecl.overrides
			MungMethodDecl fdecl.overrides
			fdecl.munged=fdecl.overrides.munged
			Return
		Endif
		
		Local funcs:=funcMungs.Get( fdecl.ident )
		If funcs
			For Local tdecl:=Eachin funcs
				If fdecl.EqualsArgs( tdecl )
					fdecl.munged=tdecl.munged
					Return
				Endif
			Next
		Else
			funcs=New FuncDeclList
			funcMungs.Set fdecl.ident,funcs
		Endif
		
		Local id:=fdecl.ident
		If mungedFuncs.Contains( id )
			Local n:=1
			Repeat
				n+=1
				id=fdecl.ident+String(n)
			Until Not mungedFuncs.Contains( id )
		Endif
		
		mungedFuncs.Set id,fdecl
		fdecl.munged="p_"+id
		funcs.AddLast fdecl
	End
	
	Method MungDecl( decl:Decl )

		If decl.munged Return

		Local fdecl:FuncDecl=FuncDecl( decl )
		If fdecl And fdecl.IsMethod() Return MungMethodDecl( fdecl )
		
		Local id:=decl.ident,munged$,scope$
		
		If LocalDecl( decl )
			scope="$"
			munged="t_"+id
		Else If ClassDecl( decl )
			scope=""
			munged="c_"+id
		Else If ModuleDecl( decl )
			scope=""
			munged="bb_"+id
		Else If ClassDecl( decl.scope )
			scope=decl.scope.munged
			munged="m_"+id
		Else If ModuleDecl( decl.scope )
			If ENV_LANG="cs" Or ENV_LANG="java"
				scope=decl.scope.munged
				munged="g_"+id
			Else
				scope=""
				munged=decl.scope.munged+"_"+id
			Endif
		Else
			InternalErr
		Endif
		
		Local set:=mungedScopes.Get( scope )
		If set
			If set.Contains( munged.ToLower() )
				Local id=1
				Repeat
					id+=1
					Local t$=munged+String(id)
					If set.Contains( t.ToLower() ) Continue
					munged=t
					Exit
				Forever
			Endif
		Else
			If scope="$"
				Print "OOPS2"
				InternalErr
			Endif
			set=New StringSet
			mungedScopes.Set scope,set
		Endif
		set.Insert munged.ToLower()
		decl.munged=munged
	End
	
	Method Bra$( str$ )
		If str.StartsWith( "(" ) And str.EndsWith( ")" )
			Local n=1
			For Local i=1 Until str.Length-1
				Select str[i..i+1]
				Case "("
					n+=1
				Case ")"
					n-=1
					If Not n Return "("+str+")"
				End
			Next
			If n=1 Return str
		Endif
		Return "("+str+")"
	End
	
	'Utility C/C++ style...
	Method Enquote$( str$ )
		Return .Enquote( str,ENV_LANG )
	End

	Method TransUnaryOp$( op$ )
		Select op
		Case "+" Return "+"
		Case "-" Return "-"
		Case "~~" Return op
		Case "not" Return "!"
		End Select
		InternalErr
	End
	
	Method TransBinaryOp$( op$,rhs$ )
		Select op
		Case "+","-"
			If rhs.StartsWith( op ) Return op+" "
			Return op
		Case "*","/" Return op
		Case "shl" Return "<<"
		Case "shr" Return ">>"
		Case "mod" Return " % "
		Case "and" Return " && "
		Case "or" Return " || "
		Case "=" Return "=="
		Case "<>" Return "!="
		Case "<","<=",">",">=" Return op
		Case "&","|" Return op
		Case "~~" Return "^"
		End Select
		InternalErr
	End
	
	Method TransAssignOp$( op$ )
		Select op
		Case "~~=" Return "^="
		Case "mod=" Return "%="
		Case "shl=" Return "<<="
		Case "shr=" Return ">>="
		End
		Return op
	End
	
	Method ExprPri( expr:Expr )
		'
		'1=primary,
		'2=postfix
		'3=prefix
		'
		If NewObjectExpr( expr )
			Return 3
		Else If UnaryExpr( expr )
			Select UnaryExpr( expr ).op
			Case "+","-","~~","not" Return 3
			End Select
			InternalErr
		Else If BinaryExpr( expr )
			Select BinaryExpr( expr ).op
			Case "*","/","mod" Return 4
			Case "+","-" Return 5
			Case "shl","shr" Return 6
			Case "<","<=",">",">=" Return 7
			Case "=","<>" Return 8
			Case "&" Return 9
			Case "~~" Return 10
			Case "|" Return 11
			Case "and" Return 12
			Case "or" Return 13
			End
			InternalErr
		Endif
		Return 2
	End
	
	Method TransSubExpr$( expr:Expr,pri=2 )
		Local t_expr$=expr.Trans()
		If ExprPri( expr )>pri t_expr=Bra( t_expr )
		Return t_expr
	End
	
	Method TransExprNS$( expr:Expr )
		If Not expr.SideEffects() Return expr.Trans()
		Return CreateLocal( expr )
	End

	Method CreateLocal$( expr:Expr )
		Local tmp:=New LocalDecl( "",0,expr.exprType,expr )
		MungDecl tmp
		Emit TransLocalDecl( tmp.munged,expr )+";"
		Return tmp.munged
	End
	
	Method EmitEnter( func:FuncDecl )
	End
	
	Method EmitEnterBlock()
	End
	
	Method EmitSetErr( errInfo$ )
	End
	
	Method EmitLeaveBlock()
	End
	
	Method EmitLeave()
	End
	
	'***** Simple statements *****
	
	'Expressions
	Method TransStmtExpr$( expr:StmtExpr )
		Local t$=expr.stmt.Trans()
		If t Emit t+";"
		Return expr.expr.Trans()
	End
	
	Method TransVarExpr$( expr:VarExpr )
		Local decl:=VarDecl( expr.decl )
		
		If decl.munged.StartsWith( "$" ) Return TransIntrinsicExpr( decl,Null,[] )
		
		If LocalDecl( decl ) Return decl.munged
		
		If FieldDecl( decl ) Return TransField( FieldDecl( decl ),Null )
		
		If GlobalDecl( decl ) Return TransGlobal( GlobalDecl( decl ) )
		
		InternalErr
	End
	
	Method TransMemberVarExpr$( expr:MemberVarExpr )
		Local decl:=VarDecl( expr.decl )
		
		If decl.munged.StartsWith( "$" ) Return TransIntrinsicExpr( decl,expr.expr,[] )
		
		If FieldDecl( decl ) Return TransField( FieldDecl( decl ),expr.expr )

		InternalErr
	End
	
	Method TransInvokeExpr$( expr:InvokeExpr )
		Local decl:=FuncDecl( expr.decl ),t$
		
		If decl.munged.StartsWith( "$" ) Return TransIntrinsicExpr( decl,Null,expr.args )
		
		If decl Return TransFunc( FuncDecl(decl),expr.args,Null )
		
		InternalErr
	End
	
	Method TransInvokeMemberExpr$( expr:InvokeMemberExpr )
		Local decl:=FuncDecl( expr.decl ),t$

		If decl.munged.StartsWith( "$" ) Return TransIntrinsicExpr( decl,expr.expr,expr.args )
		
		If decl Return TransFunc( FuncDecl(decl),expr.args,expr.expr )	
		
		InternalErr
	End
	
	Method TransInvokeSuperExpr$( expr:InvokeSuperExpr )
		Local decl:=FuncDecl( expr.funcDecl ),t$

		If decl.munged.StartsWith( "$" ) Return TransIntrinsicExpr( decl,expr,[] )
		
		If decl Return TransSuperFunc( FuncDecl( decl ),expr.args )
		
		InternalErr
	End
	
	Method TransExprStmt$( stmt:ExprStmt )
		Return stmt.expr.TransStmt()
	End
	
	Method TransAssignStmt$( stmt:AssignStmt )
		If Not stmt.rhs
			Return stmt.lhs.Trans()
		Endif
		
		If stmt.tmp1
			MungDecl stmt.tmp1
			Emit TransLocalDecl( stmt.tmp1.munged,stmt.tmp1.init )+";"
		Endif
		If stmt.tmp2
			MungDecl stmt.tmp2
			Emit TransLocalDecl( stmt.tmp2.munged,stmt.tmp2.init )+";"
		Endif
		
		Return TransAssignStmt2( stmt )
	End
	
	Method TransAssignStmt2$( stmt:AssignStmt )
		Return stmt.lhs.TransVar()+TransAssignOp( stmt.op )+stmt.rhs.Trans()
	End
	
	Method TransReturnStmt$( stmt:ReturnStmt )
		Local t$="return"
		If stmt.expr t+=" "+stmt.expr.Trans()
		unreachable=True
		Return t
	End
	
	Method TransContinueStmt$( stmt:ContinueStmt )
		unreachable=True
		Return "continue"
	End
	
	Method TransBreakStmt$( stmt:BreakStmt )
		unreachable=True
		broken+=1
		Return "break"
	End
	
	'***** Block statements - all very C like! *****
	
	Method BeginLoop()
	End
	
	Method EndLoop()
	End
	
	Method Emit( t$ )
		If Not t Return
		If t.StartsWith( "}" )
			indent=indent[..indent.Length-1]
		Endif
		lines.Push indent+t
		If t.EndsWith( "{" )
			indent+="~t"
		Endif
	End
	
	Method JoinLines$()
		Local code$=lines.Join( "~n" )
		lines.Clear
		Return code
	End
	
	Method TransBlock$( block:BlockDecl )
		EmitBlock block,False
	End
	
	'returns unreachable status!
	'
	Method EmitBlock( block:BlockDecl,realBlock?=True )
	
		PushEnv block
		
		Local func:=FuncDecl( block )
		
		If func
			emitDebugInfo=ENV_CONFIG<>"release"
			If func.attrs & DECL_NODEBUG emitDebugInfo=False
			If emitDebugInfo EmitEnter func
		Else
			If emitDebugInfo And realBlock EmitEnterBlock
		Endif
		
		Local lastStmt:Stmt=Null
		For Local stmt:Stmt=Eachin block.stmts
		
			_errInfo=stmt.errInfo
			
			If unreachable Exit
			
			lastStmt=stmt
			
			If emitDebugInfo
				Local rs:=ReturnStmt( stmt )
				If rs
					If rs.expr
						'
						If stmt.errInfo EmitSetErr stmt.errInfo
						'
						Local t_expr:=TransExprNS( rs.expr )
						EmitLeave
						Emit "return "+t_expr+";"
					Else
						EmitLeave
						Emit "return;"
					Endif
					unreachable=True
					Continue
				Endif
				If stmt.errInfo EmitSetErr stmt.errInfo
			Endif
			
			Local t$=stmt.Trans()
			If t Emit t+";"
			
		Next
		
		_errInfo=""
		
		Local unr=unreachable
		unreachable=False
		
		If unr

			'Actionscript's reachability analysis is...weird.
			If func And ENV_LANG="as" And Not VoidType( func.retType )
				If Not ReturnStmt( lastStmt ) Emit "return "+TransValue( func.retType,"" )+";"
			Endif
		
		Else If func
		
			If emitDebugInfo EmitLeave
			
			If Not VoidType( func.retType )
				If func.IsCtor()
					Emit "return this;"
				Else
					If func.ModuleScope().IsStrict()
						_errInfo=func.errInfo
						Err "Missing return statement."
					Endif
					Emit "return "+TransValue( func.retType,"" )+";"
				Endif
			Endif
		Else

			If emitDebugInfo And realBlock EmitLeaveBlock

		Endif

		PopEnv
		
		Return unr
	End
	
	Method TransDeclStmt$( stmt:DeclStmt )
		Local decl:=LocalDecl( stmt.decl )
		If decl
			MungDecl decl
			Return TransLocalDecl( decl.munged,decl.init )
		Endif
		Local cdecl:=ConstDecl( stmt.decl )
		If cdecl
			Return
		Endif
		InternalErr
	End
	
	Method TransIfStmt$( stmt:IfStmt )
		If ConstExpr( stmt.expr ) And ENV_LANG<>"java"	'ignore If Const in java...
			If ConstExpr( stmt.expr ).value
				If Not stmt.thenBlock.stmts.IsEmpty()
					Emit "if(true){"
					If EmitBlock( stmt.thenBlock ) unreachable=True
					Emit "}"
				Endif
			Else
				If Not stmt.elseBlock.stmts.IsEmpty()
					Emit "if(true){"
					If EmitBlock( stmt.elseBlock ) unreachable=True
					Emit "}"
				Endif
			Endif
		Else If Not stmt.elseBlock.stmts.IsEmpty()
			Emit "if"+Bra( stmt.expr.Trans() )+"{"
			Local unr=EmitBlock( stmt.thenBlock )
			Emit "}else{"
			Local unr2=EmitBlock( stmt.elseBlock )
			Emit "}"
			If unr And unr2 unreachable=True
		Else
			Emit "if"+Bra( stmt.expr.Trans() )+"{"
			Local unr=EmitBlock( stmt.thenBlock )
			Emit "}"
		Endif
	End
	
	Method TransWhileStmt$( stmt:WhileStmt )
		Local nbroken=broken
		
		Emit "while"+Bra( stmt.expr.Trans() )+"{"
		BeginLoop
		Local unr=EmitBlock( stmt.block )
		EndLoop
		Emit "}"
		
		If broken=nbroken And ConstExpr( stmt.expr ) And ConstExpr( stmt.expr ).value unreachable=True
		broken=nbroken
	End

	Method TransRepeatStmt$( stmt:RepeatStmt )
		Local nbroken=broken

		Emit "do{"
		BeginLoop
		Local unr=EmitBlock( stmt.block )
		EndLoop
		Emit "}while(!"+Bra( stmt.expr.Trans() )+");"

		If broken=nbroken And ConstExpr( stmt.expr ) And Not ConstExpr( stmt.expr ).value unreachable=True
		broken=nbroken
	End

	Method TransForStmt$( stmt:ForStmt )
		Local nbroken=broken

		Local init$=stmt.init.Trans()
		Local expr$=stmt.expr.Trans()
		Local incr$=stmt.incr.Trans()

		Emit "for("+init+";"+expr+";"+incr+"){"
		BeginLoop
		Local unr=EmitBlock( stmt.block )
		EndLoop
		Emit "}"
		
		If broken=nbroken And ConstExpr( stmt.expr ) And ConstExpr( stmt.expr ).value unreachable=True
		broken=nbroken
	End

	Method TransTryStmt$( stmt:TryStmt )
		Err "TODO!"
	End
	
	Method TransThrowStmt$( stmt:ThrowStmt )
		unreachable=True
		Return "throw "+stmt.expr.Trans()
	End
	
	Method PostProcess$( source$ ) 
		Return source
	End
	
End
