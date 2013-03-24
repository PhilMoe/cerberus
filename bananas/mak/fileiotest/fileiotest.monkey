
Import mojo
Import brl

Class MyApp Extends App

	Field data:DataBuffer
	Field data2:DataBuffer
	Field image:Image

	Method OnCreate()
	
		data=DataBuffer.Load( "monkey://data/test.dat" )
		If data
#If LANG<>"js" And LANG<>"as"		
			Local file:=FileStream.Open( "monkey://internal/test.png","w" )
			If file
				file.Write data,0,data.Length
				file.Close
				data2=DataBuffer.Load( "monkey://internal/test.png" )
				image=LoadImage( "monkey://internal/test.png",1,Image.MidHandle )
			Endif
#endif			
		Endif

		SetUpdateRate 60
	End
	
	Method OnUpdate()
	End
	
	Method OnRender()
		Cls 128,0,255
		DrawText "Hello World!",0,0
		If data DrawText "data.Length="+data.Length,0,12
		If data2 DrawText "data2.Length="+data.Length,0,24
		If image DrawImage image,DeviceWidth/2,DeviceHeight/2
	End
End

Function Main()

	New MyApp
End
