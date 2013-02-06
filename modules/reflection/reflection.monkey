
Private

'box implementations private!
Alias BoolObject=monkey.boxes.BoolObject
Alias IntObject=monkey.boxes.IntObject
Alias FloatObject=monkey.boxes.FloatObject
Alias StringObject=monkey.boxes.StringObject
Alias ArrayObject=monkey.boxes.ArrayObject

Const ARRAY_PREFIX:="monkey.boxes.ArrayObject<"

Public

'Bitmasks returned by Attributes() methods
Const ATTRIBUTE_EXTERN=		$01
Const ATTRIBUTE_PRIVATE=	$02
Const ATTRIBUTE_ABSTRACT=	$04
Const ATTRIBUTE_FINAL=		$08
Const ATTRIBUTE_INTERFACE=	$10

Function BoolClass:ClassInfo()
	Return _boolClass
End

Function IntClass:ClassInfo()
	Return _intClass
End

Function FloatClass:ClassInfo()
	Return _floatClass
End

Function StringClass:ClassInfo()
	Return _stringClass
End

Function ArrayClass:ClassInfo( elemType:String )
	Return GetClass( ARRAY_PREFIX+elemType+">" )
End

Class ConstInfo

	Method New( name$,attrs,type:ClassInfo,value:Object )
		_name=name
		_attrs=attrs
		_type=type
		_value=value
	End
	
	Method Name$() Property
		Return _name
	End
	
	Method Attributes() Property
		Return _attrs
	End

	Method Type:ClassInfo() Property
		Return _type
	End

	Method GetValue:Object()
		Return _value
	End

	Private

	Field _name$
	Field _attrs
	Field _type:ClassInfo
	Field _value:Object
			
End

Class GlobalInfo

	Method New( name$,attrs,type:ClassInfo )
		_name=name
		_attrs=attrs
		_type=type
	End

	Method Name$() Property
		Return _name
	End

	Method Attributes() Property
		Return _attrs
	End
	
	Method Type:ClassInfo() Property
		Return _type
	End
	
	Method GetValue:Object() Abstract

	Method SetValue:Void( obj:Object ) Abstract
	
	Private

	Field _name$
	Field _attrs	
	Field _type:ClassInfo
			
End

Class FieldInfo

	Method New( name$,attrs,type:ClassInfo )
		_name=name
		_attrs=attrs
		_type=type
	End

	Method Name$() Property
		Return _name
	End
	
	Method Attributes() Property
		Return _attrs
	End

	Method Type:ClassInfo() Property
		Return _type
	End

	Method GetValue:Object( inst:Object ) Abstract
	
	Method SetValue:Void( inst:Object,value:Object ) Abstract
	
	Private

	Field _name$	
	Field _attrs
	Field _type:ClassInfo
			
End

Class MethodInfo

	Method New( name$,attrs,retType:ClassInfo,argTypes:ClassInfo[] )
		_name=name
		_attrs=attrs
		_retType=retType
		_argTypes=argTypes
	End

	Method Name$() Property
		Return _name
	End

	Method Attributes() Property
		Return _attrs
	End

	Method ReturnType:ClassInfo() Property
		Return _retType
	End
	
	Method ParameterTypes:ClassInfo[]() Property
		Return _argTypes
	End

	Method Invoke:Object( inst:Object,args:Object[] ) Abstract

	Private
		
	Field _name$
	Field _attrs	
	Field _retType:ClassInfo
	Field _argTypes:ClassInfo[]
			
End

Class FunctionInfo

	Method New( name$,attrs,retType:ClassInfo,argTypes:ClassInfo[] )
		_name=name
		_attrs=attrs
		_retType=retType
		_argTypes=argTypes
	End

	Method Name$() Property
		Return _name
	End

	Method Attributes() Property
		Return _attrs
	End

	Method ReturnType:ClassInfo() Property
		Return _retType
	End
	
	Method ParameterTypes:ClassInfo[]() Property
		Return _argTypes
	End

	Method Invoke:Object( args:Object[] ) Abstract

	Private
		
	Field _name$
	Field _attrs
	Field _retType:ClassInfo
	Field _argTypes:ClassInfo[]
			
End

Class ClassInfo

	Method New( name$,attrs,sclass:ClassInfo,ifaces:ClassInfo[] )
		_name=name
		_attrs=attrs
		_sclass=sclass
		_ifaces=ifaces
	End			
	
	Method Name$() Property
		Return _name
	End
	
	Method Attributes() Property
		Return _attrs
	End
	
	Method SuperClass:ClassInfo() Property
		Return _sclass
	End
	
	Method Interfaces:ClassInfo[]() Property
		Return _ifaces
	End
	
	Method ElementType:ClassInfo() Property
		Return Null
	End
	
	Method ArrayLength:Int( inst:Object )
		Error "Class is not an array class"
	End
	
	Method GetElement:Object( inst:Object,index )
		Error "Class is not an array class"
	End
	
	Method SetElement:Void( inst:Object,index,value:Object )
		Error "Class is not an array class"
	End
	
	Method NewInstance:Object()
		Error "Can't create instance of class"
	End
	
	Method NewArray:Object( length )
		Error "Can't create instance of array"
	End
	
	Method ExtendsClass?( clas:ClassInfo )

		If clas=Self Return True
		
		If clas._attrs & ATTRIBUTE_INTERFACE
			For Local t:=Eachin _ifaces
				If t.ExtendsClass( clas ) Return True
			Next
		Endif
		
		If _sclass Return _sclass.ExtendsClass( clas )

		Return False
	End
	
	Method GetConsts:ConstInfo[]( recursive? )
		If recursive Return _rconsts
		Return _consts
	End
	
	Method GetConst:ConstInfo( name$,recursive?=True )
		If Not _constsMap
			_constsMap=New StringMap<ConstInfo>
			For Local t:=Eachin _consts
				_constsMap.Set t.Name(),t
			Next
		Endif
		Local t:=_constsMap.Get( name )
		If Not t And _sclass And recursive Return _sclass.GetConst( name,True )
		Return t
	End
	
	Method GetGlobals:GlobalInfo[]( recursive? )
		If recursive Return _rglobals
		Return _globals
	End
	
	Method GetGlobal:GlobalInfo( name$,recursive?=True )
		If Not _globalsMap
			_globalsMap=New StringMap<GlobalInfo>
			For Local g:=Eachin _globals
				_globalsMap.Set g.Name(),g
			Next
		Endif
		Local g:=_globalsMap.Get( name )
		If Not g And _sclass And recursive Return _sclass.GetGlobal( name,True )
		Return g
	End
	
	Method GetFields:FieldInfo[]( recursive? )
		If recursive Return _rfields
		Return _fields
	End
	
	Method GetField:FieldInfo( name$,recursive?=True )
		If Not _fieldsMap
			_fieldsMap=New StringMap<FieldInfo>
			For Local f:=Eachin _fields
				_fieldsMap.Set f.Name(),f
			Next
		Endif
		Local f:=_fieldsMap.Get( name )
		If Not f And _sclass And recursive Return _sclass.GetField( name,True )
		Return f
	End
	
	Method GetMethods:MethodInfo[]( recursive? )
		If recursive Return _rmethods
		Return _methods
	End
	
	Method GetMethod:MethodInfo( name$,argTypes:ClassInfo[],recursive?=True )
		If Not _methodsMap
			_methodsMap=New StringMap<List<MethodInfo>>
			For Local f:=Eachin _methods
				Local list:=_methodsMap.Get( f.Name() )
				If Not list
					list=New List<MethodInfo>
					_methodsMap.Set f.Name(),list
				Endif
				list.AddLast f
			Next
		Endif
		Local list:=_methodsMap.Get( name )
		If list
			For Local f:=Eachin list
				If CmpArgs( f.ParameterTypes(),argTypes ) Return f
			Next
		Endif
		If _sclass And recursive Return _sclass.GetMethod( name,argTypes,True )
		Return Null
	End
	
	Method GetFunctions:FunctionInfo[]( recursive? )
		If recursive Return _rfunctions
		Return _functions
	End
	
	Method GetFunction:FunctionInfo( name$,argTypes:ClassInfo[],recursive?=True )
		If Not _functionsMap
			_functionsMap=New StringMap<List<FunctionInfo>>
			For Local f:=Eachin _functions
				Local list:=_functionsMap.Get( f.Name() )
				If Not list
					list=New List<FunctionInfo>
					_functionsMap.Set f.Name(),list
				Endif
				list.AddLast f
			Next
		Endif
		Local list:=_functionsMap.Get( name )
		If list
			For Local f:=Eachin list
				If CmpArgs( f.ParameterTypes(),argTypes ) Return f
			Next
		Endif
		If _sclass And recursive Return _sclass.GetFunction( name,argTypes,True )
		Return Null
	End
	
	Method GetConstructors:FunctionInfo[]()
		Return _ctors
	End
	
	Method GetConstructor:FunctionInfo( argTypes:ClassInfo[] )
		For Local f:=Eachin _ctors
			If CmpArgs( f.ParameterTypes(),argTypes ) Return f
		Next
		Return Null
	End
	
	Private
	
	Field _name$
	Field _attrs:Int
	Field _sclass:ClassInfo
	Field _ifaces:ClassInfo[]
	
	Field _consts:ConstInfo[]
	Field _fields:FieldInfo[]
	Field _globals:GlobalInfo[]
	Field _methods:MethodInfo[]
	Field _functions:FunctionInfo[]
	Field _ctors:FunctionInfo[]

	Field _rconsts:ConstInfo[]
	Field _rglobals:GlobalInfo[]
	Field _rfields:FieldInfo[]
	Field _rmethods:MethodInfo[]
	Field _rfunctions:FunctionInfo[]

	Field _globalsMap:StringMap<GlobalInfo>
	Field _fieldsMap:StringMap<FieldInfo>
	Field _methodsMap:StringMap<List<MethodInfo>>
	Field _functionsMap:StringMap<List<FunctionInfo>>

	Method Init()
	End
	
	Method InitR()
		If _sclass
			Local consts:=New Stack<ConstInfo>( _sclass._rconsts )
			For Local t:=Eachin _consts
				consts.Push t
			Next
			_rconsts=consts.ToArray()
			Local fields:=New Stack<FieldInfo>( _sclass._rfields )
			For Local t:=Eachin _fields
				fields.Push t
			Next
			_rfields=fields.ToArray()
			Local globals:=New Stack<GlobalInfo>( _sclass._rglobals )
			For Local t:=Eachin _globals
				globals.Push t
			Next
			_rglobals=globals.ToArray()
			Local methods:=New Stack<MethodInfo>( _sclass._rmethods )
			For Local t:=Eachin _methods
				methods.Push t
			Next
			_rmethods=methods.ToArray()
			Local functions:=New Stack<FunctionInfo>( _sclass._rfunctions )
			For Local t:=Eachin _functions
				functions.Push t
			Next
			_rfunctions=functions.ToArray()
		Else
			_rconsts=_consts
			_rfields=_fields
			_rglobals=_globals
			_rmethods=_methods
			_rfunctions=_functions
		Endif
	End

End

Function GetClasses:ClassInfo[]()
	Return _classes
End

Function GetClass:ClassInfo( name$ )
	If Not _classesMap
		_classesMap=New StringMap<ClassInfo>
		For Local c:=Eachin _classes
			Local name:=c.Name()
			_classesMap.Set name,c
			Local i:=name.FindLast( "." )
			If i=-1 Continue
			name=name[i+1..]
			If _classesMap.Contains( name )
				_classesMap.Set name,Null
			Else
				_classesMap.Set name,c
			Endif
		Next
	Endif
	Return _classesMap.Get( name )
End

Function GetClass:ClassInfo( obj:Object )
	Return _getClass.GetClass( obj )
End

Function GetConsts:ConstInfo[]()
	Return _consts
End

Function GetConst:ConstInfo( name$ )
	If Not _constsMap
		_constsMap=New StringMap<ConstInfo>
		For Local t:=Eachin _consts
			Local name:=t.Name()
			_constsMap.Set name,t
			Local i:=name.FindLast( "." )
			If i<>-1
				name=name[i+1..]
				If _constsMap.Contains( name )
					_constsMap.Set name,Null
				Else
					_constsMap.Set name,t
				Endif
			Endif
		Next
	Endif
	Return _constsMap.Get( name )
End

Function GetGlobals:GlobalInfo[]()
	Return _globals
End

Function GetGlobal:GlobalInfo( name$ )
	If Not _globalsMap
		_globalsMap=New StringMap<GlobalInfo>
		For Local g:=Eachin _globals
			Local name:=g.Name()
			_globalsMap.Set name,g
			Local i:=name.FindLast( "." )
			If i<>-1
				name=name[i+1..]
				If _globalsMap.Contains( name )
					_globalsMap.Set name,Null
				Else
					_globalsMap.Set name,g
				Endif
			Endif
		Next
	Endif
	Return _globalsMap.Get( name )
End

Function GetFunctions:FunctionInfo[]()
	Return _functions
End

Function GetFunction:FunctionInfo( name$,argTypes:ClassInfo[] )
	If Not _functionsMap
		_functionsMap=New StringMap<List<FunctionInfo>>
		For Local f:=Eachin _functions

			Local name:=f.Name()
			
			Local list:=_functionsMap.Get( name )
			If Not list
				list=New List<FunctionInfo>
				_functionsMap.Set name,list
			Endif
			list.AddLast f

			Local i:=name.FindLast( "." )
			If i=-1 Continue
			name=name[i+1..]

			list=_functionsMap.Get( name )
			
			If list
				Local found:=False
				For Local f2:=Eachin list
					If CmpArgs( f.ParameterTypes(),f2.ParameterTypes() )
						found=True
						Exit
					Endif
				Next
				If found
					_functionsMap.Set name,Null
					Continue
				Endif
			Else
				If _functionsMap.Contains( name ) Continue
				list=New List<FunctionInfo>
				_functionsMap.Set name,list
			Endif
			list.AddLast f
		Next
	Endif
	Local list:=_functionsMap.Get( name )
	If list
		For Local f:=Eachin list
			If CmpArgs( f.ParameterTypes(),argTypes ) Return f
		Next
	Endif
	Return Null
End

Private

Function InternalErr()
	Error "Error!"
End

Global _boolClass:ClassInfo
Global _intClass:ClassInfo
Global _floatClass:ClassInfo
Global _stringClass:ClassInfo
Global _unknownClass:ClassInfo=New UnknownClass

Global _classes:ClassInfo[]
Global _consts:ConstInfo[]
Global _globals:GlobalInfo[]
Global _functions:FunctionInfo[]

Global _classesMap:StringMap<ClassInfo>
Global _constsMap:StringMap<ConstInfo>
Global _globalsMap:StringMap<GlobalInfo>
Global _functionsMap:StringMap<List<FunctionInfo>>

Class _GetClass
	Method GetClass:ClassInfo( obj:Object ) Abstract
End

Global _getClass:_GetClass

Function CmpArgs( args0:ClassInfo[],args1:ClassInfo[] )
	If args0.Length<>args1.Length Return False
	For Local i=0 Until args0.Length
		If args0[i]<>args1[i] Return False
	Next
	Return True
End

Class UnknownClass Extends ClassInfo

	Method New()
		Super.New( "?",0,Null,[] )
	End

End
