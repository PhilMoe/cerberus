
// ***** gxtkGraphics.h *****

class gxtkSurface;

class gxtkGraphics : public Object{
public:
	gxtkGraphics();
	
	virtual int Width();
	virtual int Height();
	
	virtual int  BeginRender();
	virtual void EndRender();
	virtual void DiscardGraphics();
	
	virtual bool LoadSurface__UNSAFE__( gxtkSurface *surface,String path );
	virtual gxtkSurface *LoadSurface( String path );
	virtual gxtkSurface *CreateSurface( int width,int height );
	
	virtual int Cls( float red,float g,float b );
	virtual int SetAlpha( float alpha );
	virtual int SetColor( float r,float g,float b );
	virtual int SetMatrix( float ix,float iy,float jx,float jy,float tx,float ty );
	virtual int SetScissor( int x,int y,int width,int height );
	virtual int SetBlend( int blend );
	
	virtual int DrawPoint( float x,float y );
	virtual int DrawRect( float x,float y,float w,float h );
	virtual int DrawLine( float x1,float y1,float x2,float y2 );
	virtual int DrawOval( float x,float y,float w,float h );
	virtual int DrawPoly( Array<Float> verts );
	virtual int DrawPoly2( Array<Float> verts,gxtkSurface *surface,int srcx,int srcy );
	virtual int DrawSurface( gxtkSurface *surface,float x,float y );
	virtual int DrawSurface2( gxtkSurface *surface,float x,float y,int srcx,int srcy,int srcw,int srch );
	
	virtual int ReadPixels( Array<int> pixels,int x,int y,int width,int height,int offset,int pitch );
	virtual int WritePixels2( gxtkSurface *surface,Array<int> pixels,int x,int y,int width,int height,int offset,int pitch );
	
	// ***** INTERNAL *****
	struct Vertex{
		float x,y,u,v;
		unsigned int color;
	};
	
	struct ShaderConstants{
		DirectX::XMFLOAT4X4 projection;
	};

	enum{
		MAX_VERTS=1024,
		MAX_POINTS=MAX_VERTS,
		MAX_LINES=MAX_VERTS/2,
		MAX_QUADS=MAX_VERTS/4
	};
	
	Vertex *primVerts,*nextVert;
	int primType;
	gxtkSurface *primSurf;
	gxtkSurface *devPrimSurf;
	D3D11_PRIMITIVE_TOPOLOGY primTop;
	
	unsigned short quadIndices[MAX_QUADS*6]; 

	int gwidth,gheight;
	int dwidth,dheight;
	DirectX::XMFLOAT4X4 omatrix;
	
	float ix,iy,jx,jy,tx,ty;
	bool tformed;

	float r,g,b,alpha;
	unsigned int color;
	
	D3D11_RECT scissorRect;
	
	ShaderConstants shaderConstants;
	
	ComPtr<ID3D11Device1> d3dDevice;
	ComPtr<ID3D11DeviceContext1> d3dContext;
	
	// ***** D3d resources *****
	//
	ComPtr<ID3D11VertexShader> simpleVertexShader;
	ComPtr<ID3D11PixelShader> simplePixelShader;
	ComPtr<ID3D11VertexShader> textureVertexShader;
	ComPtr<ID3D11PixelShader> texturePixelShader;
	
	ComPtr<ID3D11InputLayout> inputLayout;
	ComPtr<ID3D11Buffer> vertexBuffer;
	ComPtr<ID3D11Buffer> indexBuffer;
	ComPtr<ID3D11Buffer> indexBuffer2;
	ComPtr<ID3D11Buffer> constantBuffer;
	
	ComPtr<ID3D11BlendState> alphaBlendState;
	ComPtr<ID3D11BlendState> additiveBlendState;
	ComPtr<ID3D11RasterizerState> rasterizerState;
	ComPtr<ID3D11DepthStencilState> depthStencilState;
	ComPtr<ID3D11SamplerState> samplerState;

	void MapVB();
	void UnmapVB();
	void CreateD3dResources();
	void Flush();
	void ValidateSize();
	D3D11_RECT DisplayRect( int x,int y,int w,int h );
	Vertex *Begin( int type,gxtkSurface *surf );

};

class gxtkSurface : public Object{
public:
	int seq;
	unsigned char *data;
	int width,height,format;
	float uscale,vscale;
	ComPtr<ID3D11Texture2D> texture;
	ComPtr<ID3D11ShaderResourceView> resourceView;

	gxtkSurface();
	~gxtkSurface();
	
	void SetData( unsigned char *data,int width,int height,int format );
	void SetSubData( int x,int y,int w,int h,unsigned *src,int pitch );
	
	void Validate();
	
	virtual int Width();
	virtual int Height();
	virtual int Discard();
	virtual bool OnUnsafeLoadComplete();
};

//***** gxtkGraphics.cpp *****

using namespace DirectX;
using namespace Windows::Graphics::Display;

static int graphics_seq=1;

gxtkGraphics::gxtkGraphics(){

	CreateD3dResources();
	
	ValidateSize();
	
	primType=0;
}

void gxtkGraphics::ValidateSize(){

	gwidth=dwidth=BBWinrtGame::WinrtGame()->GetDeviceWidthX();
	gheight=dheight=BBWinrtGame::WinrtGame()->GetDeviceHeightX();
	
	ZEROMEM( omatrix );
	
	int devrot=BBWinrtGame::WinrtGame()->GetDeviceRotationX();
	
	switch( devrot ){
	case 0:
		omatrix._11=omatrix._22=1;
		omatrix._33=omatrix._44=1;
		break;
	case 1:
		omatrix._11= 0;omatrix._12=-1;
		omatrix._21= 1;omatrix._22= 0;
		omatrix._33=omatrix._44=1;
		break;
	case 2:
		omatrix._11=-1;omatrix._12= 0;
		omatrix._21= 0;omatrix._22=-1;
		omatrix._33=omatrix._44=1;
		break;
	case 3:
		omatrix._11= 0;omatrix._12= 1;
		omatrix._21=-1;omatrix._22= 0;
		omatrix._33=omatrix._44=1;
		break;
	}
	
	if( devrot & 1 ){
		gwidth=dheight;
		gheight=dwidth;
	}
}

D3D11_RECT gxtkGraphics::DisplayRect( int x,int y,int width,int height ){

	int x0,y0,x1,y1;
	
	int devrot=BBWinrtGame::WinrtGame()->GetDeviceRotationX();

	switch( devrot ){
	case 0:
		x0=x;
		y0=y;
		x1=x0+width;
		y1=y0+height;
		break;
	case 1:
		x0=dwidth-y-height;
		y0=x;
		x1=x0+height;
		y1=y0+width;
		break;
	case 2:
		x0=dwidth-x-width;
		y0=dheight-y-height;
		x1=x0+width;
		y1=y0+height;
		break;
	case 3:
		x0=y;
		y0=dheight-x-width;
		x1=x0+height;
		y1=y0+width;
		break;
	}
	D3D11_RECT rect={x0,y0,x1,y1};
	return rect;
}

void gxtkGraphics::MapVB(){
	if( primVerts ) return;
	D3D11_MAPPED_SUBRESOURCE msr;
	d3dContext->Map( vertexBuffer.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&msr );
	primVerts=(Vertex*)msr.pData;
}

void gxtkGraphics::UnmapVB(){
	if( !primVerts ) return;
	d3dContext->Unmap( vertexBuffer.Get(),0 );
	primVerts=0;
}

void gxtkGraphics::Flush(){
	if( !primType ) return;

	int n=nextVert-primVerts;
	
	UnmapVB();
	
	if( primSurf!=devPrimSurf ){
		if( devPrimSurf=primSurf ){
			primSurf->Validate();
			d3dContext->VSSetShader( textureVertexShader.Get(),0,0 );
			d3dContext->PSSetShader( texturePixelShader.Get(),0,0 );
			d3dContext->PSSetShaderResources( 0,1,primSurf->resourceView.GetAddressOf() );
		}else{
			d3dContext->VSSetShader( simpleVertexShader.Get(),0,0 );
			d3dContext->PSSetShader( simplePixelShader.Get(),0,0 );
			d3dContext->PSSetShaderResources( 0,0,0 );
		}
	}
	
	switch( primType ){
	case 1:
		if( primTop!=D3D11_PRIMITIVE_TOPOLOGY_POINTLIST ){
			primTop=D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			d3dContext->IASetPrimitiveTopology( primTop );
		}
		d3dContext->Draw( n,0 );
		break;
	case 2:
		if( primTop!=D3D11_PRIMITIVE_TOPOLOGY_LINELIST ){
			primTop=D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			d3dContext->IASetPrimitiveTopology( primTop );
		}
		d3dContext->Draw( n,0 );
		break;
	case 3:
		if( primTop!=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST ){
			primTop=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			d3dContext->IASetPrimitiveTopology( primTop );
		}
		d3dContext->Draw( n,0 );
		break;
	case 4:
		if( primTop!=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST ){
			primTop=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			d3dContext->IASetPrimitiveTopology( primTop );
		}
		d3dContext->DrawIndexed( n/4*6,0,0 );
		break;
	default:
		if( primTop!=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST ){
			primTop=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			d3dContext->IASetPrimitiveTopology( primTop );
		}
		d3dContext->IASetIndexBuffer( indexBuffer2.Get(),DXGI_FORMAT_R16_UINT,0 );
		for( int j=0;j<n;j+=primType){
			d3dContext->DrawIndexed( (primType-2)*3,0,j );
		}
		d3dContext->IASetIndexBuffer( indexBuffer.Get(),DXGI_FORMAT_R16_UINT,0 );
		break;	
	}
	
	primType=0;
}

gxtkGraphics::Vertex *gxtkGraphics::Begin( int type,gxtkSurface *surf ){
	if( type!=primType || surf!=primSurf || (nextVert+type)-primVerts>MAX_VERTS ){
		Flush();
		MapVB();
		nextVert=primVerts;
		primType=type;
		primSurf=surf;
	}
	Vertex *v=nextVert;
	nextVert+=type;
	return v;
}

//***** gxtk API *****

int gxtkGraphics::Width(){
	return gwidth;
}

int gxtkGraphics::Height(){
	return gheight;
}

int gxtkGraphics::BeginRender(){

	if( BBWinrtGame::WinrtGame()->GetD3dDevice()!=d3dDevice.Get() ){
	
		++graphics_seq;
	
		CreateD3dResources();
	}
	
	d3dContext=BBWinrtGame::WinrtGame()->GetD3dContext();
	
	ValidateSize();
	
	XMStoreFloat4x4( 
		&shaderConstants.projection,
		XMMatrixTranspose( 
			XMMatrixMultiply( 
				XMMatrixOrthographicOffCenterLH( 0,Width(),Height(),0,-1,1 ),
				XMLoadFloat4x4( &omatrix ) ) ) );

	ID3D11RenderTargetView *rtv=BBWinrtGame::WinrtGame()->GetRenderTargetView();
	
	d3dContext->OMSetRenderTargets( 1,&rtv,0 );
	
	d3dContext->OMSetDepthStencilState( depthStencilState.Get(),0 );
	
	d3dContext->UpdateSubresource( constantBuffer.Get(),0,0,&shaderConstants,0,0 );
	
	UINT stride=sizeof( Vertex ),offset=0;
	d3dContext->IASetVertexBuffers( 0,1,vertexBuffer.GetAddressOf(),&stride,&offset );
	
	d3dContext->IASetIndexBuffer( indexBuffer.Get(),DXGI_FORMAT_R16_UINT,0 );

	d3dContext->IASetInputLayout( inputLayout.Get() );

	d3dContext->VSSetConstantBuffers( 0,1,constantBuffer.GetAddressOf()	);

	d3dContext->OMSetBlendState( alphaBlendState.Get(),0,~0 );

	d3dContext->VSSetShader( simpleVertexShader.Get(),0,0 );

	d3dContext->PSSetShader( simplePixelShader.Get(),0,0 );
	
	d3dContext->PSSetSamplers( 0,1,samplerState.GetAddressOf() );
	
	d3dContext->RSSetState( rasterizerState.Get() );
	
	d3dContext->RSSetScissorRects( 0,0 );
	
	D3D11_VIEWPORT viewport={ 0,0,dwidth,dheight,0,1 };
//	D3D11_VIEWPORT viewport={ 0,0,Width(),Height(),0,1 };
	d3dContext->RSSetViewports( 1,&viewport );
	
	primVerts=0;
	primType=0;
	primSurf=0;
	devPrimSurf=0;
	primTop=D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	
	ix=1;iy=0;jx=0;jy=1;tx=0;ty=0;tformed=false;
	
	r=255;g=255;b=255;alpha=1;color=0xffffffff;

	return 1;
}

void gxtkGraphics::EndRender(){

	Flush();
}

void gxtkGraphics::DiscardGraphics(){
}

bool gxtkGraphics::LoadSurface__UNSAFE__( gxtkSurface *surface,String path ){

	int width,height,format;
	unsigned char *data=BBWinrtGame::WinrtGame()->LoadImageData( path,&width,&height,&format );
	if( !data ) return 0;
	
	if( format==4 ){
		unsigned char *p=data;
		for( int n=width*height;n>0;--n ){ p[0]=p[0]*p[3]/255;p[1]=p[1]*p[3]/255;p[2]=p[2]*p[3]/255;p+=4; }
	}else if( format==3 ){
		unsigned char *out=(unsigned char*)malloc( width*height*4 );
		unsigned char *s=data,*d=out;
		for( int n=width*height;n>0;--n ){ *d++=*s++;*d++=*s++;*d++=*s++;*d++=255; }
		free( data );
		data=out;
		format=4;
	}else{
		bbPrint( String( "Bad image format: path=" )+path+", format="+format );
		free( data );
		return false;
	}
	
	surface->SetData( data,width,height,format );

	return true;
}

gxtkSurface *gxtkGraphics::LoadSurface( String path ){

	gxtkSurface *surf=new gxtkSurface();
	
	if( !LoadSurface__UNSAFE__( surf,path ) ) return 0;
	
	return surf;
}

gxtkSurface *gxtkGraphics::CreateSurface( int width,int height ){

	gxtkSurface *surface=new gxtkSurface;
	
	surface->SetData( 0,width,height,4 );
	
	return surface;
}

int gxtkGraphics::WritePixels2( gxtkSurface *surface,Array<int> pixels,int x,int y,int width,int height,int offset,int pitch ){

	surface->SetSubData( x,y,width,height,(unsigned*)&pixels[offset],pitch );
	
	return 0;
}

int gxtkGraphics::Cls( float r,float g,float b ){

	if( scissorRect.left!=0 || scissorRect.top!=0 || scissorRect.right!=gwidth || scissorRect.bottom!=gheight ){
	
		float x0=scissorRect.left;
		float x1=scissorRect.right;
		float y0=scissorRect.top;
		float y1=scissorRect.bottom;
		unsigned color=0xff000000 | (int(b)<<16) | (int(g)<<8) | int(r);
		
		MapVB();
		primType=4;
		primSurf=0;
		Vertex *v=primVerts;
		nextVert=v+4;
		
		v[0].x=x0;v[0].y=y0;v[0].color=color;
		v[1].x=x1;v[1].y=y0;v[1].color=color;
		v[2].x=x1;v[2].y=y1;v[2].color=color;
		v[3].x=x0;v[3].y=y1;v[3].color=color;
		
	}else{
	
		UnmapVB();
		primType=0;
		
		float rgba[]={ r/255.0f,g/255.0f,b/255.0f,1.0f };
		d3dContext->ClearRenderTargetView( BBWinrtGame::WinrtGame()->GetRenderTargetView(),rgba );
	}
	
	return 0;
}


int gxtkGraphics::SetAlpha( float alpha ){
	this->alpha=alpha;
	
	color=(int(alpha*255)<<24) | (int(b*alpha)<<16) | (int(g*alpha)<<8) | int(r*alpha);
	
	return 0;
}

int gxtkGraphics::SetColor( float r,float g,float b ){
	this->r=r;this->g=g;this->b=b;
	
	color=(int(alpha*255)<<24) | (int(b*alpha)<<16) | (int(g*alpha)<<8) | int(r*alpha);

	return 0;
}

int gxtkGraphics::SetMatrix( float ix,float iy,float jx,float jy,float tx,float ty ){

	this->ix=ix;this->iy=iy;this->jx=jx;this->jy=jy;this->tx=tx;this->ty=ty;

	tformed=(ix!=1 || iy!=0 || jx!=0 || jy!=1 || tx!=0 || ty!=0);

	return 0;
}

int gxtkGraphics::SetScissor( int x,int y,int width,int height ){

	Flush();
	
	scissorRect.left=x;scissorRect.top=y;
	scissorRect.right=x+width;scissorRect.bottom=y+height;
	
	D3D11_RECT rect=DisplayRect( x,y,width,height );
	d3dContext->RSSetScissorRects( 1,&rect );

	return 0;
}

int gxtkGraphics::SetBlend( int blend ){
	
	Flush();
	
	switch( blend ){
	case 1:
		d3dContext->OMSetBlendState( additiveBlendState.Get(),0,~0 );
		break;
	default:
		d3dContext->OMSetBlendState( alphaBlendState.Get(),0,~0 );
	}

	return 0;
}

int gxtkGraphics::DrawPoint( float x0,float y0 ){

	if( tformed ){
		float tx0=x0;
		x0=tx0 * ix + y0 * jx + tx;
		y0=tx0 * iy + y0 * jy + ty;
	}
	
	Vertex *v=Begin( 1,0 );

	v[0].x=x0+.5f;v[0].y=y0+.5f;v[0].color=color;

	return 0;
}

int gxtkGraphics::DrawLine( float x0,float y0,float x1,float y1 ){

	if( tformed ){
		float tx0=x0,tx1=x1;
		x0=tx0 * ix + y0 * jx + tx;
		y0=tx0 * iy + y0 * jy + ty;
		x1=tx1 * ix + y1 * jx + tx;
		y1=tx1 * iy + y1 * jy + ty;
	}

	Vertex *v=Begin( 2,0 );

	v[0].x=x0+.5f;v[0].y=y0+.5f;v[0].color=color;
	v[1].x=x1+.5f;v[1].y=y1+.5f;v[1].color=color;

	return 0;
}

int gxtkGraphics::DrawRect( float x,float y,float w,float h ){

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
	
	Vertex *v=Begin( 4,0 );
	
	v[0].x=x0;v[0].y=y0;v[0].color=color;
	v[1].x=x1;v[1].y=y1;v[1].color=color;
	v[2].x=x2;v[2].y=y2;v[2].color=color;
	v[3].x=x3;v[3].y=y3;v[3].color=color;
	
	return 0;
}

int gxtkGraphics::DrawOval( float x,float y,float w,float h ){

	float xr=w/2.0f;
	float yr=h/2.0f;

	int segs;
	if( tformed ){
		float dx_x=xr * ix;
		float dx_y=xr * iy;
		float dx=sqrtf( dx_x*dx_x+dx_y*dx_y );
		float dy_x=yr * jx;
		float dy_y=yr * jy;
		float dy=sqrtf( dy_x*dy_x+dy_y*dy_y );
		segs=(int)( dx+dy );
	}else{
		segs=(int)( abs( xr )+abs( yr ) );
	}
	
	if( segs<12 ){
		segs=12;
	}else if( segs>MAX_VERTS ){
		segs=MAX_VERTS;
	}else{
		segs&=~3;
	}
	
	x+=xr;
	y+=yr;
	
	Vertex *v=Begin( segs,0 );

	for( int i=0;i<segs;++i ){
	
		float th=i * 6.28318531f / segs;

		float x0=x+cosf( th ) * xr;
		float y0=y-sinf( th ) * yr;
		
		if( tformed ){
			float tx0=x0;
			x0=tx0 * ix + y0 * jx + tx;
			y0=tx0 * iy + y0 * jy + ty;
		}
		v[i].x=x0;v[i].y=y0;v[i].color=color;
	}

	return 0;
}

int gxtkGraphics::DrawPoly( Array<Float> verts ){
	int n=verts.Length()/2;
	if( n<1 || n>MAX_VERTS ) return 0;

	Vertex *v=Begin( n,0 );
	
	for( int i=0;i<n;++i ){
		float x0=verts[i*2];
		float y0=verts[i*2+1];
		if( tformed ){
			float tx0=x0;
			x0=tx0 * ix + y0 * jx + tx;
			y0=tx0 * iy + y0 * jy + ty;
		}
		v[i].x=x0;v[i].y=y0;v[i].color=color;
	}
	return 0;
}

int gxtkGraphics::DrawPoly2( Array<Float> verts,gxtkSurface *surface,int srcx,int srcy ){
	int n=verts.Length()/4;
	if( n<1 || n>MAX_VERTS ) return 0;

	Vertex *v=Begin( n,surface );
	
	for( int i=0;i<n;++i ){
		int j=i*4;
		float x0=verts[j];
		float y0=verts[j+1];
		if( tformed ){
			float tx0=x0;
			x0=tx0 * ix + y0 * jx + tx;
			y0=tx0 * iy + y0 * jy + ty;
		}
		v[i].x=x0;
		v[i].y=y0;
		v[i].u=(srcx+verts[j+2])*surface->uscale;
		v[i].v=(srcy+verts[j+3])*surface->vscale;
		v[i].color=color;
	}
	return 0;
}

int gxtkGraphics::DrawSurface( gxtkSurface *surf,float x,float y ){
	float w=surf->Width();
	float h=surf->Height();
	float x0=x,x1=x+w,x2=x+w,x3=x;
	float y0=y,y1=y,y2=y+h,y3=y+h;
	float u0=0,u1=w*surf->uscale;
	float v0=0,v1=h*surf->vscale;

	if( tformed ){
		float tx0=x0,tx1=x1,tx2=x2,tx3=x3;
		x0=tx0 * ix + y0 * jx + tx;y0=tx0 * iy + y0 * jy + ty;
		x1=tx1 * ix + y1 * jx + tx;y1=tx1 * iy + y1 * jy + ty;
		x2=tx2 * ix + y2 * jx + tx;y2=tx2 * iy + y2 * jy + ty;
		x3=tx3 * ix + y3 * jx + tx;y3=tx3 * iy + y3 * jy + ty;
	}
	
	Vertex *v=Begin( 4,surf );
	
	v[0].x=x0;v[0].y=y0;v[0].u=u0;v[0].v=v0;v[0].color=color;
	v[1].x=x1;v[1].y=y1;v[1].u=u1;v[1].v=v0;v[1].color=color;
	v[2].x=x2;v[2].y=y2;v[2].u=u1;v[2].v=v1;v[2].color=color;
	v[3].x=x3;v[3].y=y3;v[3].u=u0;v[3].v=v1;v[3].color=color;
	
	return 0;
}

int gxtkGraphics::DrawSurface2( gxtkSurface *surf,float x,float y,int srcx,int srcy,int srcw,int srch ){
	float w=srcw;
	float h=srch;
	float x0=x,x1=x+w,x2=x+w,x3=x;
	float y0=y,y1=y,y2=y+h,y3=y+h;
	float u0=srcx*surf->uscale,u1=(srcx+srcw)*surf->uscale;
	float v0=srcy*surf->vscale,v1=(srcy+srch)*surf->vscale;

	if( tformed ){
		float tx0=x0,tx1=x1,tx2=x2,tx3=x3;
		x0=tx0 * ix + y0 * jx + tx;y0=tx0 * iy + y0 * jy + ty;
		x1=tx1 * ix + y1 * jx + tx;y1=tx1 * iy + y1 * jy + ty;
		x2=tx2 * ix + y2 * jx + tx;y2=tx2 * iy + y2 * jy + ty;
		x3=tx3 * ix + y3 * jx + tx;y3=tx3 * iy + y3 * jy + ty;
	}
	
	Vertex *v=Begin( 4,surf );
	
	v[0].x=x0;v[0].y=y0;v[0].u=u0;v[0].v=v0;v[0].color=color;
	v[1].x=x1;v[1].y=y1;v[1].u=u1;v[1].v=v0;v[1].color=color;
	v[2].x=x2;v[2].y=y2;v[2].u=u1;v[2].v=v1;v[2].color=color;
	v[3].x=x3;v[3].y=y3;v[3].u=u0;v[3].v=v1;v[3].color=color;
	
	return 0;
}

int gxtkGraphics::ReadPixels( Array<int> pixels,int x,int y,int width,int height,int offset,int pitch ){

	Flush();
	
	ID3D11Resource *resource=0;
	BBWinrtGame::WinrtGame()->GetRenderTargetView()->GetResource( &resource );

	ID3D11Texture2D *backbuf=0;
	resource->QueryInterface( __uuidof(ID3D11Texture2D),(void**)&backbuf );

	D3D11_RECT r=DisplayRect( x,y,width,height );
	
	D3D11_TEXTURE2D_DESC txdesc;
	ZEROMEM( txdesc );
	txdesc.Width=r.right-r.left;
	txdesc.Height=r.bottom-r.top;
	txdesc.MipLevels=1;
	txdesc.ArraySize=1;
	txdesc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
	txdesc.SampleDesc.Count=1;
	txdesc.SampleDesc.Quality=0;
	txdesc.Usage=D3D11_USAGE_STAGING;
	txdesc.BindFlags=0;
	txdesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
	txdesc.MiscFlags=0;

	ID3D11Texture2D *texture=0;
	DXASS( d3dDevice->CreateTexture2D( &txdesc,0,&texture ) );
	
	D3D11_BOX box={r.left,r.top,0,r.right,r.bottom,1};
	d3dContext->CopySubresourceRegion( texture,0,0,0,0,backbuf,0,&box );
	
	D3D11_MAPPED_SUBRESOURCE msr;
	ZEROMEM( msr );
	d3dContext->Map( texture,0,D3D11_MAP_READ,0,&msr );
	
	unsigned char *pData=(unsigned char*)msr.pData;

	int devrot=BBWinrtGame::WinrtGame()->GetDeviceRotationX();

	if( devrot==0 ){
		for( int py=0;py<height;++py ){
			memcpy( &pixels[offset+py*pitch],pData+py*msr.RowPitch,width*4 );
		}
	}else if( devrot==1 ){
		for( int py=0;py<height;++py ){
			int *d=&pixels[offset+py*pitch];
			unsigned char *p=pData+(height-py-1)*4;
			for( int px=0;px<width;++px ){
				*d++=*(int*)p;
				p+=msr.RowPitch;
			}
		}
	}else if( devrot==2 ){
		for( int py=0;py<height;++py ){
			int *d=&pixels[offset+py*pitch];
			unsigned char *p=pData+(height-py-1)*msr.RowPitch+(width-1)*4;
			for( int px=0;px<width;++px ){
				*d++=*(int*)p;
				p-=4;
			}
		}
	}else if( devrot==3 ){
		for( int py=0;py<height;++py ){
			int *d=&pixels[offset+py*pitch];
			unsigned char *p=pData+(width-1)*msr.RowPitch+py*4;
			for( int px=0;px<width;++px ){
				*d++=*(int*)p;
				p-=msr.RowPitch;
			}
		}
	}

	d3dContext->Unmap( texture,0 );
	
	texture->Release();
	
	backbuf->Release();
	
	resource->Release();

	return 0;
}

static void *loadData( String path,int *sz ){
	FILE *f;
	if( _wfopen_s( &f,path.ToCString<wchar_t>(),L"rb" ) ) return 0;
	fseek( f,0,SEEK_END );
	int n=ftell( f );
	fseek( f,0,SEEK_SET );
	void *p=malloc( n );
	if( fread( p,1,n,f )!=n ) abort();
	fclose( f );
	*sz=n;
	return p;
}

void gxtkGraphics::CreateD3dResources(){

	d3dDevice=BBWinrtGame::WinrtGame()->GetD3dDevice();

	String path=Windows::ApplicationModel::Package::Current->InstalledLocation->Path;
	
	int sz;
	void *vs,*ps;

	vs=loadData( path+"/SimpleVertexShader.cso",&sz );
	if( !vs ) vs=loadData( path+"/Assets/SimpleVertexShader.cso",&sz );
	if( !vs ) abort();
	DXASS( d3dDevice->CreateVertexShader( vs,sz,0,&simpleVertexShader ) );
	free( vs );
	
	vs=loadData( path+"/TextureVertexShader.cso",&sz );
	if( !vs ) vs=loadData( path+"/Assets/TextureVertexShader.cso",&sz );
	if( !vs ) abort();
	DXASS( d3dDevice->CreateVertexShader( vs,sz,0,&textureVertexShader ) );
	const D3D11_INPUT_ELEMENT_DESC vertexDesc[]={
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	DXASS( d3dDevice->CreateInputLayout( vertexDesc,ARRAYSIZE(vertexDesc),vs,sz,&inputLayout ) );
	free( vs );

	ps=loadData( path+"/SimplePixelShader.cso",&sz );
	if( !ps ) ps=loadData( path+"/Assets/SimplePixelShader.cso",&sz );
	if( !ps ) abort();
	DXASS( d3dDevice->CreatePixelShader( ps,sz,0,&simplePixelShader ) );
	free( ps );

	ps=loadData( path+"/TexturePixelShader.cso",&sz );
	if( !ps ) ps=loadData( path+"/Assets/TexturePixelShader.cso",&sz );
	if( !ps ) abort();
	DXASS( d3dDevice->CreatePixelShader( ps,sz,0,&texturePixelShader ) );
	free( ps );

	D3D11_BUFFER_DESC vbdesc;
	ZEROMEM( vbdesc );
	vbdesc.ByteWidth=MAX_VERTS*sizeof( Vertex );
	vbdesc.Usage=D3D11_USAGE_DYNAMIC;
	vbdesc.BindFlags=D3D11_BIND_VERTEX_BUFFER;
	vbdesc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
	DXASS( d3dDevice->CreateBuffer( &vbdesc,0,&vertexBuffer ) );
	
	//Create quad index buffer
	D3D11_BUFFER_DESC ibdesc;
	ZEROMEM( ibdesc );
	ibdesc.ByteWidth=MAX_QUADS * 6 * sizeof( unsigned short );
	ibdesc.Usage=D3D11_USAGE_DEFAULT;
	ibdesc.BindFlags=D3D11_BIND_INDEX_BUFFER;
	//
	unsigned short *indices=new unsigned short[ MAX_QUADS*6 ];
	for( int i=0;i<MAX_QUADS;++i ){
		indices[i*6  ]=(unsigned short)(i*4);
		indices[i*6+1]=(unsigned short)(i*4+1);
		indices[i*6+2]=(unsigned short)(i*4+2);
		indices[i*6+3]=(unsigned short)(i*4);
		indices[i*6+4]=(unsigned short)(i*4+2);
		indices[i*6+5]=(unsigned short)(i*4+3);
	}
	D3D11_SUBRESOURCE_DATA ibdata;
	ZEROMEM( ibdata );
	ibdata.pSysMem=indices;
	//
	DXASS( d3dDevice->CreateBuffer( &ibdesc,&ibdata,&indexBuffer ) );
	//
	delete[] indices;
	
	//Create trifan index buffer
	D3D11_BUFFER_DESC ibdesc2;
	ZEROMEM( ibdesc2 );
	ibdesc2.ByteWidth=(MAX_VERTS-2) * 3 * sizeof( unsigned short );
	ibdesc2.Usage=D3D11_USAGE_DEFAULT;
	ibdesc2.BindFlags=D3D11_BIND_INDEX_BUFFER;
	//
	unsigned short *indices2=new unsigned short[ (MAX_VERTS-2)*3 ];
	for( int i=0;i<MAX_VERTS-2;++i ){
		indices2[i*3  ]=(unsigned short)0;
		indices2[i*3+1]=(unsigned short)(i+1);
		indices2[i*3+2]=(unsigned short)(i+2);
	}
	D3D11_SUBRESOURCE_DATA ibdata2;
	ZEROMEM( ibdata2 );
	ibdata2.pSysMem=indices2;
	//
	DXASS( d3dDevice->CreateBuffer( &ibdesc2,&ibdata2,&indexBuffer2 ) );
	//
	delete[] indices2;
	
	//Create shader consts buffer
	D3D11_BUFFER_DESC cbdesc;
	ZEROMEM( cbdesc );
	cbdesc.ByteWidth=sizeof( ShaderConstants );
	cbdesc.Usage=D3D11_USAGE_DEFAULT;
	cbdesc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
	DXASS( d3dDevice->CreateBuffer( &cbdesc,0,&constantBuffer ) );
	
	//Create alphaBlendState
	D3D11_BLEND_DESC abdesc;
	ZEROMEM( abdesc );
	abdesc.AlphaToCoverageEnable=FALSE;
	abdesc.IndependentBlendEnable=FALSE;
	abdesc.RenderTarget[0].BlendEnable=TRUE;
	abdesc.RenderTarget[0].SrcBlend=D3D11_BLEND_ONE;
	abdesc.RenderTarget[0].DestBlend=D3D11_BLEND_INV_SRC_ALPHA;
	abdesc.RenderTarget[0].BlendOp=D3D11_BLEND_OP_ADD;
	abdesc.RenderTarget[0].SrcBlendAlpha=D3D11_BLEND_ONE;
	abdesc.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_ZERO;
	abdesc.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;
	abdesc.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
	DXASS( d3dDevice->CreateBlendState( &abdesc,&alphaBlendState ) );
	
	//Additive blend state
	D3D11_BLEND_DESC pbdesc;
	ZEROMEM( pbdesc );
	memset( &pbdesc,0,sizeof(pbdesc) );
	pbdesc.AlphaToCoverageEnable=FALSE;
	pbdesc.IndependentBlendEnable=FALSE;
	pbdesc.RenderTarget[0].BlendEnable=TRUE;
	pbdesc.RenderTarget[0].SrcBlend=D3D11_BLEND_ONE;
	pbdesc.RenderTarget[0].DestBlend=D3D11_BLEND_ONE;
	pbdesc.RenderTarget[0].BlendOp=D3D11_BLEND_OP_ADD;
	pbdesc.RenderTarget[0].SrcBlendAlpha=D3D11_BLEND_ONE;
	pbdesc.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_ZERO;
	pbdesc.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;
	pbdesc.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
	DXASS( d3dDevice->CreateBlendState( &pbdesc,&additiveBlendState ) );
	
	//Create RasterizerState
	D3D11_RASTERIZER_DESC rsdesc;
	ZEROMEM( rsdesc );
	rsdesc.FillMode=D3D11_FILL_SOLID;
	rsdesc.CullMode=D3D11_CULL_NONE;
	rsdesc.DepthClipEnable=TRUE;
	rsdesc.ScissorEnable=TRUE;
	rsdesc.MultisampleEnable=FALSE;
	rsdesc.AntialiasedLineEnable=FALSE;
	DXASS( d3dDevice->CreateRasterizerState( &rsdesc,&rasterizerState ) );
	
	// Create DepthStencilState
	D3D11_DEPTH_STENCIL_DESC dsdesc;
	ZEROMEM( dsdesc );
	dsdesc.DepthEnable=FALSE;
	dsdesc.StencilEnable=FALSE;
	DXASS( d3dDevice->CreateDepthStencilState( &dsdesc,&depthStencilState ) );

	// Create SamplerState
	D3D11_SAMPLER_DESC ssdesc;
	ZEROMEM( ssdesc );
	ssdesc.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	ssdesc.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;
	ssdesc.AddressV=D3D11_TEXTURE_ADDRESS_CLAMP;
	ssdesc.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
	ssdesc.MinLOD=-FLT_MAX;
	ssdesc.MaxLOD=+FLT_MAX;
	ssdesc.MaxAnisotropy=16;
	ssdesc.ComparisonFunc=D3D11_COMPARISON_NEVER;
	DXASS( d3dDevice->CreateSamplerState( &ssdesc,&samplerState ) );
}

gxtkSurface::gxtkSurface():seq(0),data(0),width(0),height(0),format(0),uscale(0),vscale(0){
}

gxtkSurface::~gxtkSurface(){
	Discard();
}

void gxtkSurface::SetData( unsigned char *data,int width,int height,int format ){
	this->seq=0;
	this->data=data;
	this->width=width;
	this->height=height;
	this->format=format;
	this->uscale=1.0f/width;
	this->vscale=1.0f/height;
}

void gxtkSurface::SetSubData( int x,int y,int w,int h,unsigned *src,int pitch ){
	if( !data ) data=(unsigned char*)malloc( width*height*4 );

	unsigned *dst=(unsigned*)data+y*width+x;
	for( int py=0;py<h;++py ){
		unsigned *d=dst+py*width;
		unsigned *s=src+py*pitch;
		for( int px=0;px<w;++px ){
			unsigned p=*s++;
			unsigned a=p>>24;
			*d++=(a<<24) | ((p>>0&0xff)*a/255<<16) | ((p>>8&0xff)*a/255<<8) | ((p>>16&0xff)*a/255);
		}
	}

	if( seq==graphics_seq ){
		D3D11_BOX box={x,y,0,x+width,y+height,1};
		BBWinrtGame::WinrtGame()->GetD3dContext()->UpdateSubresource( texture.Get(),0,&box,dst,width*4,0 );
	}
}

void gxtkSurface::Validate(){

	if( seq==graphics_seq ) return;
	
	seq=graphics_seq;

	ID3D11Device1 *d3dDevice=BBWinrtGame::WinrtGame()->GetD3dDevice();	

	D3D11_TEXTURE2D_DESC txdesc;
	txdesc.Width=width;
	txdesc.Height=height;
	txdesc.MipLevels=1;
	txdesc.ArraySize=1;
	txdesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
	txdesc.SampleDesc.Count=1;
	txdesc.SampleDesc.Quality=0;
	txdesc.Usage=D3D11_USAGE_DEFAULT;
	txdesc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
	txdesc.CPUAccessFlags=0;
	txdesc.MiscFlags=0;
	
	if( data ){
		D3D11_SUBRESOURCE_DATA txdata={ data,width*4,0 };
		DXASS( d3dDevice->CreateTexture2D( &txdesc,&txdata,&texture ) );
	}else{
		DXASS( d3dDevice->CreateTexture2D( &txdesc,0,&texture ) );
	}
	
	D3D11_SHADER_RESOURCE_VIEW_DESC rvdesc;
	ZEROMEM( rvdesc );
	rvdesc.Format=txdesc.Format;
	rvdesc.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;
	rvdesc.Texture2D.MostDetailedMip=0;
	rvdesc.Texture2D.MipLevels=1;
	
	DXASS( d3dDevice->CreateShaderResourceView( texture.Get(),&rvdesc,&resourceView ) );
}

bool gxtkSurface::OnUnsafeLoadComplete(){
	return true;
}

int gxtkSurface::Discard(){
	free( data );
	data=nullptr;
	texture=nullptr;
	resourceView=nullptr;
	return 0;
}

int gxtkSurface::Width(){
	return width;
}

int gxtkSurface::Height(){
	return height;
}

//***** gxtkAudio.h *****

class gxtkSample;
class VoiceCallback;
class MediaEngineNotify;

struct gxtkChannel{
	int state,id;
	gxtkSample *sample;
	IXAudio2SourceVoice *voice;
	float volume;
	float pan;
	float rate;
	
	gxtkChannel():state(0),id(0),sample(0),voice(0),volume(1),pan(0),rate(1){}
};

class gxtkAudio : public Object{
public:

	static gxtkAudio *audio;

	IXAudio2 *xaudio2;
	IXAudio2MasteringVoice *masterVoice;
	
	gxtkChannel channels[33];
	
	MediaEngineNotify *mediaEngineNotify;
	IMFMediaEngine *mediaEngine;
	
	int musicState;
	bool musicLoop;
	float musicVolume;
	
	void MediaEvent( DWORD ev );
	
	gxtkAudio();
	
	virtual void mark();
	
	virtual int Suspend();
	virtual int Resume();
	
	virtual bool LoadSample__UNSAFE__( gxtkSample *sample,String path );
	virtual gxtkSample *LoadSample( String path );
	
	virtual int PlaySample( gxtkSample *sample,int channel,int flags );
	virtual int StopChannel( int channel );
	virtual int PauseChannel( int channel );
	virtual int ResumeChannel( int channel );
	virtual int ChannelState( int channel );
	virtual int SetVolume( int channel,float volume );
	virtual int SetPan( int channel,float pan );
	virtual int SetRate( int channel,float rate );

	virtual int PlayMusic( String path,int flags );
	virtual int StopMusic();
	virtual int PauseMusic();
	virtual int ResumeMusic();
	virtual int MusicState();
	virtual int SetMusicVolume( float volume );
};

class gxtkSample : public Object{
public:

	WAVEFORMATEX wformat;
	XAUDIO2_BUFFER xbuffer;
	VoiceCallback *callbacks[16];
	IXAudio2SourceVoice *voices[16];
	bool free[16],discarded,marked;
	
	static std::vector<gxtkSample*> discardedSamples;

	gxtkSample();
	~gxtkSample();
	
	virtual int Discard();
	
	void SetData( unsigned char *data,int length,int channels,int format,int hertz );
	
	int AllocVoice( bool loop );

	void Destroy();

	static void FlushDiscarded();
};

//***** gxtkAudio.cpp *****

gxtkAudio *gxtkAudio::audio;

class MediaEngineNotify : public IMFMediaEngineNotify{
    long _refs;
public:
	MediaEngineNotify():_refs( 1 ){
	}
	
	STDMETHODIMP QueryInterface( REFIID riid,void **ppv ){
		if( riid==__uuidof( IMFMediaEngineNotify ) ){
			*ppv=static_cast<IMFMediaEngineNotify*>(this);
		}else{
			*ppv=0;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}      
	
	STDMETHODIMP_(ULONG) AddRef(){
		return InterlockedIncrement( &_refs );
	}
	
	STDMETHODIMP_(ULONG) Release(){
		LONG refs=InterlockedDecrement( &_refs );
		if( !refs ) delete this;
		return refs;
	}

	STDMETHODIMP EventNotify( DWORD meEvent,DWORD_PTR param1,DWORD param2 ){
		if( meEvent==MF_MEDIA_ENGINE_EVENT_NOTIFYSTABLESTATE ){
			SetEvent( reinterpret_cast<HANDLE>( param1 ) );
		}else{
			gxtkAudio::audio->MediaEvent( meEvent );
		}
		return S_OK;
	}
};

gxtkAudio::gxtkAudio():musicState( 0 ),musicLoop( false ),musicVolume( 1.0 ){

	audio=this;

	DXASS( MFStartup( MF_VERSION ) );

	//Create xaudio2
	DXASS( XAudio2Create( &xaudio2,0,XAUDIO2_DEFAULT_PROCESSOR ) );
	DXASS( xaudio2->CreateMasteringVoice( &masterVoice ) );
	
	//Media engine attrs
	mediaEngineNotify=new MediaEngineNotify;
	
	IMFAttributes *attrs;
	DXASS( MFCreateAttributes( &attrs,1 ) );
	DXASS( attrs->SetUnknown( MF_MEDIA_ENGINE_CALLBACK,(IUnknown*)mediaEngineNotify ) );
	
	//Create media engine
	IMFMediaEngineClassFactory *factory;
	DXASS( CoCreateInstance( CLSID_MFMediaEngineClassFactory,0,CLSCTX_ALL,IID_PPV_ARGS( &factory ) ) );
	
#if WINDOWS_PHONE_8
	DXASS( factory->CreateInstance( 0,attrs,&mediaEngine ) );
#else
	DXASS( factory->CreateInstance( MF_MEDIA_ENGINE_AUDIOONLY,attrs,&mediaEngine ) );
#endif

	factory->Release();
	attrs->Release();
}

void gxtkAudio::MediaEvent( DWORD ev ){
	if( ev==MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA ){
//		bbPrint( "MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA" );
	}else if( ev==MF_MEDIA_ENGINE_EVENT_LOADEDDATA ){
//		bbPrint( "MF_MEDIA_ENGINE_EVENT_LOADEDDATA" );
	}else if( ev==MF_MEDIA_ENGINE_EVENT_CANPLAY ){
//		bbPrint( "MF_MEDIA_ENGINE_EVENT_CANPLAY" );
	}else if( ev==MF_MEDIA_ENGINE_EVENT_CANPLAYTHROUGH ){
//		bbPrint( "MF_MEDIA_ENGINE_EVENT_CANPLAYTHROUGH" );
	}else if( ev==MF_MEDIA_ENGINE_EVENT_ERROR ){
//		bbPrint( "MF_MEDIA_ENGINE_EVENT_ERROR" );
		musicState=0;
	}else{
//		bbPrint( String( "MF_MEDIA_ENGINE_EVENT:" )+(int)ev );
	}
}

void gxtkAudio::mark(){
	for( int i=0;i<32;++i ){
		gxtkChannel *chan=&channels[i];
		if( chan->state && chan->sample->free[chan->id] ) chan->state=0;
		if( chan->state ) gc_mark( chan->sample );
	}
}

bool gxtkAudio::LoadSample__UNSAFE__( gxtkSample *sample,String path ){
	int length=0,channels=0,format=0,hertz=0;
	
	unsigned char *data=BBWinrtGame::WinrtGame()->LoadAudioData( path,&length,&channels,&format,&hertz );
	if( !data ) return false;
	
	sample->SetData( data,length,channels,format,hertz );
	return true;
}

gxtkSample *gxtkAudio::LoadSample( String path ){

	gxtkSample::FlushDiscarded();
	
	gxtkSample *samp=new gxtkSample();
	
	if( !LoadSample__UNSAFE__( samp,path ) ) return 0;
	
	return samp;
}

int gxtkAudio::Suspend(){

	return 0;
}

int gxtkAudio::Resume(){

#if WINDOWS_PHONE_8	
	if( MusicState() ){
		//These appear to get 'lost' when app loses focus...?
		mediaEngine->SetLoop( musicLoop );
		mediaEngine->SetVolume( musicVolume );
		//Ditto, engine doesn't restart...?
		if( musicState==1 ) mediaEngine->Play();
	}
#endif
	
	return 0;
}

int gxtkAudio::PlaySample( gxtkSample *sample,int channel,int flags ){

	gxtkSample::FlushDiscarded();

	gxtkChannel *chan=&channels[channel];
	
	StopChannel( channel );
	
	int id=sample->AllocVoice( (flags&1)==1 );
	if( id<0 ) return -1;
	
	IXAudio2SourceVoice *voice=sample->voices[id];
	
	chan->state=1;
	chan->id=id;
	chan->sample=sample;
	chan->voice=voice;
	
	voice->SetVolume( chan->volume );
	voice->SetFrequencyRatio( chan->rate );
			
	voice->Start();
	return 0;
}

int gxtkAudio::StopChannel( int channel ){

	gxtkChannel *chan=&channels[channel];
	
	if( chan->state!=0 ){
		chan->voice->Stop( 0,0 );
		chan->voice->FlushSourceBuffers();
		chan->state=0;
	}

	return 0;
}

int gxtkAudio::PauseChannel( int channel ){

	gxtkChannel *chan=&channels[channel];
	
	if( chan->state==1 ){
		chan->voice->Stop( 0,0 );
		chan->state=2;
	}
	return 0;
}

int gxtkAudio::ResumeChannel( int channel ){

	gxtkChannel *chan=&channels[channel];
	
	if( chan->state==2 ){
		chan->voice->Start();
		chan->state=1;
	}

	return 0;
}

int gxtkAudio::ChannelState( int channel ){

	gxtkChannel *chan=&channels[channel];
	
	if( chan->state && chan->sample->free[chan->id] ) chan->state=0;
	
	return chan->state;
}

int gxtkAudio::SetVolume( int channel,float volume ){

	gxtkChannel *chan=&channels[channel];
	
	chan->volume=volume;
	
	if( chan->state ) chan->voice->SetVolume( volume );

	return 0;
}

int gxtkAudio::SetPan( int channel,float pan ){
	return 0;
}

int gxtkAudio::SetRate( int channel,float rate ){

	gxtkChannel *chan=&channels[channel];
	
	chan->rate=rate;
	
	if( chan->state ) chan->voice->SetFrequencyRatio( rate );

	return 0;
}

int gxtkAudio::PlayMusic( String path,int flags ){

	StopMusic();

	//should really be PathToUrl...?
	path=BBWinrtGame::WinrtGame()->PathToFilePath( path );
	
	int sz=path.Length()*2;
	int *p=(int*)malloc( 4+sz+2 );
	*p=sz;memcpy( p+1,path.ToCString<wchar_t>(),sz+2 );

	musicLoop=(flags&1)==1;

	mediaEngine->SetLoop( musicLoop );
	
	mediaEngine->SetVolume( musicVolume );
	
	mediaEngine->SetSource( (BSTR)(p+1) );
	
	mediaEngine->Play();
	
	musicState=1;
	
	free( p );
	
	return 0;
}

int gxtkAudio::StopMusic(){

	if( !musicState ) return 0;
	
	mediaEngine->Pause();
	
	musicState=0;

	return 0;
}

int gxtkAudio::PauseMusic(){

	if( musicState!=1 ) return 0;
	
	mediaEngine->Pause();
	
	musicState=2;
	
	return 0;
}

int gxtkAudio::ResumeMusic(){

	if( musicState!=2 ) return 0;
	
	mediaEngine->Play();
	
	musicState=1;
	
	return 0;
}

int gxtkAudio::MusicState(){

	if( musicState && !musicLoop && mediaEngine->IsEnded() ) musicState=0;
	
	return musicState;
}

int gxtkAudio::SetMusicVolume( float volume ){

#if WINDOWS_PHONE_8
	volume=pow( volume,.1 );
#endif

	musicVolume=volume;

	mediaEngine->SetVolume( musicVolume );

	return 0;
}

// ***** gxtkSample *****

std::vector<gxtkSample*> gxtkSample::discardedSamples;

class VoiceCallback : public IXAudio2VoiceCallback{
public:

	gxtkSample *sample;
	int id;

	VoiceCallback( gxtkSample *sample,int id ){
		this->sample=sample;
		this->id=id;
	}

	void _stdcall OnStreamEnd(){
	}
	
	void _stdcall OnVoiceProcessingPassEnd(){
	}
	
	void _stdcall OnVoiceProcessingPassStart( UINT32 SamplesRequired ){
	}
	
	void _stdcall OnBufferEnd( void *pBufferContext ){
//		OutputDebugStringA( "OnBufferEnd\n" );
		sample->free[id]=true;
	}
	
	void _stdcall OnBufferStart( void *pBufferContext ) {
	}
	
	void _stdcall OnLoopEnd( void *pBufferContext ){
	}
	
	void _stdcall OnVoiceError( void *pBufferContext,HRESULT Error ){
	}
};

gxtkSample::gxtkSample():discarded( false ){
	ZEROMEM( wformat );
	ZEROMEM( xbuffer );
	ZEROMEM( voices );
	ZEROMEM( callbacks );
	ZEROMEM( free );
}

gxtkSample::~gxtkSample(){
	if( !discarded ) Destroy();
}

void gxtkSample::SetData( unsigned char *data,int length,int channels,int format,int hertz ){
	wformat.wFormatTag=WAVE_FORMAT_PCM;
	wformat.nChannels=channels;
	wformat.nSamplesPerSec=hertz;
	wformat.nAvgBytesPerSec=channels*format*hertz;
	wformat.nBlockAlign=channels*format;
	wformat.wBitsPerSample=format*8;
	xbuffer.AudioBytes=length*channels*format;
	xbuffer.pAudioData=data;
}

int gxtkSample::AllocVoice( bool loop ){
	if( discarded ) return -1;

	int st=loop ? 8 : 0;
	
	for( int i=st;i<st+8;++i ){

		IXAudio2SourceVoice *voice=voices[i];
		
		if( !voice ){
			VoiceCallback *cb=new VoiceCallback( this,i );
			if( FAILED( gxtkAudio::audio->xaudio2->CreateSourceVoice( &voice,&wformat,0,XAUDIO2_DEFAULT_FREQ_RATIO,cb,0,0 ) ) ){
				delete cb;
				return 0;
			}
			callbacks[i]=cb;
			voices[i]=voice;
			
		}else if( !free[i] ){
			continue;
		}
		
		xbuffer.LoopCount=loop ? XAUDIO2_LOOP_INFINITE : 0;
		voice->SubmitSourceBuffer( &xbuffer,0 );
		free[i]=false;
		
		return i;
	}
	return -1;
}

int gxtkSample::Discard(){
	if( !discarded ){
		discardedSamples.push_back( this );
		discarded=true;
	}
	return 0;
}

void gxtkSample::Destroy(){
	for( int i=0;i<16;++i ){
		if( voices[i] ){
			voices[i]->DestroyVoice();
			delete callbacks[i];
		}
	}
	::free( (void*)xbuffer.pAudioData );
}

void gxtkSample::FlushDiscarded(){

	if( !discardedSamples.size() ) return;
	
	for( int i=0;i<discardedSamples.size();++i ){
		gxtkSample *sample=discardedSamples[i];
		sample->marked=false;
	}
	
	for( int i=0;i<32;++i ){
		gxtkChannel *chan=&gxtkAudio::audio->channels[i];
		if( chan->state && chan->sample->free[chan->id] ) chan->state=0;
		if( chan->state ) chan->sample->marked=true;
	}
	
	int out=0;
	for( int i=0;i<discardedSamples.size();++i ){
		gxtkSample *sample=discardedSamples[i];
		if( sample->marked ){
			discardedSamples[out++]=sample;
		}else{
			sample->Destroy();
		}
	}
	discardedSamples.resize( out );
}
