echo off

RMDIR /S /Q build
RMDIR /S /Q dist

py -3.7 setup.py bdist_wheel --use-double
py -3.8 setup.py bdist_wheel --use-double
py -3.9 setup.py bdist_wheel --use-double
py -3.10 setup.py bdist_wheel --use-double
py -3.11 setup.py bdist_wheel --use-double
