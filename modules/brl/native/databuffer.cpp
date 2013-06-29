
// ***** databuffer.h *****

class BBDataBuffer : public Object{

public:
	
	BBDataBuffer();
	~BBDataBuffer();
	
	bool _New( int length );
	bool _Load( String path );
	void _LoadAsync( String path,BBThread *thread );

	const void *ReadPointer( int offset=0 );
	void *WritePointer( int offset=0 );
	
	void Discard();
	int Length();
	
	void PokeByte( int addr,int value );
	void PokeShort( int addr,int value );
	void PokeInt( int addr,int value );
	void PokeFloat( int addr,float value );
	
	int PeekByte( int addr );
	int PeekShort( int addr );
	int PeekInt( int addr );
	float PeekFloat( int addr );
	
private:
	signed char *_data;
	int _length;
};

// ***** databuffer.cpp *****

BBDataBuffer::BBDataBuffer():_data(0),_length(0){
}

BBDataBuffer::~BBDataBuffer(){
	if( _data ) free( _data );
}

bool BBDataBuffer::_New( int length ){
	if( _data ) return false;
	_data=(signed char*)malloc( length );
	_length=length;
	return true;
}

bool BBDataBuffer::_Load( String path ){
	if( _data ) return false;
	
	_data=(signed char*)BBGame::Game()->LoadData( path,&_length );
	if( !_data ) return false;
	
	return true;
}

void BBDataBuffer::_LoadAsync( String path,BBThread *thread ){
	if( _Load( path ) ) thread->SetResult( this );
}

const void *BBDataBuffer::ReadPointer( int offset ){
	return _data+offset;
}

void *BBDataBuffer::WritePointer( int offset ){
	return _data+offset;
}

void BBDataBuffer::Discard(){
	if( !_data ) return;
	free( _data );
	_data=0;
	_length=0;
}

int BBDataBuffer::Length(){
	return _length;
}

void BBDataBuffer::PokeByte( int addr,int value ){
	*(_data+addr)=value;
}

void BBDataBuffer::PokeShort( int addr,int value ){
	*(short*)(_data+addr)=value;
}

void BBDataBuffer::PokeInt( int addr,int value ){
	*(int*)(_data+addr)=value;
}

void BBDataBuffer::PokeFloat( int addr,float value ){
	*(float*)(_data+addr)=value;
}

int BBDataBuffer::PeekByte( int addr ){
	return *(_data+addr);
}

int BBDataBuffer::PeekShort( int addr ){
	return *(short*)(_data+addr);
}

int BBDataBuffer::PeekInt( int addr ){
	return *(int*)(_data+addr);
}

float BBDataBuffer::PeekFloat( int addr ){
	return *(float*)(_data+addr);
}
