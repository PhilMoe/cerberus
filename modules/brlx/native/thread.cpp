
// ***** thread.h *****

class BBThread : public Object{
public:
	BBThread();
	~BBThread();
	
	virtual void Start();
	virtual bool IsRunning();
	virtual void Wait();
	
	virtual void Run()=0;
	
private:

	int _state;	//0=INIT, 1=RUNNING, 2=FINISHED.

#if _WIN32

	DWORD _id;
	HANDLE _handle;
	
	static DWORD WINAPI entry( void *p );
		
#else

	pthread_t handle;
	
	static void *entry( void *p );

#endif

};

// ***** thread.cpp *****

BBThread::BBThread():_state(0){
}

BBThread::~BBThread(){
	Wait();
}

bool BBThread::IsRunning(){
	return _state==1;
}

#if _WIN32

void BBThread::Start(){
	if( _state==1 ) return;
	
	if( _state==2 )	CloseHandle( _handle );

	_state=1;
	
	_handle=CreateThread( 0,0,entry,this,flags,&_id );
}

void BBThread::Wait(){
	if( !_state ) return;
	
	WaitForSingleObject( _handle,INFINITE );
	CloseHandle( _handle );
	
	_state=0;
}

DWORD WINAPI BBThread::entry( void *p ){
	BBThread *thread=(BBThread*)p;

	thread->Run();
	
	thread->_state=2;
	
	return 0;
}

#else

BBThread::BBThread():_state(0){
}

void BBThread::Start(){
	if( _state==1 ) return;
	
	if( _state==2 ) pthread_join( _handle,0 );
	
	_state=1;
	
	pthread_create( &_handle,0,entry,thread );
}

void BBThread::Wait(){
	if( !_state ) return;
	
	pthread_join( _handle,0 );
	
	_state=0;
}

void *BBThread::entry( void *p ){
	BBThread *thread=(BBThread*)p;

	thread->Run();
	
	thread->_state=2;
	
	return 0;
}

#endif
