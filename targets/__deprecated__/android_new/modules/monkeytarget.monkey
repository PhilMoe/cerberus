
#BRL_GAMETARGET_IMPLEMENTED=True

#ANDROID_LANGUTIL_ENABLED=True

#ANDROID_LIBGOOGLEPLAY_AVAILABLE=True

Import brl.gametarget

Import "native/androidgame.java"
Import "native/monkeytarget.java"

Extern

Function LoadState_V66b:String()="BBAndroidGame.LoadState_V66b"
Function SaveState_V66b:Void( state:String )="BBAndroidGame.SaveState_V66b"
