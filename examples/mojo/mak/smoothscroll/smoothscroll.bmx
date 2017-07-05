
Global DeviceWidth=640
Global DeviceHeight=480

SetGraphicsDriver GLMax2DDriver()
Graphics DeviceWidth,DeviceHeight

Local scroll_x#=0
Local scroll_mod#=DeviceWidth*3

While Not KeyHit( KEY_ESC )

	'update
	scroll_x=(scroll_x+2) Mod scroll_mod

	'render
	SeedRnd 1234
	Cls 
	For Local i=1 To 100
		Local x#=Rnd( scroll_mod )
		Local y#=Rnd( DeviceHeight )
		Local w#=Rnd( 256 )
		Local h#=Rnd( 256 )
		SetColor Rnd( 256 ),Rnd( 256 ),Rnd( 256 )
		DrawRect x-scroll_x,y,w,h
		If x<DeviceWidth DrawRect x+scroll_mod-scroll_x,y,w,h
		If x+w>scroll_mod DrawRect x-scroll_mod-scroll_x,y,w,h
	Next
	SetColor 0,255,0
	DrawText scroll_x,0,0
	
	Flip 1
	
Wend
