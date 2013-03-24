
Import mojo
Import monkey.math

' The overall Game object, handling loading, mouse position, high-level game control and rendering...

Class point
	Field x:Float,y:Float
End

Function MidHandle (image:Image)
	image.SetHandle image.Width () * 0.5, image.Height () * 0.5
End

Class BlobMonster
	'x and y coords
	Field x:Float, y:Float
	
	'speed, try changing it
	Field speed:Float = 1
	
	'number of nodes along the body, try changing it to 100
	Field segments:Float = 10
	
	'array to hold the points along the body
	Field tail:point[10]
	Field time:Float = 0

	Field blob:Image

	Method New(inx:Float,iny:Float,inimage:Image)
		x = inx
		y = iny
		'give the tail some coordinates, just make them the same as the main x and y for now
		For Local i:Int = 0 To segments - 1
			tail[i] = New point
			tail[i].x = inx
			tail[i].y = iny
		Next
		blob=inimage
	End

	Method Update:Int()
		'time is a bit misleading, it's used for all sorts of things
		time+=speed
		
		'here the x and y coordinates are updated.
		'this uses the following as a basic rule for moving things
		'around a point in 2d space:
		'x=radius*cos(angle)+xOrigin
		'y=raduis*sin(angle)+yOrigin
		'this basically is the basis for anything that moves in this example
		'
		'the 2 lines of code below make the monster move around, but
		'you can change this to anything you like, try setting x and y to the mouse
		'coordinates for example
		y = (15 * Cos(time * -6)) + (DeviceHeight/2 + (180 * Sin(time * 1.3)))
		x = (15 * Sin(time * -6)) + (DeviceWidth/2 + (200 * Cos(time / 1.5)))
		
		'put the head of the tail at x,y coords
		tail[0].x = x
		tail[0].y = y
	
		'update the tail
		'basically, the points don't move unless they're further that 7 pixels 
		'from the previous point. this gives the kind of springy effect as the 
		'body stretches
		For Local i:Int = 1 To segments - 1
   			'calculate distance between the current point and the previous
		    	Local distX:Float = (tail[i - 1].x - tail[i].x)
        		Local distY:Float = (tail[i - 1].y - tail[i].y)
			Local dist:Float = Sqrt(distX * distX + distY * distY)
      		'move if too far away
         		If dist > 7 Then
				'the (distX*0.2) bit makes the point move 
				'just 20% of the distance. this makes the 
				'movement smoother, and the point decelerate
				'as it gets closer to the target point.
				'try changing it to 1 (i.e 100%) to see what happens
				tail[i].x = tail[i].x + (distX * (0.3))
            		tail[i].y = tail[i].y + (distY * (0.3))
         		Endif
			
		Next
	
		Return False
	End Method

	Method Draw()
		'time to draw stuff!
		
		'this sets the blend mode to LIGHTBLEND, or additive blending, which makes
		'the images progressively more bright as they overlap
		SetBlend 1
		'###########
		'draw the main bit of the body
		'start by setting the images handle (i.e the origin of the image) to it's center
		blob.SetHandle(blob.Width()*0.5,blob.Height()*0.5)
		
		
		'begin looping through the segments of the body
		For Local i:Int = 0 To segments - 1
			'set the alpha transparency vaue to 0.15, pretty transparent
			SetAlpha 0.15
			'the  (0.5*sin(i*35)) bit basically bulges the size of the images being
			'drawn as it gets closer to the center of the monsters body, and tapers off in size as it gets 
			'to the end. try changing the 0.5 to a higher number to see the effect.
			
			'draw the image
			DrawImage blob, tail[i].x, tail[i].y,0,1 + (0.5 * Sin(i * 35)), 1 + (0.5 * Sin(i * 35))
			
			'this next chunk just draws smaller dots in the center of each segment of the body
			SetAlpha 0.8
			
			DrawImage blob, tail[i].x, tail[i].y,0,0.1, 0.1
		Next
		
		'#########################
		'draw little spikes on tail
		'note that the x and y scales are different
		
		'move the image handle to halfway down the left edge, this'll make the image
		'appear to the side of the coordinate it is drawn too, rather than the 
		'center as we had for the body sections
		blob.SetHandle 0,blob.Height()*0.5
		
		'rotate the 1st tail image. basically, we're calculating the angle between
		'the last 2 points of the tail, and then adding an extra wobble (the 10*sin(time*10) bit)
		'to make the pincer type effect.
		
		
		DrawImage blob, tail[segments - 1].x, tail[segments - 1].y,10 * Sin(time * 10) + -calculateAngle(tail[segments - 1].x, tail[segments - 1].y, tail[segments - 5].x, tail[segments - 5].y) + 270,0.6, 0.1
		
		'second tail image uses negative time to make it move in the opposite direction
		
		DrawImage blob, tail[segments - 1].x, tail[segments - 1].y,10 * Sin(-time * 10) + -calculateAngle(tail[segments - 1].x, tail[segments - 1].y, tail[segments - 5].x, tail[segments - 5].y) + 270,0.6, 0.1

		
		
		'#####################
		'draw little fins/arms
		SetAlpha 1
		
		'begin looping through the body sections again. Note that we don't want fins
		'on the first and last section because we want other things at those coords.
		For Local i:Int = 1 To segments - 2
			'like the bulging body, we want the fins to grow larger in the center, and smaller
			'at the end, so the same sort of thing is used here.
			
			
			'rotate the image. We want the fins to stick out sideways from the body (the calculateangle() bit)
			'and also to move a little on their own. the 33 * Sin(time * 5 + i * 30) makes the 
			'fin rotate based in the i index variable, so that all the fins look like they're moving 
			'one after the other.
			DrawImage blob, tail[i].x, tail[i].y,33 * Sin(time * 5 + i * 30) + -calculateAngle(tail[i].x, tail[i].y, tail[i - 1].x, tail[i - 1].y),0.1 + (0.6 * Sin(i * 30)), 0.05
			
			
			'rotate the opposte fin, note that the signs have changes (-time and -i*30)
			'to reflect the rotations of the other fin
			DrawImage blob, tail[i].x, tail[i].y,33 * Sin(-time * 5 - i * 30) + -calculateAngle(tail[i].x, tail[i].y, tail[i - 1].x, tail[i - 1].y) + 180,0.1 + (0.6 * Sin(i * 30)), 0.05
			
			
		Next
		
		
		'###################
		'center the image handle
		MidHandle blob
		'Draw the eyes. These are just at 90 degrees to the head of the tail.
		SetAlpha 0.1
		Local ang:Float = calculateAngle(tail[0].x, tail[0].y, tail[1].x, tail[1].y)
		DrawImage blob, x + (7 * Cos(ang + 50)), y + (7 * Sin(ang + 50)),0,0.6, 0.6
		DrawImage blob, x + (7 * Cos(ang + 140)), y + (7 * Sin(ang + 140)),0,0.6, 0.6
		
		SetAlpha 0.5
		DrawImage blob, x + (7 * Cos(ang + 50)), y + (7 * Sin(ang + 50)),0,0.1, 0.1
		DrawImage blob, x + (7 * Cos(ang + 140)), y + (7 * Sin(ang + 140)),0,0.1, 0.1
	
		'draw beaky thing
		SetAlpha 0.8
		blob.SetHandle 0, blob.Height()*0.5
		
		DrawImage blob, x, y,-ang + 275,0.3, 0.1
		
		'yellow light
		MidHandle blob
		SetAlpha 0.2
		DrawImage blob, x, y,0,4, 4
		
		'Finished!
	End Method
End	

'This function calculates and returns the angle between two 2d coordinates
Function calculateAngle:Float(x1:Float,y1:Float,x2:Float,y2:Float)
	Local theX:Float=x1-x2
	Local theY:Float=y1-y2
	Local theAngle:Float=-ATan2(theX,theY)
	Return theAngle
End Function

Class blob_monster Extends App

	Field blobimage:Image
	Field theMonster:BlobMonster
	
	 ' Stuff to do on startup...
	Method OnCreate ()
		
		blobimage=LoadImage( "blob.png" )
		
		theMonster=New BlobMonster(10,10,blobimage)
		
		SetUpdateRate 60

	End

	' Stuff to do while running...
	Method OnUpdate ()
	
		theMonster.Update()
	End

	' Drawing code...
	Method OnRender ()
		Cls(0, 0, 0)
		
		theMonster.Draw()
		
	End

End

' Here we go!

Function Main ()
	New blob_monster								' RocketGame extends App, so monkey will handle running from here...
End