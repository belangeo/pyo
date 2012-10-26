echo off

echo *** Compile for python2.6 - single precision ***
C:\Python26\python.exe setup.py install

RMDIR /S /Q build

echo *** Compile for python2.6 - double precision ***
C:\Python26\python.exe setup.py install --use-double

RMDIR /S /Q build

echo *** Compile for python2.7 - single precision ***
C:\Python27\python.exe setup.py install

RMDIR /S /Q build

echo *** Compile for python2.7 - double precision ***
C:\Python27\python.exe setup.py install --use-double

RMDIR /S /Q build
