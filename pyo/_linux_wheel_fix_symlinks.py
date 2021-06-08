"""
Copyright 2009-2019 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""

# At first run, found and symlink system's libasound and libjack into
# ~/.pyo/X.X_ARCH/libs.
# At runtime, dlopen them before running any other code so that pyo
# use these libraries instead of those embedded in the wheels (alsa
# and jack just don't work with embedded ones).

import os
import sys
import struct
import ctypes
import pyo

bitdepth = struct.calcsize("P") * 8
version = "%d.%d_%d" % (sys.version_info[0], sys.version_info[1], bitdepth)
userlibdir = os.path.join(os.path.expanduser("~"), ".pyo", version, "libs")
try:
    os.makedirs(userlibdir)
except:
    pass

libs = os.listdir("{}{}".format(os.path.dirname(pyo.__file__), ".libs"))

withlibasound = withlibjack = False
libasound = libjack = ""

for lib in libs:
    if "libasound" in lib:
        libasound = os.path.join(userlibdir, lib)
        withlibasound = True
    if "libjack" in lib:
        libjack = os.path.join(userlibdir, lib)
        withlibjack = True

# If libdir already exists, we still need to check if the symlinks are the
# good ones. For a new installation, lib files will have different names.
need_symlinks = False
if not os.path.islink(libasound):
    need_symlinks = True

if need_symlinks:
    libasoundfound = libjackfound = False
    libasoundpath = libjackpath = ""
    folders = [
        "/usr/local/lib",
        "/usr/local/lib%d" % bitdepth,
        "/usr/lib",
        "/usr/lib%d" % bitdepth,
        "/lib",
        "/lib%d" % bitdepth,
    ]
    for path in folders:
        for root, dirs, files in os.walk(path):
            for f in files:
                if libasound:
                    if not libasoundfound and "libasound.so" in f:
                        libasoundpath = os.path.join(root, f)
                        libasoundfound = True
                if libjack:
                    if not libjackfound and "libjack.so" in f:
                        libjackpath = os.path.join(root, f)
                        libjackfound = True
                if withlibasound == libasoundfound and withlibjack == libjackfound:
                    break
                    break

    if withlibasound and libasoundfound:
        os.symlink(libasoundpath, libasound)

    if withlibjack and libjackfound:
        os.symlink(libjackpath, libjack)

# Now, we preload the libraries, before importing _pyo.
if withlibasound:
    libasound = ctypes.CDLL(libasound, mode=ctypes.RTLD_GLOBAL)

if withlibjack and os.path.islink(libjack):
    libjack = ctypes.CDLL(libjack, mode=ctypes.RTLD_GLOBAL)
