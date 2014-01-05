
#If MOJO_VERSION_X
#Error "Mojo version error"
#Endif

Import brl.gametarget

#If Not BRL_GAMETARGET_IMPLEMENTED
#Error "Native Game class not implemented"
#Endif

#If Not MOJO_DRIVER_IMPLEMENTED
#If TARGET="android" Or TARGET="flash" Or TARGET="glfw" Or TARGET="html5" Or TARGET="ios" Or TARGET="psm" Or TARGET="winrt" Or TARGET="xna"
Import "native/mojo.${TARGET}.${LANG}"
#MOJO_DRIVER_IMPLEMENTED=True
#Endif
#Endif

#If Not MOJO_DRIVER_IMPLEMENTED
#Error "Native Mojo driver not implemented"
#Endif

