
//***** .h *****

class BBVertexBuffer{
public:
	BBVertexBuffer();
	
	bool _New( int length,Array<int> format,int flags );
	
	void SetData( int first,int count,BBDataBuffer *data,int offset );
	
	void GetData( int first,int count,BBDataBuffer *data,int offset );
	
private:
	struct AttribInfo{
		GLint size;
		GLenum type;
		int offset;
	}
	GLuint _glbuf;
	AttribInfo _attribs[32];
	int _length;
	int _count;
	int _flags;
	int _pitch;
};

class BBIndexBuffer{
public:
	BBIndexBuffer();
	
	bool _New( int length,int format,int flags );
	
	void SetData( int first,int count,BBDataBuffer *data,int offset );
	
	void GetData( int first,int count,BBDataBuffer *data,int offset );
	
private:
	GLuint _glbuf;
	GLenum _gltype;
	int _length;
	int _flags;
	int _pitch;
};

//***** VertexBuffer *****

BBVertexBuffer::BBVertexBuffer():_glbuf(0),_length(0),_count(0),_pitch(0),_flags(0){
}

bool BBVertexBuffer::_New( int length,Array<int> format,int flags ){
	if( _glbuf ) return false;
	
	_length=length;
	_count=format.Length()
	_flags=flags;
	_pitch=0;
	
	for( int i=0;i<_count;++i ){
		_attribs[i].offset=_pitch;
		switch( format[i] ){
		case 1:
			_attribs[i].size=3;
			_attribs[i].type=GL_FLOAT;
			_pitch+=12;
			break;
		case 2:
			_attribs[i].size=3;
			_attribs[i].type=GL_FLOAT;
			_pitch+=12;
			break;
		case 3:
			_attribs[i].size=4;
			_attribs[i].type=GL_FLOAT;
			_pitch+=16;
			break;
		case 4:
			_attribs[i].size=2;
			_attribs[i].type=GL_FLOAT;
			_pitch+=8;
			break;
		default:
			abort();
		}
	}
	
	glGenBuffers( 1,&_glbuf );
	glBindBuffer( GL_ARRAY_BUFFER,_glbuf );
	glBufferData( GL_ARRAY_BUFFER,_length*_pitch,0,GL_STATIC_DRAW );
	
	return true;
}

void BBVertexBuffer::SetData( int first,int count,BBDataBuffer *data,int offset ){
	glBindBuffer( GL_ARRAY_BUFFER,_glbuf );
	glBufferSubData( GL_ARRAY_BUFFER,first*_pitch,count*_pitch,data->ReadPointer(offset) );
}

void BBVertexBuffer::GetData( int first,int count,BBDataBuffer *data,int offset ){
	glBindBuffer( GL_ARRAY_BUFFER,_glbuf );
	glGetBufferSubData( GL_ARRAY_BUFFER,first*_pitch,count*_pitch,data->WritePointer(offset) );
}

//***** IndexBuffer *****

BBIndexBuffer::BBIndexBuffer():_glbuf(0),_glsize(0),_gltype(0),_length(0),_flags(0),_pitch(0){
}

bool BBIndexBuffer::_New( int length,int format,int flags ){
	if( _glbuf ) return false;
	
	_length=length;
	_flags=flags;
	
	switch( format ){
	case 1:
		_gltype=GL_UNSIGNED_BYTE;
		_pitch=1;
		break;
	case 2:
		_gltype=GL_UNSIGNED_SHORT;
		_pitch=2;
		break;
	case 3:
		_gltype=GL_UNSIGNED_INT;
		_pitch=4;
		break;
	default:
		abort();
	}
	
	glGenBuffers( 1,&_glbuf );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,_glbuf );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER,_length*_pitch,0,GL_STATIC_DRAW );
	
	return true;
}

void BBIndexBuffer::SetData( int first,int count,BBDataBuffer *data,int offset ){
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,_glbuf );
	glBufferSubData( GL_ELEMENT_ARRAY_BUFFER,first*_pitch,count*_pitch,data->ReadPointer(offset) );
}

void BBIndexBuffer::GetData( int first,int count,BBDataBuffer *data,int offset ){
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,_glbuf );
	glGetBufferSubData( GL_ELEMENT_ARRAY_BUFFER,first*_pitch,count*_pitch,data->WritePointer(offset) );
}


