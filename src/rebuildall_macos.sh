#Quick and nasty linux shell rebuild all
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

#Make transcc
echo "building transcc"
clang++ -O3 -DNDEBUG -o ../bin/transcc_macos transcc/transcc.build/cpptool/main.cpp -Wno-bitwise-op-parentheses -Wno-logical-op-parentheses -mmacosx-version-min=10.9 -std=gnu++0x -stdlib=libc++

#Make makedocs
echo
echo "building makedocs"
../bin/transcc_macos -target=C++_Tool -builddir=makedocs.build  -clean -config=release +CPP_GC_MODE=0 makedocs/makedocs.cxs
mv makedocs/makedocs.build/cpptool/main_macos ../bin/makedocs_macos
rm -rf makedocs/makedocs.build

#Make cserver
echo
echo "building cserver"
../bin/transcc_macos "-target=Desktop_Game" -builddir=cserver.build -clean -config=release +CPP_GC_MODE=1 cserver/cserver.cxs

mv cserver/cserver.build/glfw3/xcode/build/Release/CerberusGame.app ../bin/cserver_macos.app
rm -rf cserver/cserver.build

#Make launcher
echo
echo "building launcher"
defaults write $DIR/launcher/xcode/cerberus-launcher/info.plist CFBundleIdentifier -string "com.whiteskygames.cserver_macos"
xcodebuild -scheme Cerberus -configuration release -project  $DIR/launcher/xcode/Cerberus.xcodeproj -derivedDataPath $DIR/launcher/launcher.build
mv launcher/launcher.build/Build/Products/Release/Cerberus.app ../Cerberus.app
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
    echo "Testing for qmake in $HOME/Qt/$QTVER/clang_64"

    export PATH="$HOME/Qt:$HOME/Qt/$QTVER/clang_64/bin:$PATH"
    if hash qmake 2>/dev/null; then
        qmake CONFIG+=release ../ted/ted.pro
    else
        echo "Cannot locate qmake. Check your Qt SDK installation and the version passed."
        exit 1
    fi
    
    make
    make install
fi

cd ..
rm -rf build-ted-Desktop-Release
