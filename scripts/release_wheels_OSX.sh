#!/bin/sh

#  release_wheels_OSX.sh
#
# To upload wheels on test.pypi.org:
#   twine upload --repository-url https://test.pypi.org/legacy/ dist/*
#
# To upload wheels on pypi.org:
#   twine upload dist/*
#
# To update older pip:
#   curl https://bootstrap.pypa.io/get-pip.py | python(3)
#

version=1.0.0
replace=XXX

#### Clean up.
sudo rm -rf build dist

#### Source distribution.
sudo /usr/local/bin/python3.7 setup.py sdist

#### Prepare support libraries.
mkdir temp_libs

sudo cp /usr/local/lib/liblo.7.dylib temp_libs/liblo.7.dylib
sudo cp /usr/local/lib/libportaudio.2.dylib temp_libs/libportaudio.2.dylib
sudo cp /usr/local/lib/libportmidi.dylib temp_libs/libportmidi.dylib
sudo cp /usr/local/lib/libsndfile.1.dylib temp_libs/libsndfile.1.dylib
sudo cp /usr/local/lib/libFLAC.8.dylib temp_libs/libFLAC.8.dylib
sudo cp /usr/local/lib/libvorbisenc.2.dylib temp_libs/libvorbisenc.2.dylib
sudo cp /usr/local/lib/libvorbis.0.dylib temp_libs/libvorbis.0.dylib
sudo cp /usr/local/lib/libogg.0.dylib temp_libs/libogg.0.dylib

cd temp_libs
sudo install_name_tool -change /usr/local/opt/flac/lib/libFLAC.8.dylib @loader_path/libFLAC.8.dylib libsndfile.1.dylib
sudo install_name_tool -change /usr/local/opt/libogg/lib/libogg.0.dylib @loader_path/libogg.0.dylib libsndfile.1.dylib
sudo install_name_tool -change /usr/local/opt/libvorbis/lib/libvorbis.0.dylib @loader_path/libvorbis.0.dylib libsndfile.1.dylib
sudo install_name_tool -change /usr/local/opt/libvorbis/lib/libvorbisenc.2.dylib @loader_path/libvorbisenc.2.dylib libsndfile.1.dylib
#sudo install_name_tool -change /usr/local/opt/libogg/lib/libogg.0.dylib @loader_path/libogg.0.dylib libvorbis.0.dylib
sudo install_name_tool -change /usr/local/lib/libogg.0.dylib @loader_path/libogg.0.dylib libvorbis.0.dylib
#sudo install_name_tool -change /usr/local/opt/libogg/lib/libogg.0.dylib @loader_path/libogg.0.dylib libvorbisenc.2.dylib
sudo install_name_tool -change /usr/local/lib/libogg.0.dylib @loader_path/libogg.0.dylib libvorbisenc.2.dylib
sudo install_name_tool -change /usr/local/lib/libvorbis.0.dylib @loader_path/libvorbis.0.dylib libvorbisenc.2.dylib
#sudo install_name_tool -change /usr/local/opt/libogg/lib/libogg.0.dylib @loader_path/libogg.0.dylib libFLAC.8.dylib

### Make sure libvorbis version is correct in this path!
#sudo install_name_tool -change /usr/local/Cellar/libvorbis/1.3.6/lib/libvorbis.0.dylib @loader_path/libvorbis.0.dylib libvorbisenc.2.dylib

cd ..

### Build pyo for python 2.7
sudo python2 setup.py bdist_wheel --use-coreaudio --use-double

wheel_file=pyo-XXX-cp27-cp27m-macosx_10_9_x86_64.whl
dist_info=pyo-XXX.dist-info

if cd dist; then
    sudo unzip ${wheel_file/$replace/$version}
    sudo install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib @loader_path/libportmidi.dylib pyo/_pyo.so
    sudo install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib @loader_path/libportmidi.dylib pyo/_pyo64.so
    sudo install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo.so
    sudo install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo64.so
    sudo install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo.so
    sudo install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo64.so
    sudo install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo.so
    sudo install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo64.so
    sudo zip -r -X ${wheel_file/$replace/$version} ${dist_info/$replace/$version} pyo pyo64
    sudo rm -rf ${dist_info/$replace/$version} pyo pyo64
    cd ..
else
    echo "*** Something went wrong when building for python 2.7..."
fi


### Build pyo for python 3.5
sudo /usr/local/bin/python3.5 setup.py bdist_wheel --use-coreaudio --use-double

wheel_file=pyo-XXX-cp35-cp35m-macosx_10_6_intel.whl
dist_info=pyo-XXX.dist-info

if cd dist; then
    sudo unzip ${wheel_file/$replace/$version}
    sudo install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib @loader_path/libportmidi.dylib pyo/_pyo.cpython-35m-darwin.so
    sudo install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib @loader_path/libportmidi.dylib pyo/_pyo64.cpython-35m-darwin.so
    sudo install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo.cpython-35m-darwin.so
    sudo install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo64.cpython-35m-darwin.so
    sudo install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo.cpython-35m-darwin.so
    sudo install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo64.cpython-35m-darwin.so
    sudo install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo.cpython-35m-darwin.so
    sudo install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo64.cpython-35m-darwin.so
    sudo zip -r -X ${wheel_file/$replace/$version} ${dist_info/$replace/$version} pyo pyo64
    sudo rm -rf ${dist_info/$replace/$version} pyo pyo64
    cd ..
else
    echo "*** Something went wrong when building for python 3.5..."
fi

#### Build pyo for python 3.6
sudo /usr/local/bin/python3.6 setup.py bdist_wheel --use-coreaudio --use-double

wheel_file=pyo-XXX-cp36-cp36m-macosx_10_9_x86_64.whl
dist_info=pyo-XXX.dist-info

if cd dist; then
    sudo unzip ${wheel_file/$replace/$version}
    sudo install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib @loader_path/libportmidi.dylib pyo/_pyo.cpython-36m-darwin.so
    sudo install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib @loader_path/libportmidi.dylib pyo/_pyo64.cpython-36m-darwin.so
    sudo install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo.cpython-36m-darwin.so
    sudo install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo64.cpython-36m-darwin.so
    sudo install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo.cpython-36m-darwin.so
    sudo install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo64.cpython-36m-darwin.so
    sudo install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo.cpython-36m-darwin.so
    sudo install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo64.cpython-36m-darwin.so
    sudo zip -r -X ${wheel_file/$replace/$version} ${dist_info/$replace/$version} pyo pyo64
    sudo rm -rf ${dist_info/$replace/$version} pyo pyo64
    cd ..
else
    echo "*** Something went wrong when building for python 3.6..."
fi

### Build pyo for python 3.7
sudo /usr/local/bin/python3.7 setup.py bdist_wheel --use-coreaudio --use-double

wheel_file=pyo-XXX-cp37-cp37m-macosx_10_9_x86_64.whl
dist_info=pyo-XXX.dist-info

if cd dist; then
    sudo unzip ${wheel_file/$replace/$version}
    sudo install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib @loader_path/libportmidi.dylib pyo/_pyo.cpython-37m-darwin.so
    sudo install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib @loader_path/libportmidi.dylib pyo/_pyo64.cpython-37m-darwin.so
    sudo install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo.cpython-37m-darwin.so
    sudo install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo64.cpython-37m-darwin.so
    sudo install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo.cpython-37m-darwin.so
    sudo install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo64.cpython-37m-darwin.so
    sudo install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo.cpython-37m-darwin.so
    sudo install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo64.cpython-37m-darwin.so
    sudo zip -r -X ${wheel_file/$replace/$version} ${dist_info/$replace/$version} pyo pyo64
    sudo rm -rf ${dist_info/$replace/$version} pyo pyo64
    cd ..
else
    echo "*** Something went wrong when building for python 3.7..."
fi

sudo rm -rf temp_libs

