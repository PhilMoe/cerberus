
import tv.ouya.console.api.*;

import org.json.JSONException;
import org.json.JSONObject;

import java.security.KeyFactory;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.spec.X509EncodedKeySpec;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

class BBProduct{
	public boolean valid;
	public Product product;
	public String title;
	public String identifier;
	public String description;
	public String price;
	public int type;
	public boolean owned;
}

class BBMonkeyStore extends ActivityDelegate{

	Activity _activity;

	BBProduct[] _products;

	boolean _running;
	int _result;
	
	PublicKey _publicKey;
	
	class OpenStoreListener implements OuyaResponseListener<ArrayList<Product>>{

		public void onSuccess( ArrayList<Product> products ){
			for( int i=0;i<products.size();++i ){
				Product p=products.get( i );
				BBProduct q=FindProduct( p.getIdentifier() );
				if( q==null ) continue;
				q.product=p;
				q.title=p.getName();
				q.description=p.getName();
				q.price=String.valueOf( p.getPriceInCents() );
				q.valid=true;
			}
			_result=0;
			_running=false;
		}

		public void onFailure( int errorCode,String errorMessage,Bundle optionalData ){
			_running=false;
		}
		
		public void onCancel(){
			_running=false;
		}
	}
	
	class BuyProductListener implements OuyaResponseListener<String>{
	
		public void onSuccess( String response ){
			_result=0;
			_running=false;
		}

		public void onFailure( int errorCode,String errorMessage,Bundle optionalData ){
			_result=-1;
			_running=false;
		}
		
		public void onCancel(){
			_result=1;
			_running=false;
		}
	}
	
	class GetOwnedProductsListener implements OuyaResponseListener<String>{

		public void onSuccess( String receiptResponse ){
		
			OuyaEncryptionHelper helper=new OuyaEncryptionHelper();
			List<Receipt> receipts=null;
			try{
				JSONObject response=new JSONObject( receiptResponse );
				receipts=helper.decryptReceiptResponse( response,_publicKey );
				for( Receipt r : receipts ){
					BBProduct p=FindProduct( r.getIdentifier() );
					if( p!=null ) p.owned=true;
				}
				_result=0;
			}catch( Exception ex ){
			}
			_running=false;
		}
		
		public void onFailure( int errorCode,String errorMessage,Bundle optionalData ){
			_result=-1;
			_running=false;
		}
		
		public void onCancel(){
			_result=1;
			_running=false;
		}
	}
	
	public void onDestroy(){
		OuyaFacade.getInstance().shutdown();	
	}
	
	BBProduct FindProduct( String id ){
		for( int i=0;i<_products.length;++i ){
			if( _products[i].identifier.equals( id ) ) return _products[i];
		}
		return null;
	}

	public BBMonkeyStore(){
		_activity=BBAndroidGame.AndroidGame().GetActivity();
		OuyaFacade.getInstance().init( _activity,CerberusConfig.ANDROID_OUYA_DEVELOPER_UUID );
		BBAndroidGame.AndroidGame().AddActivityDelegate( this );
	}

	public void OpenStoreAsync( BBProduct[] products ){

		_result=-1;
		
		try{
			byte[] appKey=BBAndroidGame.AndroidGame().LoadData( "cerberus://data/key.der" );
			if( appKey!=null ){
				X509EncodedKeySpec keySpec=new X509EncodedKeySpec( appKey );
				KeyFactory keyFactory=KeyFactory.getInstance( "RSA" );
				_publicKey=keyFactory.generatePublic( keySpec );
			}
		}catch( Exception ex ){
		}
		if( _publicKey==null ){
			bb_std_lang.print( "Failed to create public key" );
			return;
		}
	
		_products=products;
		
		ArrayList<Purchasable> ps=new ArrayList<Purchasable>();
		
		for( int i=0;i<_products.length;++i ){
			ps.add( new Purchasable( _products[i].identifier ) );
		}
		
		_running=true;
		
		OuyaFacade.getInstance().requestProductList( ps,new OpenStoreListener() );
	}
	
	public void BuyProductAsync( BBProduct prod ){
	
		_result=-1;

		try{
			Product product=prod.product;
		
			SecureRandom sr=SecureRandom.getInstance( "SHA1PRNG" );
	
	        // This is an ID that allows you to associate a successful purchase with
	        // it's original request. The server does nothing with this string except
	        // pass it back to you, so it only needs to be unique within this instance
	        // of your app to allow you to pair responses with requests.
			String uniqueId=Long.toHexString( sr.nextLong() );
	
			JSONObject purchaseRequest = new JSONObject();
			purchaseRequest.put( "uuid",uniqueId );
			purchaseRequest.put( "identifier",product.getIdentifier() );
			// This value is only needed for testing, not setting it results in a live purchase
			purchaseRequest.put( "testing","true" ); 
			String purchaseRequestJson=purchaseRequest.toString();
	
			byte[] keyBytes=new byte[16];
			sr.nextBytes( keyBytes );
			SecretKey key=new SecretKeySpec( keyBytes,"AES" );
	
			byte[] ivBytes=new byte[16];
			sr.nextBytes( ivBytes );
			IvParameterSpec iv=new IvParameterSpec( ivBytes );
	
			Cipher cipher=Cipher.getInstance( "AES/CBC/PKCS5Padding","BC" );
			cipher.init( Cipher.ENCRYPT_MODE,key,iv );
			byte[] payload=cipher.doFinal( purchaseRequestJson.getBytes("UTF-8") );
	
			cipher=Cipher.getInstance( "RSA/ECB/PKCS1Padding","BC" );
			cipher.init( Cipher.ENCRYPT_MODE,_publicKey );
			byte[] encryptedKey=cipher.doFinal( keyBytes );
	
			Purchasable purchasable=new Purchasable(
				product.getIdentifier(),
				Base64.encodeToString( encryptedKey,Base64.NO_WRAP ),
				Base64.encodeToString( ivBytes,Base64.NO_WRAP ),
				Base64.encodeToString( payload,Base64.NO_WRAP ) );
	        
	        _running=true;
				
	        OuyaFacade.getInstance().requestPurchase( purchasable,new BuyProductListener() );
	        
       }catch( Exception ex ){
       }
	}

	public void GetOwnedProductsAsync(){

		_result=-1;
		
		_running=true;

		OuyaFacade.getInstance().requestReceipts( new GetOwnedProductsListener() );
	}

	public boolean IsRunning(){
		return _running;
	}
	
	public int GetResult(){
		return _result;
	}
}
