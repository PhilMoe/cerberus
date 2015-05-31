#!/bin/bash
#
# Little script to make Ted.app standalone
#
cd `dirname $0`
cd ../../bin
#macdeployqt-4.8 Ted.app
/Developer/Tools/Qt/macdeployqt Ted.app
cd Ted.app/Contents/Frameworks
rm -r -f QtDeclarative.framework
rm -r -f QtScript.framework
rm -r -f QtSql.framework
rm -r -f QtSvg.framework
rm -r -f QtXmlPatterns.framework
