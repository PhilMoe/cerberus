#Quick and nasty linux shell rebuild cserver

#Make cserver
echo
echo "building cserver"
../bin/transcc_linux "-target=Desktop_Game" -builddir=cserver.build -clean -config=release +CPP_GC_MODE=1 cserver/cserver.cxs
mv cserver/cserver.build/glfw3/gcc_linux/Release/CerberusGame ../bin/cserver_linux

