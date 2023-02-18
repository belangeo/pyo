#!/bin/sh

#  release_wheels_OSX_arm64.sh
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

version=1.0.4
replace=XXX

#### Clean up.
rm -rf build dist

#### Source distribution.
/usr/local/bin/python3.10 setup.py sdist

### Build pyo for python 3.8
/usr/local/bin/python3.8 setup.py bdist_wheel --use-coreaudio --use-double -p macosx_13_0_arm64

wheel_file=pyo-XXX-cp38-cp38-macosx_13_0_arm64.whl
dist_info=pyo-XXX.dist-info

if cd dist; then
    unzip ${wheel_file/$replace/$version}
    install_name_tool -change /opt/homebrew/opt/portmidi/lib/libportmidi.2.dylib @loader_path/libportmidi.2.0.3.dylib pyo/_pyo.cpython-38-darwin.so
    install_name_tool -change /opt/homebrew/opt/portmidi/lib/libportmidi.2.dylib @loader_path/libportmidi.2.0.3.dylib pyo/_pyo64.cpython-38-darwin.so
    install_name_tool -change /opt/homebrew/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo.cpython-38-darwin.so
    install_name_tool -change /opt/homebrew/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo64.cpython-38-darwin.so
    install_name_tool -change /opt/homebrew/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo.cpython-38-darwin.so
    install_name_tool -change /opt/homebrew/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo64.cpython-38-darwin.so
    install_name_tool -change /opt/homebrew/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo.cpython-38-darwin.so
    install_name_tool -change /opt/homebrew/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo64.cpython-38-darwin.so

    zip -r -X ${wheel_file/$replace/$version} ${dist_info/$replace/$version} pyo pyo64
    rm -rf ${dist_info/$replace/$version} pyo pyo64
    cd ..
else
    echo "*** Something went wrong when building for python 3.8..."
fi

### Build pyo for python 3.9
/usr/local/bin/python3.9 setup.py bdist_wheel --use-coreaudio --use-double -p macosx_13_0_arm64

wheel_file=pyo-XXX-cp39-cp39-macosx_13_0_arm64.whl
dist_info=pyo-XXX.dist-info

if cd dist; then
    unzip ${wheel_file/$replace/$version}
    install_name_tool -change /opt/homebrew/opt/portmidi/lib/libportmidi.2.dylib @loader_path/libportmidi.2.0.3.dylib pyo/_pyo.cpython-39-darwin.so
    install_name_tool -change /opt/homebrew/opt/portmidi/lib/libportmidi.2.dylib @loader_path/libportmidi.2.0.3.dylib pyo/_pyo64.cpython-39-darwin.so
    install_name_tool -change /opt/homebrew/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo.cpython-39-darwin.so
    install_name_tool -change /opt/homebrew/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo64.cpython-39-darwin.so
    install_name_tool -change /opt/homebrew/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo.cpython-39-darwin.so
    install_name_tool -change /opt/homebrew/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo64.cpython-39-darwin.so
    install_name_tool -change /opt/homebrew/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo.cpython-39-darwin.so
    install_name_tool -change /opt/homebrew/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo64.cpython-39-darwin.so

    zip -r -X ${wheel_file/$replace/$version} ${dist_info/$replace/$version} pyo pyo64
    rm -rf ${dist_info/$replace/$version} pyo pyo64
    cd ..
else
    echo "*** Something went wrong when building for python 3.9..."
fi

### Build pyo for python 3.10
/usr/local/bin/python3.10 setup.py bdist_wheel --use-coreaudio --use-double -p macosx_13_0_arm64

wheel_file=pyo-XXX-cp310-cp310-macosx_13_0_arm64.whl
dist_info=pyo-XXX.dist-info

if cd dist; then
    unzip ${wheel_file/$replace/$version}
    install_name_tool -change /opt/homebrew/opt/portmidi/lib/libportmidi.2.dylib @loader_path/libportmidi.2.0.3.dylib pyo/_pyo.cpython-310-darwin.so
    install_name_tool -change /opt/homebrew/opt/portmidi/lib/libportmidi.2.dylib @loader_path/libportmidi.2.0.3.dylib pyo/_pyo64.cpython-310-darwin.so
    install_name_tool -change /opt/homebrew/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo.cpython-310-darwin.so
    install_name_tool -change /opt/homebrew/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo64.cpython-310-darwin.so
    install_name_tool -change /opt/homebrew/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo.cpython-310-darwin.so
    install_name_tool -change /opt/homebrew/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo64.cpython-310-darwin.so
    install_name_tool -change /opt/homebrew/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo.cpython-310-darwin.so
    install_name_tool -change /opt/homebrew/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo64.cpython-310-darwin.so

    zip -r -X ${wheel_file/$replace/$version} ${dist_info/$replace/$version} pyo pyo64
    rm -rf ${dist_info/$replace/$version} pyo pyo64
    cd ..
else
    echo "*** Something went wrong when building for python 3.10..."
fi

### Build pyo for python 3.11
/usr/local/bin/python3.11 setup.py bdist_wheel --use-coreaudio --use-double -p macosx_13_0_arm64

wheel_file=pyo-XXX-cp311-cp311-macosx_13_0_arm64.whl
dist_info=pyo-XXX.dist-info

if cd dist; then
    unzip ${wheel_file/$replace/$version}
    install_name_tool -change /opt/homebrew/opt/portmidi/lib/libportmidi.2.dylib @loader_path/libportmidi.2.0.3.dylib pyo/_pyo.cpython-311-darwin.so
    install_name_tool -change /opt/homebrew/opt/portmidi/lib/libportmidi.2.dylib @loader_path/libportmidi.2.0.3.dylib pyo/_pyo64.cpython-311-darwin.so
    install_name_tool -change /opt/homebrew/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo.cpython-311-darwin.so
    install_name_tool -change /opt/homebrew/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib pyo/_pyo64.cpython-311-darwin.so
    install_name_tool -change /opt/homebrew/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo.cpython-311-darwin.so
    install_name_tool -change /opt/homebrew/opt/liblo/lib/liblo.7.dylib @loader_path/liblo.7.dylib pyo/_pyo64.cpython-311-darwin.so
    install_name_tool -change /opt/homebrew/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo.cpython-311-darwin.so
    install_name_tool -change /opt/homebrew/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib pyo/_pyo64.cpython-311-darwin.so

    zip -r -X ${wheel_file/$replace/$version} ${dist_info/$replace/$version} pyo pyo64
    rm -rf ${dist_info/$replace/$version} pyo pyo64
    cd ..
else
    echo "*** Something went wrong when building for python 3.11..."
fi
