'******************************************************************
'* Diffusion Example
'* Author: Richard R Betson
'* Date: 02/20/11
'* Language: Monkey
'* Target: GLFW							  
'******************************************************************
' Linsence - Public Domain
'******************************************************************


Import monkey
Import mojo


Global flag=0
Global clra:Int


Function Main()
	New Diffusion
End Function


Class Diffusion Extends App


Global fps,fp,fps_t
Global mapx:Int[77900]
Global mapx1:Int[77900]
Global mapx2:Int[77900]
Global mapx3:Int[77900]
Global mapx4:Int[77900]

Global clrr:Int[77900]
Global clrb:Int[77900]

Global clrr1:Int[77900]
Global clrb1:Int[77900]

Global ii
Global fxsel_ttl
Global fxsel

	Method OnCreate()
		fxsel_ttl=Millisecs()+9000
		SetUpdateRate(30)
		SetLookUp()
	End Method
	
	Method OnUpdate()
		If fxsel_ttl<Millisecs()
			fxsel=fxsel+1
			fxsel_ttl=Millisecs()+9000
			If fxsel>4 Then fxsel=0
		Endif 
	
		ii=ii+1
		Local x2=(160 + (Sin(ii)*90) )
		Local y2=(120 + (Cos(ii)*90) )
		Local x3=(160 + (-Sin(ii)*90) )
		Local y3=(120 + (-Cos(ii)*90) )
		
		'Draw Line
		LineB(160,120,x2,y2)
		LineB(160,120,x3,y3)
		
		For Local zz=0 To 100
			Local xz=Int(Rnd(50))+135
			Local yz=Int(Rnd(50))+95
			clrb1[ ( 320 *yz ) +xz ]=255
			If zz>60 Then clrr1[ ( 320 *yz ) +xz ]=200

		Next

		For Local y=1 To 240 
			For Local x= 1 To 320 
				'Apply mapping via lookup table
				If fxsel=0
				clrr[ ( 320 *y ) +x ] = clrr1[ mapx[( 320 *y ) +x] ]
				clrb[ ( 320 *y ) +x ] = clrb1[ mapx[( 320 *y ) +x] ]
				Else If fxsel=1
				clrr[ ( 320 *y ) +x ] = clrr1[ mapx1[( 320 *y ) +x] ]
				clrb[ ( 320 *y ) +x ] = clrb1[ mapx1[( 320 *y ) +x] ]
				Else If fxsel=2
				clrr[ ( 320 *y ) +x ] = clrr1[ mapx2[( 320 *y ) +x] ]
				clrb[ ( 320 *y ) +x ] = clrb1[ mapx2[( 320 *y ) +x] ]
				Else If fxsel=3
				clrr[ ( 320 *y ) +x ] = clrr1[ mapx3[( 320 *y ) +x] ]
				clrb[ ( 320 *y ) +x ] = clrb1[ mapx3[( 320 *y ) +x] ]
				Else If fxsel=4
				clrr[ ( 320 *y ) +x ] = clrr1[ mapx4[( 320 *y ) +x] ]
				clrb[ ( 320 *y ) +x ] = clrb1[ mapx4[( 320 *y ) +x] ]

				Endif
			Next
		Next

	End Method
	
	Method OnRender()
		fps=fps+1
		If fps_t<Millisecs()
		fp=(fps)
		fps_t=1000+Millisecs()
		fps=0
		Endif

		PushMatrix()

		For Local y=1 To 240
			For Local x= 1 To 320
				'Blur Image and Fade Color
				Local r=clrr[ ( 320 *y ) + x   ]
				Local sumr = ((r *4) + clrr[ ( 320 *y ) + x+1   ] + clrr[ ( 320 *y ) + x-1   ] + clrr[ ( 320 *y-1 ) + x   ] + clrr[ ( 320 *y+1 ) + x   ] ) /8  
				Local b=clrb[ ( 320 *y ) + x   ]
				Local sumb = ((b *4) + clrb[ ( 320 *y ) + x+1   ] + clrb[ ( 320 *y ) + x-1   ] + clrb[ ( 320 *y-1 ) + x   ] + clrb[ ( 320 *y+1 ) + x   ] ) /8  
				Local sumg,g
				r=sumr-4
				b=sumb-4

				If r<10 Then r=10
				If r>255 Then r=255
				If b<0 Then b=0
				If b>255 Then b=255

				clrr1[ ( 320 *y ) + x ]=r
				clrb1[ ( 320 *y ) + x ]=b

				SetColor(r,0,b)
				DrawRect(x*2,y*2,2,2)
			Next
		Next
		
		'Uncomment to see FPS
		'SetColor(255,255,255)				
		'DrawText(fp,10,10)

		PopMatrix()
	End Method

	Function LineB(x1,y1,x2,y2)
		'Ported
		'Bresenham Line Algorithm 
		'Source - GameDev.Net - Mark Feldman
		'Public Domain
		
		Local deltax = Abs(x2 - x1)
		Local deltay = Abs(y2 - y1) 
		
		Local numpixels,d,dinc1,dinc2,xinc1,xinc2,yinc1,yinc2,x,y,i
		
		If deltax >= deltay 
			 numpixels = deltax + 1
			d = (2 * deltay) - deltax
			dinc1 = deltay Shl 1
			dinc2 = (deltay - deltax) Shl 1
			xinc1 = 1
			xinc2 = 1
			yinc1 = 0
			yinc2 = 1
		Else 
			 numpixels = deltay + 1
			d = (2 * deltax) - deltay
			dinc1 = deltax Shl 1
			dinc2 = (deltax - deltay) Shl 1
			xinc1 = 0
			xinc2 = 1
			yinc1 = 1
			yinc2 = 1
		Endif
		
		If x1 > x2
			xinc1 = -xinc1
			xinc2 = -xinc2
		Endif
		
		If y1 > y2 
			yinc1 = -yinc1
			yinc2 = -yinc2
		
		Endif
		
		x = x1
		y = y1
		
		For i = 1 To numpixels 
			
			If d < 0 
				d = d + dinc1
				x = x + xinc1
				y = y + yinc1
			Else
				d = d + dinc2
				x = x + xinc2
				y = y + yinc2
			Endif
			
			'Draw line 
			If x>0 And x<320 And y>0 And y<240
				clrr1[ (( 320 *y )+x) ]=255
			Endif
			
		
		Next

	End Function

	
	Method SetLookUp()
	Local ang
	
	For Local lui=0 To 4
	
	ang=0
		For Local y=1 To 240
		
			For Local x= 1 To 320
				ang=0
				Local rad#=Abs((x-160)*(x-160)) + Abs( (y-120)  * (y-120) )
				If rad>0
					rad= Sqrt(rad)
				Else
					rad=0
				Endif
				If rad=0 Then rad=1

				Local dx#=((x-160)/(rad))
				Local dy#=((y-120)/(rad))
				If lui=0
					'Parabolic
					rad= 1-( ( rad*.93 - ( Cos(-rad)*(Sin(rad*22000)) )) * (-Cos(rad*.5) ) )
		
					dx=(dx*Cos(ang) - dy*Sin(ang))
					dy=(dy*Cos(ang) + dx*Sin(ang))
		
					Local x1=Int(dx*rad)+(160)
					Local y1=Int(dy*rad)+(120)
		
					If y1<1 Then y1=1
					If y1>240 Then y1=240
					If x1<1 Then x1=1
					If x1>320 Then x1=320
					
					mapx[ ( 320 *y ) +x ] = ( 320 *y1 ) +x1'Int(x1)
				
				Else If lui=1
					'Free Float
					ang=1
					rad= 1-( ( rad- (Sin(rad*900)*-Cos(rad*900)*.5 )*3.1415926 ) *  (-Cos((3.1415926) ) )*.9  )
					Local x1=Int(dx*rad)+(160)
					Local y1=Int(dy*rad)+(120)
					x1= ( x1*Cos(ang) - y1*Sin(ang) )
					y1= ( x1*Sin(ang) + y1*Cos(ang) )
		
					If y1<1 Then y1=1
					If y1>240 Then y1=240
					If x1<1 Then x1=1
					If x1>320 Then x1=320
					
					mapx2[ ( 320 *y ) +x ] = ( 320 *y1 ) +x1'Int(x1)

				Else If lui=2
					'Burst
					rad= 2-( ( rad- (Sin(rad*4000)*(-Cos(rad*2000)*.9) )*3.1415926 ) *  (-Cos((3.1415926) ) )*.8  )
					dx=(dx*Cos(ang) - dy*Sin(ang))
					dy=(dy*Cos(ang) + dx*Sin(ang))
		
					Local x1=Int(dx*rad)+(160)
					Local y1=Int(dy*rad)+(120)
		
					If y1<1 Then y1=1
					If y1>240 Then y1=240
					If x1<1 Then x1=1
					If x1>320 Then x1=320
					
					mapx1[ ( 320 *y ) +x ] = ( 320 *y1 ) +x1'Int(x1)
		
				Else If lui=3
					'Orb
					rad= 2-( ( rad- (Sin(rad)*(Cos(rad)*2.5) )*3.1415926 ) *  (-Cos((3.1415926) ))*.97  )
					dx=(dx*Cos(ang) - dy*Sin(ang))
					dy=(dy*Cos(ang) + dx*Sin(ang))
		
					Local x1=Int(dx*rad)+(160)
					Local y1=Int(dy*rad)+(120)
		
					If y1<1 Then y1=1
					If y1>240 Then y1=240
					If x1<1 Then x1=1
					If x1>320 Then x1=320
					
					mapx3[ ( 320 *y ) +x ] = ( 320 *y1 ) +x1'Int(x1)
			Else If lui=4
					'Swirl
					 ang=-3
					rad= 1-( ( rad- (Sin(rad)*-Cos(rad)*.9 )*3.1415926 ) *  (-Cos((3.1415926) ) )*.9  )
					dx=(dx*Cos(ang) - dy*Sin(ang))
					dy=(dy*Cos(ang) + dx*Sin(ang))
		
					Local x1=Int(dx*rad)+(160)
					Local y1=Int(dy*rad)+(120)
		
					If y1<1 Then y1=1
					If y1>240 Then y1=240
					If x1<1 Then x1=1
					If x1>320 Then x1=320
					
					mapx4[ ( 320 *y ) +x ] = ( 320 *y1 ) +x1'Int(x1)
			Endif

			
			Next
		Next
	Next
	
	End Method

End Class


