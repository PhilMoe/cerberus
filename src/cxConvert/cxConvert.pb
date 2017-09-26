EnableExplicit

;UsePNGImageDecoder()

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; Utility to convert Monkey X files/projects to Cerberus X
; Author: Michael Hartlef
; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Global Window_0
Global event
Global Text_0, Text_1, strFolder, btnFolder, btnConvert, txtStatus, ckbBackup, ckbRenameImport
;Global imgCog

CompilerIf #PB_Compiler_OS = #PB_OS_Windows 
  Global slash$="\"
CompilerElse
  Global slash$="/"
CompilerEndIf

;imgCog = CatchImage(#PB_Any, ?Img_Cog)

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Procedure OpenWindow_0(x = 0, y = 0, width = 600, height = 130)
  Window_0 = OpenWindow(#PB_Any, x, y, width, height, "Monkey X to Cerberus X Converter", #PB_Window_SystemMenu | #PB_Window_ScreenCentered)
  Text_0 = TextGadget(#PB_Any, 10, 20, 100, 25, "Folder to convert:")
  strFolder = StringGadget(#PB_Any, 100, 20, 450, 20, "")
  btnFolder = ButtonGadget(#PB_Any, 560, 20, 30, 25, "...")
  
  ckbBackup = CheckBoxGadget(#PB_Any, 50, 55, 150, 25, "Backup directory")
  ckbRenameImport = CheckBoxGadget(#PB_Any, 160, 55, 250, 25, "Rename Monkey. imports, resource prefixes")
  SetGadgetState(ckbBackup,#PB_Checkbox_Checked)
  SetGadgetState(ckbRenameImport,#PB_Checkbox_Checked)
  
  ;btnConvert = ButtonImageGadget(#PB_Any, 470, 55, 80, 25, ImageID(imgCog),#PB_Button_Right     )
  ;SetGadgetText(btnConvert, "Convert")
  btnConvert = ButtonGadget(#PB_Any, 470, 55, 80, 25, "Convert")
  Text_1 = TextGadget(#PB_Any, 10, 100, 40, 25, "Status:")
  ContainerGadget(#PB_Any,50,98,540,20, #PB_Container_Flat)
  txtStatus = TextGadget(#PB_Any, 5, 1, 530, 25, "Pick a directory to convert...")
  CloseGadgetList()
  DisableGadget(btnConvert, 1)
EndProcedure

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Procedure FixFile(name$)
  Protected format
  Protected copyFName$
  Protected line$
  Protected lineNew$
  Protected lineNew2$
  copyFName$ = GetPathPart(name$)+"zz_"+GetFilePart(name$)
  ;Debug "Fix imports..."+ name$
  If CopyFile(name$,copyFName$)
    If GetExtensionPart(name$)="cxs"
      If ReadFile(0, copyFName$)
        format = ReadStringFormat(0)
        If CreateFile(1, name$, format)
          While Not Eof(0)
            line$ = ReadString(0, format)
            lineNew$ = ReplaceString(line$,"Import monkey","Import cerberus", #PB_String_NoCase,1,1)
            lineNew2$ = ReplaceString(lineNew$,"monkey://","cerberus://", #PB_String_NoCase,1,99)
            lineNew$ = ReplaceString(lineNew2$,".monkey",".cxs", #PB_String_NoCase,1,99)
            WriteStringN(1, lineNew$)
          Wend
          
          CloseFile(0)        
          CloseFile(1)
        EndIf
      EndIf
    EndIf
    DeleteFile(copyFName$, #PB_FileSystem_Force)
  Else
    Debug ("Error copying file "+name$)
  EndIf
EndProcedure

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Procedure ConvertFile(file$, newExt$)
  Protected newFName$
  SetGadgetText(txtStatus, "Rename: "+file$)
  newFName$ = GetPathPart(file$)+GetFilePart(file$, #PB_FileSystem_NoExtension)+"."+newExt$
  RenameFile(file$, newFName$)
  If GetGadgetState(ckbRenameImport) = #PB_Checkbox_Checked
    SetGadgetText(txtStatus, "Fix: "+file$)
    FixFile(newFName$)
  EndIf
  SetGadgetText(txtStatus, "Conversion: "+file$+"  -> DONE!")
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
          
        Case 1
          ScanDir(dir$+name$)
          If LCase(name$)="monkeydoc"
            RenameFile(dir$+name$,dir$+"cerberusdoc")
          EndIf
        Case 2
          If LCase(GetExtensionPart(name$))="monkey"
            ConvertFile(dir$+name$, "cxs")
          EndIf
          If LCase(GetExtensionPart(name$))="monkeydoc"
            ConvertFile(dir$+name$, "cerberusdoc")
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
            If GetGadgetState(ckbBackup) = #PB_Checkbox_Checked
              backup$ = GetGadgetText(strFolder)
              If Right(backup$,1) = slash$
                backup$ = Left(backup$,Len(backup$)-1)+"_Backup_"+FormatDate("%yyyy%mm%dd%hh%ii%ss", Date())+slash$
              Else
                backup$ = backup$+"_Backup_"+FormatDate("%yyyy%mm%dd%hh%ii%ss", Date())+slash$
              EndIf
              CopyDirectory(GetGadgetText(strFolder), backup$,"*.*",#PB_FileSystem_Recursive|#PB_FileSystem_Force)
            EndIf
            
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
  SetGadgetState(ckbRenameImport, ReadPreferenceLong("FixImport",1))
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
  WritePreferenceLong("FixImport", GetGadgetState(ckbRenameImport)) 
  WritePreferenceString("LastDirectory", GetGadgetText(strFolder))
  ClosePreferences()
EndIf
End 


; DataSection
;   Img_Cog: 
;   IncludeBinary "cog.png"
; EndDataSection
; IDE Options = PureBasic 5.61 (Windows - x64)
; CursorPosition = 85
; FirstLine = 62
; Folding = --
; EnableXP
; Executable = cxConverter.exe