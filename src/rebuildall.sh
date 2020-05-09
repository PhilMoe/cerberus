#Quick and nasty linux shell rebuild all

#Make transcc
echo "building transcc"
g++ -O3 -DNDEBUG -o ../bin/transcc_linux transcc/transcc.build/cpptool/main.cpp -lpthread

#Make makedocs
echo
echo "building makedocs"
../bin/transcc_linux -target=C++_Tool -builddir=makedocs.build  -clean -config=release +CPP_GC_MODE=0 makedocs/makedocs.cxs
mv makedocs/makedocs.build/cpptool/main_linux ../bin/makedocs_linux
rm -rf makedocs/makedocs.build

#Make cserver
echo
echo "building cserver"
../bin/transcc_linux "-target=Desktop_Game" -builddir=cserver.build -clean -config=release +CPP_GC_MODE=1 cserver/cserver.cxs
mv cserver/cserver.build/glfw3/gcc_linux/Release/CerberusGame ../bin/cserver_linux

# Make sure that there is a copy of the CX standard font in the bin directory
mv cserver/cserver.build/glfw3/gcc_linux/Release/data ../bin/data
rm -rf cserver/cserver.build

#Make launcher
echo
echo "building launcher"
../bin/transcc_linux -target=C++_Tool -builddir=launcher.build -clean -config=release +CPP_GC_MODE=0 launcher/launcher.cxs
mv launcher/launcher.build/cpptool/main_linux ../Cerberus
rm -rf launcher/launcher.build

#Make ted
echo
echo "building ted"
rm -rf build-ted-Desktop-Release
mkdir build-ted-Desktop-Release
cd build-ted-Desktop-Release

# Check to see if there is a Qt SDK directory in the users home folder.
if [ -d "$HOME/Qt" ]; then
    
    if [ -z $1 ]; then
        QTVER="5.9.2"
    else
        QTVER=$1
    fi

    echo "Qt SDK Located....."
    echo "Testing for qmake in $HOME/Qt/$QTVER/gcc_64"

    export PATH="$HOME/Qt:$HOME/Qt/$QTVER/gcc_64/bin:$PATH"
    if hash qmake 2>/dev/null; then
        qmake CONFIG+=release ../ted/ted.pro
    else
        echo "Cannot locate qmake. Check your Qt SDK installation and the version passed."
        exit 1
    fi
    
    make
    make install
else
    # The Qt SDK isn't installed in the home directory, so
    # check for one of the known qmake variants.
    if hash qmake-qt5 2>/dev/null; then
        qmake-qt5 CONFIG+=release ../ted/ted.pro
    elif hash qt5-qmake 2>/dev/null; then
        qmake-qt5 CONFIG+=release ../ted/ted.pro
    else
        echo "Unknown Qt SDK. Going with the default qmake."
        echo "Expect errors......"
        qmake CONFIG+=release ../ted/ted.pro
    fi
    make
fi

cd ..
rm -rf build-ted-Desktop-Release