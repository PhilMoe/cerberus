
class BBStream{

	int Eof(){
		return 0;
	}
	
	void Close(){
	}
	
	int Length(){
		return 0;
	}
	
	int Position(){
		return 0;
	}
	
	int Seek( int position ){
		return 0;
	}
	
	int Read( BBDataBuffer buffer,int offset,int count ){
		return 0;
	}
	
	int Write( BBDataBuffer buffer,int offset,int count ){
		return 0;
	}
}
