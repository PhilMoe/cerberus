
// Flash mojo runtime.
//
// Copyright 2011 Mark Sibly, all rights reserved.
// No warranty implied; use at your own risk.

import flash.display.*;
import flash.events.*;
import flash.media.*;
import flash.geom.*;
import flash.utils.*;
import flash.net.*;

class gxtkGraphics{

	internal var game:BBFlashGame;
	internal var stage:Stage;

	internal var bitmap:Bitmap;
	
	internal var red:Number=255;
	internal var green:Number=255;
	internal var blue:Number=255;
	internal var alpha:Number=1;
	internal var colorARGB:uint=0xffffffff;
	internal var colorTform:ColorTransform=null;
	internal var alphaTform:ColorTransform=null;
	
	internal var matrix:Matrix;
	internal var rectBMData:BitmapData;
	internal var blend:String;
	internal var clipRect:Rectangle;
	
	internal var shape:Shape;
	internal var graphics:Graphics;
	internal var bitmapData:BitmapData;
	
	internal var graphics_dirty:Boolean;

	internal var pointMat:Matrix=new Matrix;
	internal var rectMat:Matrix=new Matrix;
	internal var imageMat:Matrix=new Matrix;
	internal var pointCoords:Point=new Point;
	
	internal var image_filtering_enabled:Boolean;
	
	function gxtkGraphics(){

		game=BBFlashGame.FlashGame();
		stage=game.GetDisplayObjectContainer().stage;
		
		image_filtering_enabled=(Config.MOJO_IMAGE_FILTERING_ENABLED=="1");
		
		bitmap=new Bitmap();
		bitmap.bitmapData=new BitmapData( stage.stageWidth,stage.stageHeight,false,0xff0000ff );
		bitmap.smoothing=image_filtering_enabled;
		bitmap.width=stage.stageWidth;
		bitmap.height=stage.stageHeight;
		
		game.GetDisplayObjectContainer().addChild( bitmap );

		shape=new Shape;
		graphics=shape.graphics;
		bitmapData=bitmap.bitmapData;
	
		stage.addEventListener( Event.RESIZE,OnResize );
	
		rectBMData=new BitmapData( 1,1,false,0xffffffff );
		
		image_filtering_enabled=(Config.MOJO_IMAGE_FILTERING_ENABLED=="1");
	}
	
	internal function OnResize( e:Event ):void{
		var w:int=stage.stageWidth;
		var h:int=stage.stageHeight;
		if( w==bitmap.width && h==bitmap.height ) return;
		bitmap.bitmapData=new BitmapData( w,h,false,0xff0000ff );
		bitmap.width=w;
		bitmap.height=h;
	}

	internal function BeginRender():int{
		return 1;
	}

	internal function UseGraphics():void{
		if( graphics_dirty && alphaTform ){
			bitmapData.draw( shape,matrix,alphaTform,blend,clipRect,false );
			graphics.clear();
			return;
		}
		graphics_dirty=true;
	}

	internal function FlushGraphics():void{
		if( graphics_dirty ){
			graphics_dirty=false;
			bitmapData.draw( shape,matrix,alphaTform,blend,clipRect,false );
			graphics.clear();
		}
	}
	
	internal function EndRender():void{
		FlushGraphics();
	}
	
	internal function DiscardGraphics():void{
	}
	
	internal function updateColor():void{
	
		colorARGB=(int(alpha*255)<<24)|(int(red)<<16)|(int(green)<<8)|int(blue);
		
		if( colorARGB==0xffffffff ){
			colorTform=null;
			alphaTform=null;
		}else{
			colorTform=new ColorTransform( red/255.0,green/255.0,blue/255.0,alpha );
			if( alpha==1 ){
				alphaTform=null;
			}else{
				alphaTform=new ColorTransform( 1,1,1,alpha );
			}
		}
	}

	//***** GXTK API *****

	public function Width():int{
		return bitmap.width;
	}

	public function Height():int{
		return bitmap.height;
	}
	
	public function LoadSurface__UNSAFE__( surface:gxtkSurface,path:String ):Boolean{
		return false;
	}

	public function LoadSurface( path:String ):gxtkSurface{
		var bitmap:Bitmap=game.LoadBitmap( path );
		if( bitmap==null ) return null;
		return new gxtkSurface( bitmap );
	}
	
	public function CreateSurface( width:int,height:int ):gxtkSurface{
		var bitmapData:BitmapData=new BitmapData( width,height,true,0 );
		var bitmap:Bitmap=new Bitmap( bitmapData );
		return new gxtkSurface( bitmap );
	}
	
	public function SetAlpha( a:Number ):int{
		FlushGraphics();

		alpha=a;

		updateColor();

		return 0;
	}
	
	public function SetColor( r:Number,g:Number,b:Number ):int{
		FlushGraphics();
		
		red=r;
		green=g;
		blue=b;
	
		updateColor();
		
		return 0;
	}
	
	public function SetBlend( blend:int ):int{
		switch( blend ){
		case 1:
			this.blend=BlendMode.ADD;
			break;
		default:
			this.blend=null;
		}
		return 0;
	}
	
	public function SetScissor( x:int,y:int,w:int,h:int ):int{
		FlushGraphics();
		
		if( x!=0 || y!=0 || w!=bitmap.width || h!=bitmap.height ){
			clipRect=new Rectangle( x,y,w,h );
		}else{
			clipRect=null;
		}
		return 0;
	}

	public function SetMatrix( ix:Number,iy:Number,jx:Number,jy:Number,tx:Number,ty:Number ):int{
		FlushGraphics();
		
		if( ix!=1 || iy!=0 || jx!=0 || jy!=1 || tx!=0 || ty!=0 ){
			matrix=new Matrix( ix,iy,jx,jy,tx,ty );
		}else{
			matrix=null;
		}
		return 0;
	}

	public function Cls( r:Number,g:Number,b:Number ):int{
		FlushGraphics();

		var clsColor:uint=0xff000000|(int(r)<<16)|(int(g)<<8)|int(b);
		var rect:Rectangle=clipRect;
		if( !rect ) rect=new Rectangle( 0,0,bitmap.width,bitmap.height );
		bitmapData.fillRect( rect,clsColor );
		return 0;
	}
	
	public function DrawPoint( x:Number,y:Number ):int{
		FlushGraphics();
		
		if( matrix ){
			var px:Number=x;
			x=px * matrix.a + y * matrix.c + matrix.tx;
			y=px * matrix.b + y * matrix.d + matrix.ty;
		}
		if( clipRect || alphaTform || blend ){
			pointMat.tx=x;pointMat.ty=y;
			bitmapData.draw( rectBMData,pointMat,colorTform,blend,clipRect,false );
		}else{
			bitmapData.fillRect( new Rectangle( x,y,1,1 ),colorARGB );
		}
		return 0;
	}
	
	
	public function DrawRect( x:Number,y:Number,w:Number,h:Number ):int{
		FlushGraphics();
		
		if( matrix ){
			var mat:Matrix=new Matrix( w,0,0,h,x,y );
			mat.concat( matrix );
			bitmapData.draw( rectBMData,mat,colorTform,blend,clipRect,false );
		}else if( clipRect || alphaTform || blend ){
			rectMat.a=w;rectMat.d=h;rectMat.tx=x;rectMat.ty=y;
			bitmapData.draw( rectBMData,rectMat,colorTform,blend,clipRect,false );
		}else{
			bitmapData.fillRect( new Rectangle( x,y,w,h ),colorARGB );
		}
		return 0;
	}

	public function DrawLine( x1:Number,y1:Number,x2:Number,y2:Number ):int{
		
		if( matrix ){
		
			FlushGraphics();
			
			var x1_t:Number=x1 * matrix.a + y1 * matrix.c + matrix.tx;
			var y1_t:Number=x1 * matrix.b + y1 * matrix.d + matrix.ty;
			var x2_t:Number=x2 * matrix.a + y2 * matrix.c + matrix.tx;
			var y2_t:Number=x2 * matrix.b + y2 * matrix.d + matrix.ty;
			
			graphics.lineStyle( 1,colorARGB & 0xffffff );	//why the mask?
			graphics.moveTo( x1_t,y1_t );
			graphics.lineTo( x2_t,y2_t );
			graphics.lineStyle();
			
			bitmapData.draw( shape,null,alphaTform,blend,clipRect,false );
			graphics.clear();
			
		}else{
		
			UseGraphics();

			graphics.lineStyle( 1,colorARGB & 0xffffff );	//why the mask?
			graphics.moveTo( x1,y1 );
			graphics.lineTo( x2,y2 );
			graphics.lineStyle();
		}

		return 0;
 	}

	public function DrawOval( x:Number,y:Number,w:Number,h:Number ):int{
		UseGraphics();

		graphics.beginFill( colorARGB & 0xffffff );			//why the mask?
		graphics.drawEllipse( x,y,w,h );
		graphics.endFill();

		return 0;
	}
	
	public function DrawPoly( verts:Array ):int{
		if( verts.length<2 ) return 0;
		
		UseGraphics();
		
		graphics.beginFill( colorARGB & 0xffffff );			//why the mask?
		
		graphics.moveTo( verts[0],verts[1] );
		for( var i:int=2;i<verts.length;i+=2 ){
			graphics.lineTo( verts[i],verts[i+1] );
		}
		graphics.endFill();
		
		return 0;
	}

	public function DrawPoly2( verts:Array,surface:gxtkSurface,srcx:int,srcy:int ):int{
		if( verts.length<4 ) return 0;
		
		UseGraphics();
		
		graphics.beginFill( colorARGB & 0xffffff );			//why the mask?
		
		graphics.moveTo( verts[0],verts[1] );
		for( var i:int=4;i<verts.length;i+=4 ){
			graphics.lineTo( verts[i],verts[i+1] );
		}
		graphics.endFill();
		
		return 0;
	}

	public function DrawSurface( surface:gxtkSurface,x:Number,y:Number ):int{
		FlushGraphics();
		
		if( matrix ){
			var mat:Matrix=new Matrix( 1,0,0,1,x,y );
			mat.concat( matrix );
			bitmapData.draw( surface.bitmap.bitmapData,mat,colorTform,blend,clipRect,image_filtering_enabled );
		}else if( clipRect || colorTform || blend ){
			imageMat.tx=x;imageMat.ty=y;
			bitmapData.draw( surface.bitmap.bitmapData,imageMat,colorTform,blend,clipRect,image_filtering_enabled );
		}else{
			pointCoords.x=x;pointCoords.y=y;
			bitmapData.copyPixels( surface.bitmap.bitmapData,surface.rect,pointCoords );
		}

		return 0;
	}

	public function DrawSurface2( surface:gxtkSurface,x:Number,y:Number,srcx:int,srcy:int,srcw:int,srch:int ):int{
		if( srcw<0 ){ srcx+=srcw;srcw=-srcw; }
		if( srch<0 ){ srcy+=srch;srch=-srch; }
		if( srcw<=0 || srch<=0 ) return 0;
		
		FlushGraphics();

		var srcrect:Rectangle=new Rectangle( srcx,srcy,srcw,srch );
		
		if( matrix || clipRect || colorTform || blend ){

			var scratch:BitmapData=surface.scratch;
			if( scratch==null || srcw!=scratch.width || srch!=scratch.height ){
				if( scratch!=null ) scratch.dispose();
				scratch=new BitmapData( srcw,srch );
				surface.scratch=scratch;
			}
			pointCoords.x=0;pointCoords.y=0;
			scratch.copyPixels( surface.bitmap.bitmapData,srcrect,pointCoords );
			
			var mat:Matrix;
			if( matrix ){
				mat=new Matrix( 1,0,0,1,x,y );
				mat.concat( matrix );
			}else{
				imageMat.tx=x;imageMat.ty=y;
				mat=imageMat;
			}
			bitmapData.draw( scratch,mat,colorTform,blend,clipRect,image_filtering_enabled );
		}else{
			pointCoords.x=x;pointCoords.y=y;
			bitmapData.copyPixels( surface.bitmap.bitmapData,srcrect,pointCoords );
		}
		return 0;
	}
	
	public function ReadPixels( pixels:Array,x:int,y:int,width:int,height:int,offset:int,pitch:int ):int{
	
		FlushGraphics();
		
		var data:ByteArray=bitmapData.getPixels( new Rectangle( x,y,width,height ) );
		data.position=0;
		
		var px:int,py:int,j:int=offset,argb:int;
		
		for( py=0;py<height;++py ){
			for( px=0;px<width;++px ){
				pixels[j++]=data.readInt();
			}
			j+=pitch-width;
		}
		
		return 0;
	}
	
	public function WritePixels2( surface:gxtkSurface,pixels:Array,x:int,y:int,width:int,height:int,offset:int,pitch:int ):int{

		var data:ByteArray=new ByteArray();
		data.length=width*height;
			
		var px:int,py:int,j:int=offset,argb:int;
		
		for( py=0;py<height;++py ){
			for( px=0;px<width;++px ){
				data.writeInt( pixels[j++] );
			}
			j+=pitch-width;
		}
		data.position=0;
		
		surface.bitmap.bitmapData.setPixels( new Rectangle( x,y,width,height ),data );
		
		return 0;
	}
}

//***** gxtkSurface *****

class gxtkSurface{
	internal var bitmap:Bitmap;
	internal var rect:Rectangle;
	internal var scratch:BitmapData;
	
	function gxtkSurface( bitmap:Bitmap ){
		SetBitmap( bitmap );
	}
	
	public function SetBitmap( bitmap:Bitmap ):void{
		this.bitmap=bitmap;
		rect=new Rectangle( 0,0,bitmap.width,bitmap.height );
	}

	//***** GXTK API *****

	public function Discard():int{
		return 0;
	}
	
	public function Width():int{
		return rect.width;
	}

	public function Height():int{
		return rect.height;
	}

	public function Loaded():int{
		return 1;
	}
	
	public function OnUnsafeLoadComplete():void{
	}
}

class gxtkChannel{
	internal var channel:SoundChannel;	//null then not playing
	internal var sample:gxtkSample;
	internal var loops:Boolean;
	internal var transform:SoundTransform=new SoundTransform();
	internal var pausepos:Number;
	internal var state:int;				//0=stopped, 1=playing, 2=paused, 5=playing/suspended
}

class gxtkAudio{

	internal var busy:Boolean;

	internal var game:BBFlashGame;
	internal var music:gxtkSample;

	internal var channels:Array=new Array( 33 );

	function gxtkAudio(){
		game=BBFlashGame.FlashGame();
		for( var i:int=0;i<33;++i ){
			channels[i]=new gxtkChannel();
		}
	}
	
	internal function SoundComplete( ev:Event ):void{

		//Should never happen!	
		if( busy ){
			debugLog( "gxtkAudio.SoundComplete Error - audio is busy!" );
			return;
		}
		
		busy=true;
		
		for( var i:int=0;i<33;++i ){
			var chan:gxtkChannel=channels[i];
			if( chan.state==1 && chan.channel==ev.target ){
				if( chan.loops ){
					chan.channel=chan.sample.sound.play( 0,0,chan.transform );
					if( chan.channel ){
						chan.channel.addEventListener( Event.SOUND_COMPLETE,SoundComplete );
						continue;
					}
				}
				chan.channel=null;
				chan.sample=null;
				chan.state=0;
			}
		}
		
		busy=false;
	}
	
	//***** GXTK API *****
	
	public function Suspend():int{
	
		busy=true;
		
		for( var i:int=0;i<33;++i ){
			var chan:gxtkChannel=channels[i];
			if( chan.state==1 ){
				chan.pausepos=chan.channel.position;
				chan.channel.stop();
				chan.channel=null;
				chan.state=5;
			}
		}
		
		busy=false;
		
		return 0;
	}
	
	public function Resume():int{
	
		busy=true;
		
		for( var i:int=0;i<33;++i ){
			var chan:gxtkChannel=channels[i];
			if( chan.state==5 ){
				chan.channel=chan.sample.sound.play( chan.pausepos,0,chan.transform );
				if( chan.channel ){
					chan.channel.addEventListener( Event.SOUND_COMPLETE,SoundComplete );
					chan.state=1;
					continue;
				}
				
				debugLog( "gxtkAudio.Resume() - failed to create SoundChannel" );
				
				chan.sample=null;
				chan.state=0;
			}
		}
		
		busy=false;
		
		return 0;
	}
	
	public function LoadSample__UNSAFE__( sample:gxtkSample,path:String ):Boolean{
		return false;
	}
	
	public function LoadSample( path:String ):gxtkSample{
		var sound:Sound=game.LoadSound( path );
		if( sound ) return new gxtkSample( sound );
		return null;
	}
	
	public function PlaySample( sample:gxtkSample,channel:int,flags:int ):int{
		var chan:gxtkChannel=channels[channel];
		
		busy=true;
		
		if( chan.state!=0 ) chan.channel.stop();

		chan.sample=sample;
		chan.loops=(flags & 1)!=0;
		chan.channel=sample.sound.play( 0,0,chan.transform );
		if( chan.channel ){
			chan.channel.addEventListener( Event.SOUND_COMPLETE,SoundComplete );
			chan.state=1;
		}else{
			chan.sample=null;
			chan.state=0;
		}
		
		busy=false;

		return 0;
	}
	
	public function StopChannel( channel:int ):int{
		var chan:gxtkChannel=channels[channel];
		
		busy=true;
		
		if( chan.state!=0 ){
			if( chan.state==1 ){
				chan.channel.stop();
				chan.channel=null;
			}
			chan.sample=null;
			chan.state=0;
		}
		
		busy=false;
		
		return 0;
	}
	
	public function PauseChannel( channel:int ):int{
		var chan:gxtkChannel=channels[channel];
		
		busy=true;
		
		if( chan.state==1 ){
			chan.pausepos=chan.channel.position;
			chan.channel.stop();
			chan.channel=null;
			chan.state=2;
		}
		
		busy=false;
		
		return 0;
	}
	
	public function ResumeChannel( channel:int ):int{
		var chan:gxtkChannel=channels[channel];
		
		busy=true;
		
		if( chan.state==2 ){
			chan.channel=chan.sample.sound.play( chan.pausepos,0,chan.transform );
			if( chan.channel ){
				chan.channel.addEventListener( Event.SOUND_COMPLETE,SoundComplete );
				chan.state=1;
			}else{
				chan.sample=null;
				chan.state=0;
			}
		}
		
		busy=false;
		
		return 0;
	}
	
	public function ChannelState( channel:int ):int{
		var chan:gxtkChannel=channels[channel];
		
		return chan.state & 3;
	}
	
	public function SetVolume( channel:int,volume:Number ):int{
		var chan:gxtkChannel=channels[channel];
		
		chan.transform.volume=volume;

		if( chan.state==1 ) chan.channel.soundTransform=chan.transform;

		return 0;
	}
	
	public function SetPan( channel:int,pan:Number ):int{
		var chan:gxtkChannel=channels[channel];
		
		chan.transform.pan=pan;

		if( chan.state==1 ) chan.channel.soundTransform=chan.transform;

		return 0;
	}
	
	public function SetRate( channel:int,rate:Number ):int{
		return -1;
	}
	
	public function PlayMusic( path:String,flags:int ):int{
		StopMusic();
		
		music=LoadSample( path );
		if( !music ) return -1;
		
		PlaySample( music,32,flags );
		return 0;
	}
	
	public function StopMusic():int{
		StopChannel( 32 );
		
		if( music ){
			music.Discard();
			music=null;
		}
		return 0;
	}
	
	public function PauseMusic():int{
		PauseChannel( 32 );
		
		return 0;
	}
	
	public function ResumeMusic():int{
		ResumeChannel( 32 );
		
		return 0;
	}
	
	public function MusicState():int{
		return ChannelState( 32 );
	}
	
	public function SetMusicVolume( volume:Number ):int{
		SetVolume( 32,volume );
		return 0;
	}
}

class gxtkSample{

	internal var sound:Sound;

	function gxtkSample( sound:Sound ){
		this.sound=sound;
	}
	
	public function Discard():int{
		return 0;
	}
	
}
