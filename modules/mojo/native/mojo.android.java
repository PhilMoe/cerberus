
// Android mojo runtime.
//
// Copyright 2011 Mark Sibly, all rights reserved.
// No warranty implied; use at your own risk.

import java.io.*;
import java.nio.*;
import java.util.*;
import java.lang.reflect.*;

import android.os.*;
import android.app.*;
import android.media.*;
import android.view.*;
import android.graphics.*;
import android.content.*;
import android.util.*;
import android.hardware.*;
import android.view.inputmethod.*;

import android.opengl.*;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;

import android.content.res.AssetManager;
import android.content.res.AssetFileDescriptor;

class gxtkGraphics{

	static class RenderOp{
		int type,count,alpha;
		gxtkSurface surf;
	};

	static final int MAX_VERTICES=65536/20;
	static final int MAX_RENDEROPS=MAX_VERTICES/2;
	static final int MAX_QUAD_INDICES=MAX_VERTICES/4*6;
	
	static int seq=1;
	
	BBAndroidGame game;
	
	boolean gles20;
	int width,height;
	
	float alpha;
	float r,g,b;
	int colorARGB;
	int blend;
	float ix,iy,jx,jy,tx,ty;
	boolean tformed;
	
	RenderOp renderOps[]=new RenderOp[MAX_RENDEROPS];
	RenderOp rop,nullRop;
	int nextOp,vcount;

	float[] vertices=new float[MAX_VERTICES*4];	//x,y,u,v
	int[] colors=new int[MAX_VERTICES];	//rgba
	int vp,cp;
	
	FloatBuffer vbuffer;
	IntBuffer cbuffer;
	int vbo,vbo_seq,ibo;
	
	gxtkGraphics(){
	
		game=BBAndroidGame.AndroidGame();
		
		width=game.GetGameView().getWidth();
		height=game.GetGameView().getHeight();
		
		gles20=CerberusConfig.OPENGL_GLES20_ENABLED.equals( "1" );
		if( gles20 ) return;
	
		for( int i=0;i<MAX_RENDEROPS;++i ){
			renderOps[i]=new RenderOp();
		}
		nullRop=new RenderOp();
		nullRop.type=-1;

		vbuffer=FloatBuffer.wrap( vertices,0,MAX_VERTICES*4 );
		cbuffer=IntBuffer.wrap( colors,0,MAX_VERTICES );
	}
	
	void Reset(){
		rop=nullRop;
		nextOp=0;
		vcount=0;
	}

	void Flush(){
		if( vcount==0 ) return;
		
		//'discard' buffer contents...
		GLES11.glBufferData( GLES11.GL_ARRAY_BUFFER,MAX_VERTICES*20,null,GLES11.GL_DYNAMIC_DRAW );
		
		GLES11.glBufferSubData( GLES11.GL_ARRAY_BUFFER,0,vcount*16,vbuffer );
		GLES11.glBufferSubData( GLES11.GL_ARRAY_BUFFER,vcount*16,vcount*4,cbuffer );
		GLES11.glColorPointer( 4,GLES11.GL_UNSIGNED_BYTE,0,vcount*16 );

		GLES11.glDisable( GLES11.GL_TEXTURE_2D );
		GLES11.glDisable( GLES11.GL_BLEND );

		int index=0;
		boolean blendon=false;
		gxtkSurface surf=null;

		for( int i=0;i<nextOp;++i ){

			RenderOp op=renderOps[i];
			
			if( op.surf!=null ){
				if( op.surf!=surf ){
					if( surf==null ) GLES11.glEnable( GLES11.GL_TEXTURE_2D );
					surf=op.surf;
					surf.Bind();
				}
			}else{
				if( surf!=null ){
					GLES11.glDisable( GLES11.GL_TEXTURE_2D );
					surf=null;
				}
			}
			
			//should just have another blend mode...
			if( blend==1 || (op.alpha>>>24)!=0xff || (op.surf!=null && op.surf.hasAlpha) ){
				if( !blendon ){
					GLES11.glEnable( GLES11.GL_BLEND );
					blendon=true;
				}
			}else{
				if( blendon ){
					GLES11.glDisable( GLES11.GL_BLEND );
					blendon=false;
				}
			}
			
			switch( op.type ){
			case 1:
				GLES11.glDrawArrays( GLES11.GL_POINTS,index,op.count );
				break;
			case 2:
				GLES11.glDrawArrays( GLES11.GL_LINES,index,op.count );
				break;
			case 3:
				GLES11.glDrawArrays( GLES11.GL_TRIANGLES,index,op.count );
				break;
			case 4:
				GLES11.glDrawElements( GLES11.GL_TRIANGLES,op.count/4*6,GLES11.GL_UNSIGNED_SHORT,(index/4*6+(index&3)*MAX_QUAD_INDICES)*2 );
				break;
			default:
				for( int j=0;j<op.count;j+=op.type ){
					GLES11.glDrawArrays( GLES11.GL_TRIANGLE_FAN,index+j,op.type );
				}
			}
			
			index+=op.count;
		}
		
		Reset();
	}
	
	void Begin( int type,int count,gxtkSurface surf ){
		if( vcount+count>MAX_VERTICES ){
			Flush();
		}
		if( type!=rop.type || surf!=rop.surf ){
			if( nextOp==MAX_RENDEROPS ) Flush();
			rop=renderOps[nextOp];
			nextOp+=1;
			rop.type=type;
			rop.surf=surf;
			rop.count=0;
			rop.alpha=~0;
		}
		rop.alpha&=colorARGB;
		rop.count+=count;
		vp=vcount*4;
		cp=vcount;
		vcount+=count;
	}

	//***** GXTK API *****

	int Width(){
		return width;
	}
	
	int Height(){
		return height;
	}
	
	int BeginRender(){
	
		width=game.GetGameView().getWidth();
		height=game.GetGameView().getHeight();
		
		if( gles20 ) return 0;

		if( vbo_seq!=seq ){

			vbo_seq=seq;
			
			int[] bufs=new int[2];
			GLES11.glGenBuffers( 2,bufs,0 );

			vbo=bufs[0];
			ibo=bufs[1];
			
			GLES11.glBindBuffer( GLES11.GL_ARRAY_BUFFER,vbo );
			GLES11.glBufferData( GLES11.GL_ARRAY_BUFFER,MAX_VERTICES*20,null,GLES11.GL_DYNAMIC_DRAW );
			
			short[] idxs=new short[MAX_QUAD_INDICES*4];
			for( int j=0;j<4;++j ){
				int k=j*MAX_QUAD_INDICES;
				for( int i=0;i<MAX_QUAD_INDICES/6;++i ){
					idxs[i*6+k+0]=(short)(i*4+j);
					idxs[i*6+k+1]=(short)(i*4+j+1);
					idxs[i*6+k+2]=(short)(i*4+j+2);
					idxs[i*6+k+3]=(short)(i*4+j);
					idxs[i*6+k+4]=(short)(i*4+j+2);
					idxs[i*6+k+5]=(short)(i*4+j+3);
				}
			}
			ShortBuffer ibuffer=ShortBuffer.wrap( idxs,0,idxs.length );
			GLES11.glBindBuffer( GLES11.GL_ELEMENT_ARRAY_BUFFER,ibo );
			GLES11.glBufferData( GLES11.GL_ELEMENT_ARRAY_BUFFER,idxs.length*2,ibuffer,GLES11.GL_STATIC_DRAW );
		}
		
		GLES11.glViewport( 0,0,Width(),Height() );
		
		GLES11.glMatrixMode( GLES11.GL_PROJECTION );
		GLES11.glLoadIdentity();
		GLES11.glOrthof( 0,Width(),Height(),0,-1,1 );
		
		GLES11.glMatrixMode( GLES11.GL_MODELVIEW );
		GLES11.glLoadIdentity();
		
		GLES11.glEnable( GLES11.GL_BLEND );
		GLES11.glBlendFunc( GLES11.GL_ONE,GLES11.GL_ONE_MINUS_SRC_ALPHA );

		GLES11.glBindBuffer( GLES11.GL_ARRAY_BUFFER,vbo );
		GLES11.glBindBuffer( GLES11.GL_ELEMENT_ARRAY_BUFFER,ibo );
		GLES11.glEnableClientState( GLES11.GL_VERTEX_ARRAY );
		GLES11.glEnableClientState( GLES11.GL_TEXTURE_COORD_ARRAY );
		GLES11.glEnableClientState( GLES11.GL_COLOR_ARRAY );
		GLES11.glVertexPointer( 2,GLES11.GL_FLOAT,16,0 );
		GLES11.glTexCoordPointer( 2,GLES11.GL_FLOAT,16,8 );
		GLES11.glColorPointer( 4,GLES11.GL_UNSIGNED_BYTE,0,MAX_VERTICES*16 );

		Reset();
		
		return 1;
	}
	
	void EndRender(){
		if( gles20 ) return;
		Flush();
	}
	
	boolean LoadSurface__UNSAFE__( gxtkSurface surface,String path ){
		Bitmap bitmap=game.LoadBitmap( path );
		if( bitmap==null ) return false;
		surface.SetBitmap( bitmap );
		return true;
	}
	
	gxtkSurface LoadSurface( String path ){
		gxtkSurface surf=new gxtkSurface();
		if( !LoadSurface__UNSAFE__( surf,path ) ) return null;
		return surf;
	}
	
	gxtkSurface CreateSurface( int width,int height ){
		Bitmap bitmap=Bitmap.createBitmap( width,height,Bitmap.Config.ARGB_8888 );
		if( bitmap!=null ) return new gxtkSurface( bitmap );
		return null;
	}
	
	void DiscardGraphics(){
		gxtkSurface.FlushDiscarded( false );
		seq+=1;
	}

	int SetAlpha( float alpha ){
		this.alpha=alpha;
		int a=(int)(alpha*255);
		colorARGB=(a<<24) | ((int)(b*alpha)<<16) | ((int)(g*alpha)<<8) | (int)(r*alpha);
		return 0;
	}

	int SetColor( float r,float g,float b ){
		this.r=r;
		this.g=g;
		this.b=b;
		int a=(int)(alpha*255);
		colorARGB=(a<<24) | ((int)(b*alpha)<<16) | ((int)(g*alpha)<<8) | (int)(r*alpha);
		return 0;
	}
	
	int SetBlend( int blend ){
		if( blend==this.blend ) return 0;
		
		Flush();
		
		this.blend=blend;
		
		switch( blend ){
		case 1:
			GLES11.glBlendFunc( GLES11.GL_ONE,GLES11.GL_ONE );
			break;
		default:
			GLES11.glBlendFunc( GLES11.GL_ONE,GLES11.GL_ONE_MINUS_SRC_ALPHA );
		}
		return 0;
	}
	
	int SetScissor( int x,int y,int w,int h ){
		Flush();
		
		if( x!=0 || y!=0 || w!=Width() || h!=Height() ){
			GLES11.glEnable( GLES11.GL_SCISSOR_TEST );
			y=Height()-y-h;
			GLES11.glScissor( x,y,w,h );
		}else{
			GLES11.glDisable( GLES11.GL_SCISSOR_TEST );
		}
		return 0;
	}
	
	int SetMatrix( float ix,float iy,float jx,float jy,float tx,float ty ){
	
		tformed=(ix!=1 || iy!=0 || jx!=0 || jy!=1 || tx!=0 || ty!=0);
		this.ix=ix;
		this.iy=iy;
		this.jx=jx;
		this.jy=jy;
		this.tx=tx;
		this.ty=ty;
		
		return 0;
	}
	
	int Cls( float r,float g,float b ){
		Reset();
		
		GLES11.glClearColor( r/255.0f,g/255.0f,b/255.0f,1 );
		GLES11.glClear( GLES11.GL_COLOR_BUFFER_BIT );	//|GLES11.GL_DEPTH_BUFFER_BIT ); //GL_DEPTH_BUFFER_BIT crashes someone's phone...
		
		return 0;
	}
	
	int DrawPoint( float x,float y ){
	
		if( tformed ){
			float px=x;
			x=px * ix + y * jx + tx;
			y=px * iy + y * jy + ty;
		}
		
		Begin( 1,1,null );
		
		vertices[vp]=x+.5f;vertices[vp+1]=y+.5f;
		
		colors[cp]=colorARGB;
		
		return 0;
	}
	
	int DrawLine( float x0,float y0,float x1,float y1 ){
		
		if( tformed ){
			float tx0=x0,tx1=x1;
			x0=tx0 * ix + y0 * jx + tx;
			y0=tx0 * iy + y0 * jy + ty;
			x1=tx1 * ix + y1 * jx + tx;
			y1=tx1 * iy + y1 * jy + ty;
		}

		Begin( 2,2,null );

		vertices[vp]=x0+.5f;vertices[vp+1]=y0+.5f;
		vertices[vp+4]=x1+.5f;vertices[vp+5]=y1+.5f;

		colors[cp]=colors[cp+1]=colorARGB;

		return 0;
 	}

	int DrawRect( float x,float y,float w,float h ){
	
		float x0=x,x1=x+w,x2=x+w,x3=x;
		float y0=y,y1=y,y2=y+h,y3=y+h;
		
		if( tformed ){
			float tx0=x0,tx1=x1,tx2=x2,tx3=x3;
			x0=tx0 * ix + y0 * jx + tx;
			y0=tx0 * iy + y0 * jy + ty;
			x1=tx1 * ix + y1 * jx + tx;
			y1=tx1 * iy + y1 * jy + ty;
			x2=tx2 * ix + y2 * jx + tx;
			y2=tx2 * iy + y2 * jy + ty;
			x3=tx3 * ix + y3 * jx + tx;
			y3=tx3 * iy + y3 * jy + ty;
		}

		Begin( 4,4,null );
		
		vertices[vp]=x0;vertices[vp+1]=y0;
		vertices[vp+4]=x1;vertices[vp+5]=y1;
		vertices[vp+8]=x2;vertices[vp+9]=y2;
		vertices[vp+12]=x3;vertices[vp+13]=y3;
		
		colors[cp]=colors[cp+1]=colors[cp+2]=colors[cp+3]=colorARGB;

		return 0;
	}
	
	int DrawOval( float x,float y,float w,float h ){

		float xr=w/2.0f;
		float yr=h/2.0f;

		int n;	
		if( tformed ){
			float xx=xr*ix,xy=xr*iy,xd=(float)Math.sqrt(xx*xx+xy*xy);
			float yx=yr*jx,yy=yr*jy,yd=(float)Math.sqrt(yx*yx+yy*yy);
			n=(int)( xd+yd );
		}else{
			n=(int)( Math.abs(xr)+Math.abs(yr) );
		}

		if( n>MAX_VERTICES ){
			n=MAX_VERTICES;
		}else if( n<12 ){
			n=12;
		}else{
			n&=~3;
		}
		
		x+=xr;
		y+=yr;
		
		Begin( n,n,null );
		
		for( int i=0;i<n;++i ){
			float th=i * 6.28318531f / n;
			float x0=(float)(x+Math.cos(th)*xr);
			float y0=(float)(y+Math.sin(th)*yr);
			if( tformed ){
				float tx0=x0;
				x0=tx0 * ix + y0 * jx + tx;
				y0=tx0 * iy + y0 * jy + ty;
			}
			vertices[vp]=x0;
			vertices[vp+1]=y0;
			colors[cp]=colorARGB;
			vp+=4;
			cp+=1;
		}
		
		return 0;
	}
	
	int DrawPoly( float[] verts ){
		if( verts.length<6 || verts.length>MAX_VERTICES*2 ) return 0;

		Begin( verts.length/2,verts.length/2,null );		
		
		if( tformed ){
			for( int i=0;i<verts.length;i+=2 ){
				vertices[vp  ]=verts[i] * ix + verts[i+1] * jx + tx;
				vertices[vp+1]=verts[i] * iy + verts[i+1] * jy + ty;
				colors[cp]=colorARGB;
				vp+=4;
				cp+=1;
			}
		}else{
			for( int i=0;i<verts.length;i+=2 ){
				vertices[vp  ]=verts[i];
				vertices[vp+1]=verts[i+1];
				colors[cp]=colorARGB;
				vp+=4;
				cp+=1;
			}
		}
		
		return 0;
	}
	
	int DrawPoly2( float[] verts,gxtkSurface surface,int srcx,int srcy ){
	
		int n=verts.length/4;
		if( n<1 || n>MAX_VERTICES ) return 0;
		
		Begin( n,n,surface );
		
		for( int i=0;i<n;++i ){
			int j=i*4;
			if( tformed ){
				vertices[vp  ]=verts[j] * ix + verts[j+1] * jx + tx;
				vertices[vp+1]=verts[j] * iy + verts[j+1] * jy + ty;
			}else{
				vertices[vp  ]=verts[j];
				vertices[vp+1]=verts[j+1];
			}
			vertices[vp+2]=(srcx+verts[j+2])*surface.uscale;
			vertices[vp+3]=(srcy+verts[j+3])*surface.vscale;
			colors[cp]=colorARGB;
			vp+=4;
			cp+=1;
		}
		
		return 0;
	}
	
	int DrawSurface( gxtkSurface surface,float x,float y ){
	
		float w=surface.width;
		float h=surface.height;
		float u0=0,u1=w*surface.uscale;
		float v0=0,v1=h*surface.vscale;
		
		float x0=x,x1=x+w,x2=x+w,x3=x;
		float y0=y,y1=y,y2=y+h,y3=y+h;
		
		if( tformed ){
			float tx0=x0,tx1=x1,tx2=x2,tx3=x3;
			x0=tx0 * ix + y0 * jx + tx;
			y0=tx0 * iy + y0 * jy + ty;
			x1=tx1 * ix + y1 * jx + tx;
			y1=tx1 * iy + y1 * jy + ty;
			x2=tx2 * ix + y2 * jx + tx;
			y2=tx2 * iy + y2 * jy + ty;
			x3=tx3 * ix + y3 * jx + tx;
			y3=tx3 * iy + y3 * jy + ty;
		}
	
		Begin( 4,4,surface );
		
		vertices[vp]=x0;vertices[vp+1]=y0;vertices[vp+2]=u0;vertices[vp+3]=v0;
		vertices[vp+4]=x1;vertices[vp+5]=y1;vertices[vp+6]=u1;vertices[vp+7]=v0;
		vertices[vp+8]=x2;vertices[vp+9]=y2;vertices[vp+10]=u1;vertices[vp+11]=v1;
		vertices[vp+12]=x3;vertices[vp+13]=y3;vertices[vp+14]=u0;vertices[vp+15]=v1;

		colors[cp]=colors[cp+1]=colors[cp+2]=colors[cp+3]=colorARGB;

		return 0;
	}
	
	int DrawSurface2( gxtkSurface surface,float x,float y,int srcx,int srcy,int srcw,int srch ){
	
		float w=srcw;
		float h=srch;
		float u0=srcx*surface.uscale,u1=(srcx+srcw)*surface.uscale;
		float v0=srcy*surface.vscale,v1=(srcy+srch)*surface.vscale;
		
		float x0=x,x1=x+w,x2=x+w,x3=x;
		float y0=y,y1=y,y2=y+h,y3=y+h;
		
		if( tformed ){
			float tx0=x0,tx1=x1,tx2=x2,tx3=x3;
			x0=tx0 * ix + y0 * jx + tx;
			y0=tx0 * iy + y0 * jy + ty;
			x1=tx1 * ix + y1 * jx + tx;
			y1=tx1 * iy + y1 * jy + ty;
			x2=tx2 * ix + y2 * jx + tx;
			y2=tx2 * iy + y2 * jy + ty;
			x3=tx3 * ix + y3 * jx + tx;
			y3=tx3 * iy + y3 * jy + ty;
		}

		Begin( 4,4,surface );
		
		vertices[vp]=x0;vertices[vp+1]=y0;vertices[vp+2]=u0;vertices[vp+3]=v0;
		vertices[vp+4]=x1;vertices[vp+5]=y1;vertices[vp+6]=u1;vertices[vp+7]=v0;
		vertices[vp+8]=x2;vertices[vp+9]=y2;vertices[vp+10]=u1;vertices[vp+11]=v1;
		vertices[vp+12]=x3;vertices[vp+13]=y3;vertices[vp+14]=u0;vertices[vp+15]=v1;

		colors[cp]=colors[cp+1]=colors[cp+2]=colors[cp+3]=colorARGB;

		return 0;
	}
	
	int ReadPixels( int[] pixels,int x,int y,int width,int height,int offset,int pitch ){
	
		Flush();
		
		int[] texels=new int[width*height];
		IntBuffer buf=IntBuffer.wrap( texels );

		GLES11.glReadPixels( x,Height()-y-height,width,height,GLES11.GL_RGBA,GLES11.GL_UNSIGNED_BYTE,buf );

		int i=0;
		for( int py=height-1;py>=0;--py ){
			int j=offset+py*pitch;
			for( int px=0;px<width;++px ){
				int p=texels[i++];
				//RGBA -> BGRA, Big Endian!
				pixels[j++]=(p&0xff000000)|((p<<16)&0xff0000)|(p&0xff00)|((p>>16)&0xff);
			}
		}
	
		return 0;
	}

	int WritePixels2( gxtkSurface surface,int[] pixels,int x,int y,int width,int height,int offset,int pitch ){
	
		surface.bitmap.setPixels( pixels,offset,pitch,x,y,width,height );
		
		surface.Invalidate();
	
		return 0;
	}
	
}

class gxtkSurface{

	Bitmap bitmap;
	
	int width,height;
	int twidth,theight;
	float uscale,vscale;
	boolean hasAlpha;
	int format,type;
	int texId,seq;

	static Vector discarded=new Vector();
	
	gxtkSurface(){
	}
	
	gxtkSurface( Bitmap bitmap ){
		SetBitmap( bitmap );
	}
	
	void SetBitmap( Bitmap bitmap ){
		this.bitmap=bitmap;
		width=bitmap.getWidth();
		height=bitmap.getHeight();
		hasAlpha=bitmap.hasAlpha();
		twidth=Pow2Size( width );
		theight=Pow2Size( height );
		uscale=1.0f/(float)twidth;
		vscale=1.0f/(float)theight;
	}

	protected void finalize(){
		Discard();
	}
	
	int Pow2Size( int n ){
		int i=1;
		while( i<n ) i*=2;
		return i;
	}
	
	static void FlushDiscarded( boolean deltexs ){
		if( deltexs ){
			int n=discarded.size();
			if( n>0 ){
				int[] texs=new int[n];
				for( int i=0;i<n;++i ){
					texs[i]=((Integer)discarded.elementAt(i)).intValue();
				}
				GLES11.glDeleteTextures( n,texs,0 );
			}
		}
		discarded.clear();
	}
	
	void Invalidate(){
		if( seq==gxtkGraphics.seq ){
			discarded.add( Integer.valueOf( texId ) );
			seq=0;
		}
	}

	//Experimental version...
	//
	void Bind2(){
	
		if( seq==gxtkGraphics.seq ){
			GLES11.glBindTexture( GLES11.GL_TEXTURE_2D,texId );
			return;
		}
        
        if( bitmap==null ) throw new Error( "Attempt to use discarded image" );
		
		FlushDiscarded( true );

		int[] texs=new int[1];
		GLES11.glGenTextures( 1,texs,0 );
		texId=texs[0];
		if( texId==0 ) throw new Error( "glGenTextures failed" );
		seq=gxtkGraphics.seq;
		
		GLES11.glBindTexture( GLES11.GL_TEXTURE_2D,texId );
		
		if( CerberusConfig.MOJO_IMAGE_FILTERING_ENABLED.equals( "1" ) ){
			GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_MAG_FILTER,GLES11.GL_LINEAR );
			GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_MIN_FILTER,GLES11.GL_LINEAR );
		}else{
			GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_MAG_FILTER,GLES11.GL_NEAREST );
			GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_MIN_FILTER,GLES11.GL_NEAREST );
		}

		GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_WRAP_S,GLES11.GL_CLAMP_TO_EDGE );
		GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_WRAP_T,GLES11.GL_CLAMP_TO_EDGE );
		
		int pwidth=(width==twidth) ? width : width+1;
		int pheight=(height==theight) ? height : height+1;

		Bitmap ibitmap=bitmap,bitmap2=null;
		
		if( width!=pwidth || height!=pheight ){
		
			bitmap2=Bitmap.createBitmap( twidth,theight,bitmap.getConfig() );
			Canvas canvas=new Canvas( bitmap2 );
			canvas.drawBitmap( bitmap,0,0,null );

			if( width!=pwidth ){
				canvas.save();
				canvas.clipRect( width,0,pwidth,height );
				canvas.drawBitmap( bitmap,1,0,null );
				canvas.restore();
				if( height!=pheight ){
					canvas.save();
					canvas.clipRect( 0,height,pwidth,pheight );
					canvas.drawBitmap( bitmap2,0,1,null );
					canvas.restore();
				}
			}else if( height!=pheight ){
				canvas.save();
				canvas.clipRect( 0,height,width,pheight );
				canvas.drawBitmap( bitmap,0,1,null );
				canvas.restore();
			}

			ibitmap=bitmap2;
		}
		
		int format=GLUtils.getInternalFormat( ibitmap ),type=GLUtils.getType( ibitmap );
		
		GLES11.glTexImage2D( GLES11.GL_TEXTURE_2D,0,format,twidth,theight,0,format,type,null );
		
		GLUtils.texSubImage2D( GLES11.GL_TEXTURE_2D,0,0,0,ibitmap );

		if( bitmap2!=null ) bitmap2.recycle();
	}

	void Bind(){
	
		if( seq==gxtkGraphics.seq ){
			GLES11.glBindTexture( GLES11.GL_TEXTURE_2D,texId );
			return;
		}
        
        if( bitmap==null ) throw new Error( "Attempt to use discarded image" );
		
		FlushDiscarded( true );

		int[] texs=new int[1];
		GLES11.glGenTextures( 1,texs,0 );
		texId=texs[0];
		if( texId==0 ) throw new Error( "glGenTextures failed" );
		seq=gxtkGraphics.seq;
		
		GLES11.glBindTexture( GLES11.GL_TEXTURE_2D,texId );
		
		if( CerberusConfig.MOJO_IMAGE_FILTERING_ENABLED.equals( "1" ) ){
			GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_MAG_FILTER,GLES11.GL_LINEAR );
			GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_MIN_FILTER,GLES11.GL_LINEAR );
		}else{
			GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_MAG_FILTER,GLES11.GL_NEAREST );
			GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_MIN_FILTER,GLES11.GL_NEAREST );
		}

		GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_WRAP_S,GLES11.GL_CLAMP_TO_EDGE );
		GLES11.glTexParameteri( GLES11.GL_TEXTURE_2D,GLES11.GL_TEXTURE_WRAP_T,GLES11.GL_CLAMP_TO_EDGE );
		
		int pwidth=(width==twidth) ? width : width+1;
		int pheight=(height==theight) ? height : height+1;

		int sz=pwidth*pheight;
		int[] pixels=new int[sz];
		bitmap.getPixels( pixels,0,pwidth,0,0,width,height );
		
		//pad edges for non pow-2 images - not sexy!
		if( width!=pwidth ){
			for( int y=0;y<height;++y ){
				pixels[y*pwidth+width]=pixels[y*pwidth+width-1];
			}
		}
		if( height!=pheight ){
			for( int x=0;x<width;++x ){
				pixels[height*pwidth+x]=pixels[height*pwidth+x-pwidth];
			}
		}
		if( width!=pwidth && height!=pheight ){
			pixels[height*pwidth+width]=pixels[height*pwidth+width-pwidth-1];
		}
		
		GLES11.glPixelStorei( GLES11.GL_UNPACK_ALIGNMENT,1 );
		
		boolean hicolor_textures=CerberusConfig.MOJO_HICOLOR_TEXTURES.equals( "1" );
		
		if( hicolor_textures && hasAlpha ){

			//RGBA8888...
			ByteBuffer buf=ByteBuffer.allocate( sz*4 );
			buf.order( ByteOrder.BIG_ENDIAN );

			for( int i=0;i<sz;++i ){
				int p=pixels[i];
				int a=(p>>24) & 255;
				int r=((p>>16) & 255)*a/255;
				int g=((p>>8) & 255)*a/255;
				int b=(p & 255)*a/255;
				buf.putInt( (r<<24)|(g<<16)|(b<<8)|a );
			}
			buf.position( 0 );
			GLES11.glTexImage2D( GLES11.GL_TEXTURE_2D,0,GLES11.GL_RGBA,twidth,theight,0,GLES11.GL_RGBA,GLES11.GL_UNSIGNED_BYTE,null );
			GLES11.glTexSubImage2D( GLES11.GL_TEXTURE_2D,0,0,0,pwidth,pheight,GLES11.GL_RGBA,GLES11.GL_UNSIGNED_BYTE,buf );

		}else if( hicolor_textures && !hasAlpha ){
		
			//RGB888...
			ByteBuffer buf=ByteBuffer.allocate( sz*3 );
			buf.order( ByteOrder.BIG_ENDIAN );
			
			for( int i=0;i<sz;++i ){
				int p=pixels[i];
				int r=(p>>16) & 255;
				int g=(p>>8) & 255;
				int b=p & 255;
				buf.put( (byte)r );
				buf.put( (byte)g );
				buf.put( (byte)b );
			}
			buf.position( 0 );
			GLES11.glTexImage2D( GLES11.GL_TEXTURE_2D,0,GLES11.GL_RGB,twidth,theight,0,GLES11.GL_RGB,GLES11.GL_UNSIGNED_BYTE,null );
			GLES11.glTexSubImage2D( GLES11.GL_TEXTURE_2D,0,0,0,pwidth,pheight,GLES11.GL_RGB,GLES11.GL_UNSIGNED_BYTE,buf );
			
		}else if( !hicolor_textures && hasAlpha ){

			//16 bit RGBA...
			ByteBuffer buf=ByteBuffer.allocate( sz*2 );
			buf.order( ByteOrder.LITTLE_ENDIAN );
			
			//do we need 4 bit alpha?
			boolean a4=false;
			for( int i=0;i<sz;++i ){
				int a=(pixels[i]>>28) & 15;
				if( a!=0 && a!=15 ){
					a4=true;
					break;
				}
			}
			if( a4 ){
				//RGBA4444...
				for( int i=0;i<sz;++i ){
					int p=pixels[i];
					int a=(p>>28) & 15;
					int r=((p>>20) & 15)*a/15;
					int g=((p>>12) & 15)*a/15;
					int b=((p>> 4) & 15)*a/15;
					buf.putShort( (short)( (r<<12)|(g<<8)|(b<<4)|a ) );
				}
				buf.position( 0 );
				GLES11.glTexImage2D( GLES11.GL_TEXTURE_2D,0,GLES11.GL_RGBA,twidth,theight,0,GLES11.GL_RGBA,GLES11.GL_UNSIGNED_SHORT_4_4_4_4,null );
				GLES11.glTexSubImage2D( GLES11.GL_TEXTURE_2D,0,0,0,pwidth,pheight,GLES11.GL_RGBA,GLES11.GL_UNSIGNED_SHORT_4_4_4_4,buf );
			}else{
				//RGBA5551...
				for( int i=0;i<sz;++i ){
					int p=pixels[i];
					int a=(p>>31) & 1;
					int r=((p>>19) & 31)*a;
					int g=((p>>11) & 31)*a;
					int b=((p>> 3) & 31)*a;
					buf.putShort( (short)( (r<<11)|(g<<6)|(b<<1)|a ) );
				}
				buf.position( 0 );
				GLES11.glTexImage2D( GLES11.GL_TEXTURE_2D,0,GLES11.GL_RGBA,twidth,theight,0,GLES11.GL_RGBA,GLES11.GL_UNSIGNED_SHORT_5_5_5_1,null );
				GLES11.glTexSubImage2D( GLES11.GL_TEXTURE_2D,0,0,0,pwidth,pheight,GLES11.GL_RGBA,GLES11.GL_UNSIGNED_SHORT_5_5_5_1,buf );
			}
		}else if( !hicolor_textures && !hasAlpha ){
		
			ByteBuffer buf=ByteBuffer.allocate( sz*2 );
			buf.order( ByteOrder.LITTLE_ENDIAN );
			
			//RGB565...
			for( int i=0;i<sz;++i ){
				int p=pixels[i];
				int r=(p>>19) & 31;
				int g=(p>>10) & 63;
				int b=(p>> 3) & 31;
				buf.putShort( (short)( (r<<11)|(g<<5)|b ) );
			}
			buf.position( 0 );
			GLES11.glTexImage2D( GLES11.GL_TEXTURE_2D,0,GLES11.GL_RGB,twidth,theight,0,GLES11.GL_RGB,GLES11.GL_UNSIGNED_SHORT_5_6_5,null );
			GLES11.glTexSubImage2D( GLES11.GL_TEXTURE_2D,0,0,0,pwidth,pheight,GLES11.GL_RGB,GLES11.GL_UNSIGNED_SHORT_5_6_5,buf );
		}
	}

	//***** GXTK API *****
	
	int Discard(){
		Invalidate();
		bitmap=null;
		return 0;
	}

	int Width(){
		return width;
	}
	
	int Height(){
		return height;
	}

	int Loaded(){
		return 1;
	}
	
	void OnUnsafeLoadComplete(){
	}
}

class gxtkAudio{

	static class gxtkChannel{
		int stream;		//SoundPool stream ID, 0=none
		float volume=1;
		float rate=1;
		float pan;
		int state;
	};
	
	BBAndroidGame game;
	SoundPool pool;
	MediaPlayer music;
	float musicVolume=1;
	int musicState=0;
	
	gxtkChannel[] channels=new gxtkChannel[32];
	
	gxtkAudio(){
		game=BBAndroidGame.AndroidGame();
		pool=new SoundPool( 32,AudioManager.STREAM_MUSIC,0 );
		for( int i=0;i<32;++i ){
			channels[i]=new gxtkChannel();
		}
	}
	
	void OnDestroy(){
		for( int i=0;i<32;++i ){
			if( channels[i].state!=0 ) pool.stop( channels[i].stream );
		}
		pool.release();
		pool=null;
	}
	
	//***** GXTK API *****
	int Suspend(){
		if( musicState==1 ) music.pause();
		for( int i=0;i<32;++i ){
			if( channels[i].state==1 ) pool.pause( channels[i].stream );
		}
		return 0;
	}
	
	int Resume(){
		if( musicState==1 ) music.start();
		for( int i=0;i<32;++i ){
			if( channels[i].state==1 ) pool.resume( channels[i].stream );
		}
		return 0;
	}
	

	boolean LoadSample__UNSAFE__( gxtkSample sample,String path ){
		gxtkSample.FlushDiscarded( pool );
		int sound=game.LoadSound( path,pool );
		if( sound==0 ) return false;
		sample.SetSound( sound );
		return true;
	}
	
	gxtkSample LoadSample( String path ){
		gxtkSample sample=new gxtkSample();
		if( !LoadSample__UNSAFE__( sample,path ) ) return null;
		return sample;
	}
	
	int PlaySample( gxtkSample sample,int channel,int flags ){
		gxtkChannel chan=channels[channel];
		if( chan.state!=0 ) pool.stop( chan.stream );
		float rv=(chan.pan * .5f + .5f) * chan.volume;
		float lv=chan.volume-rv;
		int loops=(flags&1)!=0 ? -1 : 0;

		chan.stream=pool.play( sample.sound,lv,rv,0,loops,chan.rate );
		if( chan.stream!=0 ){
			chan.state=1;
			return 0;
		}
		System.out.println( "PlaySample failed" );
		return -1;
	}

	/*
		//Ugly as hell, but seems to work for now...pauses 10 secs max...
		for( int i=0;i<100;++i ){
			chan.stream=pool.play( sample.sound,lv,rv,0,loops,chan.rate );
			if( chan.stream!=0 ){
				chan.state=1;
				return 0;
			}
			try{
				Thread.sleep( 100 );
			}catch( java.lang.InterruptedException ex ){
			}
		}
		throw new Error( "PlaySample failed to play sound" );
	}
	*/
	
	int StopChannel( int channel ){
		gxtkChannel chan=channels[channel];
		if( chan.state!=0 ){
			pool.stop( chan.stream );
			chan.state=0;
		}
		return 0;
	}
	
	int PauseChannel( int channel ){
		gxtkChannel chan=channels[channel];
		if( chan.state==1 ){
			pool.pause( chan.stream );
			chan.state=2;
		}
		return 0;
	}
	
	int ResumeChannel( int channel ){
		gxtkChannel chan=channels[channel];
		if( chan.state==2 ){
			pool.resume( chan.stream );
			chan.state=1;
		}
		return 0;
	}
	
	int ChannelState( int channel ){
		return -1;
	}
	
	int SetVolume( int channel,float volume ){
		gxtkChannel chan=channels[channel];
		chan.volume=volume;
		if( chan.stream!=0 ){
			float rv=(chan.pan * .5f + .5f) * chan.volume;
			float lv=chan.volume-rv;
			pool.setVolume( chan.stream,lv,rv );
		}
		return 0;
	}
	
	int SetPan( int channel,float pan ){
		gxtkChannel chan=channels[channel];
		chan.pan=pan;
		if( chan.stream!=0 ){
			float rv=(chan.pan * .5f + .5f) * chan.volume;
			float lv=chan.volume-rv;
			pool.setVolume( chan.stream,lv,rv );
		}
		return 0;
	}

	int SetRate( int channel,float rate ){
		gxtkChannel chan=channels[channel];
		chan.rate=rate;
		if( chan.stream!=0 ){
			pool.setRate( chan.stream,chan.rate );
		}
		return 0;
	}
	
	int PlayMusic( String path,int flags ){
		StopMusic();
		music=game.OpenMedia( path );
		if( music==null ) return -1;
		music.setLooping( (flags&1)!=0 );
		music.setVolume( musicVolume,musicVolume );
		music.start();
		musicState=1;
		return 0;
	}
	
	int StopMusic(){
		if( musicState!=0 ){
			music.stop();
			music.release();
			musicState=0;
			music=null;
		}
		return 0;
	}
	
	int PauseMusic(){
		if( musicState==1 && music.isPlaying() ){
			music.pause();
			musicState=2;
		}
		return 0;
	}
	
	int ResumeMusic(){
		if( musicState==2 ){
			music.start();
			musicState=1;
		}
		return 0;
	}
	
	int MusicState(){
		if( musicState==1 && !music.isPlaying() ) musicState=0;
		return musicState;
	}
	
	int SetMusicVolume( float volume ){
		if( musicState!=0 ) music.setVolume( volume,volume );
		musicVolume=volume;
		return 0;
	}	
}

class gxtkSample{

	int sound;
	
	static Vector discarded=new Vector();
	
	gxtkSample(){
	}
	
	gxtkSample( int sound ){
		this.sound=sound;
	}
	
	void SetSound( int sound ){
		this.sound=sound;
	}
	
	protected void finalize(){
		Discard();
	}
	
	static void FlushDiscarded( SoundPool pool ){
		int n=discarded.size();
		if( n==0 ) return;
		Vector out=new Vector();
		for( int i=0;i<n;++i ){
			Integer val=(Integer)discarded.elementAt(i);
			if( pool.unload( val.intValue() ) ){
//				bb_std_lang.print( "unload OK!" );
			}else{
//				bb_std_lang.print( "unload failed!" );
				out.add( val );
			}
		}
		discarded=out;
//		bb_std_lang.print( "undiscarded="+out.size() );
	}

	//***** GXTK API *****
	
	int Discard(){
		if( sound!=0 ){
			discarded.add( Integer.valueOf( sound ) );
			sound=0;
		}
		return 0;
	}
}
