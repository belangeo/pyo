echo off

RMDIR /S /Q build

echo *** Compile for python2.7 ***
C:\Python27\python.exe setup.py install --use-double

RMDIR /S /Q build

echo *** Compile for python3.5-32 ***
C:\Users\olivier\AppData\Local\Programs\Python\Python35-32\python.exe setup.py install --use-double

RMDIR /S /Q build

echo *** Compile for python3.6-32 ***
C:\Users\olivier\AppData\Local\Programs\Python\Python36-32\python.exe setup.py install --use-double

RMDIR /S /Q build

cd utils

echo *** Build E-Pyo for python2.7 ***
C:\Python27\python.exe epyo_builder_win32.py

echo *** Build E-Pyo for python3.5-32 ***
C:\Users\olivier\AppData\Local\Programs\Python\Python35-32\python.exe epyo_builder_win32.py

echo *** Build E-Pyo for python3.6-32 ***
C:\Users\olivier\AppData\Local\Programs\Python\Python36-32\python.exe epyo_builder_win32.py
