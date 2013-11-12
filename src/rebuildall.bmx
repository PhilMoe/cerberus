
Strict

RebuildTrans 
'RebuildMakedocs
'RebuildMServer
'RebuildMonkey

End

?Win32
Const bin$="..\bin\"
Const ext$="_winnt.exe"
?MacOS
Const bin$="../bin/"
Const ext$="_macos"
?Linux
Const bin$="../bin/"
Const ext$="_linux"
?

Const QUICKTRANS=False

Const trans$=bin+"transcc"+ext

Const makedocs$=bin+"makedocs"+ext

Function system( cmd$,fail=True )
	If system_( cmd ) 
		If fail
			Print "system failed for: "+cmd
			End
		EndIf
	EndIf
End Function

Function RebuildTrans()
	If QUICKTRANS
?Win32
		system "g++ -o ..\bin\transcc_winnt.exe transcc\transcc.build\cpptool\main.cpp"
?Macos
		system "g++ -arch i386 -read_only_relocs suppress -mmacosx-version-min=10.3 -o ../bin/transcc_macos transcc/transcc.build/cpptool/main.cpp"
?Linux
		system "g++ -o ../bin/transcc_linux transcc/transcc.build/cpptool/main.cpp"
?
		Return
	EndIf
	
	Const trans_mk$=trans+" -target=C++_Tool"
	Const trans_tmp$="transcc/transcc.build/cpptool/main"+ext
	
	system trans_mk+" -clean -config=release +CPP_DOUBLE_PRECISION_FLOATS=1 +CPP_GC_MODE=0 transcc/transcc.monkey"
	
	Delay 100
	
	DeleteFile trans
	If FileType( trans )
		Print "***** ERROR ***** Failed to delete transcc"
		End
	EndIf
	
	CopyFile trans_tmp,trans
	If FileType( trans )<>FILETYPE_FILE 
		Print "***** ERROR ***** Failed to copy transcc"
		End
	EndIf

?Not win32
	system "chmod +x "+trans
?
	Print "transcc built OK!"

End Function

Function RebuildMakedocs()
	Const makedocs_tmp$="makedocs/makedocs.build/cpptool/main"+ext

	system trans+" -target=C++_Tool -clean -config=release makedocs/makedocs.monkey"
	
	DeleteFile makedocs
	If FileType( makedocs )
		Print "***** ERROR ***** Failed to delete makedocs"
		End
	Endif
	
	CopyFile makedocs_tmp,makedocs
	If FileType( makedocs )<>FILETYPE_FILE
		Print "***** ERROR ***** Failed to copy makedocs"
		End
	Endif

?Not win32
	system "chmod +x "+makedocs
?
	Print "makedocs built OK!"
End Function

Function RebuildMServer()
	system "~q"+BlitzMaxPath()+"/bin/bmk~q makeapp -h -t gui -a -r -o "+bin+"mserver"+ext+" mserver/mserver.bmx"
	Print "mserver built OK!"
End Function

Function RebuildMonkey()
	'windres resource.rc resource.o
	system "~q"+BlitzMaxPath()+"/bin/bmk~q makeapp -t gui -a -r -o ../Monkey monkey/monkey.bmx"
?MacOS
	system "cp monkey/info.plist ../Monkey.app/Contents"
	system "rm ../Monkey.app/Contents/Resources/monkey.icns"
	system "cp monkey/monkey.icns ../Monkey.app/Contents/Resources"
?	
	Print "Monkey built OK!"
End Function
