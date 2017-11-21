EnableExplicit

;UsePNGImageDecoder()

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; Utility to convert Monkey X files/projects to Cerberus X
; Author: Michael Hartlef
; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Global Window_0
Global event
Global Text_0, Text_1, strFolder, btnFolder, btnConvert, txtStatus, ckbBackup
Global ckbRenameSrcFile, ckbRenameDocFile, ckbRenameDocDir, ckbFixPath, ckbFixImport
;Global imgCog

CompilerIf #PB_Compiler_OS = #PB_OS_Windows 
  Global slash$="\"
CompilerElse
  Global slash$="/"
CompilerEndIf

;imgCog = CatchImage(#PB_Any, ?Img_Cog)

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Procedure OpenWindow_0(x = 0, y = 0, width = 600, height = 330)
  Window_0 = OpenWindow(#PB_Any, x, y, width, height, "Monkey X to Cerberus X Converter", #PB_Window_SystemMenu | #PB_Window_ScreenCentered)
  Text_0 = TextGadget(#PB_Any, 10, 20, 100, 25, "Folder to convert:")
  strFolder = StringGadget(#PB_Any, 100, 20, 450, 20, "")
  btnFolder = ButtonGadget(#PB_Any, 560, 20, 30, 25, "...")
  
  ckbBackup = CheckBoxGadget(#PB_Any, 50, 55, 150, 25, "Backup directory")
  ckbRenameSrcFile = CheckBoxGadget(#PB_Any, 50, 85, 250, 25, "Rename .monkey files")
  ckbRenameDocFile = CheckBoxGadget(#PB_Any, 50, 115, 250, 25, "Rename .monkeydoc files")
  ckbRenameDocDir  = CheckBoxGadget(#PB_Any, 50, 145, 250, 25, "Rename monkeydoc directories")
  ckbFixPath       = CheckBoxGadget(#PB_Any, 50, 175, 250, 25, "Fix monkey:// file paths")
  ckbFixImport     = CheckBoxGadget(#PB_Any, 50, 205, 250, 25, "Fix monkey imports")
  
  SetGadgetState(ckbBackup,#PB_Checkbox_Checked)
  SetGadgetState(ckbRenameSrcFile,#PB_Checkbox_Checked)
  SetGadgetState(ckbRenameDocFile,#PB_Checkbox_Checked)
  SetGadgetState(ckbRenameDocDir,#PB_Checkbox_Checked)
  SetGadgetState(ckbFixPath,#PB_Checkbox_Checked)
  SetGadgetState(ckbFixImport,#PB_Checkbox_Checked)
  
  ;btnConvert = ButtonImageGadget(#PB_Any, 470, 55, 80, 25, ImageID(imgCog),#PB_Button_Right     )
  ;SetGadgetText(btnConvert, "Convert")
  btnConvert = ButtonGadget(#PB_Any, 470, 255, 80, 25, "Convert")
  
  Text_1 = TextGadget(#PB_Any, 10, 300, 40, 25, "Status:")
  ContainerGadget(#PB_Any,50,298,540,20, #PB_Container_Flat)
  txtStatus = TextGadget(#PB_Any, 5, 1, 530, 25, "Pick a directory to convert...")
  CloseGadgetList()
  DisableGadget(btnConvert, 1)
EndProcedure

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Procedure FixFile(name$)
  Protected format
  Protected copyFName$
  ;Protected line$
  Protected lineNew$
  Protected changed = 0
  ;Protected lineNew2$
  copyFName$ = GetPathPart(name$)+"zz_"+GetFilePart(name$)
  ;Debug "Fix imports..."+ name$
  If CopyFile(name$,copyFName$)
    ;If GetExtensionPart(name$)="cxs"
      If ReadFile(0, copyFName$)
        format = ReadStringFormat(0)
        If CreateFile(1, name$, format)
          While Not Eof(0)
            lineNew$ = ReadString(0, format)
            
            If GetGadgetState(ckbFixImport) = #PB_Checkbox_Checked
              lineNew$ = ReplaceString(lineNew$,"Import monkey","Import cerberus", #PB_String_NoCase,1,1)
            EndIf
            
            If GetGadgetState(ckbFixPath) = #PB_Checkbox_Checked
              lineNew$ = ReplaceString(lineNew$,"monkey://","cerberus://", #PB_String_NoCase,1,99)
            EndIf
            
            WriteStringN(1, lineNew$)
          Wend
          
          CloseFile(0)        
          CloseFile(1)
        EndIf
      EndIf
    ;EndIf
    DeleteFile(copyFName$, #PB_FileSystem_Force)
  Else
    Debug ("Error copying file "+name$)
  EndIf
EndProcedure

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Procedure FixDoc(name$)
  Protected format
  Protected copyFName$
  Protected lineNew$
  Protected changed = 0
  copyFName$ = GetPathPart(name$)+"zz_"+GetFilePart(name$)
  If CopyFile(name$,copyFName$)
    ;If GetExtensionPart(name$)="cerberusdoc"
      If ReadFile(0, copyFName$)
        format = ReadStringFormat(0)
        If CreateFile(1, name$, format)
          While Not Eof(0)
            lineNew$ = ReadString(0, format)
            
            If GetGadgetState(ckbRenameSrcFile) = #PB_Checkbox_Checked
              lineNew$ = ReplaceString(lineNew$,".monkey",".cxs", #PB_String_NoCase,1,99)
            EndIf
            
            WriteStringN(1, lineNew$)
          Wend
          
          CloseFile(0)        
          CloseFile(1)
        EndIf
      EndIf
    ;EndIf
    DeleteFile(copyFName$, #PB_FileSystem_Force)
  Else
    Debug ("Error copying file "+name$)
  EndIf
EndProcedure

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Procedure RenameFiles(file$, newExt$)
  Protected newFName$
  SetGadgetText(txtStatus, "Rename: "+file$)
  newFName$ = GetPathPart(file$)+GetFilePart(file$, #PB_FileSystem_NoExtension)+"."+newExt$
  RenameFile(file$, newFName$)
  SetGadgetText(txtStatus, "Renaming: "+file$+"  -> DONE!")
EndProcedure

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Procedure ScanDir(dir$)
  Protected typ.l = 0
  Protected name$
  Protected dirnum.l
  If Right(dir$,1)=slash$
  Else
    dir$ = dir$ + slash$
  EndIf
  
  dirnum = ExamineDirectory(#PB_Any, dir$, "*.*")  
  If dirnum
    While NextDirectoryEntry(dirnum)
      name$ = DirectoryEntryName(dirnum)
      typ = 0
      If DirectoryEntryType(dirnum) = #PB_DirectoryEntry_File
        typ = 2
      Else
        typ = 1
      EndIf
      If name$ = "." Or name$ = ".."
        typ = 0
      EndIf
      Select typ
          
        Case 1     ; directory
          
          ; Scan that directory
          ScanDir(dir$+name$)
          
          ; Need to rename the monkeydoc directory?
          If LCase(name$)="monkeydoc" And GetGadgetState(ckbRenameDocDir) = #PB_Checkbox_Checked
            RenameFile(dir$+name$,dir$+"cerberusdoc")
          EndIf
          
        Case 2     ; source file
          
          
          ; .monkey source file
          If LCase(GetExtensionPart(name$))="monkey"
            ; Fix the monkey imports and file paths
            FixFile(dir$+name$)
            ; Change the file extension
            If GetGadgetState(ckbRenameSrcFile) = #PB_Checkbox_Checked
              RenameFiles(dir$+name$, "cxs")
            EndIf
          EndIf
          
          ; .monkeydoc file
          If LCase(GetExtensionPart(name$))="monkeydoc"
            ; Fix the monkey example file extensions
            FixDoc(dir$+name$)
            If GetGadgetState(ckbRenameDocFile) = #PB_Checkbox_Checked
              RenameFiles(dir$+name$, "cerberusdoc")
            EndIf
          EndIf
          
      EndSelect
      
      
      
    Wend
    FinishDirectory(dirnum)
    
  EndIf
EndProcedure

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Procedure Window_0_Events(event)
  Protected folder$
  Protected backup$
  
  Select event
    Case #PB_Event_CloseWindow
      If MessageRequester("Quit app...", "Do you want to close the app?", #PB_MessageRequester_YesNo) = #PB_MessageRequester_Yes    
        ProcedureReturn #False
      EndIf
      
    Case #PB_Event_Menu
      Select EventMenu()
      EndSelect
      
    Case #PB_Event_Gadget
      Select EventGadget()
          
        Case strFolder
          If FileSize(GetGadgetText(strFolder))=-2
            DisableGadget(btnConvert, 0)
            SetGadgetText(txtStatus, "Press Convert to start the conversion process!")
          Else
            DisableGadget(btnConvert, 1)
            SetGadgetText(txtStatus, "Pick a directory to convert!")
          EndIf
          
        Case btnFolder
          folder$ = PathRequester("Select folder to convert...", GetGadgetText(strFolder))
          If folder$
            SetGadgetText(strFolder, folder$)
            DisableGadget(btnConvert, 0)
          Else
            DisableGadget(btnConvert, 1)
          EndIf
          
        Case btnConvert
          If SetCurrentDirectory(GetGadgetText(strFolder))
            SetGadgetText(txtStatus, "Conversion in progress...")
            ; Need To do a backup?
            If GetGadgetState(ckbBackup) = #PB_Checkbox_Checked
              backup$ = GetGadgetText(strFolder)
              If Right(backup$,1) = slash$
                backup$ = Left(backup$,Len(backup$)-1)+"_Backup_"+FormatDate("%yyyy%mm%dd%hh%ii%ss", Date())+slash$
              Else
                backup$ = backup$+"_Backup_"+FormatDate("%yyyy%mm%dd%hh%ii%ss", Date())+slash$
              EndIf
              CopyDirectory(GetGadgetText(strFolder), backup$,"*.*",#PB_FileSystem_Recursive|#PB_FileSystem_Force)
            EndIf
            ; Scan the source directory
            ScanDir(GetGadgetText(strFolder))
            
            SetGadgetText(txtStatus, "Conversion done!")
          EndIf
      EndSelect
  EndSelect
  ProcedureReturn #True
EndProcedure


OpenWindow_0()

If OpenPreferences(GetUserDirectory(#PB_Directory_Documents)+"CerberusX/cxConverter.prefs") <> 0
  PreferenceGroup("Global")
  SetGadgetText(strFolder, ReadPreferenceString("LastDirectory",""))
  SetGadgetState(ckbBackup, ReadPreferenceLong("BackupDir",1))
  SetGadgetState(ckbFixImport, ReadPreferenceLong("FixImport",1))
  SetGadgetState(ckbFixPath, ReadPreferenceLong("FixPath",1))
  SetGadgetState(ckbRenameDocDir, ReadPreferenceLong("RenameDocDir",1))
  SetGadgetState(ckbRenameDocFile, ReadPreferenceLong("RenameDocFile",1))
  SetGadgetState(ckbRenameSrcFile, ReadPreferenceLong("RenameSrcFile",1))
  ClosePreferences()
EndIf
SetActiveGadget(strFolder)

Repeat
  Event = WaitWindowEvent()
Until Window_0_Events(Event)=#False

If FileSize(GetUserDirectory(#PB_Directory_Documents)+"CerberusX") <> -2
  CreateDirectory(GetUserDirectory(#PB_Directory_Documents)+"CerberusX")
EndIf
If CreatePreferences(GetUserDirectory(#PB_Directory_Documents)+"CerberusX/cxConverter.prefs", #PB_Preference_GroupSeparator)
  PreferenceGroup("Global")
  WritePreferenceLong("BackupDir", GetGadgetState(ckbBackup)) 
  WritePreferenceLong("FixImport", GetGadgetState(ckbFixImport)) 
  WritePreferenceLong("FixPath", GetGadgetState(ckbFixPath)) 
  WritePreferenceLong("RenameDocDir", GetGadgetState(ckbRenameDocDir)) 
  WritePreferenceLong("RenameDocFile", GetGadgetState(ckbRenameDocFile)) 
  WritePreferenceLong("RenameSrcFile", GetGadgetState(ckbRenameSrcFile)) 
  WritePreferenceString("LastDirectory", GetGadgetText(strFolder))
  ClosePreferences()
EndIf
End 


; DataSection
;   Img_Cog: 
;   IncludeBinary "cog.png"
; EndDataSection
; IDE Options = PureBasic 5.61 (Windows - x64)
; CursorPosition = 138
; FirstLine = 153
; Folding = --
; EnableXP
; Executable = cxConverter.exe