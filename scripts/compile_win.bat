echo off

echo *** Compile for python2.6 ***
C:\Python26\python.exe setup.py install --use-double

RMDIR /S /Q build

echo *** Compile for python2.7 ***
C:\Python27\python.exe setup.py install --use-double

RMDIR /S /Q build

cd utils

echo *** Build E-Pyo for python2.6 ***
C:\Python26\python.exe epyo_builder_win32.py

echo *** Build E-Pyo for python2.7 ***
C:\Python27\python.exe epyo_builder_win32.py
