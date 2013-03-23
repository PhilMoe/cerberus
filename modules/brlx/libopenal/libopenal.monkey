
#If LANG<>"cpp"
#Error "libopenal is not available on this target"
#Endif

#INCDIRS+="${CD}/native/include"

#If HOST="winnt"

#LIBDIRS+="${CD}/native/libs/Win32"

#LIBS+="OpenAL32"

#Endif
