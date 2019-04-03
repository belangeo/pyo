#! /usr/bin/env python
"""
This script removes versions of pyo prior to 1.0.0 (before pyo is installed with pip) on MacOS.

On Windows, run the uninstaller that comes with pyo to remove all its components. It
is located in the python root directory (ex.: C:\Python27) and named 'unins000.exe'.

"""
import sys, os, shutil

PLATFORM = sys.platform
VERSION_MAJOR = sys.version_info[0]
VERSION_MINOR = sys.version_info[1]
PYTHON_DIR = os.path.dirname(sys.executable)

if sys.platform == "darwin":
    PATHS = ["/Library/Python/%d.%d/site-packages/" % (VERSION_MAJOR, VERSION_MINOR),
            "/Library/Frameworks/Python.framework/Versions/%d.%d/lib/python%d.%d/site-packages/" %
            (VERSION_MAJOR, VERSION_MINOR, VERSION_MAJOR, VERSION_MINOR),
            "~/anaconda/lib/python%d.%d/site-packages/" % (VERSION_MAJOR, VERSION_MINOR)]
    for path in PATHS:
        if os.path.isfile(os.path.join(path, "pyo.py")):
            os.remove(os.path.join(path, "pyo.py"))
        if os.path.isfile(os.path.join(path, "pyo64.py")):
            os.remove(os.path.join(path, "pyo64.py"))
        if os.path.isfile(os.path.join(path, "pyo.pyc")):
            os.remove(os.path.join(path, "pyo.pyc"))
        if os.path.isfile(os.path.join(path, "pyo64.pyc")):
            os.remove(os.path.join(path, "pyo64.pyc"))
        if VERSION_MAJOR == 2:
            f32 = "_pyo.so"
            f64 = "_pyo64.so"
        else:
            f32 = "_pyo.cpython-%d%dm-darwin.so" % (VERSION_MAJOR, VERSION_MINOR)
            f64 = "_pyo64.cpython-%d%dm-darwin.so" % (VERSION_MAJOR, VERSION_MINOR)
        if os.path.isfile(os.path.join(path, f32)):
            os.remove(os.path.join(path, f32))
        if os.path.isfile(os.path.join(path, f64)):
            os.remove(os.path.join(path, f64))
        if os.path.isdir(os.path.join(path, "pyolib")):
            shutil.rmtree(os.path.join(path, "pyolib"))
elif sys.platform == "win32":
    message = """
        On Windows, run the uninstaller that comes with pyo to remove all its components. It
        is located in the python root directory (ex.: C:\Python27) and named 'unins000.exe'.
        """
    print(message)
