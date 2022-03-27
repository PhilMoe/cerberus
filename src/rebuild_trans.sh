#Quick and nasty linux shell rebuild trans

#Make transcc
echo "building transcc"
g++ -O3 -DNDEBUG -o ../bin/transcc_linux transcc/transcc.build/cpptool/main.cpp -lpthread
