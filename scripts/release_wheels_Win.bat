echo off

RMDIR /S /Q build
RMDIR /S /Q dist

py -2.7 setup.py bdist_wheel --use-double
py -3.5 setup.py bdist_wheel --use-double
py -3.6 setup.py bdist_wheel --use-double
py -3.7 setup.py bdist_wheel --use-double