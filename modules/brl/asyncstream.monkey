
#If LANG<>"cpp" And LANG<>"java"
#Error "Async streams are not available on this target"
#Endif

Import brl.stream
Import brl.asyncevent

Private

Import brl.thread
Import brl.pool

Global _readOpPool:=New Pool<AsyncReadOp>
Global _writeOpPool:=New Pool<AsyncWriteOp>
Global _readAllOpPool:=New Pool<AsyncReadAllOp>
Global _writeAllOpPool:=New Pool<AsyncWriteAllOp>

Class AsyncOp

	'Careful...called by background thread
	Method Execute__UNSAFE__:Void( stream:BBStream ) Abstract

	'Relax...called by main thread	
	Method Complete:Void( source:IAsyncEventSource ) Abstract

End

Class AsyncReadOp Extends AsyncOp

	Field data:DataBuffer
	Field offset:Int
	Field count:Int
	Field onComplete:IOnReadComplete

	Method Execute__UNSAFE__:Void( stream:BBStream )
		count=stream.Read( data,offset,count )
	End
	
	Method Complete:Void( source:IAsyncEventSource )
		onComplete.OnReadComplete( data,offset,count,source )
		_readOpPool.Free Self
	End
	
End

Class AsyncReadAllOp Extends AsyncReadOp

	Method Execute__UNSAFE__:Void( stream:BBStream )
		Local i:=0
		While i<count
			Local n:=stream.Read( data,offset+i,count-i )
			If Not n Exit
			i+=n
		Wend
		count=i
	End
	
	Method Complete:Void( source:IAsyncEventSource )
		onComplete.OnReadComplete( data,offset,count,source )
		_readAllOpPool.Free Self
	End
	
End

Class AsyncWriteOp Extends AsyncOp

	Field data:DataBuffer
	Field offset:Int
	Field count:Int
	Field onComplete:IOnWriteComplete
	
	Method Execute__UNSAFE__:Void( stream:BBStream )
		count=stream.Write( data,offset,count )
	End
	
	Method Complete:Void( source:IAsyncEventSource )
		onComplete.OnWriteComplete( data,offset,count,source )
		_writeOpPool.Free Self
	End

End

Class AsyncWriteAllOp Extends AsyncWriteOp

	Method Execute__UNSAFE__:Void( stream:BBStream )
		Local i:=0
		While i<count
			Local n:=stream.Write( data,offset+i,count-i )
			If Not n Exit
			i+=n
		Wend
		count=i
	End
	
	Method Complete:Void( source:IAsyncEventSource )
		onComplete.OnWriteComplete( data,offset,count,source )
		_writeAllOpPool.Free Self
	End

End

Class AsyncThread Extends Thread

	Const QUEUE_SIZE:=256			'how many ops can be queued. Overflow this and yer hosed.
	Const QUEUE_MASK:=QUEUE_SIZE-1

	Field stream:BBStream
	Field source:IAsyncEventSource
	Field queue:AsyncOp[QUEUE_SIZE]
	Field put:Int	'only written by Enqueue
	Field get:Int	'only written by Update
	Field nxt:Int	'only written by thread

	Method New( stream:BBStream,source:IAsyncEventSource )
		Self.stream=stream
		Self.source=source
	End
	
	Method Enqueue:Void( op:AsyncOp )
		queue[put]=op
		put=(put+1) Mod QUEUE_SIZE
		If put=get Error "AsyncThread queue overflow!"
		Start						'NOP if already running. Race condition alert! This will fail if thread is in the process of exiting!
	End
	
	Method Update:Void()
		If nxt<>put
			If Not IsRunning() Print "RACE!"
			Start			'NOP if already running. This is a kludge for the above race condition...
		Endif			
		While get<>nxt
			Local op:=queue[get]
			get=(get+1) Mod QUEUE_SIZE
			op.Complete source
		Wend
	End
	
	Private
	
	Method Run__UNSAFE__:Void()
		While nxt<>put
			queue[nxt].Execute__UNSAFE__ stream
			nxt=(nxt+1) Mod QUEUE_SIZE
		Wend
	End
	
End

Public

Interface IOnReadComplete
	Method OnReadComplete:Void( data:DataBuffer,offset:Int,count:Int,source:IAsyncEventSource )
End

Interface IOnWriteComplete
	Method OnWriteComplete:Void( data:DataBuffer,offset:Int,count:Int,source:IAsyncEventSource )
End

Class AsyncStream Implements IAsyncEventSource

	Method Read:Void( data:DataBuffer,offset:Int,count:Int,onComplete:IOnReadComplete )
		If Not _rthread Error "Not started"
		Local op:=_readOpPool.Allocate()
		op.data=data
		op.offset=offset
		op.count=count
		op.onComplete=onComplete
		_rthread.Enqueue op
	End
	
	Method ReadAll:Void( data:DataBuffer,offset:Int,count:Int,onComplete:IOnReadComplete )
		If Not _rthread Error "Not started"
		Local op:=_readAllOpPool.Allocate()
		op.data=data
		op.offset=offset
		op.count=count
		op.onComplete=onComplete
		_rthread.Enqueue op
	End
	
	Method Write:Void( data:DataBuffer,offset:Int,count:Int,onComplete:IOnWriteComplete )
		If Not _wthread Error "Not started"
		Local op:=_writeOpPool.Allocate()
		op.data=data
		op.offset=offset
		op.count=count
		op.onComplete=onComplete
		_wthread.Enqueue op
	End
	
	Method WriteAll:Void( data:DataBuffer,offset:Int,count:Int,onComplete:IOnWriteComplete )
		If Not _wthread Error "Not started"
		Local op:=_writeAllOpPool.Allocate()
		op.data=data
		op.offset=offset
		op.count=count
		op.onComplete=onComplete
		_wthread.Enqueue op
	End
	
	Method Close:Void()
		If _stream
			RemoveAsyncEventSource Self
			_rthread.Discard
			_wthread.Discard
			_stream.Close
			_rthread=Null
			_wthread=Null
			_stream=Null
		Endif
	End

	'IAsyncEventSource methods...	
	Method UpdateAsyncEvents:Void()
		If _rthread _rthread.Update
		If _wthread _wthread.Update
	End
	
	'***** INTERNAL *****
	Method Start:Void( stream:BBStream )
		_stream=stream
		_rthread=New AsyncThread( stream,Self )
		_wthread=New AsyncThread( stream,Self )
		AddAsyncEventSource Self
	End
	
	Private
	
	Field _stream:BBStream
	Field _rthread:AsyncThread
	Field _wthread:AsyncThread

End
