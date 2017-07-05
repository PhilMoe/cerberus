
// ***** cerberusstore.h *****

#if CFG_WINRT_TEST_IAP && WINDOWS_8
#define CURRENT_APP CurrentAppSimulator
#define WINDOWS_8_TEST_IAP 1
#else
#define CURRENT_APP CurrentApp
#endif

class BBProduct : public Object{
public:
	BBProduct();
	~BBProduct();
	
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
	
private:

	Array<BBProduct*> _products;
	bool _running;
	int _result;
	
	void RequestPurchase( BBProduct *product );

#if WINDOWS_8

	void ConsumePurchase( Platform::String ^productId,Platform::Guid transactionId );

#endif

};

// ***** cerberusstore.cpp *****

using namespace Windows::ApplicationModel::Store;


// ***** BBProduct *****

BBProduct::BBProduct():valid(false),type(0),owned(false),interrupted(false){
}

BBProduct::~BBProduct(){
}

// ***** BBCerberusStore *****

BBCerberusStore::BBCerberusStore():_running(false),_result(-1){
}

void BBCerberusStore::OpenStoreAsync( Array<BBProduct*> products ){

	_products=products;
	_result=-1;
	_running=true;
	
#if WINDOWS_8_TEST_IAP
	 create_task( Package::Current->InstalledLocation->GetFolderAsync( "Assets" ) ).then( [this]( Windows::Storage::StorageFolder ^proxyDataFolder ){
		
		create_task( proxyDataFolder->GetFileAsync( "WindowsStoreProxy.xml" ) ).then( [this]( Windows::Storage::StorageFile ^proxyFile ){
	
			create_task( CURRENT_APP::ReloadSimulatorAsync( proxyFile ) ).then( [this](){
#endif
				create_task( CURRENT_APP::LoadListingInformationAsync() ).then( [this]( task<ListingInformation^> currentTask ){
				
					try{
					
						auto listingInfo=currentTask.get();
				
						auto licenseInfo=CURRENT_APP::LicenseInformation;
				
						for( int i=0;i<_products.Length();++i ){		
		
							auto product=_products[i];
							auto productId=product->identifier.ToWinRTString();
							if( !listingInfo->ProductListings->HasKey( productId ) ) continue;
	
							auto listing=listingInfo->ProductListings->Lookup( productId );
							auto license=licenseInfo->ProductLicenses->Lookup( productId );
							
							if( product->type==1 ){
								if( listing->ProductType!=ProductType::Consumable ) continue;
#if WINDOWS_PHONE_8								
								if( license->IsActive ){
									CURRENT_APP::ReportProductFulfillment( productId );
									product->interrupted=true;
								}
#endif							
							}else if( product->type==2 ){
								if( listing->ProductType!=ProductType::Durable ) continue;
								product->owned=license->IsActive;
							}
							
							product->title=listing->Name;
							product->price=listing->FormattedPrice;
							product->valid=true;
						}
#if WINDOWS_8
		                // recover already purchased consumables
						create_task( CURRENT_APP::GetUnfulfilledConsumablesAsync() ).then( [this]( task<Windows::Foundation::Collections::IVectorView<UnfulfilledConsumable^>^> currentTask ){
						
							try{
							
								auto products=currentTask.get();
								
								for( unsigned int i=0;i<products->Size;++i ){
								
									auto product= products->GetAt(i);
									
									for( int j=0;j<_products.Length();++j ){
										auto p=_products[j];
										if( p->identifier!=product->ProductId ) continue;
										ConsumePurchase( product->ProductId,product->TransactionId );
										p->interrupted=true;
										break;
									}
								}
								
								_result=0;
								
							}catch( Platform::Exception ^exception ){
		                    }
		                    _running=false;
		                });
#elif WINDOWS_PHONE_8
						_result=0;
						
						_running=false;		                
#endif		                
					}catch( Platform::Exception ^ex ){
					
						_running=false;
					}
				});
#if WINDOWS_8_TEST_IAP
			});
	
		});
		
	});
#endif
}

#if WINDOWS_8

void BBCerberusStore::ConsumePurchase( Platform::String ^productId,Platform::Guid transactionId ){

	create_task( CURRENT_APP::ReportConsumableFulfillmentAsync( productId,transactionId ) ).then( [this]( task<FulfillmentResult> currentTask ){
	
		try{

			auto result=currentTask.get();
			
			switch( result ){
			case FulfillmentResult::Succeeded:
				break;
			case FulfillmentResult::NothingToFulfill:
				break;
			case FulfillmentResult::PurchasePending:
				break;
			case FulfillmentResult::PurchaseReverted:
				break;
			case FulfillmentResult::ServerError:
				break;
			}

		}catch( Platform::Exception ^exception ){
		}
	});
}

#endif

void BBCerberusStore::RequestPurchase( BBProduct *product ){

	auto productId=product->identifier.ToWinRTString();		

#if WINDOWS_8
	
	create_task( CURRENT_APP::RequestProductPurchaseAsync( productId ) ).then( [this,product,productId]( task<PurchaseResults^> currentTask ){
	
		try{
		
			auto results=currentTask.get();
			
			switch( results->Status ){
			case ProductPurchaseStatus::Succeeded:				//Yes, worked...
				if( product->type==1 ){
					ConsumePurchase( productId,results->TransactionId );
				}else{
					product->owned=true;
				}
				_result=0;
				break;
            case ProductPurchaseStatus::NotFulfilled:			//A previous consumable purchase has not been fulfilled?
            	if( product->type==1 ){
	           		ConsumePurchase( productId,results->TransactionId );
	           	}
            	break;
			case ProductPurchaseStatus::AlreadyPurchased:
				break;
			case ProductPurchaseStatus::NotPurchased:			//cancelled?
				break;
			}

		}catch( Platform::Exception ^exception ){
		}
		_running=false;
	});
	
#elif WINDOWS_PHONE_8

	// Careful! This executes on UI thread, not the update/render thread!
	//
	create_task( CURRENT_APP::RequestProductPurchaseAsync( productId,true ) ).then( [this,product,productId]( task<Platform::String^> currentTask ){
	
		try{
		
			currentTask.get();
			
			auto licenseInfo=CURRENT_APP::LicenseInformation;
			auto license=licenseInfo->ProductLicenses->Lookup( productId );
			
			if( license->IsActive ){
				if( product->type==1 ){
					CURRENT_APP::ReportProductFulfillment( productId );
				}else{
					product->owned=true;
				}
				_result=0;
			}

		}catch( Platform::Exception ^exception ){
		}
		_running=false;
	});
	
#endif

}

void BBCerberusStore::BuyProductAsync( BBProduct *product ){

	_result=-1;
	
	//already bought?
	
	auto licenseInfo=CURRENT_APP::LicenseInformation;
	auto license=licenseInfo->ProductLicenses->Lookup( product->identifier.ToWinRTString() );
	
	if( license->IsActive ){
		if( product->type==2 ) _result=0;
		return;
	}

	_running=true;
	
#if WINDOWS_8

	RequestPurchase( product );
	
#elif WINDOWS_PHONE_8

	BBWinrtGame::WinrtGame()->PostToUIThread( [this,product](){ RequestPurchase( product ); } );

#endif

}

void BBCerberusStore::GetOwnedProductsAsync(){

	_result=0;
}
