#Quick and nasty linux shell rebuild all
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

#Make transcc
echo "building transcc"
clang++ -O3 -DNDEBUG -o ../bin/transcc_macos transcc/transcc.build/cpptool/main.cpp -Wno-bitwise-op-parentheses -Wno-logical-op-parentheses -mmacosx-version-min=10.9 -std=gnu++0x -stdlib=libc++
