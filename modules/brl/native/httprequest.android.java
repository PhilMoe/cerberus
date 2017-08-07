
class BBHttpRequest extends BBThread{

	HttpURLConnection _con;
	String _response;
	int _status;
	int _recv;
	
	String _sendText,_encoding;
	
	void Open( String req,String url,int timeout,boolean httpsVerifyCertificate, boolean httpsVerifyHost ){
		try{
			ConnectivityManager connMgr = (ConnectivityManager)BBAndroidGame.AndroidGame().GetActivity().getApplicationContext().getSystemService(Context.CONNECTIVITY_SERVICE);
			NetworkInfo networkInfo = connMgr.getActiveNetworkInfo();
			if( networkInfo != null && networkInfo.isConnected() )
			{
				URL turl=new URL( url );
				_con=(HttpURLConnection)turl.openConnection();
				_con.setRequestMethod( req );
				_con.setConnectTimeout(timeout);
				_con.setReadTimeout(timeout);
				_status = 1;
			}
    }
    catch( Exception ex ){
      _response="";
      _status=-1;
      _recv=0;
    }
  }
	
	void SetHeader( String name,String value ){
		_con.setRequestProperty( name,value );
	}
	
	void Send(){
		_sendText=_encoding=null;
		Start();
	}
	
	void SendText( String text,String encoding ){
		_sendText=text;
		_encoding=encoding;
		Start();
	}
	
	String ResponseText(){
		return _response;
	}
	
	int Status(){
		return _status;
	}
	
	void Run__UNSAFE__(){
		try{
			if( _sendText!=null ){
			
				byte[] bytes=_sendText.getBytes( "UTF-8" );

				_con.setDoOutput( true );
				_con.setFixedLengthStreamingMode( bytes.length );
				
				OutputStream out=_con.getOutputStream();
				out.write( bytes,0,bytes.length );
				out.close();
			}
			
			InputStream in=_con.getInputStream();

			byte[] buf=new byte[4096];
			ByteArrayOutputStream out=new ByteArrayOutputStream( 1024 );
			for(;;){
				int n=in.read( buf );
				if( n<0 ) break;
				out.write( buf,0,n );
				_recv+=n;
			}
			in.close();
			
			_response=new String( out.toByteArray(),"UTF-8" );

			_status=_con.getResponseCode();
			
		}catch( IOException ex ){
		}
		
		_con.disconnect();
	}
	
	int BytesReceived(){
		return _recv;
	}
}
