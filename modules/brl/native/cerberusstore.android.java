
import org.json.*;//JSONObject;

import com.android.vending.billing.*;

class BBProduct{
	public boolean valid;
	public String title;
	public String identifier;
	public String description;
	public String price;
	public int type;
	public boolean owned;
	public boolean interrupted;
}

class BBCerberusStore extends ActivityDelegate implements ServiceConnection{

	Activity _activity;
	IInAppBillingService _service;
	Object _mutex=new Object();
	boolean _running;
	int _result=-1;
	int _reqCode;
	BBProduct[] _products;
	ArrayList unconsumed=new ArrayList();
	
	BBProduct FindProduct( String id ){
		for( int i=0;i<_products.length;++i ){
			if( id.equals( _products[i].identifier ) ) return _products[i];
		}
		return null;
	}
		
	class OpenStoreThread extends Thread{
	
		OpenStoreThread(){
			_running=true;
		}
		
		public void run(){
		
			//wait for service to start
			synchronized( _mutex ){
				while( _service==null ){
					try{
						_mutex.wait();
					}catch( InterruptedException ex ){
					
					}catch( IllegalMonitorStateException ex ){
					
					}
				}
			}
			
			int i0=0;
			while( i0<_products.length ){
			
				ArrayList list=new ArrayList();
				for( int i1=Math.min( i0+20,_products.length );i0<i1;++i0 ){
					list.add( _products[i0].identifier );
				}

				Bundle query=new Bundle();
				query.putStringArrayList( "ITEM_ID_LIST",list );
				
				_result=0;
	
				try{
	
					//Get product details
					Bundle details=_service.getSkuDetails( 3,_activity.getPackageName(),"inapp",query );
					ArrayList detailsList=details.getStringArrayList( "DETAILS_LIST" );
					
					if( detailsList==null ){
						_result=-1;
						_running=false;
						return;
					}
					
					for( int i=0;i<detailsList.size();++i ){
					
						JSONObject jobj=new JSONObject( (String)detailsList.get( i ) );
	
						BBProduct p=FindProduct( jobj.getString( "productId" ) );
						if( p==null ) continue;
	
						//strip (APP_NAME) from end of title					
						String title=jobj.getString( "title" );
						if( title.endsWith( ")" ) ){
							int j=title.lastIndexOf( " (" );
							if( j!=-1 ) title=title.substring( 0,j );
						}
						
						p.valid=true;
						p.title=title;
						p.description=jobj.getString( "description" );
						p.price=jobj.getString( "price" );
					}
					
					
					//Get owned products and consume consumables
					Bundle owned=_service.getPurchases( 3,_activity.getPackageName(),"inapp",null );
					ArrayList itemList=owned.getStringArrayList( "INAPP_PURCHASE_ITEM_LIST" );
					ArrayList dataList=owned.getStringArrayList( "INAPP_PURCHASE_DATA_LIST" );
	
					if( itemList==null || dataList==null ){
						_result=-1;
						_running=false;
						return;
					}
					
					//consume consumables
					for( int i=0;i<itemList.size();++i ){
					
						BBProduct p=FindProduct( (String)itemList.get( i ) );
						if( p==null ) continue;
						
						if( p.type==1 ){
	
							JSONObject jobj=new JSONObject( (String)dataList.get( i ) );
							int response=_service.consumePurchase( 3,_activity.getPackageName(),jobj.getString( "purchaseToken" ) );
							if( response!=0 ){
								p.valid=false;
								_result=-1;
								break;
							}
							p.interrupted=true;

						}else if( p.type==2 ){
	
							p.owned=true;
						}
					}
					
				}catch( RemoteException ex ){
					_result=-1;
				}catch( JSONException ex ){
					_result=-1;
				}
			}
			_running=false;
		}
	}
	
	class ConsumeProductThread extends Thread{
	
		String _token;
		
		ConsumeProductThread( String token ){
			_token=token;
		}
		
		public void run(){
		
			try{
				int response=_service.consumePurchase( 3,_activity.getPackageName(),_token );
				if( response==0 ) _result=0;
			}catch( RemoteException ex ){
			}
			
			_running=false;
		}
	}

	@Override
	public void onServiceDisconnected( ComponentName name ){
		_service=null;
	}

	@Override
	public void onServiceConnected( ComponentName name,IBinder service ){
		_service=IInAppBillingService.Stub.asInterface( service );

		BBAndroidGame.AndroidGame().AddActivityDelegate( this );
		
		_reqCode=BBAndroidGame.AndroidGame().AllocateActivityResultRequestCode();

		synchronized( _mutex ){
			try{
				_mutex.notify();
			}catch( IllegalMonitorStateException ex ){
			}
		}
	}

	@Override	
	public void onActivityResult( int requestCode,int resultCode,Intent data ){
	
		if( requestCode!=_reqCode ) return;

		if (resultCode == android.app.Activity.RESULT_CANCELED) {
			_result=1; 		//cancelled
			_running=false;
			return;
		}

//		bb_std_lang.print( "Buy result="+data.getIntExtra( "RESPONSE_CODE",12345 ) );
			
		int response=data.getIntExtra( "RESPONSE_CODE",0 );
		
		switch( response ){
		case 0:
			try{
				JSONObject pdata=new JSONObject( data.getStringExtra( "INAPP_PURCHASE_DATA" ) );
				BBProduct p=FindProduct( pdata.getString( "productId" ) );
				if( p!=null ){
					if( p.type==1 ){
						ConsumeProductThread thread=new ConsumeProductThread( pdata.getString( "purchaseToken" ) );
						thread.start();
						return;
					}else if( p.type==2 ){
						_result=0;
					}
				}
			}catch( JSONException ex ){
			}
			break;
		case 1:
			_result=1;	//cancelled
			break;
		case 7:
			_result=0;	//already purchased
			break;
		}

		_running=false;
	}

	// **** public *****	

	public BBCerberusStore(){

		_activity=BBAndroidGame.AndroidGame().GetActivity();
		
		Intent intent=new Intent( "com.android.vending.billing.InAppBillingService.BIND" );

		intent.setPackage( "com.android.vending" );
	
		_activity.bindService( intent,this,Context.BIND_AUTO_CREATE );
		
	}
	
	public void OpenStoreAsync( BBProduct[] products ){
	
		_products=products;
		
		OpenStoreThread thread=new OpenStoreThread();
		
		_result=-1;
		
		_running=true;

		thread.start();
	}
	
	public void BuyProductAsync( BBProduct p ){
	
		_result=-1;
	
		try{
			Bundle buy=_service.getBuyIntent( 3,_activity.getPackageName(),p.identifier,"inapp","NOP" );
			int response=buy.getInt( "RESPONSE_CODE" );
			
			if( response==0 ){
				
				PendingIntent intent=buy.getParcelable( "BUY_INTENT" );
				if( intent!=null ){
				
					Integer zero=Integer.valueOf( 0 );
					_activity.startIntentSenderForResult( intent.getIntentSender(),_reqCode,new Intent(),zero,zero,zero );
					
					_running=true;
					return;
				}
			}
			switch( response ){
			case 1:
				_result=1;	//cancelled
				break;
			case 7:
				_result=0;	//already purchased
				break;
			}
		}catch( IntentSender.SendIntentException ex ){
		}catch( RemoteException ex ){
		}
	}
	
	public void GetOwnedProductsAsync(){
	
		_result=0;
	}
	
	public boolean IsRunning(){

		return _running;
	}
	
	public int GetResult(){

		return _result;
	}
}
