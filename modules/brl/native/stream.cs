
class BBStream{

	public virtual int Eof(){
		return 0;
	}
	
	public virtual void Close(){
	}
	
	public virtual int Length(){
		return 0;
	}
	
	public virtual int Position(){
		return 0;
	}
	
	public virtual int Seek( int position ){
		return 0;
	}
	
	public virtual int Read( BBDataBuffer buffer,int offset,int count ){
		return 0;
	}
	
	public virtual int Write( BBDataBuffer buffer,int offset,int count ){
		return 0;
	}
}
