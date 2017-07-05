#!/bin/bash
#
# Little script to make Ted.app standalone
#
cd `dirname $0`
cd ../../bin
#macdeployqt-4.8 Ted.app
/Users/Martin/Desktop/Qt/5.5/clang_64/bin/macdeployqt Ted.app
cd Ted.app/Contents/PlugIns
rm -r -f audio
rm -r -f bearer
rm -r -f imageformats
rm -r -f mediaservice
rm -r -f printsupport
rm -r -f sqldrivers
