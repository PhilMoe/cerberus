
// ***** filestream.h *****

class BBFileStream : public BBStream{
public:

	BBFileStream();
	~BBFileStream();

	int Length();
	int Offset();

	int Seek( int offset );
	
	int Eof();
	void Close();
	int SkipBytes( int count );
	int ReadBytes( BBDataBuffer *buffer,int count,int offset );
	int WriteBytes( BBDataBuffer *buffer,int count,int offset );

	bool Open( String path,String mode );
	
private:
	FILE *_file;
	int _off,_len;
};

// ***** filestream.cpp *****

BBFileStream::BBFileStream():_file(0),_off(-1),_len(-1){
}

BBFileStream::~BBFileStream(){
	if( _file ) fclose( _file );
}

bool BBFileStream::Open( String path,String mode ){

	if( _file ) return false;
	
#if _WIN32	
	_file=_wfopen( path.ToCString<wchar_t>(),mode.ToCString<wchar_t>() );
#else
	_file=fopen( path.ToUTF8(),mode.ToUTF8() );
#endif

	if( !_file ) return false;
	
	fseek( _file,0,SEEK_END );
	_off=0;
	_len=ftell( _file );
	fseek( _file,0,SEEK_SET );
	
	return true;
}

int BBFileStream::Length(){
	return _len;
}

int BBFileStream::Offset(){
	return _off;
}

int BBFileStream::Seek( int offset ){
	if( !_file ) return -1;
	fseek( _file,0,SEEK_SET );
	_off=ftell( _file );
	return _off;
}

int BBFileStream::Eof(){
	return _file ? _off==_len : -1;
}

void BBFileStream::Close(){
	if( !_file ) return;
	fclose( _file );
	_file=0;
	_off=-1;
	_len=-1;
}

int BBFileStream::SkipBytes( int count ){
	if( !_file ) return 0;
	int n=0;
	char buf[1024];
	while( count ){
		int c=count<1024 ? count : 1024;
		int t=fread( buf,1,c,_file );
		count-=t;
		n+=t;
		if( t!=c ) break;
	}
	_off+=n;
	return n;
}

int BBFileStream::ReadBytes( BBDataBuffer *buffer,int count,int offset ){
	if( !_file ) return 0;
	int n=fread( buffer->WritePointer(offset),1,count,_file );
	_off+=n;
	return n;
}

int BBFileStream::WriteBytes( BBDataBuffer *buffer,int count,int offset ){
	if( !_file ) return 0;
	int n=fwrite( buffer->ReadPointer(offset),1,count,_file );
	_off+=n;
	if( _off>_len ) _len=_off;
	return n;
}
