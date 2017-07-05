
// XNA mojo runtime.
//
// Copyright 2011 Mark Sibly, all rights reserved.
// No warranty implied; use at your own risk.

public class gxtkGraphics{

	const int MAX_VERTS=1024;
	const int MAX_LINES=MAX_VERTS/2;
	const int MAX_QUADS=MAX_VERTS/4;

	GraphicsDevice device;
	
	int width,height;
	
	RenderTarget2D renderTarget;
	
	RasterizerState rstateScissor;
	Rectangle scissorRect;
	
	BasicEffect effect;
	
	int primType;
	int primCount;
	Texture2D primTex;

	VertexPositionColorTexture[] vertices;
	Int16[] quadIndices;
	Int16[] fanIndices;

	Color color;
	
	BlendState defaultBlend;
	BlendState additiveBlend;
	
	bool tformed=false;
	float ix,iy,jx,jy,tx,ty;
	
	public gxtkGraphics(){
		
		device=BBXnaGame.XnaGame().GetXNAGame().GraphicsDevice;
		
		width=device.PresentationParameters.BackBufferWidth;
		height=device.PresentationParameters.BackBufferHeight;
		
		effect=new BasicEffect( device );
		effect.VertexColorEnabled=true;

		vertices=new VertexPositionColorTexture[ MAX_VERTS ];
		for( int i=0;i<MAX_VERTS;++i ){
			vertices[i]=new VertexPositionColorTexture();
		}
		
		quadIndices=new Int16[ MAX_QUADS * 6 ];
		for( int i=0;i<MAX_QUADS;++i ){
			quadIndices[i*6  ]=(short)(i*4);
			quadIndices[i*6+1]=(short)(i*4+1);
			quadIndices[i*6+2]=(short)(i*4+2);
			quadIndices[i*6+3]=(short)(i*4);
			quadIndices[i*6+4]=(short)(i*4+2);
			quadIndices[i*6+5]=(short)(i*4+3);
		}
		
		fanIndices=new Int16[ MAX_VERTS * 3 ];
		for( int i=0;i<MAX_VERTS;++i ){
			fanIndices[i*3  ]=(short)(0);
			fanIndices[i*3+1]=(short)(i+1);
			fanIndices[i*3+2]=(short)(i+2);
		}

		rstateScissor=new RasterizerState();
		rstateScissor.CullMode=CullMode.None;
		rstateScissor.ScissorTestEnable=true;
		
		defaultBlend=BlendState.NonPremultiplied;
		
		//note: ColorSourceBlend must == AlphaSourceBlend in Reach profile!
		additiveBlend=new BlendState();
		additiveBlend.ColorBlendFunction=BlendFunction.Add;
		additiveBlend.ColorSourceBlend=Blend.SourceAlpha;
		additiveBlend.AlphaSourceBlend=Blend.SourceAlpha;
		additiveBlend.ColorDestinationBlend=Blend.One;
		additiveBlend.AlphaDestinationBlend=Blend.One;
	}

	public void Flush(){
		if( primCount==0 ) return;
		
		if( primTex!=null ){
	        effect.TextureEnabled=true;
    	    effect.Texture=primTex;
		}else{
	        effect.TextureEnabled=false;
		}

        foreach( EffectPass pass in effect.CurrentTechnique.Passes ){
            pass.Apply();

            switch( primType ){
			case 2:	//lines
				device.DrawUserPrimitives<VertexPositionColorTexture>(
				PrimitiveType.LineList,
				vertices,0,primCount );
				break;
			case 4:	//quads
				device.DrawUserIndexedPrimitives<VertexPositionColorTexture>(
				PrimitiveType.TriangleList,
				vertices,0,primCount*4,
				quadIndices,0,primCount*2 );
				break;
			case 5:	//trifan
				device.DrawUserIndexedPrimitives<VertexPositionColorTexture>(
				PrimitiveType.TriangleList,
				vertices,0,primCount,
				fanIndices,0,primCount-2 );
				break;
            }
        }
		primCount=0;
	}
	
	//***** GXTK API *****
	
	public virtual int Width(){
		return width;
	}
	
	public virtual int Height(){
		return height;
	}
	
	public virtual bool LoadSurface__UNSAFE__( gxtkSurface surface,String path ){
		Texture2D texture=BBXnaGame.XnaGame().LoadTexture2D( path );
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
		Texture2D texture=new Texture2D( device,width,height,false,SurfaceFormat.Color );
		if( texture!=null ) return new gxtkSurface( texture );
		return null;
	}
	
	public int BeginRender(){
	
		width=device.PresentationParameters.BackBufferWidth;
		height=device.PresentationParameters.BackBufferHeight;
	
		device.RasterizerState=RasterizerState.CullNone;
		device.DepthStencilState=DepthStencilState.None;
		device.BlendState=BlendState.NonPremultiplied;
		
		if( CerberusConfig.MOJO_IMAGE_FILTERING_ENABLED=="1" ){
			device.SamplerStates[0]=SamplerState.LinearClamp;
		}else{
			device.SamplerStates[0]=SamplerState.PointClamp;
		}
		
		if( CerberusConfig.MOJO_BACKBUFFER_ACCESS_ENABLED=="1" ){
			if( renderTarget!=null && (renderTarget.Width!=Width() || renderTarget.Height!=Height()) ){
				renderTarget.Dispose();
				renderTarget=null;
			}
			if( renderTarget==null ){
//				renderTarget=new RenderTarget2D( device,Width(),Height() );
				renderTarget=new RenderTarget2D( device,Width(),Height(),false,SurfaceFormat.Color,DepthFormat.None,0,RenderTargetUsage.PreserveContents );
			}
		}
		device.SetRenderTarget( renderTarget );
		
		effect.Projection=Matrix.CreateOrthographicOffCenter( +.5f,Width()+.5f,Height()+.5f,+.5f,0,1 );

		primCount=0;
		
		return 1;
	}

	public void EndRender(){
		Flush();
		
		if( renderTarget==null ) return;
		
		device.SetRenderTarget( null );
		
		device.BlendState=BlendState.Opaque;
		
		primType=4;
		primTex=renderTarget;
		
		float x=0,y=0;
		float w=Width();
		float h=Height();
		float u0=0,u1=1,v0=0,v1=1;
		float x0=x,x1=x+w,x2=x+w,x3=x;
		float y0=y,y1=y,y2=y+h,y3=y+h;
		
		Color color=Color.White;

		int vp=primCount++*4;
		vertices[vp  ].Position.X=x0;vertices[vp  ].Position.Y=y0;
		vertices[vp  ].TextureCoordinate.X=u0;vertices[vp  ].TextureCoordinate.Y=v0;
		vertices[vp  ].Color=color;
		vertices[vp+1].Position.X=x1;vertices[vp+1].Position.Y=y1;
		vertices[vp+1].TextureCoordinate.X=u1;vertices[vp+1].TextureCoordinate.Y=v0;
		vertices[vp+1 ].Color=color;
		vertices[vp+2].Position.X=x2;vertices[vp+2].Position.Y=y2;
		vertices[vp+2].TextureCoordinate.X=u1;vertices[vp+2].TextureCoordinate.Y=v1;
		vertices[vp+2].Color=color;
		vertices[vp+3].Position.X=x3;vertices[vp+3].Position.Y=y3;
		vertices[vp+3].TextureCoordinate.X=u0;vertices[vp+3].TextureCoordinate.Y=v1;
		vertices[vp+3].Color=color;
		
		Flush();
	}
	
	public virtual int SetAlpha( float alpha ){
		color.A=(byte)(alpha * 255);
		return 0;
	}

	public virtual int SetColor( float r,float g,float b ){
		color.R=(byte)r;
		color.G=(byte)g;
		color.B=(byte)b;
		return 0;
	}
	
	public virtual int SetBlend( int blend ){
		Flush();
	
		switch( blend ){
		case 1:
			device.BlendState=additiveBlend;
			break;
		default:
			device.BlendState=defaultBlend;
			break;
		}
		return 0;
	}
	
	public virtual int SetMatrix( float ix,float iy,float jx,float jy,float tx,float ty ){
	
		tformed=( ix!=1 || iy!=0 || jx!=0 || jy!=1 || tx!=0 || ty!=0 );
		
		this.ix=ix;this.iy=iy;
		this.jx=jx;this.jy=jy;
		this.tx=tx;this.ty=ty;

		return 0;
	}
	
	public virtual int SetScissor( int x,int y,int w,int h ){
		Flush();

		int r=Math.Min( x+w,Width() );
		int b=Math.Min( y+h,Height() );
		x=Math.Max( x,0 );
		y=Math.Max( y,0 );
		if( r>x && b>y ){
			w=r-x;
			h=b-y;
		}else{
			x=y=w=h=0;
		}
		
		if( x!=0 || y!=0 || w!=Width() || h!=Height() ){
			scissorRect.X=x;
			scissorRect.Y=y;
			scissorRect.Width=w;
			scissorRect.Height=h;
			device.RasterizerState=rstateScissor;
			device.ScissorRectangle=scissorRect;
		}else{
			device.RasterizerState=RasterizerState.CullNone;
		}
		
		return 0;
	}
	
	public virtual int Cls( float r,float g,float b ){

		if( device.RasterizerState.ScissorTestEnable ){

			Rectangle sr=device.ScissorRectangle;
			float x=sr.X,y=sr.Y,w=sr.Width,h=sr.Height;
			Color color=new Color( r/255.0f,g/255.0f,b/255.0f );
			
			primType=4;
			primCount=1;
			primTex=null;

			vertices[0].Position.X=x  ;vertices[0].Position.Y=y  ;vertices[0].Color=color;
			vertices[1].Position.X=x+w;vertices[1].Position.Y=y  ;vertices[1].Color=color;
			vertices[2].Position.X=x+w;vertices[2].Position.Y=y+h;vertices[2].Color=color;
			vertices[3].Position.X=x  ;vertices[3].Position.Y=y+h;vertices[3].Color=color;
		}else{
			primCount=0;
			device.Clear( new Color( r/255.0f,g/255.0f,b/255.0f ) );
		}
		return 0;
	}

	public virtual int DrawPoint( float x,float y ){
		if( primType!=4 || primCount==MAX_QUADS || primTex!=null ){
			Flush();
			primType=4;
			primTex=null;
		}
		
		if( tformed ){
			float px=x;
			x=px * ix + y * jx + tx;
			y=px * iy + y * jy + ty;
		}

		int vp=primCount++*4;
				
		vertices[vp  ].Position.X=x;vertices[vp  ].Position.Y=y;
		vertices[vp  ].Color=color;
		vertices[vp+1].Position.X=x+1;vertices[vp+1].Position.Y=y;
		vertices[vp+1].Color=color;
		vertices[vp+2].Position.X=x+1;vertices[vp+2].Position.Y=y+1;
		vertices[vp+2].Color=color;
		vertices[vp+3].Position.X=x;vertices[vp+3].Position.Y=y+1;
		vertices[vp+3].Color=color;
		
		return 0;
	}
	
	public virtual int DrawLine( float x0,float y0,float x1,float y1 ){
		if( primType!=2 || primCount==MAX_LINES || primTex!=null ){
			Flush();
			primType=2;
			primTex=null;
		}
		
		if( tformed ){
			float tx0=x0,tx1=x1;
			x0=tx0 * ix + y0 * jx + tx;
			y0=tx0 * iy + y0 * jy + ty;
			x1=tx1 * ix + y1 * jx + tx;
			y1=tx1 * iy + y1 * jy + ty;
		}
		
		int vp=primCount++*2;
		
		vertices[vp  ].Position.X=x0+.5f;vertices[vp  ].Position.Y=y0+.5f;
		vertices[vp  ].Color=color;
		vertices[vp+1].Position.X=x1+.5f;vertices[vp+1].Position.Y=y1+.5f;
		vertices[vp+1].Color=color;
		
		return 0;
	}

	
	
	public virtual int DrawRect( float x,float y,float w,float h ){
		if( primType!=4 || primCount==MAX_QUADS || primTex!=null ){
			Flush();
			primType=4;
			primTex=null;
		}
		
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

		int vp=primCount++*4;
				
		vertices[vp  ].Position.X=x0;vertices[vp  ].Position.Y=y0;
		vertices[vp  ].Color=color;
		vertices[vp+1].Position.X=x1;vertices[vp+1].Position.Y=y1;
		vertices[vp+1].Color=color;
		vertices[vp+2].Position.X=x2;vertices[vp+2].Position.Y=y2;
		vertices[vp+2].Color=color;
		vertices[vp+3].Position.X=x3;vertices[vp+3].Position.Y=y3;
		vertices[vp+3].Color=color;
		
		return 0;
	}

	public virtual int DrawOval( float x,float y,float w,float h ){
		Flush();
		primType=5;
		primTex=null;
		
		float xr=w/2.0f;
		float yr=h/2.0f;

		int segs;
		if( tformed ){
			float dx_x=xr * ix;
			float dx_y=xr * iy;
			float dx=(float)Math.Sqrt( dx_x*dx_x+dx_y*dx_y );
			float dy_x=yr * jx;
			float dy_y=yr * jy;
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
			
			if( tformed ){
				float ppx=px;
				px=ppx * ix + py * jx + tx;
				py=ppx * iy + py * jy + ty;
			}
			
			vertices[i].Position.X=px;vertices[i].Position.Y=py;
			vertices[i].Color=color;
		}
		
		primCount=segs;

		Flush();
		
		return 0;
	}
	
	public virtual int DrawPoly( float[] verts ){
		int n=verts.Length/2;
		if( n<1 || n>MAX_VERTS ) return 0;
		
		Flush();
		primType=5;
		primTex=null;
		
		for( int i=0;i<n;++i ){
		
			float px=verts[i*2];
			float py=verts[i*2+1];
			
			if( tformed ){
				float ppx=px;
				px=ppx * ix + py * jx + tx;
				py=ppx * iy + py * jy + ty;
			}
			
			vertices[i].Position.X=px;vertices[i].Position.Y=py;
			vertices[i].Color=color;
		}

		primCount=n;
		
		Flush();
		
		return 0;
	}
	
	public virtual int DrawPoly2( float[] verts,gxtkSurface surf,int srcx,int srcy ){
		int n=verts.Length/2;
		if( n<1 || n>MAX_VERTS ) return 0;
		
		Flush();
		primType=5;
		primTex=null;
		
		for( int i=0;i<n;++i ){
		
			float px=verts[i*4];
			float py=verts[i*4+1];
			
			if( tformed ){
				float ppx=px;
				px=ppx * ix + py * jx + tx;
				py=ppx * iy + py * jy + ty;
			}
			
			vertices[i].Position.X=px;vertices[i].Position.Y=py;
			vertices[i].Color=color;
		}

		primCount=n;
		
		Flush();
		
		return 0;
	}
	
	public virtual int DrawSurface( gxtkSurface surf,float x,float y ){
		if( primType!=4 || primCount==MAX_QUADS || surf.texture!=primTex ){
			Flush();
			primType=4;
			primTex=surf.texture;
		}
		
		float w=surf.Width();
		float h=surf.Height();
		float u0=0,u1=1,v0=0,v1=1;
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

		int vp=primCount++*4;
				
		vertices[vp  ].Position.X=x0;vertices[vp  ].Position.Y=y0;
		vertices[vp  ].TextureCoordinate.X=u0;vertices[vp  ].TextureCoordinate.Y=v0;
		vertices[vp  ].Color=color;
		vertices[vp+1].Position.X=x1;vertices[vp+1].Position.Y=y1;
		vertices[vp+1].TextureCoordinate.X=u1;vertices[vp+1].TextureCoordinate.Y=v0;
		vertices[vp+1 ].Color=color;
		vertices[vp+2].Position.X=x2;vertices[vp+2].Position.Y=y2;
		vertices[vp+2].TextureCoordinate.X=u1;vertices[vp+2].TextureCoordinate.Y=v1;
		vertices[vp+2].Color=color;
		vertices[vp+3].Position.X=x3;vertices[vp+3].Position.Y=y3;
		vertices[vp+3].TextureCoordinate.X=u0;vertices[vp+3].TextureCoordinate.Y=v1;
		vertices[vp+3].Color=color;
		
		return 0;
	}

	public virtual int DrawSurface2( gxtkSurface surf,float x,float y,int srcx,int srcy,int srcw,int srch ){
		if( primType!=4 || primCount==MAX_QUADS || surf.texture!=primTex ){
			Flush();
			primType=4;
			primTex=surf.texture;
		}
		
		float w=surf.Width();
		float h=surf.Height();
		float u0=srcx/w,u1=(srcx+srcw)/w;
		float v0=srcy/h,v1=(srcy+srch)/h;
		float x0=x,x1=x+srcw,x2=x+srcw,x3=x;
		float y0=y,y1=y,y2=y+srch,y3=y+srch;
		
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

		int vp=primCount++*4;
				
		vertices[vp  ].Position.X=x0;vertices[vp  ].Position.Y=y0;
		vertices[vp  ].TextureCoordinate.X=u0;vertices[vp  ].TextureCoordinate.Y=v0;
		vertices[vp  ].Color=color;
		vertices[vp+1].Position.X=x1;vertices[vp+1].Position.Y=y1;
		vertices[vp+1].TextureCoordinate.X=u1;vertices[vp+1].TextureCoordinate.Y=v0;
		vertices[vp+1 ].Color=color;
		vertices[vp+2].Position.X=x2;vertices[vp+2].Position.Y=y2;
		vertices[vp+2].TextureCoordinate.X=u1;vertices[vp+2].TextureCoordinate.Y=v1;
		vertices[vp+2].Color=color;
		vertices[vp+3].Position.X=x3;vertices[vp+3].Position.Y=y3;
		vertices[vp+3].TextureCoordinate.X=u0;vertices[vp+3].TextureCoordinate.Y=v1;
		vertices[vp+3].Color=color;
		
		return 0;
	}
	
	public virtual int ReadPixels( int[] pixels,int x,int y,int width,int height,int offset,int pitch ){
		Flush();
		
		Color[] data=new Color[width*height];

		device.SetRenderTarget( null );
		
		renderTarget.GetData( 0,new Rectangle( x,y,width,height ),data,0,data.Length );
		
		device.SetRenderTarget( renderTarget );
		
		int i=0;
		for( int py=0;py<height;++py ){
			int j=offset+py*pitch;
			for( int px=0;px<width;++px ){
				Color c=data[i++];
				pixels[j++]=(c.A<<24) | (c.R<<16) | (c.G<<8) | c.B;
			}
		}
		
		return 0;
	}
	
	public virtual int WritePixels2( gxtkSurface surface,int[] pixels,int x,int y,int width,int height,int offset,int pitch ){
	
		Color[] data=new Color[width*height];

		int i=0;
		for( int py=0;py<height;++py ){
			int j=offset+py*pitch;
			for( int px=0;px<width;++px ){
				int argb=pixels[j++];
				data[i++]=new Color( (argb>>16) & 0xff,(argb>>8) & 0xff,argb & 0xff,(argb>>24) & 0xff );
			}
		}
		
		surface.texture.SetData( 0,new Rectangle( x,y,width,height ),data,0,data.Length );
		
		return 0;
	}
	
	public virtual void DiscardGraphics(){
	}
}

//***** gxtkSurface *****

public class gxtkSurface{
	public Texture2D texture;
	
	public gxtkSurface(){
	}
	
	public gxtkSurface( Texture2D texture ){
		this.texture=texture;
	}
	
	public void SetTexture( Texture2D texture ){
		this.texture=texture;
	}
	
	//***** GXTK API *****
	
	public virtual int Discard(){
		texture=null;
		return 0;
	}
	
	public virtual int Width(){
		return texture.Width;
	}
	
	public virtual int Height(){
		return texture.Height;
	}
	
	public virtual int Loaded(){
		return 1;
	}
	
	public virtual bool OnUnsafeLoadComplete(){
		return true;
	}
}

public class gxtkChannel{
	public gxtkSample sample;
	public SoundEffectInstance inst;
	public float volume=1;
	public float pan=0;
	public float rate=1;
	public int state=0;
};

public class gxtkAudio{

	bool musicEnabled;
	int musicState;
	gxtkChannel[] channels=new gxtkChannel[33];
	
	public gxtkAudio(){
		
		musicEnabled=MediaPlayer.GameHasControl;
		
		for( int i=0;i<33;++i ){
			channels[i]=new gxtkChannel();
		}
	}
	
	//***** GXTK API *****
	//
	public virtual int Suspend(){
		if( musicEnabled && musicState==1 ){
			if( MediaPlayer.State==MediaState.Playing ){
				MediaPlayer.Pause();
				musicState=3;
			}else{
				musicState=0;
			}
		}
		for( int i=0;i<33;++i ){
			gxtkChannel chan=channels[i];
			if( chan.state!=1 ) continue;
			if( chan.inst.State!=SoundState.Playing ){
				chan.state=0;
				continue;
			}
			chan.inst.Pause();
			chan.state=3;
		}
		return 0;
	}
	
	public virtual int Resume(){
		for( int i=0;i<33;++i ){
			gxtkChannel chan=channels[i];
			if( chan.state!=3 ) continue;
			chan.inst.Resume();
			chan.state=1;
		}
		if( musicEnabled && musicState==3 ){
			MediaPlayer.Resume();
			musicState=1;
		}
		return 0;
	}

	public virtual bool LoadSample__UNSAFE__( gxtkSample sample,String path ){
		SoundEffect sound=BBXnaGame.XnaGame().LoadSoundEffect( path );
		if( sound==null ) return false;
		sample.SetSound( sound );
		return true;
	}
	
	public virtual gxtkSample LoadSample( String path ){
		gxtkSample samp=new gxtkSample();
		if( !LoadSample__UNSAFE__( samp,path ) ) return null;
		return samp;
	}
	
	public virtual int PlaySample( gxtkSample sample,int channel,int flags ){
		gxtkChannel chan=channels[channel];

		if( chan.state!=0 ) chan.inst.Stop();

		SoundEffectInstance inst=sample.AllocInstance( (flags&1)!=0 );
		if( inst==null ){
			chan.state=0;
			return -1;
		}
		
		for( int i=0;i<33;++i ){
			gxtkChannel chan2=channels[i];
			if( chan2.inst==inst ){
				chan2.sample=null;
				chan2.inst=null;
				chan2.state=0;
				break;
			}
		}
		
		inst.Volume=chan.volume;
		inst.Pan=chan.pan;
		inst.Pitch=(float)( Math.Log(chan.rate)/Math.Log(2) );
		inst.Play();

		chan.sample=sample;
		chan.inst=inst;
		chan.state=1;
		
		return 0;
	}
	
	public virtual int StopChannel( int channel ){
		gxtkChannel chan=channels[channel];
		
		if( chan.state!=0 ){
			chan.inst.Stop();
			chan.state=0;
		}
		return 0;
	}
	
	public virtual int PauseChannel( int channel ){
		gxtkChannel chan=channels[channel];
		
		if( chan.state==1 ){
			chan.inst.Pause();
			chan.state=2;
		}
		return 0;
	}
	
	public virtual int ResumeChannel( int channel ){
		gxtkChannel chan=channels[channel];
		
		if( chan.state==2 ){
			chan.inst.Resume();
			chan.state=1;
		}
		return 0;
	}
	
	public virtual int ChannelState( int channel ){
		gxtkChannel chan=channels[channel];
		
		if( chan.state==1 ){
			if( chan.inst.State!=SoundState.Playing ) chan.state=0;
		}else if( chan.state==3 ){
			return 1;
		}
		
		return chan.state;
	}
	
	public virtual int SetVolume( int channel,float volume ){
		gxtkChannel chan=channels[channel];
		
		if( chan.state!=0 ) chan.inst.Volume=volume;
		
		chan.volume=volume;
		return 0;
	}
	
	public virtual int SetPan( int channel,float pan ){
		gxtkChannel chan=channels[channel];
		
		if( chan.state!=0 ) chan.inst.Pan=pan;
		
		chan.pan=pan;
		return 0;
	}
	
	public virtual int SetRate( int channel,float rate ){
		gxtkChannel chan=channels[channel];
		
		if( chan.state!=0 ) chan.inst.Pitch=(float)( Math.Log(rate)/Math.Log(2) );
		
		chan.rate=rate;
		return 0;
	}
	
	public virtual int PlayMusic( String path,int flags ){
		if( !musicEnabled ) return -1;
		
		if( musicState==1 ) MediaPlayer.Stop();
		
		Song song=BBXnaGame.XnaGame().LoadSong( path );
		if( song==null ){
			musicState=0;
			return -1;
		}
		
		MediaPlayer.IsRepeating=(flags&1)!=0;
		
		MediaPlayer.Play( song );
		musicState=1;
		
		return 0;
	}
	
	public virtual int StopMusic(){
		if( !musicEnabled ) return -1;
		
		if( musicState!=0 ){
			MediaPlayer.Stop();
			musicState=0;
		}
		return 0;
	}
	
	public virtual int PauseMusic(){
		if( !musicEnabled ) return -1;
		
		if( musicState==1 ){
			MediaPlayer.Pause();
			musicState=2;
		}
		return 0;
	}
	
	public virtual int ResumeMusic(){
		if( !musicEnabled ) return -1;
		
		if( musicState==2 ){
			MediaPlayer.Resume();
			musicState=1;
		}
		return 0;
	}
	
	public virtual int MusicState(){
		if( !musicEnabled ) return -1;
		
		if( musicState==1 ){
			if( MediaPlayer.State!=MediaState.Playing ) musicState=0;
		}else if( musicState==3 ){
			return 1;
		}
		return musicState;
	}
	
	public virtual int SetMusicVolume( float volume ){
		if( !musicEnabled ) return -1;
		
		MediaPlayer.Volume=volume;
		
		return 0;
	}
}

public class gxtkSample{

	SoundEffect sound;
	
	//first 8 non-looped, second 8 looped.
	SoundEffectInstance[] insts=new SoundEffectInstance[16];	
	
	public gxtkSample(){
	}
	
	public gxtkSample( SoundEffect sound ){
		this.sound=sound;
	}
	
	public void SetSound( SoundEffect sound ){
		this.sound=sound;
	}

	public SoundEffectInstance AllocInstance( bool looped ){
		int st=looped ? 8 : 0;
		for( int i=st;i<st+8;++i ){
			SoundEffectInstance inst=insts[i];
			if( inst!=null ){
				if( inst.State!=SoundState.Playing ) return inst;
			}else{
				inst=sound.CreateInstance();
				inst.IsLooped=looped;
				insts[i]=inst;
				return inst;
			}
		}
		return null;
	}

	//***** GXTK API *****
	//
	public virtual int Discard(){	
		if( sound!=null ){
			sound=null;
			for( int i=0;i<16;++i ){
				insts[i]=null;
			}
		}
		return 0;
	}	
}
