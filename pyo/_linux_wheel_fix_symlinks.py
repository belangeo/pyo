# At first run, found and symlink system's libasound and libjack into ~/.pyo/X.X/libs.
# At runtime, dlopen them before running any other code so that pyo use these libraries
# instead of those embedded in the wheels (alsa and jack just don't work with embedded ones).

import os
import sys
import struct
import ctypes
import pyo

bitdepth = struct.calcsize("P") * 8
version = "%d.%d_%d" % (sys.version_info[0], sys.version_info[1], bitdepth)
userlibdir = os.path.join(os.path.expanduser("~"), ".pyo", version, "libs")
os.makedirs(userlibdir, exist_ok=True)

libs = os.listdir(os.path.join(os.path.dirname(pyo.__file__), ".libs"))

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
    for path in ["/usr", "/lib", "/lib%d" % bitdepth]:
        for root, dirs, files in os.walk(path):
            for f in files:
                if libasound:
                    asound = os.path.split(libasound)[1]
                    length = len("libasound")
                    if f == asound[:length] + asound[length+9:]:
                        libasoundpath = os.path.join(root, f)
                        libasoundfound = True
                if libjack:
                    ajack = os.path.split(libjack)[1]
                    length = len("libjack")
                    if f == ajack[:length] + ajack[length+9:]:
                        libjackpath = os.path.join(root, f)
                        libjackfound = True

                if withlibasound == libasoundfound and withlibjack == libjackfound:
                    break; break

    if withlibasound and libasoundfound:
        os.symlink(libasoundpath, libasound)

    if withlibjack and libjackfound:
        os.symlink(libjackpath, libjack)

# Now, we preload the libs, before importing _pyo.
if (withlibasound):
    libasound = ctypes.CDLL(libasound, mode = ctypes.RTLD_GLOBAL)

if (withlibjack and os.path.islink(libjack)):
    libjack = ctypes.CDLL(libjack, mode = ctypes.RTLD_GLOBAL)
