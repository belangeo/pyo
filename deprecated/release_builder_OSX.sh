#!/bin/sh

# macOS release builder.

# Update sources
git pull

# Build float and double for both python2 and python3
if [ -d build ]; then
    sudo rm -rf build/;
fi    

sudo python2.7 setup.py install --use-coreaudio --use-double
sudo python3.5 setup.py install --use-coreaudio --use-double
sudo python3.6 setup.py install --use-coreaudio --use-double

# Compile E-Pyo for both python2 and python3
cd utils

if [ -d E-Pyo_OSX_py2 ]; then
   sudo rm -rf E-Pyo_OSX_py2/;
fi    

if [ -d E-Pyo_OSX_py35 ]; then
    sudo rm -rf E-Pyo_OSX_py35/;
fi    

if [ -d E-Pyo_OSX_py36 ]; then
    sudo rm -rf E-Pyo_OSX_py36/;
fi    

sh epyo_builder_OSX_py2.sh
sh epyo_builder_OSX_py3.5.sh
sh epyo_builder_OSX_py3.6.sh

# Build the packages
cd ../installers/osx

sudo rm -rf *.dmg

sudo sh release_x86_64_py2.sh
sudo sh release_x86_64_py3.5.sh
sudo sh release_x86_64_py3.6.sh
