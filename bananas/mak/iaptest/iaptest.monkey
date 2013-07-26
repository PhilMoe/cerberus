
#rem

IMPORTANT!

This wont work 'as is' - you'll need to generate a keystore file, and have added your app/products to GooglePlay/iTunes Connect.

#end

Import mojo
Import brl.json
Import brl.filestream
Import brl.monkeystore

#ANDROID_APP_TITLE="Bouncy Aliens"
#ANDROID_APP_PACKAGE="com.monkeycoder.bouncyaliens"
#ANDROID_SIGN_APP=True

#rem
#ANDROID_KEY_STORE="../../release-key.keystore"
#ANDROID_KEY_ALIAS="release-key-alias"
#ANDROID_KEY_STORE_PASSWORD="password"
#ANDROID_KEY_ALIAS_PASSWORD="password"
#end

Global CONSUMABLES:=["bulletboost","speedboost"]

Global NON_CONSUMABLES:=["shipupgrade2"]

Class MyApp Extends App Implements IOnOpenStoreComplete,IOnBuyProductComplete,IOnGetOwnedProductsComplete

	Field store:MonkeyStore
	Field purchases:=New JsonObject
	
	Method LoadPurchases:Void()
		Local f:=FileStream.Open( "monkey://internal/.purchases","r" )
		If Not f Return
		Local json:=f.ReadString()
		Print "LoadPurchases: Json="+json
		purchases=New JsonObject( json )
		f.Close()
	End
	
	Method SavePurchases:Void()
		Local f:=FileStream.Open( "monkey://internal/.purchases","w" )
		If Not f Error "Unable to save purchases"
		Local json:=purchases.ToJson()
		Print "SavePurchases: Json="+json
		f.WriteString( json )
		f.Close()
	End
	
	Method OnOpenStoreComplete:Void( result:Int )
		Print "OpenStoreComplete, result="+result
		If result<>0 Error "Store unavailable"
	End
	
	Method OnBuyProductComplete:Void( result:Int,product:Product )
		Print "BuyProductComplete, result="+result
		If result<>0 Return
		Select product.Type
		Case 1 purchases.SetInt product.Identifier,purchases.GetInt( product.Identifier )+1
		Case 2 purchases.SetBool product.Identifier,True
		End
		SavePurchases
	End
	
	Method OnGetOwnedProductsComplete:Void( result:Int,products:Product[] )
		Print "GetOwnedProductsComplete, result="+result
		If result<>0 Return
		If Not products Return
		For Local p:=Eachin products
			purchases.SetBool p.Identifier,True
		Next
		SavePurchases
	End
	
	Method OnCreate()
	
		LoadPurchases
	
		store=New MonkeyStore
		
		store.AddProducts( CONSUMABLES,1 )
		store.AddProducts( NON_CONSUMABLES,2 )
		
		store.OpenStoreAsync( Self )
		
		SetUpdateRate 60
	End
	
	Method OnUpdate()
	
		UpdateAsyncEvents
		
		If store.IsOpen() And Not store.IsBusy() And MouseHit( 0 )
			Local my:=MouseY*(480.0/DeviceHeight)
			
			If my>=440-6 And my<440+6
				store.GetOwnedProductsAsync( Self )
			Else
				Local y:=40
				For Local p:=Eachin store.GetProducts()
					If my>=y-6 And my<y+6
						store.BuyProductAsync p,Self
						Exit
					Endif
					y+=24
				Next
			Endif
			
		Endif

	End
	
	Method OnRender()

		Scale DeviceWidth/320.0,DeviceHeight/480.0
		Cls
		DrawText Millisecs,0,0
		
		If store.IsOpen()
			SetColor 255,255,255
			If store.IsBusy() SetColor 128,128,128
			
			DrawText "Restore Owned Products",160,440,.5,.5
			
			Local y:=40
			For Local p:=Eachin store.GetProducts()
				Local t:="Buy "+p.Title
				Select p.Type
				Case 1 t+=" ("+purchases.GetInt( p.Identifier )+")"
				Case 2 If purchases.GetBool( p.Identifier ) t+=" (owned)"
				End
				DrawText t,160,y,.5,.5
				y+=24
			Next
		Else
			DrawText "Opening store...",160,40,.5,.5
		Endif
	End

End

Function Main()
	New MyApp
End
