
class BBThread implements Runnable{

	Object _result;
	boolean _running;
	Thread _thread;
	
	boolean IsRunning(){
		return _running;
	}
	
	void Start(){
		if( _running ) return;
		_result=null;
		_running=true;
		_thread=new Thread( this );
		_thread.start();
	}
	
	void SetResult( Object result ){
		_result=result;
	}
	
	Object Result(){
		return _result;
	}
	
	void Wait(){
		while( _running ){
			try{
				_thread.join();
			}catch( InterruptedException ex ){
			}
		}
	}
	
	void Run__UNSAFE__(){
	}

	public void run(){
		Run__UNSAFE__();
		_running=false;
	}
}
