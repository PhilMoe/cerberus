#Quick and nasty linux shell rebuild all

#Make Ted
g++ -O3 -DNDEBUG -o ../bin/transcc_linux transcc/transcc.build/cpptool/main.cpp -lpthread

#Make makedocs
../bin/transcc_linux -target=C++_Tool -builddir=makedocs.build  -clean -config=release +CPP_GC_MODE=0 makedocs/makedocs.monkey
cp makedocs/makedocs.build/cpptool/main_linux ../bin/makedocs_linux

#Make mserver
../bin/transcc_linux "-target=Desktop_Game_(Glfw3)" -builddir=mserver.build -clean -config=release +CPP_GC_MODE=1 mserver/mserver.monkey
cp mserver/mserver.build/glfw3/gcc_linux/Release/MonkeyGame ../bin/mserver_linux

#Make launcher
../bin/transcc_linux -target=C++_Tool -builddir=launcher.build -clean -config=release +CPP_GC_MODE=0 launcher/launcher.monkey
cp launcher/launcher.build/cpptool/main_linux ../Monkey

#Make ted
mkdir ted.build
cd ted.build
qmake ../ted/ted.pro
make
cd ..
