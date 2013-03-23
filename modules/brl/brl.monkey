
#If LANG="cpp" Or LANG="java" Or LANG="cs" Or LANG="js" Or LANG="as"
Import databuffer
Import ringbuffer
Import datastream
Import asyncevent
Import asyncdataloader
#Endif

#If LANG="cpp" Or LANG="java" Or LANG="cs"
Import filestream
#Endif

#If (LANG="cpp" And TARGET<>"win8") Or LANG="java"
Import tcpstream
Import asyncstream
Import asynctcpstream
Import asynctcpconnector
Import httprequest
#Endif
