
'Cerberus launcher app for win32

Strict

Framework brl.blitz
Import pub.stdc
Import brl.standardio

?Win32
Import "resource.o"
Import "-ladvapi32"
?

Const VERSION=67

?Win32
Extern "win32"

Const HKEY_CLASSES_ROOT=	$80000000
Const HKEY_CURRENT_USER=	$80000001
Const HKEY_LOCAL_MACHINE=	$80000002
Const HKEY_USERS=			$80000003
Const HKEY_CURRENT_CONFIG=	$80000005

Const REG_OPTION_VOLATILE=	1
Const REG_OPTION_NON_VOLATILE=0

Const KEY_QUERY_VALUE=		$00001
Const KEY_SET_VALUE=		$00002
Const KEY_CREATE_SUB_KEY=	$00004
Const KEY_ENUMERATE_SUB_KEYS=$00008 
Const KEY_READ=				$20019
Const KEY_ALL_ACCESS=		$f003f

Const REG_SZ=				1

Const RRF_RT_REG_SZ=		2

Const MB_OK=0


Function RegOpenKeyExW( root,subkey$w,options,access,hkey:Byte Ptr Ptr )
Function RegCreateKeyExW( root,subkey$w,reserved,clas,options,access,attrs,hkey:Byte Ptr Ptr,disp:Int Ptr )
Function RegCloseKey( hkey:Byte Ptr )
Function RegSetValueExW( hkey:Byte Ptr,name$w,reserved,dwtype,data$w,size )
Function RegQueryValueExW( hkey:Byte Ptr,name$w,reserved,dwtype:Int Ptr,data:Short Ptr,size:Int Ptr )

Function MessageBoxW( hwnd,text$w,caption$w,utype )

End Extern

Function SetKey( root,subkey$,name$,value$ )
	Local hkey:Byte Ptr
	If RegCreateKeyExW( root,subkey,0,0,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,0,Varptr hkey,Null )=0
		If RegSetValueExW( hkey,name,0,REG_SZ,value,value.length*2+2 )=0
			RegCloseKey( hkey )
			Return True
		EndIf
		RegCloseKey( hkey )
	EndIf
	Return False
End Function

Function GetKey$( root,subkey$,name$ )
	Local hkey:Byte Ptr
	If RegOpenKeyExW( root,subkey,REG_OPTION_NON_VOLATILE,KEY_QUERY_VALUE,Varptr hkey )=0
		Local ty,buf:Short[1024],sz=1024*2
		If RegQueryValueExW( hkey,name,0,Varptr ty,buf,Varptr sz )=0
			If ty=REG_SZ
				RegCloseKey( hkey )
				buf[1023]=0
				Return String.FromWString( buf )
			EndIf
		EndIf
		RegCloseKey( hkey )
	EndIf
	Return ""
End Function

?

Local args$
For Local i=1 Until AppArgs.Length
	Local arg$=AppArgs[i]
	If arg.Contains(" ") arg="~q"+arg+"~q"
	args:+" "+arg
Next

'If args MessageBoxW( 0,args,"AppArgs",MB_OK )

?Win32

Local app$=AppFile.Replace("/","\")

'Needs special privs...?
'
'Local root=HKEY_LOCAL_MACHINE

Local root=HKEY_CURRENT_USER
Local type_path$="Software\Classes\.cxs"
Local tool_path$="Software\Classes\cerberus-x.com"

If VERSION>Int( GetKey( root,tool_path+"\Version","" ) )
	SetKey( root,tool_path,"","Simple Cerberus IDE" )
	SetKey( root,tool_path+"\Version","",String(VERSION) )
	SetKey( root,tool_path+"\DefaultIcon","",app )
	SetKey(root,tool_path+"\Shell\Open\Command","","~q"+app+"~q ~q%1~q" )
EndIf

If GetKey( root,type_path,"" )=""
	SetKey( root,type_path,"","cerberus-x.com" )
EndIf

system_ "bin\Ted.exe"+args

?Macos

Local cmd$="bin/Ted.app"
If args cmd="open -n "+cmd+" --args"+args Else cmd="open "+cmd
system_ cmd
Delay 100

?Linux

system_ "bin/Ted"+args+" >/dev/null 2>/dev/null &"

?
