
// ***** appstore.h *****

#import <StoreKit/StoreKit.h>

class BBCerberusStore;

@interface BBCerberusStoreDelegate : NSObject<SKProductsRequestDelegate,SKPaymentTransactionObserver>{
@private
BBCerberusStore *_peer;
}
-(id)initWithPeer:(BBCerberusStore*)peer;
-(void)productsRequest:(SKProductsRequest*)request didReceiveResponse:(SKProductsResponse*)response;
-(void)paymentQueue:(SKPaymentQueue*)queue updatedTransactions:(NSArray*)transactions;
-(void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue*)queue;
-(void)paymentQueue:(SKPaymentQueue*)queue restoreCompletedTransactionsFailedWithError:(NSError*)error;
-(void)request:(SKRequest*)request didFailWithError:(NSError*)error;
@end

class BBProduct : public Object{
public:
	BBProduct();
	~BBProduct();
	SKProduct *product;
	
	bool valid;
	String title;
	String identifier;
	String description;
	String price;
	int type;
	bool owned;
	bool interrupted;
};

class BBCerberusStore : public Object{
public:
	BBCerberusStore();
	
	void OpenStoreAsync( Array<BBProduct*> products );
	void BuyProductAsync( BBProduct* product );
	void GetOwnedProductsAsync();
	
	bool IsRunning(){ return _running; }
	int GetResult(){ return _result; }
	
	void OnRequestProductDataResponse( SKProductsRequest *request,SKProductsResponse *response );
	void OnUpdatedTransactions( SKPaymentQueue *queue,NSArray *transactions );
	void OnRestoreTransactionsFinished( SKPaymentQueue *queue,NSError *error );
	void OnRequestFailed( SKRequest *request,NSError *error );
	
private:

	bool _running;
	int _result;
	
	Array<BBProduct*> _products;
	BBCerberusStoreDelegate *_delegate;
	NSNumberFormatter *_priceFormatter;
	
	virtual void mark();
	
	BBProduct *FindProduct( String id );
};

// ***** appstore.cpp *****

@implementation BBCerberusStoreDelegate

-(id)initWithPeer:(BBCerberusStore*)peer{
	if( self=[super init] ){
		_peer=peer;
	}
	return self;
}

-(void)productsRequest:(SKProductsRequest*)request didReceiveResponse:(SKProductsResponse*)response{
	_peer->OnRequestProductDataResponse( request,response );
}

-(void)paymentQueue:(SKPaymentQueue*)queue updatedTransactions:(NSArray*)transactions{
	_peer->OnUpdatedTransactions( queue,transactions );
}

-(void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue*)queue{
	_peer->OnRestoreTransactionsFinished( queue,0 );
}

-(void)paymentQueue:(SKPaymentQueue*)queue restoreCompletedTransactionsFailedWithError:(NSError*)error{
	_peer->OnRestoreTransactionsFinished( queue,error );
}

-(void)request:(SKRequest*)request didFailWithError:(NSError*)error{
	_peer->OnRequestFailed( request,error );
}

@end

BBProduct::BBProduct():product(0),valid(false),type(0),owned(false),interrupted(false){
}

BBProduct::~BBProduct(){

	[product release];
}

BBCerberusStore::BBCerberusStore():_running( false ),_products( 0 ),_result( -1 ){

	_delegate=[[BBCerberusStoreDelegate alloc] initWithPeer:this];
	
	[[SKPaymentQueue defaultQueue] addTransactionObserver:_delegate];
	
	_priceFormatter=[[NSNumberFormatter alloc] init];
	[_priceFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
	[_priceFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
}

void BBCerberusStore::OpenStoreAsync( Array<BBProduct*> products ){

	_result=-1;
	
	if( ![SKPaymentQueue canMakePayments] ) return;
	
	_products=products;
	
	id *objs=new id[products.Length()];
	for( int i=0;i<products.Length();++i ){
		objs[i]=products[i]->identifier.ToNSString();
	}
	
	
	NSSet *set=[NSSet setWithObjects:objs count:products.Length()];
	
	SKProductsRequest *request=[[SKProductsRequest alloc] initWithProductIdentifiers:set];
    request.delegate=_delegate;
    
    _running=true;

    [request start];
}

void BBCerberusStore::BuyProductAsync( BBProduct *prod ){

	_result=-1;

	if( ![SKPaymentQueue canMakePayments] ) return;
	
	SKMutablePayment *payment=[SKMutablePayment paymentWithProduct:prod->product];
	
	_running=true;
	
	[[SKPaymentQueue defaultQueue] addPayment:payment];
}

void BBCerberusStore::GetOwnedProductsAsync(){

	_result=-1;
	
	if( ![SKPaymentQueue canMakePayments] ) return;
	
	_running=true;
	
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

void BBCerberusStore::mark(){
	gc_mark( _products );
}

BBProduct *BBCerberusStore::FindProduct( String id ){
	for( int i=0;i<_products.Length();++i ){
		BBProduct *p=_products[i];
		if( p->identifier==id ) return p;
	}
	return 0;
}

void BBCerberusStore::OnRequestProductDataResponse( SKProductsRequest *request,SKProductsResponse *response ){

	//Get product details
	for( SKProduct *p in response.products ){
	
		BBProduct *prod=FindProduct( p.productIdentifier );
		if( !prod ) continue;
		
		[_priceFormatter setLocale:p.priceLocale];
		
		prod->valid=true;
		prod->product=[p retain];
		prod->title=p.localizedTitle;
		prod->identifier=p.productIdentifier;
		prod->description=p.localizedDescription;
		prod->price=[_priceFormatter stringFromNumber:p.price];
	}
	
	_result=0;
	
	_running=false;

	//Get owned products	
//	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

void BBCerberusStore::OnUpdatedTransactions( SKPaymentQueue *queue,NSArray *transactions ){

	_result=-1;

	for( SKPaymentTransaction *transaction in transactions ){
	
		if( transaction.transactionState==SKPaymentTransactionStatePurchased ){
		
			_result=0;
			
			_running=false;
			
		}else if( transaction.transactionState==SKPaymentTransactionStateFailed ){
		
			_result=(transaction.error.code==SKErrorPaymentCancelled) ? 1 : -1;
			
			_running=false;
			
		}else if( transaction.transactionState==SKPaymentTransactionStateRestored ){
		
			if( BBProduct *p=FindProduct( transaction.payment.productIdentifier ) ) p->owned=true;
		
		}else{
		
			continue;
		}
		
		[queue finishTransaction:transaction];
	}
}

void BBCerberusStore::OnRestoreTransactionsFinished( SKPaymentQueue *queue,NSError *error ){

	_result=error ? (error.code==SKErrorPaymentCancelled ? 1 : -1) : 0;
	
	_running=false;
}

void BBCerberusStore::OnRequestFailed( SKRequest *request,NSError *error ){

	_running=false;
}
