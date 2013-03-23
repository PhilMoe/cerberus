
class BBVertexBuffer : public Object{
public:

	BBVertexBuffer():_desc(0),_pitch(0){
	}

	bool _New( int length,Array<int> format ){
	
		if( _desc ) return false;
		
		_desc=new D3D11_INPUT_ELEMENT_DESC[ format.Length() ];
		D3D11_INPUT_ELEMENT_DESC *p=_desc;
		
		memset( p,0,format.Length()*sizeof(*p) );
		_pitch=0;
		
		for( int i=0;i<format.Length();++i ){
		
			p->InputSlotClass=D3D11_INPUT_PER_VERTEX_DATA;
			p->AlignedByteOffset=_pitch;
		
			switch( format[i] ){
			case 1:
				p->SemanticName=L"POSITION";
				p->Format=DXGI_FORMAT_R32G32B32_FLOAT;
				_pitch+=12;
				break;
			case 2:
				p->SemanticName=L"NORMAL";
				p->Format=DXGI_FORMAT_R32G32B32_FLOAT;
				_pitch+=12;
				break;
			case 3:
				p->SemanticName=L"TANGENT";
				p->Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
				_pitch+=16;
				break;
			case 4:
				p->SemanticName=L"TEXTURE0";
				p->Format=DXGI_FORMAT_R32G32_FLOAT;
				_pitch+=8;
				break;
			default:
				abort();
			}
		}
		return true;
	}
	
	void SetData( int first,int count,BBDataBuffer *data,int offset,int pitch ){
	
		D3D11_BUFFER_DESC vbdesc={0};
		
		vbdesc.Usage=D3D11_USAGE_DEFAULT;
		vbdesc.ByteWidth=_length*_pitch;
		vbdesc.BindFlags=D3D11_BIND_VERTEX_BUFFER;
		vbdesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ|D3D11_CPU_ACCESS_WRITE;	//0;
		vbdesc.MiscFlags=0;
		
		
		
		D3D11_SUBRESOURCE_DATA vbdata={0};
		
		vertexBufferData.pSysMem=data->ReadPointer(0);
		vertexBufferData.SysMemPitch=0;
		vertexBufferData.SysMemSlicePitch=0;
		
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
				)
			);
	
	}
	
	void GetData( int first,int count,BBDataBuffer *data,int offset,int pitch ){
	}
	
private:
	int _length;
	int _pitch;
	D3D11_INPUT_ELEMENT_DESC *_elemDesc;
	
	Microsoft::WRL::ComPtr<ID3D11InputLayout> _inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;
};
