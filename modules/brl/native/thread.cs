
class BBThread{

	private bool _running;
	private Thread _thread;
	
	public virtual void Start(){
		if( _running ) return;
		_running=true;
		_thread=new Thread( new ThreadStart( this.run ) );
		_thread.Start();
	}
	
	public virtual bool IsRunning(){
		return _running;
	}

	public virtual void Run__UNSAFE__(){
	}

	private void run(){
		Run__UNSAFE__();
		_running=false;
	}
}
