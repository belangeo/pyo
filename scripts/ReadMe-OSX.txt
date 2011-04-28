pyo (version 0.03)

This package installs all the required components to run pyo inside your current Python installation. Python 2.5, 2.6 (preferred) or 2.7 (32-bit Mac OS X Installer) must be already installed on your system.

This package is divided into two separate installers. If you do not require one of them, please unselect the package in custom installation mode.

1. pyo extension (version 0.03):
The following components will be installed in the site-packages folder of the current Python Framework:

_pyo.so
_pyo64.so
pyo.py
pyo64.py
pyolib (folder)

2. Support libraries:
This component will install a number of dynamic libraries on which pyo depends. If you already have these, then you can skip this installation.

Warning: this installation will overwrite any previously installed libraries. These are the libraries that will be installed in your /usr/local/lib directory:

liblo.0.dylib
libportaudio.2.dylib
libportmidi.dylib
libsndfile.1.dylib

Olivier Bélanger, 2011