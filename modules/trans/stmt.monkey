
' Module trans.stmt
'
' Placed into the public domain 24/02/2011.
' No warranty implied; use at your own risk.

Import trans

Class Stmt
	Field errInfo$
	
	Method New()
		errInfo=_errInfo
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl ) Abstract
	
	Method OnSemant() Abstract

	Method Copy:Stmt( scope:ScopeDecl )
		Local t:=OnCopy( scope )
		t.errInfo=errInfo
		Return t
	End
	
	Method Semant()
		PushErr errInfo
		OnSemant
		PopErr
	End
	
	Method Trans$() Abstract

End

Class DeclStmt Extends Stmt
	Field decl:Decl
	
	Method New( decl:Decl )
		Self.decl=decl
	End
	
	Method New( id$,ty:Type,init:Expr )
		Self.decl=New LocalDecl( id,0,ty,init )	
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New DeclStmt( decl.Copy() )
	End
	
	Method OnSemant()
		decl.Semant
		_env.InsertDecl decl
	End
	
	Method Trans$()
		Return _trans.TransDeclStmt( Self )
	End
End

Class AssignStmt Extends Stmt
	Field op$
	Field lhs:Expr
	Field rhs:Expr
	Field tmp1:LocalDecl
	Field tmp2:LocalDecl
	
	Method New( op$,lhs:Expr,rhs:Expr )
		Self.op=op
		Self.lhs=lhs
		Self.rhs=rhs
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New AssignStmt( op,lhs.Copy(),rhs.Copy() )
	End
	
	Method FixSideEffects()
		'
		'Ok, this is ugly stuff...but we need to be able to expand
		'x op= y to x=x op y without evaluating any bits of x that have side effects
		'twice.
		'
		Local e1:=MemberVarExpr( lhs )
		If e1
			If e1.expr.SideEffects()
				tmp1=New LocalDecl( "",0,e1.expr.exprType,e1.expr )
				tmp1.Semant()
				lhs=New MemberVarExpr( New VarExpr(tmp1),e1.decl )
			Endif
		Endif

		Local e2:=IndexExpr( lhs )
		If e2
			Local expr:=e2.expr
			Local index:=e2.index
			If expr.SideEffects() Or index.SideEffects()
				If expr.SideEffects()
					tmp1=New LocalDecl( "",0,expr.exprType,expr )
					tmp1.Semant()
					expr=New VarExpr( tmp1 )
				Endif
				If index.SideEffects()
					tmp2=New LocalDecl( "",0,index.exprType,index )
					tmp2.Semant()
					index=New VarExpr( tmp2 )
				Endif
				lhs=New IndexExpr( expr,index ).Semant()
			Endif
		Endif

	End
	
	Method OnSemant()

		rhs=rhs.Semant()
		lhs=lhs.SemantSet( op,rhs )
		
		If InvokeExpr( lhs ) Or InvokeMemberExpr( lhs )
			rhs=Null
			Return
		Endif
		
		Local kludge:=True	'use x=x op y
		
		Select op
		Case "="
			rhs=rhs.Cast( lhs.exprType )
			kludge=False
		Case "*=","/=","+=","-="
			If NumericType( lhs.exprType ) And lhs.exprType.EqualsType( rhs.exprType )
				'OK to use x op= y form
				kludge=False
				'
				'hack for /= with ints in JS!
				'
				If ENV_LANG="js"
					If op="/=" And IntType( lhs.exprType )
						kludge=True
					Endif
				Endif
				'
			Endif
		Case "&=","|=","~~=","shl=","shr=","mod="
			If IntType( lhs.exprType ) And lhs.exprType.EqualsType( rhs.exprType )
				'Ok to use x op= y form
				kludge=False
				'
			Endif
		Default
			InternalErr
		End
		
		'simple kludge for 'no lang!'
		If ENV_LANG="" kludge=True
		
		If kludge
			FixSideEffects
			rhs=New BinaryMathExpr( op[..-1],lhs,rhs ).Semant().Cast( lhs.exprType )
			op="="
		Endif

	End
	
	Method Trans$()
		_errInfo=errInfo
		Return _trans.TransAssignStmt( Self )
	End
		
End

Class ExprStmt Extends Stmt
	Field expr:Expr
	
	Method New( expr:Expr )
		Self.expr=expr
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New ExprStmt( expr.Copy() )
	End
	
	Method OnSemant()
		expr=expr.Semant()
		If Not expr InternalErr
	End

	Method Trans$()
		Return _trans.TransExprStmt( Self )
	End
End

Class ReturnStmt Extends Stmt
	Field expr:Expr

	Method New( expr:Expr )
		Self.expr=expr
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl )
		If expr Return New ReturnStmt( expr.Copy() )
		Return New ReturnStmt( Null )
	End
	
	Method OnSemant()
		Local fdecl:FuncDecl=_env.FuncScope()
		If expr
			If fdecl.IsCtor() Err "Constructors may not return a value."
			If VoidType( fdecl.retType ) Err "Void functions may not return a value."
			expr=expr.Semant( fdecl.retType )
		Else If fdecl.IsCtor()
			expr=New SelfExpr().Semant()
		Else If Not VoidType( fdecl.retType )
			If _env.ModuleScope().IsStrict() Err "Missing return expression."
			expr=New ConstExpr( fdecl.retType,"" ).Semant()
		Endif
	End
	
	Method Trans$()
		Return _trans.TransReturnStmt( Self )
	End
End

Class BreakStmt Extends Stmt

	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New BreakStmt
	End
	
	Method OnSemant()
		If Not _loopnest Err "Exit statement must appear inside a loop."
	End
	
	Method Trans$()
		Return _trans.TransBreakStmt( Self )
	End
	
End

Class ContinueStmt Extends Stmt

	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New ContinueStmt
	End
	
	Method OnSemant()
		If Not _loopnest Err "Continue statement must appear inside a loop."
	End
	
	Method Trans$()
		Return _trans.TransContinueStmt( Self )
	End
	
End

Class IfStmt Extends Stmt
	Field expr:Expr
	Field thenBlock:BlockDecl
	Field elseBlock:BlockDecl
	
	Method New( expr:Expr,thenBlock:BlockDecl,elseBlock:BlockDecl )
		Self.expr=expr
		Self.thenBlock=thenBlock
		Self.elseBlock=elseBlock
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New IfStmt( expr.Copy(),thenBlock.CopyBlock( scope ),elseBlock.CopyBlock( scope ) )
	End
	
	Method OnSemant()
		expr=expr.Semant( Type.boolType,CAST_EXPLICIT )
		thenBlock.Semant
		elseBlock.Semant
	End
	
	Method Trans$()
		Return _trans.TransIfStmt( Self )
	End
End

Class WhileStmt Extends Stmt
	Field expr:Expr
	Field block:BlockDecl
	
	Method New( expr:Expr,block:BlockDecl )
		Self.expr=expr
		Self.block=block
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New WhileStmt( expr.Copy(),block.CopyBlock( scope ) )
	End
	
	Method OnSemant()
		expr=expr.Semant( Type.boolType,CAST_EXPLICIT )
		_loopnest+=1
		block.Semant
		_loopnest-=1
	End
	
	Method Trans$()
		Return _trans.TransWhileStmt( Self )
	End
End

Class RepeatStmt Extends Stmt
	Field block:BlockDecl
	Field expr:Expr
	
	Method New( block:BlockDecl,expr:Expr )
		Self.block=block
		Self.expr=expr
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New RepeatStmt( block.CopyBlock( scope ),expr.Copy() )
	End
	
	Method OnSemant()
		_loopnest+=1
		block.Semant
		_loopnest-=1
		expr=expr.Semant( Type.boolType,CAST_EXPLICIT )
	End
	
	Method Trans$()
		Return _trans.TransRepeatStmt( Self )
	End
End

Class ForStmt Extends Stmt
	Field init:Stmt	'assignment or local decl...
	Field expr:Expr
	Field incr:Stmt	'assignment...
	Field block:BlockDecl
	
	Method New( init:Stmt,expr:Expr,incr:Stmt,block:BlockDecl )
		Self.init=init
		Self.expr=expr
		Self.incr=incr
		Self.block=block
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New ForStmt( init.Copy( scope ),expr.Copy(),incr.Copy( scope ),block.CopyBlock( scope ) )
	End
	
	Method OnSemant()

		PushEnv block

		init.Semant

		expr=expr.Semant()
		
		_loopnest+=1
		
		block.Semant
		
		_loopnest-=1

		incr.Semant
		
		PopEnv
		
		'dodgy as hell! Reverse comparison for backward loops!
		Local assop:AssignStmt=AssignStmt( incr )
		Local addop:BinaryExpr=BinaryExpr( assop.rhs )
		If Not addop Err "Invalid step expression"
		Local stpval$=addop.rhs.Eval()
		If stpval.StartsWith( "-" )
			Local bexpr:BinaryExpr=BinaryExpr( expr )
			Select bexpr.op
			Case "<" bexpr.op=">"
			Case "<=" bexpr.op=">="
			End Select
		Endif
		
	End
	
	Method Trans$()
		Return _trans.TransForStmt( Self )
	End
End

Class TryStmt Extends Stmt

	Field block:BlockDecl
	Field catches:CatchStmt[]
	
	Method New( block:BlockDecl,catches:CatchStmt[] )
		Self.block=block
		Self.catches=catches
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl )
		Local tcatches:=Self.catches[..]
		For Local i=0 Until tcatches.Length
			tcatches[i]=CatchStmt( tcatches[i].Copy( scope ) )
		Next
		Return New TryStmt( block.CopyBlock( scope ),tcatches )
	End
	
	Method OnSemant()
		block.Semant
		For Local i=0 Until catches.Length
			catches[i].Semant
			For Local j=0 Until i
				If catches[i].init.type.ExtendsType( catches[j].init.type )
					PushErr catches[i].errInfo
					Err "Catch variable class extends earlier catch variable class"
				Endif
			Next
		Next
	End
	
	Method Trans$()
		Return _trans.TransTryStmt( Self )
	End
	
End

Class CatchStmt Extends Stmt

	Field init:LocalDecl
	Field block:BlockDecl
	
	Method New( init:LocalDecl,block:BlockDecl )
		Self.init=init
		Self.block=block
	End

	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New CatchStmt( LocalDecl( init.Copy() ),block.CopyBlock( scope ) )
	End
	
	Method OnSemant()
		init.Semant
		If Not ObjectType( init.type ) Err "Variable type must extend Throwable"
		If Not init.type.GetClass().IsThrowable() Err "Variable type must extend Throwable"
		block.InsertDecl init
		block.Semant
	End
	
	Method Trans$()
	End

End

Class ThrowStmt Extends Stmt

	Field expr:Expr
	
	Method New( expr:Expr )
		Self.expr=expr
	End
	
	Method OnCopy:Stmt( scope:ScopeDecl )
		Return New ThrowStmt( expr.Copy() )
	End
	
	Method OnSemant()
		expr=expr.Semant()
		If Not ObjectType( expr.exprType ) Err "Expression type must extend Throwable"
		If Not expr.exprType.GetClass().IsThrowable() Err "Expression type must extend Throwable"
	End
	
	Method Trans$()
		Return _trans.TransThrowStmt( Self )
	End
End
