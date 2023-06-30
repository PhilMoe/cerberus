
// ***** filestream.h *****

class BBFileInputStream : public BBStream{
public:

	BBFileInputStream();
	~BBFileInputStream();

	void Close();
	int Eof();
	int Length();
	int Position();
	int Seek( int position );
	int Read( BBDataBuffer *buffer,int offset,int count );
	int Write( BBDataBuffer *buffer,int offset,int count );

	bool Open( String path,String mode );
	
private:
	FILE *_file;
	int _position;
	int _length;
};

// ***** fileinputstream.cpp *****

BBFileInputStream::BBFileInputStream():_file(0),_position(0),_length(0){
}

BBFileInputStream::~BBFileInputStream(){
	if( _file ) fclose( _file );
}

bool BBFileInputStream::Open( String path,String mode="r" ){
	if( _file ) return false;

	String fmode;	
	if( mode=="r" ){
		fmode="rb";
	}else if( mode=="w" ){
		fmode="wb";
	}else if( mode=="u" ){
		fmode="rb+";
	}else{
		return false;
	}

	_file=BBGame::Game()->OpenFile( path,fmode );
	if( !_file && mode=="u" ) _file=BBGame::Game()->OpenFile( path,"wb+" );
	if( !_file ) return false;
	
	fseek( _file,0,SEEK_END );
	_length=ftell( _file );
	fseek( _file,0,SEEK_SET );
	_position=0;
	
	return true;
}

void BBFileInputStream::Close(){
	if( !_file ) return;
	
	fclose( _file );
	_file=0;
	_position=0;
	_length=0;
}

int BBFileInputStream::Eof(){
	if( !_file ) return -1;
	
	return _position==_length;
}

int BBFileInputStream::Length(){
	return _length;
}

int BBFileInputStream::Position(){
	return _position;
}

int BBFileInputStream::Seek( int position ){
	if( !_file ) return 0;
	
	fseek( _file,position,SEEK_SET );
	_position=ftell( _file );
	return _position;
}

int BBFileInputStream::Read( BBDataBuffer *buffer,int offset,int count ){
	if( !_file ) return 0;
	
	int n=fread( buffer->WritePointer(offset),1,count,_file );
	_position+=n;
	return n;
}

int BBFileInputStream::Write( BBDataBuffer *buffer,int offset,int count ){
	if( !_file ) return 0;
	
	int n=fwrite( buffer->ReadPointer(offset),1,count,_file );
	_position+=n;
	if( _position>_length ) _length=_position;
	return n;
}
