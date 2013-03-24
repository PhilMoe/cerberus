Strict

#ANDROID_SCREEN_ORIENTATION="landscape"

Import mojo


Function Main:Int()
	New PicPuzzle							
	Return 0
End


' Simple picture puzzle game
' Gerry Quinn 18 August 2012
Class PicPuzzle Extends App
		
	Field rightBar:Image					' right-side of game screen has four virtual buttons
	
	Field pictures:Image[]					' a set of 480x480 images (padded to power of two is best, some devices want that)
	
	Field picture:Image						' the current image
	
	Field validDivisions:Int[]				' options for divisions 
	
	Field divisionChoice:Int				' which option is selected
		
	Field positions:Int[][][]				' stores coords of the image section shown at every screen position
	
	Field xSelect:Int						' selected piece (will be swapped on next click)
	Field ySelect:Int						' xSelect is -1 for no selection

	
	Method OnCreate:Int()
		
		rightBar = LoadImage( "rightbar.png" )
		
		' Load all images with names pic000.jpg, pic001.jpg etc.
		Local pictureList:List< Image > = New List< Image >
		For Local count:Int = 0 Until 1000
			Const ZERO:Int = 48
			Local chars:Int[] = New Int[ 3 ]
			Local val:Int = count
			chars[ 2 ] = ZERO + val Mod 10
			val /= 10
			chars[ 1 ] = ZERO + val Mod 10
			val /= 10
			chars[ 0 ] = ZERO + val Mod 10
			Local name:String = "pic" + String.FromChars( chars ) + ".jpg"
			Local pic:Image = LoadImage( name )
			If pic <> Null
				pictureList.AddLast( pic )
			Else
				Exit
			End
		Next
		pictures = pictureList.ToArray()		
		If pictures.Length() < 1
			Error( "No pictures found!" )
		End
		
		picture = pictures[ 0 ]
		
		validDivisions = [ 3, 4, 5, 6, 8, 10, 12 ]
		
		divisionChoice = 1
				
		NewGame( False )
					
		SetUpdateRate( 30 )
		Return 0
	End
	
	
	Method OnUpdate:Int()

		If MouseHit( MOUSE_LEFT ) > 0
			Local xMouse:Int = MouseX()
			Local yMouse:Int = MouseY()
			If xMouse >= 480
				If yMouse >= 96
					Local button:Int = ( yMouse - 96 ) / 96
					Select button
					Case 0					
						NewPic()
						NewGame()
					Case 1					
						If divisionChoice > 0
							divisionChoice -= 1
							NewGame()
						End
					Case 2
						If divisionChoice < validDivisions.Length() - 1
							divisionChoice += 1
							NewGame()
						End
					Case 3
						NewGame( False )
					End Select 
				End
			Else
				Local divisions:Int = validDivisions[ divisionChoice ]
 				Local wBit:Int = 480 / divisions
				Local hBit:Int = 480 / divisions
				Local xBit:Int = xMouse / wBit
				Local yBit:Int = yMouse / hBit
				If xBit >= 0 And xBit < divisions
					If yBit >= 0 And yBit < divisions
						If positions[ xBit ][ yBit ][ 0 ] <> xBit Or positions[ xBit ][ yBit ][ 1 ] <> yBit
							If xSelect < 0
								xSelect = xBit 
								ySelect = yBit
							ElseIf xBit = xSelect And yBit = ySelect
								xSelect = -1
							Else
								SwapPositions( xBit, yBit, xSelect, ySelect )
								xSelect = -1
							End
						End
					End
				End
			End
		End
		
		Return 0
	End
	
	
	' Assumes 640 x 480 device size
	Method OnRender:Int()		
		DrawPicture()
		DrawImageRect( rightBar, 480, 0, 0, 0, 160, 480 )
		Return 0
	End
	
	
	' Select a new picture
	Method NewPic:Void()
		Local nPics:Int = pictures.Length()
		If nPics = 1
			Return
		End
		Local newPic:Image
		Repeat
			newPic = pictures[ Int( Rnd( nPics ) ) ]	
		Until newPic <> picture
		picture = newPic
	End
		
	
	' Generate a new puzzle based on current picture
	Method NewGame:Void( randomise:Bool = True )
	
		' Consider using RealMillisecs() from diddy module at start to set random seed
		Seed += Millisecs()
				
		Local divisions:Int = validDivisions[ divisionChoice ]
		positions = New Int[ divisions ][][]
		For Local x:Int = 0 Until divisions
			positions[ x ] = New Int[ divisions ][]
			For Local y:Int = 0 Until divisions
				positions[ x ][ y ] = New Int[ 2 ]
				positions[ x ][ y ][ 0 ] = x
				positions[ x ][ y ][ 1 ] = y
			Next
		Next
		xSelect = -1
		
		If randomise
			For Local x:Int = 0 until divisions
				For Local y:Int = 0 Until divisions
					Repeat
						' Ensure that no piece is ever swapped to the correct position
						Local x2:Int = Int( Rnd( divisions ) )
						Local y2:Int = Int( Rnd( divisions ) )
						If positions[ x2 ][ y2 ][ 0 ] = x And positions[ x2 ][ y2 ][ 1 ] = y
							Continue
						End
						If positions[ x ][ y ][ 0 ] = x2 And positions[ x ][ y ][ 1 ] = y2
							Continue
						End
						SwapPositions( x, y, x2, y2 )
						Exit
					Forever						
				Next
			Next
		End
	End
	
	
	' Swap positions of two pieces
	Method SwapPositions:Void( x1:Int, y1:Int, x2:Int, y2:Int )
		Local xTmp:Int = positions[ x2 ][ y2 ][ 0 ]
		Local yTmp:Int = positions[ x2 ][ y2 ][ 1 ]
		positions[ x2 ][ y2 ][ 0 ] = positions[ x1 ][ y1 ][ 0 ]
		positions[ x2 ][ y2 ][ 1 ] = positions[ x1 ][ y1 ][ 1 ]
		positions[ x1 ][ y1 ][ 0 ] = xTmp
		positions[ x1 ][ y1 ][ 1 ] = yTmp
	End
	
	
	' Draw picture
	Method DrawPicture:Void()
		SetColor( 255, 255, 255 )
		Local divisions:Int = validDivisions[ divisionChoice ]
		Local wBit:Int = 480 / divisions
		Local hBit:Int = 480 / divisions
		For Local x:Int = 0 Until divisions
			For Local y:Int = 0 Until divisions
				Local xSrc:Int = positions[ x ][ y ][ 0 ]
				Local ySrc:Int = positions[ x ][ y ][ 1 ] 
				DrawImageRect( picture, x * wBit, y * hBit, xSrc * wBit, ySrc * hBit, wBit, hBit )
				
				' Simple colour overlay to distinguish bits that are wrong or selected
				' Room for improvement here
				Local correct:Bool = x = xSrc And y = ySrc
				If Not correct
					SetAlpha( 0.3 )
					If x = xSelect And y = ySelect
						SetColor( 0, 255, 0 )
					Else
						SetColor( 255, 0, 0 )
					End
					DrawRect( x * wBit, y * hBit, wBit, hBit )
					SetColor( 255, 255, 255 )
					SetAlpha( 1.0 )
				End					
			Next
		Next		
	End
	
End



