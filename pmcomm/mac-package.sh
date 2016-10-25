#!/bin/bash
# This script must be run from the directory containing PMComm.app

parentDir="$(dirname "$(pwd)")"

# Make sure the project is compiled
make

# Arrange the contents
rm -rf mac-dmg-contents
mkdir mac-dmg-contents
cp -r PMComm.app mac-dmg-contents/
ln -s /Applications mac-dmg-contents/
cp ../LICENSE.txt mac-dmg-contents/
cp ../THIRD-PARTY.txt mac-dmg-contents/

# Only modify the copied version
cd mac-dmg-contents

# Run the deploy script to get the Qt libraries
macdeployqt PMComm.app

# Get libpmcomm.dylib
install_name_tool -id "@executable_path/../Frameworks/libpmcomm.dylib" "PMComm.app/Contents/Frameworks/libpmcomm.dylib"
install_name_tool -change "$parentDir/libpmcomm/libpmcomm.dylib" "@executable_path/../Frameworks/libpmcomm.dylib" "PMComm.app/Contents/MacOS/PMComm"

# Sign the application
IDENTITY="Bogart Engineering"

# HACK to deal with broken macdeployqt
for FRAMEWORK in PMComm.app/Contents/Frameworks/*.framework; do
	FRAMEWORKNAME=$(echo "$FRAMEWORK" | sed "s/.*\/Frameworks\/\([^\/]*\)/\1/g")
	RESOURCESDIR="PMComm.app/Contents/Frameworks/$FRAMEWORKNAME/Resources/"
	mkdir -p "$RESOURCESDIR"
	cp "$HOME/Qt5.1.1/5.1.1/clang_64/lib/$FRAMEWORKNAME/Contents/Info.plist" "$RESOURCESDIR"
done

codesign --deep -s "$IDENTITY" PMComm.app

cd ..

# Make .dmg
rm -f PMComm2.dmg
hdiutil create "PMComm2.dmg" -srcfolder mac-dmg-contents -format UDZO -volname "PMComm 2"
