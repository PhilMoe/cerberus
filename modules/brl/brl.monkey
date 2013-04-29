
#If LANG="cpp" Or LANG="java" Or LANG="cs" Or LANG="js" Or LANG="as"
Import databuffer
Import ringbuffer
Import stream
Import asyncevent
Import datastream
Import asyncdataloader

#If LANG="cpp" Or LANG="java" Or LANG="cs"
Import filestream

#If (LANG="cpp" Or LANG="java") 'And TARGET<>"win8"
Import tcpstream
Import asyncstream
Import asynctcpstream
Import asynctcpconnector
Import httprequest
#Endif

#Endif

#Endif
