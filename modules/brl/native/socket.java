
class BBSocketAddress{
	InetSocketAddress sa;
	
	void Set( String host,int port ){
		sa=new InetSocketAddress( host,port );
	}
	
	void Set( BBSocketAddress address ){
		sa=address.sa;
	}
	
	void Set( InetSocketAddress sa ){
		this.sa=sa;
	}
	
	String Host(){
		return sa!=null ? sa.getHostName() : "";
	}
	
	int Port(){
		return sa!=null ? sa.getPort() : 0;
	}
}

class BBSocket{
	Socket _stream;
	ServerSocket _server;
	DatagramSocket _datagram;
	
	InputStream _input;
	OutputStream _output;
	
	DatagramPacket _recv;
	DatagramPacket _send;
	
	Socket _accepted;
	
	BBSocket(){
	}
	
	BBSocket( Socket stream ){
		_stream=stream;
		try{
			_input=_stream.getInputStream();
			_output=_stream.getOutputStream();
		}catch( IOException ex ){
		}			
	}
	
	boolean Open( int proto ){
		try{
			switch( proto ){
			case 1:
				_stream=new Socket();
				return true;
			case 2:
				_server=new ServerSocket();
				return true;
			case 3:
				_datagram=new DatagramSocket( null );
				_recv=new DatagramPacket( new byte[0],0 );
				_send=new DatagramPacket( new byte[0],0,new InetSocketAddress(0) );
				return true;
			}
		}catch( IOException ex ){
		}catch( SecurityException ex ){
		}
		return false;
	}
	
	void Close(){
	}
	
	boolean Bind( String host,int port ){
		try{
			InetSocketAddress addr=(host.length()!=0) ? new InetSocketAddress( host,port ) : new InetSocketAddress( port );
			if( _stream!=null ){
				_stream.bind( addr );
				return true;
			}else if( _server!=null ){
				_server.bind( addr );
				return true;
			}else if( _datagram!=null ){
				_datagram.bind( addr );
				return true;
			}
		}catch( IOException ex ){
		}
		return false;
	}
	
	
	boolean Connect( String host,int port ){
		try{
			if( _stream!=null ){
				_stream.connect( new InetSocketAddress( host,port ) );
				_input=_stream.getInputStream();
				_output=_stream.getOutputStream();
				return true;
			}else if( _datagram!=null ){
				_datagram.connect( new InetSocketAddress( host,port ) );
				return true;
			}
		}catch( IOException ex ){
		}
		return false;
	}
	
	boolean Listen( int backlog ){
		return _server!=null;
	}
	
	boolean Accept(){
		try{
			_accepted=_server.accept();
			if( _accepted!=null ) return true;
		}catch( IOException ex ){
		}
		return false;
	}
	
	BBSocket Accepted(){
		return _accepted!=null ? new BBSocket( _accepted ) : null;
	}
	
	int Send( BBDataBuffer data,int offset,int count ){
		try{
			if( _stream!=null ){
				_output.write( data._data.array(),offset,count );
				return count;
			}else if( _datagram!=null ){
				_send.setData( data._data.array(),offset,count );
				_send.setSocketAddress( _datagram.getRemoteSocketAddress() );
				_datagram.send( _send );
//				DatagramPacket p=new DatagramPacket( data._data.array(),offset,count,_datagram.getRemoteSocketAddress() );
//				_datagram.send( p );
				return count;
			}
		}catch( IOException ex ){
		}
		return 0;
	}

	int SendTo( BBDataBuffer data,int offset,int count,BBSocketAddress address ){
		try{
			if( _datagram!=null ){
				_send.setData( data._data.array(),offset,count );
				_send.setSocketAddress( address.sa );
				_datagram.send( _send );
//				DatagramPacket p=new DatagramPacket( data._data.array(),offset,count,address.sa );
	//			_datagram.send( p );
				return count;
			}
		}catch( IOException ex ){
		}
		return 0;
	}
	
	int Receive( BBDataBuffer data,int offset,int count ){
		try{
			if( _stream!=null ){
				int n=_input.read( data._data.array(),offset,count );
				if( n>=0 ) return n;
				return 0;
			}else if( _datagram!=null ){
				_recv.setData( data._data.array(),offset,count );
				_datagram.receive( _recv );
				return _recv.getLength();
//				DatagramPacket p=new DatagramPacket( data._data.array(),offset,count );
//				_datagram.receive( p );
//				return p.getLength();
			}
		}catch( IOException ex ){
		}
		return 0;
	}
	
	int ReceiveFrom( BBDataBuffer data,int offset,int count,BBSocketAddress address ){
		try{
			if( _datagram!=null ){
				_recv.setData( data._data.array(),offset,count );
				_datagram.receive( _recv );
				address.sa=(InetSocketAddress)_recv.getSocketAddress();
				return _recv.getLength();
//				DatagramPacket p=new DatagramPacket( data._data.array(),offset,count );
//				_datagram.receive( p );
//				address.sa=(InetSocketAddress)p.getSocketAddress();
//				return p.getLength();
			}
		}catch( IOException ex ){
		}
		return 0;
	}
	
	void GetLocalAddress( BBSocketAddress address ){
		if( _stream!=null ){
			address.sa=(InetSocketAddress)_stream.getLocalSocketAddress();
		}else if( _server!=null ){
			address.sa=(InetSocketAddress)_server.getLocalSocketAddress();
		}else if( _datagram!=null ){
			address.sa=(InetSocketAddress)_datagram.getLocalSocketAddress();
		}
	}
	
	void GetRemoteAddress( BBSocketAddress address ){
		if( _stream!=null ){
			address.sa=(InetSocketAddress)_stream.getRemoteSocketAddress();
		}else if( _server!=null ){
			address.sa=null;
		}else if( _datagram!=null ){
			address.sa=(InetSocketAddress)_datagram.getRemoteSocketAddress();
		}
	}
}
