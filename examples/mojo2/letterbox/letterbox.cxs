
'Run in glfw3 and resize window!

Import mojo2

#GLFW_WINDOW_WIDTH=800
#GLFW_WINDOW_HEIGHT=400
#GLFW_WINDOW_RESIZABLE=True
#GLFW_WINDOW_RENDER_WHILE_RESIZING=True

Const VWIDTH:=320
Const VHEIGHT:=240

Class MyApp Extends App

	Field canvas:Canvas
	Field splitScreen:Bool
		
	Method OnCreate()
	
		canvas=New Canvas
		
	End
	
	Method OnUpdate()
	
		If KeyHit( KEY_SPACE ) splitScreen=Not splitScreen
		
	end
	
	Method CalcLetterbox:Void( vwidth:Float,vheight:Float,devrect:Int[],vprect:Int[] )
	
		Local vaspect:=vwidth/vheight
		Local daspect:=Float(devrect[2])/devrect[3]

		If daspect>vaspect
			vprect[2]=devrect[3]*vaspect
			vprect[3]=devrect[3]
			vprect[0]=(devrect[2]-vprect[2])/2+devrect[0]
			vprect[1]=devrect[1]
		Else
			vprect[2]=devrect[2]
			vprect[3]=devrect[2]/vaspect
			vprect[0]=devrect[0]
			vprect[1]=(devrect[3]-vprect[3])/2+devrect[1]
		Endif
	
	End
	
	Method RenderScene:Void( msg:String,devrect:Int[] )
	
		Local vprect:Int[4]
			
		CalcLetterbox( VWIDTH,VHEIGHT,devrect,vprect )

		canvas.SetViewport vprect[0],vprect[1],vprect[2],vprect[3]
		
		canvas.SetProjection2d 0,VWIDTH,0,VHEIGHT

		canvas.Clear 0,0,1
		
		canvas.DrawText msg,VWIDTH/2,VHEIGHT/2,.5,.5
		
	End
	
	Method OnRender()
	
		canvas.SetViewport 0,0,DeviceWidth,DeviceHeight

		canvas.Clear 0,0,0
	
		If splitScreen
		
			Local h:=DeviceHeight/2

			RenderScene( "PLAYER 1 READY",[0,0,DeviceWidth,h] )
		
			RenderScene( "PLAYER 2 READY",[0,h,DeviceWidth,h] )
		
		Else
		
			RenderScene( "SPACE TO TOGGLE SPLITSCREEN",[0,0,DeviceWidth,DeviceHeight] )

		Endif
		
		canvas.Flush
	End
	
End

Function Main()
	New MyApp
End
