#!/bin/sh

# Need Xcode 3.2.6 or later (pkgbuild and productbuild)
# with python 2.7.8 (32/64-bit) and wxpython 3.0.1.1 (cocoa) installed
# 1. update pyo sources
# 2. compile and install pyo float and double
# 3. cd utils and build E-Pyo
# 4. cd installer/osx and build the realease, only x86_64 version

export PACKAGE_NAME=pyo_0.7.5_x86_64.pkg
export DMG_DIR="pyo 0.7.5 Universal"
export DMG_NAME="pyo_0.7.5_OSX-universal.dmg"
export INSTALLER_DIR=`pwd`/installer
export PYO_MODULE_DIR=$INSTALLER_DIR/PyoModule/Package_Contents/tmp
export SUPPORT_LIBS_DIR=$INSTALLER_DIR/SupportLibs/Package_Contents/usr/local/lib
export BUILD_RESOURCES=$INSTALLER_DIR/PkgResources/English.lproj
export PKG_RESOURCES=$INSTALLER_DIR/../PkgResources_x86_64

mkdir -p $PYO_MODULE_DIR
mkdir -p $SUPPORT_LIBS_DIR
mkdir -p $BUILD_RESOURCES

cp $PKG_RESOURCES/License.rtf $BUILD_RESOURCES/License.rtf
cp $PKG_RESOURCES/Welcome.rtf $BUILD_RESOURCES/Welcome.rtf
cp $PKG_RESOURCES/ReadMe.rtf $BUILD_RESOURCES/ReadMe.rtf

svn export ../.. installer/pyo-build
cd installer/pyo-build

echo "building pyo for python 2.6 (64-bit)..."
sudo /usr/bin/python setup.py install --use-coreaudio

sudo rm -rf build/temp.macosx-10.6-universal-2.6
sudo /usr/bin/python setup.py install --use-coreaudio --only-double

sudo cp -R build/lib.macosx-10.6-universal-2.6 $PYO_MODULE_DIR/python26

echo "building pyo for python 2.7 (64-bit)..."
sudo /usr/local/bin/python2.7 setup.py install --use-coreaudio --use-double

sudo cp -R build/lib.macosx-10.6-intel-2.7 $PYO_MODULE_DIR/python27

sudo install_name_tool -change libportmidi.dylib /usr/local/lib/libportmidi.dylib $PYO_MODULE_DIR/python26/_pyo.so
sudo install_name_tool -change libportmidi.dylib /usr/local/lib/libportmidi.dylib $PYO_MODULE_DIR/python26/_pyo64.so
sudo install_name_tool -change libportmidi.dylib /usr/local/lib/libportmidi.dylib $PYO_MODULE_DIR/python27/_pyo.so
sudo install_name_tool -change libportmidi.dylib /usr/local/lib/libportmidi.dylib $PYO_MODULE_DIR/python27/_pyo64.so

cd ..

echo "copying support libs..."
sudo cp /usr/local/lib/liblo.7.dylib $SUPPORT_LIBS_DIR/liblo.7.dylib
sudo cp /usr/local/lib/libportaudio.2.dylib $SUPPORT_LIBS_DIR/libportaudio.2.dylib
sudo cp /usr/local/lib/libportmidi.dylib $SUPPORT_LIBS_DIR/libportmidi.dylib
sudo cp /usr/local/lib/libsndfile.1.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
sudo cp /usr/local/lib/libFLAC.8.dylib $SUPPORT_LIBS_DIR/libFLAC.8.dylib
sudo cp /usr/local/lib/libvorbisenc.2.dylib $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib
sudo cp /usr/local/lib/libvorbis.0.dylib $SUPPORT_LIBS_DIR/libvorbis.0.dylib
sudo cp /usr/local/lib/libogg.0.dylib $SUPPORT_LIBS_DIR/libogg.0.dylib

echo "setting permissions..."
sudo chgrp -R admin PyoModule/Package_Contents/tmp
sudo chown -R root PyoModule/Package_Contents/tmp
sudo chmod -R 755 PyoModule/Package_Contents/tmp

sudo chgrp -R wheel SupportLibs/Package_Contents/usr
sudo chown -R root SupportLibs/Package_Contents/usr
sudo chmod -R 755 SupportLibs/Package_Contents/usr

echo "building packages..."

pkgbuild    --identifier com.iact.umontreal.ca.pyo.tmp.pkg \
            --root PyoModule/Package_Contents/ \
            --version 1.0 \
            --scripts $PKG_RESOURCES \
            PyoModule.pkg

pkgbuild    --identifier com.iact.umontreal.ca.pyo.usr.pkg \
            --root SupportLibs/Package_Contents/ \
            --version 1.0 \
            SupportLibs.pkg

echo "building product..."
productbuild --distribution ../Distribution.dist --resources $BUILD_RESOURCES $PACKAGE_NAME

echo "assembling DMG..."
mkdir "$DMG_DIR"
cd "$DMG_DIR"
cp ../$PACKAGE_NAME .
cp -R ../../../../utils/E-Pyo.app .
ln -s /Applications .
cd ..

hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR"

cd ..
mv installer/$DMG_NAME .

echo "clean up resources..."
sudo rm -rf installer


