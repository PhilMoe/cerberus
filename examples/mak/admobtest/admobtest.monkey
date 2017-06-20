
#If TARGET<>"android" And TARGET<>"ios"
#Error "Admob is only supported on Android and iOS targets"
#End

#ADMOB_PUBLISHER_ID="ca-app-pub-3940256099942544/6300978111"    'replace with id from your admob account
#ADMOB_ANDROID_TEST_DEVICE1="TEST_EMULATOR"
#ADMOB_ANDROID_TEST_DEVICE2="TEST_EMULATOR"

Import mojo
Import brl.admob

Class MyApp Extends App

    Field admob:Admob
    Field layout:=1
    Field enabled:=True
    
    Method OnCreate()
        admob=Admob.GetAdmob()
        admob.ShowAdView 1,layout
        SetUpdateRate 60
    End
    
    Method OnUpdate()
        If MouseHit( 0 )
            If enabled
                admob.HideAdView
                enabled=False
            Else
                layout+=1
                If layout=7 layout=1
                admob.ShowAdView 1,layout
                enabled=True
            Endif
        End
    End
    
    Method OnRender()
    	Local en:="disabled"
    	If enabled en="enabled"
        Cls
        PushMatrix
        Scale 2,2
        DrawText "Click to toggle ads! ads are currently "+en,DeviceWidth/4,DeviceHeight/4,.5,.5
        PopMatrix
    End
    
End

Function Main()
    New MyApp
End
