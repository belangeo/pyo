# -*- coding: utf-8 -*-
"""
Copyright 2009-2020 Olivier Belanger

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
from setuptools import setup, Extension
from distutils.sysconfig import get_python_lib
import os, sys, py_compile, subprocess, platform


pyo_version = "1.0.3"
compile_externals = False
win_arch = platform.architecture()[0]

macros = []
extension_names = ["pyo._pyo"]
extra_macros_per_extension = [[]]
packages = ["pyo", "pyo.lib"]

if "--use-double" in sys.argv:
    sys.argv.remove("--use-double")
    packages.append("pyo64")
    extension_names.append("pyo._pyo64")
    extra_macros_per_extension.append([("USE_DOUBLE", None)])

if "--no-messages" in sys.argv:
    sys.argv.remove("--no-messages")
    macros.append(("NO_MESSAGES", None))

if "--compile-externals" in sys.argv:
    compile_externals = True
    sys.argv.remove("--compile-externals")
    macros.append(("COMPILE_EXTERNALS", None))

if "--debug" in sys.argv:
    sys.argv.remove("--debug")
    gflag = ["-g3", "-UNDEBUG"]
else:
    gflag = ["-g0", "-DNDEBUG"]

if "--fast-compile" in sys.argv:
    sys.argv.remove("--fast-compile")
    oflag = ["-O0"]
else:
    oflag = ["-O3"]

path = "src/engine"
files = [
    "pyomodule.c",
    "streammodule.c",
    "servermodule.c",
    "pvstreammodule.c",
    "dummymodule.c",
    "mixmodule.c",
    "inputfadermodule.c",
    "interpolation.c",
    "fft.c",
    "wind.c",
    "vbap.c",
]
source_files = [os.path.join(path, f) for f in files]

path = "src/objects"
files = [
    "mmlmodule.c",
    "filtremodule.c",
    "arithmeticmodule.c",
    "oscilmodule.c",
    "randommodule.c",
    "analysismodule.c",
    "oscbankmodule.c",
    "lfomodule.c",
    "exprmodule.c",
    "utilsmodule.c",
    "granulatormodule.c",
    "matrixmodule.c",
    "noisemodule.c",
    "distomodule.c",
    "tablemodule.c",
    "wgverbmodule.c",
    "inputmodule.c",
    "fadermodule.c",
    "delaymodule.c",
    "recordmodule.c",
    "metromodule.c",
    "trigmodule.c",
    "patternmodule.c",
    "bandsplitmodule.c",
    "hilbertmodule.c",
    "panmodule.c",
    "selectmodule.c",
    "compressmodule.c",
    "freeverbmodule.c",
    "phasevocmodule.c",
    "fftmodule.c",
    "convolvemodule.c",
    "sigmodule.c",
    "matrixprocessmodule.c",
    "harmonizermodule.c",
    "chorusmodule.c",
]

if compile_externals:
    source_files = source_files + ["externals/externalmodule.c"] + [os.path.join(path, f) for f in files]
else:
    source_files = source_files + [os.path.join(path, f) for f in files]

libraries = []
# Platform-specific build settings for the pyo extension(s).
if sys.platform == "win32":
    if win_arch == "32bit":
        print("setup.py is no more configured to compile on 32-bit windows.")
        sys.exit()
    else:
        include_dirs = [
            r"C:\msys64\mingw64\include", # which file?
            "include",
        ]
        library_dirs = [
            r"C:\msys64\mingw64\\bin", # idem
        ]
        macros.append(("MS_WIN64", None))
else:
    include_dirs = ["include", "/usr/include", "/usr/local/include"]
    if sys.platform == "darwin":
        include_dirs.append("/opt/local/include")
    else:
        libraries += ["rt"]
    library_dirs = ["/usr/lib", "/usr/local/lib"]

libraries += ["m"]
extra_compile_args = ["-Wno-strict-prototypes", "-Wno-strict-aliasing"] + oflag + gflag

extensions = []
for extension_name, extra_macros in zip(extension_names, extra_macros_per_extension):
    extensions.append(
        Extension(
            extension_name,
            source_files,
            libraries=libraries,
            library_dirs=library_dirs,
            include_dirs=include_dirs,
            extra_compile_args=extra_compile_args,
            define_macros=macros + extra_macros,
        )
    )

if compile_externals:
    include_dirs.append("externals")
    os.system("cp externals/external.py pyo/lib/")

short_desc = "Python module to build digital signal processing program."
long_desc = """
pyo is a Python module containing classes for a wide variety of audio signal processing types. 
With pyo, user will be able to include signal processing chains directly in Python scripts or 
projects, and to manipulate them in real time through the interpreter. Tools in pyo module offer 
primitives, like mathematical operations on audio signal, basic signal processing (filters, 
delays, synthesis generators, etc.), but also complex algorithms to create sound granulation 
and others creative audio manipulations. pyo supports OSC protocol (Open Sound Control), to ease 
communications between softwares, and MIDI protocol, for generating sound events and controlling 
process parameters. pyo allows creation of sophisticated signal processing chains with all the 
benefits of a mature, and widely used, general programming language.
"""

classifiers = [
    # How mature is this project? Common values are
    #   3 - Alpha
    #   4 - Beta
    #   5 - Production/Stable
    "Development Status :: 5 - Production/Stable",
    # Indicate who your project is intended for
    "Intended Audience :: Developers",
    "Intended Audience :: End Users/Desktop",
    "Intended Audience :: Science/Research",
    "Intended Audience :: Other Audience",
    # Operating systems
    "Operating System :: MacOS :: MacOS X",
    "Operating System :: Microsoft :: Windows",
    "Operating System :: POSIX :: Linux",
    # Pick your license as you wish (should match "license" above)
    "License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)",
    # Topics
    "Topic :: Multimedia :: Sound/Audio",
    "Topic :: Multimedia :: Sound/Audio :: Analysis",
    "Topic :: Multimedia :: Sound/Audio :: Capture/Recording",
    "Topic :: Multimedia :: Sound/Audio :: Sound Synthesis",
    # Specify the Python versions you support here. In particular, ensure
    # that you indicate whether you support Python 2, Python 3 or both.
    "Programming Language :: Python :: 3",
    "Programming Language :: Python :: 3.6",
    "Programming Language :: Python :: 3.7",
    "Programming Language :: Python :: 3.8",
    "Programming Language :: Python :: 3.9",
]

setup(
    name="pyo",
    author="Olivier Belanger",
    author_email="belangeo@gmail.com",
    version=pyo_version,
    description=short_desc,
    long_description=long_desc,
    url="http://ajaxsoundstudio.com/software/pyo/",
    project_urls={
        "Bug Tracker": "https://github.com/belangeo/pyo/issues",
        "Documentation": "http://ajaxsoundstudio.com/pyodoc/",
        "Source Code": "https://github.com/belangeo/pyo",
    },
    classifiers=classifiers,
    keywords="audio sound dsp synthesis signal-processing music",
    license="LGPLv3+",
    python_requires=">=3.6, <4",
    zip_safe=False,
    packages=packages,
    ext_modules=extensions,
)

if compile_externals:
    os.system("rm pyo/lib/external.py")
