
'Simple drawlist rendering demo. Note: RenderDrawListEx is experimental!

Import mojo2

Class MyApp Extends App

	Field canvas:Canvas
	
	Field drawList:DrawList
	
	Method OnCreate()

		canvas=New Canvas
		
		drawList=New DrawList
		
		For Local i:=0 Until 100
		
			drawList.SetColor Rnd(),Rnd(),Rnd()
			
			drawList.DrawCircle Rnd(DeviceWidth)-DeviceWidth/2,Rnd(DeviceHeight)-DeviceHeight/2,Rnd(10,20)
		Next

	End
	
	Method OnRender()
	
		canvas.Clear 0,0,1

		canvas.RenderDrawList drawList,MouseX,MouseY,Millisecs*.01
	
		canvas.Flush
	End

End

Function Main()
	New MyApp
End
