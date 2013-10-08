
#If TARGET<>"glfw"
#Error "glfw only!"
#End

#TEXT_FILES="*.txt|*.fnt"

#GLFW_WINDOW_TITLE="Monkey Module Manager"
#GLFW_WINDOW_WIDTH=640
#GLFW_WINDOW_HEIGHT=512
#GLFW_WINDOW_RESIZABLE=False
#GLFW_WINDOW_FULLSCREEN=False

Import mojo
Import os
Import brl.process
Import modpath
Import microgui

Alias GraphicsContext=microgui.GraphicsContext

Global AppDir:String
Global MonkeyDir:String
Global ModulesDir:String

Class Repos

	Const UNKNOWN:=0
	Const INSTALLED:=2
	Const UPDATEABLE:=3
	Const UNINSTALLED:=4
	Const INSTALLABLE:=5
	Const UPTODATE:=6
		
	Field type:String	'hg, git, svn
	Field url:String
	Field name:String
	Field ident:String
	Field desc:String

	Field state:Int

	Field check:=New CheckBox
	Field label:=New Label
	
	Field cmd:String
	Field cmdcd:String
	Field proc:Process
	Field strbuf:=New StringStack
	Field out:String

	Global tmpbuf:=New DataBuffer( 1024 )
	
	Method New( type:String,url:String,name:String,ident:String,desc:String )
		Self.type=type
		Self.url=url
		Self.name=name
		Self.ident=ident
		Self.desc=desc
		check.IsEnabled=False
		Select FileType( ModulesDir+"/"+ident )
		Case FILETYPE_DIR
			state=INSTALLED
		Case FILETYPE_NONE
			state=UNINSTALLED
		End
	End
	
	Method StartNextProc:Void()
		Local cmd:=Self.cmd
		Local i:=cmd.Find( ";" )
		Self.cmd=cmd[i+1..]
		cmd=cmd[..i]
		Local cd:=CurrentDir()
		ChangeDir cmdcd
		proc=New Process
		If Not proc.Start( cmd )
			Print "Failed to start process! cmd="+cmd
			proc=Null
		Endif
		ChangeDir cd
	End
	
	Method ProcBusy:Bool()
		If Not proc And Not cmd Return False
		
		If proc
			While proc.StdoutAvail
				Local n:=proc.ReadStdout( tmpbuf,0,1024 )
				Local str:=tmpbuf.PeekString( 0,n,"ascii" )
				strbuf.Push str
			Wend
			If proc.IsRunning Return True
			proc=Null
		Endif
		
		If cmd
			StartNextProc
			Return True
		Endif
		
		out=strbuf.Join()
		strbuf.Clear
		
		Return False
	End
	
	Method StartProc:Void( cmd:String )
		Self.cmd=cmd+";"
		Self.cmdcd=CurrentDir()
		StartNextProc
	End
	
	Method BeginUpdate:Void()
		Select state
		Case INSTALLED
			label.Text="Busy..."
		Case UNINSTALLED
			label.Text="Busy..."
		Case UPDATEABLE
			label.Text="Updating..."
		Case INSTALLABLE
			label.Text="Installing..."
		End
		Local cd:=CurrentDir()
		ChangeDir ModulesDir
		Select type
		Case "hg" 
			BeginUpdate_hg()
		Case "git" 
			BeginUpdate_git()
		End
		ChangeDir cd
	End
	
	Method EndUpdate:Void()
		Local cd:=CurrentDir()
		ChangeDir ModulesDir
		Select type
		Case "hg" 
			state=EndUpdate_hg()
		Case "git" 
			state=EndUpdate_git()
		End
		ChangeDir cd
		Select state
		Case INSTALLED
			label.Text="<Installed>"
		Case UNINSTALLED
			label.Text="<Uninstalled>"
		Case UPDATEABLE
			label.Text="Update available"
			check.IsEnabled=True
		Case INSTALLABLE
			label.Text="Uninstalled"
			check.IsEnabled=True
		Case UPTODATE
			label.Text="Installed"
			check.IsChecked=False
			check.IsEnabled=False
		Default
			label.Text="<Unknown>"
		End
	End
	
	Method BeginUpdate_hg:Void()
		Select state
		Case UNINSTALLED
			StartProc( "hg identify "+url )
		Case INSTALLABLE
			StartProc( "hg clone "+url+" "+ident )
		Case INSTALLED
			ChangeDir ident
			StartProc( "hg incoming" )
		Case UPDATEABLE
			ChangeDir ident
			StartProc( "hg pull -u" )
		End
	End
	
	Method EndUpdate_hg:Int()
		Select state
		Case UNINSTALLED
			If Not out.StartsWith( "abort:" )
				Return INSTALLABLE
			Endif
		Case INSTALLABLE
			If FileType( ident )=FILETYPE_DIR And out.Contains( " files updated, 0 files merged, 0 files removed, 0 files unresolved" )
				Return UPTODATE
			Endif
			DeleteDir ident,True
		Case INSTALLED
			If out.Contains( "changeset:" )
				Return UPDATEABLE
			Else If out.Contains( "no changes found" )
				Return UPTODATE
			Endif
		Case UPDATEABLE
			If out.Contains( " files updated, 0 files merged, 0 files removed, 0 files unresolved" )
				Return UPTODATE
			Endif
		End
		Return UNKNOWN
	End
	
	Method BeginUpdate_git:Void()
		Select state
		Case UNINSTALLED
			StartProc( "git ls-remote "+url )
		Case INSTALLABLE
			StartProc( "git clone "+url+" "+ident )
		Case INSTALLED
			ChangeDir ident
			StartProc( "git fetch;git log ..origin/master" )
		Case UPDATEABLE
			ChangeDir ident
			StartProc( "git merge origin" )
		End
	End

	Method EndUpdate_git:Int()
		Select state
		Case UNINSTALLED
			If out.Contains( "~tHEAD~n" )
				Return INSTALLABLE
			Endif
		Case INSTALLABLE
			If FileType( ident )=FILETYPE_DIR
				Return UPTODATE
			Endif
		Case INSTALLED
			If out.Trim()
				Return UPDATEABLE
			End
			Return UPTODATE
		Case UPDATEABLE
			Return UPTODATE
		End
		Return UNKNOWN
	End

End

Class MyApp Extends App Implements IViewListener

	Field mleft:Bool
	Field mousex:Int
	Field mousey:Int
	
	Field window:Window
	Field start:Button
	Field selected:Label
	
	Field busy:Bool
	Field mods:=New Stack<Repos>
	
	Method UpdateMods:Void()
	
		If Not busy Return
	
		busy=False

		For Local m:=Eachin mods
			If Not m.outfile Continue
			
			Local out:=os.LoadString( m.outfile )
			If out
				DeleteFile m.outfile
				If FileType( m.outfile )<>FILETYPE_NONE out=""
			Endif
			If Not out
				busy=True
				Continue
			Endif
			m.outfile=""
			
			m.EndUpdate out
		Next

	End
	
	Method OnCreate()
	
		AppDir=CurrentDir()
		
		Print "AppDir="+AppDir
		
		While FileType( "modules" )<>FILETYPE_DIR
			ChangeDir ".."
		Wend
		
		MonkeyDir=CurrentDir()
		
		Local modpathcfg:=LoadModpath()[1..-1]
		
		For Local path:=Eachin modpathcfg.Split( ";" )
			If StripDir( path )="modules_ext"
				ModulesDir=path
				Exit
			Endif
		Next
		If Not ModulesDir
			Error "modules_ext not found in MODPATH"
		Endif
		
		ChangeDir AppDir
				
		Select HostOS
		Case "macos"
			SetEnv "PATH",GetEnv( "PATH" )+":/usr/local/bin"
		End
		
		Local hg_version:=Process.Execute( "hg --version" )
		Local git_version:=Process.Execute( "git --version" )

		Local hg_avail:=hg_version.StartsWith( "Mercurial " )
		Local git_avail:=git_version.StartsWith( "git " )

		Print hg_version
		Print git_version
		
		Local repos_list:=mojo.LoadString( "monkey://data/repos_list.txt" )
		
		For Local line:=Eachin repos_list.Split( "~n" )
			line=line.Trim()
			If Not line Or line.StartsWith( "'" ) Continue
			Local bits:=line.Split( "," )
			If bits.Length<>5 Continue
			Select bits[0]
			Case "hg"
				If Not hg_avail Continue
			Case "git"
				If Not git_avail Continue
			Default
				Continue
			End
			mods.Push New Repos( bits[0],bits[1],bits[2],bits[3],bits[4] )
		Next

		For Local m:=Eachin mods
			Select m.state
			Case Repos.INSTALLED,Repos.UNINSTALLED
				m.BeginUpdate
			End
		Next
		busy=True
		
		'create module list
		Local list:=New GridView( 4,mods.Length+1 )
		list.SetView 0,0,New Label( "Module" )
		list.SetView 1,0,New Label( "Description" )
		list.SetView 2,0,New Label( "Status             " )
		selected=New Label( "Selected" )
		selected.AddListener Self
		list.SetView 3,0,selected
		list.GetView( 0,0 ).Alignment=Alignment.Left
		list.GetView( 1,0 ).Alignment=Alignment.Left
		list.GetView( 2,0 ).Alignment=Alignment.Left
		list.GetView( 3,0 ).Alignment=Alignment.Center
		list.GetView( 0,0 ).Color=[1.0,0.01,0.01,1.0]
		list.GetView( 1,0 ).Color=[1.0,0.01,0.01,1.0]
		list.GetView( 2,0 ).Color=[1.0,0.01,0.01,1.0]
		list.GetView( 3,0 ).Color=[1.0,0.01,0.01,1.0]
		For Local j:=0 Until mods.Length
			Local m:=mods.Get( j )
			list.SetView 0,j+1,New Label( m.name )
			list.SetView 1,j+1,New Label( m.desc )
			list.SetView 2,j+1,m.label	'New Label( m.state )
			list.SetView 3,j+1,m.check'New CheckBox( "" )
			list.GetView( 0,j+1 ).Alignment=Alignment.Left
			list.GetView( 1,j+1 ).Alignment=Alignment.CenterY
			list.GetView( 2,j+1 ).Alignment=Alignment.Left
			list.GetView( 3,j+1 ).Alignment=Alignment.Center
		Next
		Local scroller:=New ScrollView( list )
		scroller.Alignment=Alignment.FillTop
		
		start=New Button( "Update/Install selected" )
		start.Alignment=Alignment.Bottom
		start.IsEnabled=False
		start.AddListener Self
		
		'create panel
		Local panel:=New GridView( 1,5 )
		panel.Padding=8
		panel.SetView 0,0,New Label( "Monkey Module Manager" )
		panel.GetView( 0,0 ).Font=panel.GetView( 0,0 ).Skin.BigFont
		panel.GetView( 0,0 ).Color=[.01,.5,1.0,1.0]
		panel.SetView 0,1,New Divider( Alignment.FillTop )
		panel.SetView 0,2,scroller
		panel.SetView 0,3,New Spacer( Alignment.Fill )
		panel.SetView 0,4,start
		
		window=New Window( panel )

		window.SetShape 0,0,DeviceWidth,DeviceHeight
		
		SetUpdateRate 10
	End
	
	Method OnUpdate()
	
		If busy
			busy=False
			For Local m:=Eachin mods
				If Not m.proc Continue
				
				If m.ProcBusy()
					busy=True
					Continue
				Endif

				m.EndUpdate
			Next
			If Not busy
				start.IsEnabled=True
			Endif
		Endif
			
		Local mevent:=0
		
		If MouseHit( MOUSE_LEFT ) And Not mleft
			mleft=True
			mevent=MouseEvent.LeftButtonDown
		Else If mleft And Not MouseDown( MOUSE_LEFT )
			mleft=False
			mevent=MouseEvent.LeftButtonUp
		Else If MouseX<>mousex Or MouseY<>mousex
			mevent=MouseEvent.Movement
		Endif
		
		If mevent
			mousex=MouseX
			mousey=MouseY
			window.SendMouseEvent mevent,mousex,mousey
		Endif
		
		window.SetShape 0,0,DeviceWidth,DeviceHeight

	End

	Method OnRender()
	
		Cls
		
		Local gc:=New GraphicsContext
		
		gc.Reset
		
		window.Render gc

	End
	
	Method OnClose()
		EndApp
	End
	
	Method OnSignal:Void( signal:Int,view:View )
		Select view
		Case start
			For Local m:=Eachin mods
				If Not m.check.IsChecked Continue
				Select m.state
				Case Repos.INSTALLABLE,Repos.UPDATEABLE
					m.BeginUpdate
				End
			Next
			start.IsEnabled=False
			busy=True
		Case selected
			For Local m:=Eachin mods
				If m.check.IsEnabled m.check.IsChecked=Not m.check.IsChecked
			Next
		End
	End
End

Function Main()
	New MyApp
End
