
// PSM mojo runtime.
//
// Copyright 2011 Mark Sibly, all rights reserved.
// No warranty implied; use at your own risk.

public class gxtkGraphics{

	const int MAX_VERTS=1024;
	const int MAX_LINES=MAX_VERTS/2;
	const int MAX_QUADS=MAX_VERTS/4;
	
	GraphicsContext _gc;
	int _width,_height;
	
	ShaderProgram _simpleShader;
	ShaderProgram _textureShader;
	int _texUnit;
	
	VertexBuffer _vertBuf,_quadBuf;
	int _primType,_vertCount;
	gxtkSurface _primSurf;
	
	float _red,_green,_blue,_alpha;
	uint _color;
	bool _tformed;
	float _ix,_iy,_jx,_jy,_tx,_ty;
	
	float[] _verts=new float[MAX_VERTS*2];
	float[] _texcs=new float[MAX_VERTS*2];
	uint[] _colors=new uint[MAX_VERTS];
	
	public gxtkGraphics(){
	
		_gc=BBPsmGame.PsmGame().GetGraphicsContext();

		ImageRect r=_gc.GetViewport();
		_width=r.Width;
		_height=r.Height;

		//Shaders....
		//
		_simpleShader=new ShaderProgram( "/Application/shaders/Simple.cgx" );
		_textureShader=new ShaderProgram( "/Application/shaders/Texture.cgx" );

		float[] ortho=new float[]{
			2.0f/(float)_width,0.0f,0.0f,0.0f,
			0.0f,-2.0f/(float)_height,0.0f,0.0f,
			0.0f,0.0f,1.0f,0.0f,
			-1.0f,1.0f,0.0f,1.0f };
		
		_simpleShader.SetUniformValue( _simpleShader.FindUniform( "WorldViewProj" ),ortho );
		_textureShader.SetUniformValue( _textureShader.FindUniform( "WorldViewProj" ),ortho );
		_texUnit=_textureShader.GetUniformTexture( _textureShader.FindUniform( "Texture0" ) );

		//Vertex buffers...
		//
		_vertBuf=new VertexBuffer( MAX_VERTS,VertexFormat.Float2,VertexFormat.UByte4N );

		_quadBuf=new VertexBuffer( MAX_VERTS,MAX_QUADS*6,VertexFormat.Float2,VertexFormat.UByte4N,VertexFormat.Float2 );
		ushort[] idxs=new ushort[MAX_QUADS*6];
		for( int i=0;i<MAX_QUADS;++i ){
			idxs[i*6+0]=(ushort)(i*4);
			idxs[i*6+1]=(ushort)(i*4+1);
			idxs[i*6+2]=(ushort)(i*4+2);
			idxs[i*6+3]=(ushort)(i*4);
			idxs[i*6+4]=(ushort)(i*4+2);
			idxs[i*6+5]=(ushort)(i*4+3);
		}
		_quadBuf.SetIndices( idxs );
	}
	
	public void Flush(){
		if( _vertCount==0 ) return;
		
		if( _primType==4 ){

			_quadBuf.SetVertices( 0,_verts,0,0,_vertCount );
			_quadBuf.SetVertices( 1,_colors,0,0,_vertCount );
		
			if( _primSurf!=null ){
				_quadBuf.SetVertices( 2,_texcs,0,0,_vertCount );
				_gc.SetShaderProgram( _textureShader );
				_gc.SetTexture( _texUnit,_primSurf._texture );
			}else{
				_gc.SetShaderProgram( _simpleShader );
			}
			_gc.SetVertexBuffer( 0,_quadBuf );
			_gc.DrawArrays( DrawMode.Triangles,0,_vertCount/4*6 );

		}else{

			_vertBuf.SetVertices( 0,_verts,0,0,_vertCount );
			_vertBuf.SetVertices( 1,_colors,0,0,_vertCount );
			
			_gc.SetShaderProgram( _simpleShader );
			_gc.SetVertexBuffer( 0,_vertBuf );
			
			switch( _primType ){
			case 1:_gc.DrawArrays( DrawMode.Points,0,_vertCount );break;
			case 2:_gc.DrawArrays( DrawMode.Lines,0,_vertCount );break;
			case 5:_gc.DrawArrays( DrawMode.TriangleFan,0,_vertCount );break;
			}
		}
		
		_vertCount=0;
	}
	
	//***** GXTK API *****
	
	public virtual int Width(){
		return _width;
	}
	
	public virtual int Height(){
		return _height;
	}
	
	public virtual bool LoadSurface__UNSAFE__( gxtkSurface surface,String path ){
		Texture2D texture=BBPsmGame.PsmGame().LoadTexture2D( path );
		if( texture==null ) return false;
		surface.SetTexture( texture );
		return true;
	}
	
	public virtual gxtkSurface LoadSurface( String path ){
		gxtkSurface surf=new gxtkSurface();
		if( !LoadSurface__UNSAFE__( surf,path ) ) return null;
		return surf;
	}
	
	public virtual gxtkSurface CreateSurface( int width,int height ){
		Texture2D texture=new Texture2D( width,height,false,PixelFormat.Rgba );
		if( texture!=null ) return new gxtkSurface( texture );
		return null;
	}
	
	public int BeginRender(){
		ImageRect r=_gc.GetViewport();
		_width=r.Width;
		_height=r.Height;
		_gc.Disable( EnableMode.All );
		_gc.Enable( EnableMode.Blend );
		_gc.SetBlendFunc( BlendFuncMode.Add,BlendFuncFactor.One,BlendFuncFactor.OneMinusSrcAlpha );
		return 1;
	}
	
	public void EndRender(){
		Flush();
		_gc.SwapBuffers();
	}
	
	public virtual int SetAlpha( float alpha ){
		_alpha=alpha;
		_color=((uint)(_alpha*255.0f)<<24) | ((uint)(_blue*_alpha)<<16) | ((uint)(_green*_alpha)<<8) | (uint)(_red*_alpha);
		return 0;
	}

	public virtual int SetColor( float r,float g,float b ){
		_red=r;
		_green=g;
		_blue=b;
		_color=((uint)(_alpha*255.0f)<<24) | ((uint)(_blue*_alpha)<<16) | ((uint)(_green*_alpha)<<8) | (uint)(_red*_alpha);
		return 0;
	}
	
	public virtual int SetBlend( int blend ){
		Flush();

		switch( blend ){
		case 1:
			_gc.SetBlendFunc( BlendFuncMode.Add,BlendFuncFactor.One,BlendFuncFactor.One );
			break;
		default:
			_gc.SetBlendFunc( BlendFuncMode.Add,BlendFuncFactor.One,BlendFuncFactor.OneMinusSrcAlpha );
			break;
		}
		return 0;
	}
	
	public virtual int SetMatrix( float ix,float iy,float jx,float jy,float tx,float ty ){
		_tformed=(ix!=1.0f || iy!=0.0f || jx!=0.0f || jy!=1.0f || tx!=0.0f || ty!=0.0f);
		_ix=ix;_iy=iy;
		_jx=jx;_jy=jy;
		_tx=tx;_ty=ty;
		return 0;
	}
	
	public virtual int SetScissor( int x,int y,int w,int h ){
		Flush();
		
		if( x!=0 || y!=0 || w!=Width() || h!=Height() ){
			_gc.Enable( EnableMode.ScissorTest );
			y=Height()-y-h;
			_gc.SetScissor( x,y,w,h );
		}else{
			_gc.Disable( EnableMode.ScissorTest );
		}
		return 0;
	}
	
	public virtual int Cls( float r,float g,float b ){
		_gc.SetClearColor( r/255.0f,g/255.0f,b/255.0f,1.0f );
		_gc.Clear();
		return 0;
	}

	public virtual int DrawPoint( float x,float y ){
		if( _primType!=1 || _vertCount==MAX_VERTS || _primSurf!=null ){
			Flush();
			_primType=1;
			_primSurf=null;
		}
	
		if( _tformed ){
			float tx=x;
			x=tx * _ix + y * _jx + _tx;
			y=tx * _iy + y * _jy + _ty;
		}
		
		int i=_vertCount*2,j=_vertCount;
		
		_verts[i ]=x;_verts[i+1]=y;_colors[j]=_color;
		
		_vertCount+=1;
		
		return 0;
	}
	
	public virtual int DrawRect( float x,float y,float w,float h ){
		if( _primType!=4 || _vertCount==MAX_VERTS || _primSurf!=null ){
			Flush();
			_primType=4;
			_primSurf=null;
		}
	
		float x0=x,x1=x+w,x2=x+w,x3=x;
		float y0=y,y1=y,y2=y+h,y3=y+h;
	
		if( _tformed ){
			float tx0=x0,tx1=x1,tx2=x2,tx3=x3;
			x0=tx0 * _ix + y0 * _jx + _tx; y0=tx0 * _iy + y0 * _jy + _ty;
			x1=tx1 * _ix + y1 * _jx + _tx; y1=tx1 * _iy + y1 * _jy + _ty;
			x2=tx2 * _ix + y2 * _jx + _tx; y2=tx2 * _iy + y2 * _jy + _ty;
			x3=tx3 * _ix + y3 * _jx + _tx; y3=tx3 * _iy + y3 * _jy + _ty;
		}
		
		int i=_vertCount*2,j=_vertCount;
		
		_verts[i  ]=x0;_verts[i+1]=y0;_colors[j  ]=_color;
		_verts[i+2]=x1;_verts[i+3]=y1;_colors[j+1]=_color;
		_verts[i+4]=x2;_verts[i+5]=y2;_colors[j+2]=_color;
		_verts[i+6]=x3;_verts[i+7]=y3;_colors[j+3]=_color;
		
		_vertCount+=4;

		return 0;
	}

	public virtual int DrawLine( float x0,float y0,float x1,float y1 ){
		if( _primType!=2 || _vertCount==MAX_VERTS || _primSurf!=null ){
			Flush();
			_primType=2;
			_primSurf=null;
		}
	
		if( _tformed ){
			float tx0=x0,tx1=x1;
			x0=tx0 * _ix + y0 * _jx + _tx;y0=tx0 * _iy + y0 * _jy + _ty;
			x1=tx1 * _ix + y1 * _jx + _tx;y1=tx1 * _iy + y1 * _jy + _ty;
		}
		
		int i=_vertCount*2,j=_vertCount;
		
		_verts[i+0]=x0;_verts[i+1]=y0;_colors[j+0]=_color;
		_verts[i+2]=x1;_verts[i+3]=y1;_colors[j+1]=_color;

		_vertCount+=2;
		
		return 0;
	}

	public virtual int DrawOval( float x,float y,float w,float h ){
		Flush();
		
		float xr=w/2.0f;
		float yr=h/2.0f;

		int segs;
		if( _tformed ){
			float dx_x=xr * _ix;
			float dx_y=xr * _iy;
			float dx=(float)Math.Sqrt( dx_x*dx_x+dx_y*dx_y );
			float dy_x=yr * _jx;
			float dy_y=yr * _jy;
			float dy=(float)Math.Sqrt( dy_x*dy_x+dy_y*dy_y );
			segs=(int)( dx+dy );
		}else{
			segs=(int)( Math.Abs( xr )+Math.Abs( yr ) );
		}
		segs=Math.Max( segs,12 ) & ~3;
		segs=Math.Min( segs,MAX_VERTS );

		float x0=x+xr,y0=y+yr;

		for( int i=0;i<segs;++i ){
		
			float th=-(float)i * (float)(Math.PI*2.0) / (float)segs;

			float px=x0+(float)Math.Cos( th ) * xr;
			float py=y0-(float)Math.Sin( th ) * yr;
			
			if( _tformed ){
				float ppx=px;
				px=ppx * _ix + py * _jx + _tx;
				py=ppx * _iy + py * _jy + _ty;
			}
			
			_verts[i*2]=px;_verts[i*2+1]=py;_colors[i]=_color;
		}
		
		_primType=5;
		_primSurf=null;
		_vertCount=segs;

		Flush();
		
		return 0;
	}
	
	public virtual int DrawPoly( float[] verts ){
		int n=verts.Length/2;
		if( n<1 || n>MAX_VERTS ) return 0;
		
		Flush();
		
		for( int i=0;i<n;++i ){
		
			float px=verts[i*2];
			float py=verts[i*2+1];
			
			if( _tformed ){
				float ppx=px;
				px=ppx * _ix + py * _jx + _tx;
				py=ppx * _iy + py * _jy + _ty;
			}
			
			_verts[i*2]=px;_verts[i*2+1]=py;_colors[i]=_color;
		}

		_primType=5;
		_primSurf=null;
		_vertCount=n;
		
		Flush();
		
		return 0;
	}
	
	public virtual int DrawPoly2( float[] verts,gxtkSurface surf,int srcx,int srcy ){
		int n=verts.Length/4;
		if( n<1 || n>MAX_VERTS ) return 0;
		
		Flush();
		
		for( int i=0;i<n;++i ){
		
			float px=verts[i*4];
			float py=verts[i*4+1];
			
			if( _tformed ){
				float ppx=px;
				px=ppx * _ix + py * _jx + _tx;
				py=ppx * _iy + py * _jy + _ty;
			}
			
			_verts[i*2]=px;_verts[i*2+1]=py;_colors[i]=_color;
		}

		_primType=5;
		_primSurf=null;
		_vertCount=n;
		
		Flush();
		
		return 0;
	}

	public virtual int DrawSurface( gxtkSurface surf,float x,float y ){
		if( _primType!=4 || _vertCount==MAX_VERTS || _primSurf!=surf ){
			Flush();
			_primType=4;
			_primSurf=surf;
		}
		
		float w=surf.Width();
		float h=surf.Height();
		float u0=0,u1=1,v0=0,v1=1;
		float x0=x,x1=x+w,x2=x+w,x3=x;
		float y0=y,y1=y,y2=y+h,y3=y+h;
		
		if( _tformed ){
			float tx0=x0,tx1=x1,tx2=x2,tx3=x3;
			x0=tx0 * _ix + y0 * _jx + _tx; y0=tx0 * _iy + y0 * _jy + _ty;
			x1=tx1 * _ix + y1 * _jx + _tx; y1=tx1 * _iy + y1 * _jy + _ty;
			x2=tx2 * _ix + y2 * _jx + _tx; y2=tx2 * _iy + y2 * _jy + _ty;
			x3=tx3 * _ix + y3 * _jx + _tx; y3=tx3 * _iy + y3 * _jy + _ty;
		}

		int i=_vertCount*2,j=_vertCount;
		
		_verts[i+0]=x0;_verts[i+1]=y0;_texcs[i+0]=u0;_texcs[i+1]=v0;_colors[j+0]=_color;
		_verts[i+2]=x1;_verts[i+3]=y1;_texcs[i+2]=u1;_texcs[i+3]=v0;_colors[j+1]=_color;
		_verts[i+4]=x2;_verts[i+5]=y2;_texcs[i+4]=u1;_texcs[i+5]=v1;_colors[j+2]=_color;
		_verts[i+6]=x3;_verts[i+7]=y3;_texcs[i+6]=u0;_texcs[i+7]=v1;_colors[j+3]=_color;
		
		_vertCount+=4;
		
		return 0;
	}

	public virtual int DrawSurface2( gxtkSurface surf,float x,float y,int srcx,int srcy,int srcw,int srch ){
		if( _primType!=4 || _vertCount==MAX_VERTS || _primSurf!=surf ){
			Flush();
			_primType=4;
			_primSurf=surf;
		}
		
		float w=surf.Width();
		float h=surf.Height();
		float u0=srcx/w,u1=(srcx+srcw)/w;
		float v0=srcy/h,v1=(srcy+srch)/h;
		float x0=x,x1=x+srcw,x2=x+srcw,x3=x;
		float y0=y,y1=y,y2=y+srch,y3=y+srch;
		
		if( _tformed ){
			float tx0=x0,tx1=x1,tx2=x2,tx3=x3;
			x0=tx0 * _ix + y0 * _jx + _tx; y0=tx0 * _iy + y0 * _jy + _ty;
			x1=tx1 * _ix + y1 * _jx + _tx; y1=tx1 * _iy + y1 * _jy + _ty;
			x2=tx2 * _ix + y2 * _jx + _tx; y2=tx2 * _iy + y2 * _jy + _ty;
			x3=tx3 * _ix + y3 * _jx + _tx; y3=tx3 * _iy + y3 * _jy + _ty;
		}
	
		int i=_vertCount*2,j=_vertCount;
		
		_verts[i+0]=x0;_verts[i+1]=y0;_texcs[i+0]=u0;_texcs[i+1]=v0;_colors[j+0]=_color;
		_verts[i+2]=x1;_verts[i+3]=y1;_texcs[i+2]=u1;_texcs[i+3]=v0;_colors[j+1]=_color;
		_verts[i+4]=x2;_verts[i+5]=y2;_texcs[i+4]=u1;_texcs[i+5]=v1;_colors[j+2]=_color;
		_verts[i+6]=x3;_verts[i+7]=y3;_texcs[i+6]=u0;_texcs[i+7]=v1;_colors[j+3]=_color;
		
		_vertCount+=4;
		
		return 0;
	}
	
	public virtual int ReadPixels( int[] pixels,int x,int y,int width,int height,int offset,int pitch ){

		Flush();
		
		byte[] data=new byte[width*height*4];
		
		_gc.ReadPixels( data,PixelFormat.Rgba,x,_height-y-height,width,height );
		
		int i=0;
		for( int py=height-1;py>=0;--py ){
			int j=offset+py*pitch;
			for( int px=0;px<width;++px ){
				pixels[j++]=(data[i+3]<<24)|(data[i]<<16)|(data[i+1]<<8)|data[i+2];
				i+=4;
			}
		}
		
		return 0;
	}
	
	public virtual int WritePixels2( gxtkSurface surface,int[] pixels,int x,int y,int width,int height,int offset,int pitch ){
	
		byte[] data=new byte[width*height*4];

		int i=0;
		for( int py=0;py<height;++py ){
			int j=offset+py*pitch;
			for( int px=0;px<width;++px ){
				int argb=pixels[j++];
				data[i  ]=(byte)(argb>>16);
				data[i+1]=(byte)(argb>>8 );
				data[i+2]=(byte)(argb);
				data[i+3]=(byte)(argb>>24);
				i+=4;
			}
		}
		
		surface._texture.SetPixels( 0,data,PixelFormat.Rgba,0,width*4,x,y,width,height );
		
		return 0;
	}
	
	public virtual void DiscardGraphics(){
	}
}

public class gxtkSurface{

	public Texture2D _texture;
	
	public gxtkSurface(){
	}
	
	public gxtkSurface( Texture2D texture ){
		_texture=texture;
	}
	
	public void SetTexture( Texture2D texture ){
		_texture=texture;
	}
	
	public virtual int Discard(){
		if( _texture!=null ){
			_texture.Dispose();
			_texture=null;
		}
		return 0;
	}
	
	public virtual int Width(){
		return _texture.Width;
	}
	
	public virtual int Height(){
		return _texture.Height;
	}
	
	public virtual void OnUnsafeLoadComplete(){
	}
}

public class gxtkChannel{
	public SoundPlayer player;
	public Sound sound;
	public float volume=1,pan=0,rate=1;
}

public class gxtkAudio{

	gxtkChannel[] _channels=new gxtkChannel[32];

	Bgm _music;
	BgmPlayer _musicPlayer;
	float _musicVolume=1;
	
	public gxtkAudio(){
		for( int i=0;i<32;++i ){
			_channels[i]=new gxtkChannel();
		}
	}
	
	//***** GXTK API *****
	
	public virtual int Suspend(){
		return 0;
	}
	
	public virtual int Resume(){
		return 0;
	}

	public virtual bool LoadSample__UNSAFE__( gxtkSample sample,String path ){
		Sound sound=BBPsmGame.PsmGame().LoadSound( path );
		if( sound==null ) return false;
		sample.SetSound( sound );
		return true;
	}
	
	public virtual gxtkSample LoadSample( String path ){
		gxtkSample sample=new gxtkSample();
		if( !LoadSample__UNSAFE__( sample,path ) ) return null;
		return sample;
	}
	
	public virtual int PlaySample( gxtkSample sample,int channel,int flags ){
		gxtkChannel chan=_channels[channel];
	
		for( int i=0;i<32;++i ){
			gxtkChannel chan2=_channels[i];
			if( chan2.sound==sample._sound && chan2.player.Status==SoundStatus.Stopped ){
				chan2.player.Dispose();
				chan2.player=null;
				chan2.sound=null;
			}
		}
		
		SoundPlayer player=sample._sound.CreatePlayer();
		if( player==null ) return -1;
		
		if( chan.player!=null ){
			chan.player.Stop();
			chan.player.Dispose();
		}
		
		player.Volume=chan.volume;
		player.Pan=chan.pan;
		player.PlaybackRate=chan.rate;
		player.Loop=(flags&1)!=0 ? true : false;
		player.Play();
		
		chan.player=player;
		chan.sound=sample._sound;
		
		return 0;
	}
	
	public virtual int StopChannel( int channel ){
		gxtkChannel chan=_channels[channel];
		
		if( chan.player!=null ){
			chan.player.Stop();
			chan.player.Dispose();
			chan.player=null;
			chan.sound=null;
		}
	
		return 0;
	}
	
	public virtual int PauseChannel( int channel ){
		gxtkChannel chan=_channels[channel];
		
		return -1;
	}
	
	public virtual int ResumeChannel( int channel ){
		gxtkChannel chan=_channels[channel];
		
		return -1;
	}
	
	public virtual int ChannelState( int channel ){
		gxtkChannel chan=_channels[channel];
		
		if( chan.player!=null && chan.player.Status==SoundStatus.Playing ) return 1;

		return 0;
	}
	
	public virtual int SetVolume( int channel,float volume ){
		gxtkChannel chan=_channels[channel];
		
		if( chan.player!=null ) chan.player.Volume=volume;
		chan.volume=volume;
		
		return 0;
	}
	
	public virtual int SetPan( int channel,float pan ){
		gxtkChannel chan=_channels[channel];

		if( chan.player!=null ) chan.player.Pan=pan;
		chan.pan=pan;

		return 0;
	}
	
	public virtual int SetRate( int channel,float rate ){
		gxtkChannel chan=_channels[channel];

		if( chan.player!=null ) chan.player.PlaybackRate=rate;
		chan.rate=rate;

		return 0;
	}
	
	public virtual int PlayMusic( String path,int flags ){
		StopMusic();
		
		_music=BBPsmGame.PsmGame().LoadBgm( path );
		if( _music==null ) return -1;
		
		_musicPlayer=_music.CreatePlayer();
		if( _musicPlayer==null ){
			_music=null;
			return -1;
		}
		
		_musicPlayer.Loop=(flags & 1)!=0;
		_musicPlayer.Volume=_musicVolume;
		_musicPlayer.Play();
		
		return -1;
	}
	
	public virtual int StopMusic(){
		if( _musicPlayer!=null ){
			_musicPlayer.Stop();
			_musicPlayer.Dispose();
			_musicPlayer=null;
			_music=null;
		}
		return 0;
	}
	
	public virtual int PauseMusic(){
		if( _musicPlayer!=null ) _musicPlayer.Pause();
		return 0;
	}
	
	public virtual int ResumeMusic(){
		if( _musicPlayer!=null ) _musicPlayer.Resume();
		return 0;
	}
	
	public virtual int MusicState(){
		if( _musicPlayer!=null ){
			if( _musicPlayer.Status==BgmStatus.Playing ) return 1;
			if( _musicPlayer.Status==BgmStatus.Paused ) return 2;
		}
		return 0;
	}
	
	public virtual int SetMusicVolume( float volume ){
		_musicVolume=volume;
		if( _musicPlayer!=null ) _musicPlayer.Volume=volume;
		return 0;
	}
}

public class gxtkSample{

	public Sound _sound;
	
	public gxtkSample(){
	}
	
	public gxtkSample( Sound sound ){
		_sound=sound;
	}
	
	public void SetSound( Sound sound ){
		_sound=sound;
	}

	//***** GXTK API *****

	public virtual int Discard(){
		if( _sound!=null ){
			_sound.Dispose();
			_sound=null;
		}
		return 0;
	}	
}
