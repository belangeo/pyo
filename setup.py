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
import os, sys, subprocess, platform, glob

if sys.platform == "win32":
    with open("setup.cfg", "w") as f:
        f.write("[build]\ncompiler = mingw32")

JACK1_MIN_VERSION = "0.125.0"
JACK2_MIN_VERSION = "1.9.11"

def get_jack_api():
    try:
        output = subprocess.check_output(
            ['pkg-config', '--modversion', 'jack'],
            shell=True, text=True
        )
    except:
        # jack2-dbus is probably installed instead of jackd.
        # If jack2-dbus version is >= 1.9.11, we need JACK_NEW_API.
        return "JACK_NEW_API"

    if output != "":
        if (output.startswith("0") and output < JACK1_MIN_VERSION) or (output.startswith("1") and output < "JACK2_MIN_VERSION"):
            return "JACK_OLD_API"
        else:
            return "JACK_NEW_API"
    else:
        return "JACK_NEW_API"


pyo_version = "1.0.4-5"
build_with_jack_support = False
compile_externals = False
win_arch = platform.architecture()[0]

macros = []
extension_names = ["pyo._pyo"]
extra_macros_per_extension = [[]]
packages = [
    "pyo",
    "pyo.lib",
    "pyo.lib.snds",
    "pyo.editor",
    "pyo.editor.styles",
    "pyo.editor.snippets",
    "pyo.editor.snippets.Audio",
    "pyo.editor.snippets.Control",
    "pyo.editor.snippets.Interface",
    "pyo.editor.snippets.Utilities",
    "pyo.examples",
    "pyo.examples.01-intro",
    "pyo.examples.02-controls",
    "pyo.examples.03-generators",
    "pyo.examples.04-soundfiles",
    "pyo.examples.05-envelopes",
    "pyo.examples.06-filters",
    "pyo.examples.07-effects",
    "pyo.examples.08-dynamics",
    "pyo.examples.09-callbacks",
    "pyo.examples.10-tables",
    "pyo.examples.16-midi",
    "pyo.examples.17-osc",
    "pyo.examples.19-multirate",
    "pyo.examples.20-multicore",
    "pyo.examples.21-utilities",
    "pyo.examples.22-events",
    "pyo.examples.23-expression",
    "pyo.examples.algorithmic",
    "pyo.examples.fft",
    "pyo.examples.matrix",
    "pyo.examples.sampling",
    "pyo.examples.sequencing",
    "pyo.examples.snds",
    "pyo.examples.synthesis",
    "pyo.examples.wxgui",
]

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

# Specific audio drivers source files to compile
ad_files = []
obj_files = []
libraries = []

# Special flag to build without portaudio, portmidi and liblo deps.
if "--minimal" in sys.argv:
    sys.argv.remove("--minimal")
else:
    # portaudio
    macros.append(("USE_PORTAUDIO", None))
    ad_files.append("ad_portaudio.c")
    libraries.append("portaudio")
    # portmidi
    macros.append(("USE_PORTMIDI", None))
    ad_files.append("md_portmidi.c")
    ad_files.append("midilistenermodule.c")
    libraries.append("portmidi")
    # liblo
    macros.append(("USE_OSC", None))
    ad_files.append("osclistenermodule.c")
    obj_files.append("oscmodule.c")
    libraries.append("liblo" if sys.platform == "win32" else "lo")

# libsndfile
libraries += ["sndfile"]

# Optional Audio / Midi drivers
if "--use-jack" in sys.argv:
    sys.argv.remove("--use-jack")
    build_with_jack_support = True
    macros.append(("USE_JACK", None))
    if "--jack-force-old-api" in sys.argv:
        sys.argv.remove("--jack-force-old-api")
        macros.append(("JACK_OLD_API", None))
    else:
        # Don't use the old API anymore
        macros.append(("JACK_NEW_API", None))
    ad_files.append("ad_jack.c")

if "--use-coreaudio" in sys.argv:
    sys.argv.remove("--use-coreaudio")
    macros.append(("USE_COREAUDIO", None))
    ad_files.append("ad_coreaudio.c")

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
] + ad_files
source_files = [os.path.join(path, f) for f in files]

path = "src/objects"
files = [
    "mmlmodule.c",
    "hrtfmodule.c",
    "filtremodule.c",
    "arithmeticmodule.c",
    "oscilmodule.c",
    "randommodule.c",
    "analysismodule.c",
    "sfplayermodule.c",
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
    "midimodule.c",
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
] + obj_files

if compile_externals:
    source_files = source_files + ["externals/externalmodule.c"] + [os.path.join(path, f) for f in files]
else:
    source_files = source_files + [os.path.join(path, f) for f in files]

# Platform-specific build settings for the pyo extension(s).
if sys.platform == "win32":
    def vcpkg_path(root, pkg, triplet):
        return os.path.join(root, "_".join((pkg, triplet)))

    pkgs_3rdpary = {
        #package flags: (include, lib, bin)
        "libflac": (False, False, True),
        "liblo": (True, True, True),
        "libogg": (False, False, True),
        "libsndfile": (True, True, True),
        "libvorbis": (False, False, True),
        "opus": (False, False, True),
        "portaudio": (True, True, True),
        "portmidi": (True, True, True),
        "mp3lame": (False, False, True),
        "mpg123": (False, False, True)
    }
    vcpkg_root = os.environ.get("VCPKG_ROOT", "../vcpkg")
    vcpkg_packages_root = os.environ.get("VCPKG_PACKAGES_ROOT", os.path.join(vcpkg_root, "packages"))
    vcpkg_triplet = os.environ.get("VCPKG_DEFAULT_TRIPLET", "x64-windows")
    msys2_mingw_root = os.environ.get("MSYS2_MINGW_ROOT", r"C:\msys64\mingw64")

    include_dirs = ["include"]
    library_dirs = []
    binary_dirs = []

    if win_arch == "32bit":
        print("setup.py is no more configured to compile on 32-bit windows.")
        sys.exit()
    else:
        for pkg, req in pkgs_3rdpary.items():
            pkg_dir = vcpkg_path(vcpkg_packages_root, pkg, vcpkg_triplet)
            if req[0]:
                include_dirs.append(os.path.join(pkg_dir, "include"))
            if req[1]:
                library_dirs.append(os.path.join(pkg_dir, "lib"))
            if req[2]:
                binary_dirs.append(os.path.join(pkg_dir, "bin"))
        
        # newer vcpkg stores unified deps in root/installed/triplet/(include|bin|lib)
        vcpkg_shared_base = os.path.join(vcpkg_root, "installed", vcpkg_triplet)
        include_dirs.append(os.path.join(vcpkg_shared_base, "include"))
        library_dirs.append(os.path.join(vcpkg_shared_base, "lib"))
        binary_dirs.append(os.path.join(vcpkg_shared_base, "bin"))
        
        include_dirs.extend([
            os.path.join(msys2_mingw_root, "include"),
            "include",
        ])

        libraries.append("sndfile")

        macros.append(("MS_WIN64", None))
elif sys.platform == "darwin":
    pkgs_3rdpary = {
        #package flags: (include, lib, version)
        "flac": (False, True, "1.4.2"),
        "liblo": (True, True, "0.31"),
        "libogg": (False, True, "1.3.5"),
        "libsndfile": (True, True, "1.2.0"),
        "libvorbis": (False, True, "1.3.7"),
        "opus": (False, True, "1.3.1"),
        "portaudio": (True, True, "19.7.0"),
        "portmidi": (True, True, "2.0.4"),
        "lame": (False, True, "3.100")
    }

    # Intel, brew's packages directory is usually /usr/local/Cellar
    brew_packages_root = os.environ.get("BREW_PACKAGES_ROOT", "/opt/homebrew/Cellar")

    include_dirs = ["include"]
    library_dirs = []

    for pkg, req in pkgs_3rdpary.items():
        pkg_dir = os.path.join(brew_packages_root, pkg, req[2])
        if req[0]:
            include_dirs.append(os.path.join(pkg_dir, "include"))
        if req[1]:
            library_dirs.append(os.path.join(pkg_dir, "lib"))

else:
    include_dirs = ["include", "/usr/include", "/usr/local/include"]
    libraries += ["rt"]
    library_dirs = ["/usr/lib", "/usr/local/lib"]
    if build_with_jack_support:
        libraries.append("jack")

# Platform-specific data files
if sys.platform == "win32":
    if "bdist_wheel" in sys.argv:
        data_files_dest = os.path.join("Lib", "site-packages", "pyo")
    else:
        data_files_dest = "pyo"
    dlls = []
    for bind in binary_dirs:
        dlls.extend(glob.glob(os.path.join(bind, "*.dll")))
    dlls = [item for item in dlls if "FLAC++" not in item]  # Lame: remove this manually
    dlls.extend(glob.glob(os.path.join(msys2_mingw_root, "bin", "lib*pthread*.dll")))
    data_files = ((data_files_dest, dlls),)
elif sys.platform == "darwin":
    if "bdist_wheel" in sys.argv:
        dylibs = []
        for bind in library_dirs:
            dylibs.extend(glob.glob(os.path.join(bind, "*.dylib")))
        dylibs = [dylib for dylib in dylibs if not os.path.islink(dylib)]
        dylibs = [dylib for dylib in dylibs if "FLAC++" not in dylib and "portaudiocpp" not in dylib]
        data_files = (("/pyo", dylibs),)
    else:
        data_files = []
else:
    data_files = []

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

soundfiles = [f for f in os.listdir(os.path.join("pyo", "lib", "snds")) if f[-3:] in ["aif", "wav"]]
soundfiles.extend(["ControlRead_example_test_000", "ControlRead_example_test_001"])
soundfiles.extend(["NoteinRead_example_test_000", "NoteinRead_example_test_001"])
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
    "Programming Language :: Python :: 3.7",
    "Programming Language :: Python :: 3.8",
    "Programming Language :: Python :: 3.9",
    "Programming Language :: Python :: 3.10",
    "Programming Language :: Python :: 3.11",
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
    python_requires=">=3.7, <4",
    zip_safe=False,
    packages=packages,
    package_data={
        "pyo.lib.snds": soundfiles,
        "pyo.editor.styles": [
            "Custom",
            "Default",
            "Espresso",
            "Smooth",
            "Soft",
            "Monokai-Soda",
            "Solarized (dark)",
            "Solarized (light)",
        ],
        "pyo.editor.snippets.Audio": ["SoundPlayer", "TableOsc"],
        "pyo.editor.snippets.Control": ["ChorusJit", "Vibrato"],
        "pyo.editor.snippets.Interface": ["NewFrame", "PaintPanel"],
        "pyo.editor.snippets.Utilities": ["ChooseAudioDev", "Incrementor"],
        "pyo.examples.23-expression": ["utils.expr", "filters.expr", "generators.expr"],
        "pyo.examples.snds": [
            "alum1.wav",
            "alum2.wav",
            "alum3.wav",
            "alum4.wav",
            "baseballmajeur_m.aif",
            "drumloop.wav",
            "flute.aif",
            "ounkmaster.aif",
            "snd_1.aif",
            "snd_2.aif",
            "snd_3.aif",
            "snd_4.aif",
            "snd_5.aif",
            "snd_6.aif",
            "mapleleafrag.mid",
        ],
    },
    ext_modules=extensions,
    # To install files outside the package (third-party libs).
    data_files=data_files,
    entry_points={"console_scripts": ["epyo = pyo.editor.EPyo:main"]},
)

if compile_externals:
    os.system("rm pyo/lib/external.py")

if sys.platform == "win32" and os.path.isfile("setup.cfg"):
    os.remove("setup.cfg")
