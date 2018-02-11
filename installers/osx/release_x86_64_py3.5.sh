#!/bin/sh

# Need Xcode 3.2.6 or later (pkgbuild and productbuild)
# with python 3.5.4 (32/64-bit) and wxpython 4.0.0b1 (phoenix) installed

# 1. update pyo sources
# 2. compile and install pyo float and double for python3.5
# 3. cd utils and build E-Pyo for python3.5
# 4. cd installers/osx and build the release for python3.5

export PACKAGE_NAME=pyo_0.8.9_x86_64_py35.pkg
export DMG_DIR="pyo 0.8.9 py35 Universal"
export DMG_NAME="pyo_0.8.9_OSX_py35-universal.dmg"
export INSTALLER_DIR=`pwd`/installer
export PYO_MODULE_DIR=$INSTALLER_DIR/PyoModule/Package_Contents/tmp
export SUPPORT_LIBS_DIR=$INSTALLER_DIR/SupportLibs/Package_Contents/usr/local/lib
export BUILD_RESOURCES=$INSTALLER_DIR/PkgResources/English.lproj
export PKG_RESOURCES=$INSTALLER_DIR/../PkgResources_x86_64_py35

mkdir -p $PYO_MODULE_DIR
mkdir -p $SUPPORT_LIBS_DIR
mkdir -p $BUILD_RESOURCES

cp $PKG_RESOURCES/License.rtf $BUILD_RESOURCES/License.rtf
cp $PKG_RESOURCES/Welcome.rtf $BUILD_RESOURCES/Welcome.rtf
cp $PKG_RESOURCES/ReadMe.rtf $BUILD_RESOURCES/ReadMe.rtf

cd ../..
git checkout-index -a -f --prefix=installers/osx/installer/pyo-build/
cd installers/osx/installer/pyo-build

echo "building pyo for python 3.5 (64-bit)..."
sudo /usr/local/bin/python3.5 setup.py install --use-coreaudio --use-double

sudo cp -R build/lib.macosx-10.6-intel-3.5 $PYO_MODULE_DIR/python35

sudo install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib /usr/local/lib/libportmidi.dylib $PYO_MODULE_DIR/python35/_pyo.cpython-35m-darwin.so
sudo install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib /usr/local/lib/libportmidi.dylib $PYO_MODULE_DIR/python35/_pyo64.cpython-35m-darwin.so
sudo install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib /usr/local/lib/libportaudio.2.dylib $PYO_MODULE_DIR/python35/_pyo.cpython-35m-darwin.so
sudo install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib /usr/local/lib/libportaudio.2.dylib $PYO_MODULE_DIR/python35/_pyo64.cpython-35m-darwin.so
sudo install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib /usr/local/lib/liblo.7.dylib $PYO_MODULE_DIR/python35/_pyo.cpython-35m-darwin.so
sudo install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib /usr/local/lib/liblo.7.dylib $PYO_MODULE_DIR/python35/_pyo64.cpython-35m-darwin.so
sudo install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib /usr/local/lib/libsndfile.1.dylib $PYO_MODULE_DIR/python35/_pyo.cpython-35m-darwin.so
sudo install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib /usr/local/lib/libsndfile.1.dylib $PYO_MODULE_DIR/python35/_pyo64.cpython-35m-darwin.so

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

sudo install_name_tool -change /usr/local/opt/flac/lib/libFLAC.8.dylib /usr/local/lib/libFLAC.8.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
sudo install_name_tool -change /usr/local/opt/libogg/lib/libogg.0.dylib /usr/local/lib/libogg.0.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
sudo install_name_tool -change /usr/local/opt/libvorbis/lib/libvorbis.0.dylib /usr/local/lib/libvorbis.0.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
sudo install_name_tool -change /usr/local/opt/libvorbis/lib/libvorbisenc.2.dylib /usr/local/lib/libvorbisenc.2.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
sudo install_name_tool -change /usr/local/opt/libogg/lib/libogg.0.dylib /usr/local/lib/libogg.0.dylib $SUPPORT_LIBS_DIR/libvorbis.0.dylib
sudo install_name_tool -change /usr/local/opt/libogg/lib/libogg.0.dylib /usr/local/lib/libogg.0.dylib $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib
sudo install_name_tool -change /usr/local/Cellar/libvorbis/1.3.5/lib/libvorbis.0.dylib /usr/local/lib/libvorbis.0.dylib $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib

echo "setting permissions..."
sudo chgrp -R admin PyoModule/Package_Contents/tmp
sudo chown -R root PyoModule/Package_Contents/tmp
sudo chmod -R 755 PyoModule/Package_Contents/tmp

sudo chgrp -R wheel SupportLibs/Package_Contents/usr
sudo chown -R root SupportLibs/Package_Contents/usr
sudo chmod -R 755 SupportLibs/Package_Contents/usr

echo "building packages..."

pkgbuild    --identifier com.iact.umontreal.ca.pyo.py35.tmp.pkg \
            --root PyoModule/Package_Contents/ \
            --version 1.0 \
            --scripts $PKG_RESOURCES \
            PyoModule.pkg

pkgbuild    --identifier com.iact.umontreal.ca.pyo.py35.usr.pkg \
            --root SupportLibs/Package_Contents/ \
            --version 1.0 \
            SupportLibs.pkg

echo "building product..."
productbuild --distribution ../Distribution.dist --resources $BUILD_RESOURCES $PACKAGE_NAME

echo "assembling DMG..."
mkdir "$DMG_DIR"
cd "$DMG_DIR"
cp ../$PACKAGE_NAME .
cp -R ../../../../utils/E-Pyo_OSX_py35/E-Pyo.app .
ln -s /Applications .
cd ..

hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR"

cd ..
mv installer/$DMG_NAME .

echo "clean up resources..."
sudo rm -rf installer


