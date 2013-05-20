
Private

Import mojo

Class Char
	Field id:Int
	Field x:Int
	Field y:Int
	Field width:Int
	Field height:Int
	Field xoffset:Int
	Field yoffset:Int
	Field xadvance:Int
	Field yadvance:Int
	Field page:Int
	Field chnl:Int
End

Class Kern
	Field first:Int
	Field second:Int
	Field amount:Int
End

Public

Class Font

	Method New( path:String )
		Load path
	End
	
	Method New( name:String,size:Int )
		Load name+"_"+size+".fnt"
	End

	Method Width:Int( text:String )
		Local x:=0.0
		Local last:=-1
		For Local c:=Eachin text
			If c>=0 And c<256 And _chars[c]
				Local char:=_chars[c]
				Local xoffset:=char.xoffset
				If last<>-1 And _kerns[last]
					For Local k:=Eachin _kerns[last]
						If k.second<>c Continue
						xoffset+=k.amount
						Exit
					Next
				Endif
				x+=char.xadvance
				last=c
			Else
				last=-1
			Endif
		Next
		Return Ceil( x )
	End

	Method Height:Int() Property
		Return _lineHeight
	End

	Private
	
	'common
	Field _base:Int
	Field _lineHeight:Int
	Field _chars:Char[256]
	Field _kerns:List<Kern>[256]
	Field _image:Image
	
	Method Load:Void( path:String )
	
		Local fnt:=LoadString( path )
		
		Local p:=New StringMap<String>
		
		For Local line:=Eachin fnt.Split( "~n" )
		
			line=line.Trim()
			Local i:=line.Find( " " )
			If i=-1 Continue
			Local tag:=line[..i]
			line=line[i+1..].Trim()
			
			p.Clear
			While line
				Local i:=line.Find( " " )
				If i=-1 i=line.Length
				Local bit:=line[..i]
				line=line[i+1..].Trim()
				Local j:=bit.Find( "=" )
				If j=-1 Exit
				p.Set bit[..j],bit[j+1..]
			Wend
			
			Select tag
			Case "common"
				_lineHeight=Int( p.Get( "lineHeight" ) )
				_base=Int( p.Get( "base" ) )
			Case "page"
				Local file:=p.Get( "file" )[1..-1]
				_image=New Image( file )
			Case "char"
				Local c:=New Char
				c.id=Int( p.Get( "id" ) )
				c.x=Int( p.Get( "x" ) )
				c.y=Int( p.Get( "y" ) )
				c.width=Int( p.Get( "width" ) )
				c.height=Int( p.Get( "height" ) )
				c.xoffset=Int( p.Get( "xoffset" ) )
				c.yoffset=Int( p.Get( "yoffset" ) )
				c.xadvance=Int( p.Get( "xadvance" ) )
				c.page=Int( p.Get( "page" ) )
				c.chnl=Int( p.Get( "chnl" ) )
				_chars[c.id]=c
			Case "kerning"
				Local k:=New Kern
				k.first=Int( p.Get( "first" ) )
				k.second=Int( p.Get( "second" ) )
				k.amount=Int( p.Get( "amount" ) )
				If Not _kerns[k.first] _kerns[k.first]=New List<Kern>
				_kerns[k.first].AddLast k
			End
		Next
	End
	
	Method DrawText:Void( gc:GraphicsContext,text:String,x:Float,y:Float )
		Local last:=-1
		For Local c:=Eachin text
			If c>=0 And c<256 And _chars[c]
				Local char:=_chars[c]
				Local xoffset:=char.xoffset
				If last<>-1 And _kerns[last]
					For Local k:=Eachin _kerns[last]
						If k.second<>c Continue
						xoffset+=k.amount
						Exit
					Next
				Endif
				gc.DrawImage _image,x+xoffset,y+char.yoffset,char.x,char.y,char.width,char.height
				x+=char.xadvance
				last=c
			Else
				last=-1
			Endif
		Next
	End
	
End

Class Image

	Method New( path:String )
		_path=path
		_image=mojo.LoadImage( path )
	End

	Method Width:Int() Property
		Return _image.Width
	End
	
	Method Height:Int() Property
		Return _image.Height
	End
	
	Private
	
	Field _path:String
	Field _image:mojo.Image

End

Class GraphicsContext

	Method Reset:Void()
		Color=[1.0,1.0,1.0,1.0]
		Matrix=[1.0,0.0,0.0,1.0,0.0,0.0]
		Scissor=[0.0,0.0,Float(mojo.DeviceWidth),Float(mojo.DeviceHeight)]
	End
	
	Method Font:Void( font:Font ) Property
		_font=font
	End
	
	Method Font:Font() Property
		Return _font
	End
	
	Method Color:Void( color:Float[] ) Property
		_color=color
		mojo.SetColor _color[0]*255,_color[1]*255,_color[2]*255
		mojo.SetAlpha _color[3]
	End
	
	Method Color:Float[]() Property
		Return _color
	End
	
	Method Matrix:Void( matrix:Float[] ) Property
		_matrix=matrix
		mojo.SetMatrix _matrix[0],_matrix[1],_matrix[2],_matrix[3],_matrix[4],_matrix[5]
	End
	
	Method Matrix:Float[]() Property
		Return _matrix
	End
	
	Method Scissor:Void( scissor:Float[] ) Property
		_scissor=scissor
		mojo.SetScissor _scissor[0],_scissor[1],_scissor[2],_scissor[3]
	End
	
	Method Scissor:Float[]() Property
		Return _scissor
	End
	
	Method DrawLine:Void( x:Float,y:Float,x2:Float,y2:Float )
		mojo.DrawLine x,y,x2,y2
	end
	
	Method DrawRect:Void( x:Float,y:Float,width:Float,height:Float,solid:Bool=True )
		If solid
			mojo.DrawRect x,y,width,height
		Else
			mojo.DrawRect x,y,width,1
			mojo.DrawRect x,y,1,height
			mojo.DrawRect x+width-1,y,1,height
			mojo.DrawRect x,y+height-1,width,1
		Endif
	End
	
	Method DrawImage:Void( image:Image,x:Float,y:Float )
		mojo.DrawImage image._image,x,y
	End
	
	Method DrawImage:Void( image:Image,x:Float,y:Float,srcx:Int,srcy:Int,srcw:Int,srch:Int )
		mojo.DrawImageRect image._image,x,y,srcx,srcy,srcw,srch
	End
	
	Method DrawText:Void( text:String,x:Float,y:Float )
		_font.DrawText Self,text,x,y
	End
	
	Private
	
	Field _font:Font
	Field _color:Float[]
	Field _matrix:Float[]
	Field _scissor:Float[]
End


