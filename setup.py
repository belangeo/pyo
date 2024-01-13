# -*- coding: utf-8 -*-
"""
Copyright 2009-2023 Olivier Belanger

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
License along with pyo. If not, see <http://www.gnu.org/licenses/>.
"""
from setuptools import setup, Extension
import os, sys, subprocess, platform, glob, shutil

if sys.platform == "win32":
    with open("setup.cfg", "w") as f:
        f.write("[build]\ncompiler = mingw32")

build_with_jack_support = False
compile_externals = False
win_arch = platform.architecture()[0]

macros = []
extension_names = ["pyo._pyo"]
extra_macros_per_extension = [[]]

if "--use-double" in sys.argv:
    sys.argv.remove("--use-double")
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

# Source files
path = "src/engine"
files = [
    f for f in os.listdir(path)
        if not f.startswith("ad_") and
        not f.startswith("md_") and
        "listener" not in f
] + ad_files
source_files = [os.path.join(path, f) for f in files]

path = "src/objects"
files = [
    f for f in os.listdir(path) if "oscmodule" not in f
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
        "lame": (False, True, "3.100"),
        "mpg123": (False, True, "1.31.2"),
    }

    ### TODO: MacOS, script argument / conditional statement to automate the default path
    # Intel: /usr/local/Cellar
    # arm64: /opt/homebrew/Cellar
    brew_packages_root = os.environ.get("BREW_PACKAGES_ROOT", "/usr/local/Cellar")

    ### TODO: MacOS, Copy lib files and run install_name_tool here...

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
dlls = []
data_files = []
if sys.platform == "win32":
    for bind in binary_dirs:
        dlls.extend(glob.glob(os.path.join(bind, "*.dll")))
    dlls = [item for item in dlls if "FLAC++" not in item]  # Lame: remove this manually
    dlls.extend(glob.glob(os.path.join(msys2_mingw_root, "bin", "lib*pthread*.dll")))
elif sys.platform == "darwin":
    if "bdist_wheel" in sys.argv:
        dylibs = []
        for bind in library_dirs:
            dylibs.extend(glob.glob(os.path.join(bind, "*.dylib")))
        dylibs = [dylib for dylib in dylibs if not os.path.islink(dylib)]
        dylibs = [dylib for dylib in dylibs if "FLAC++" not in dylib and "portaudiocpp" not in dylib]
        data_files = (("/pyo", dylibs),)

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

# Copy dlls to package to pyo folder 
if sys.platform == "win32":
    for dll in dlls:
        shutil.copy(dll, "pyo/")

setup(
    ext_modules=extensions,
    # To install files outside the package (third-party libs).
    data_files=data_files,
)

if compile_externals:
    os.system("rm pyo/lib/external.py")

if sys.platform == "win32" and os.path.isfile("setup.cfg"):
    os.remove("setup.cfg")

# Remove packaged dlls from pyo folder 
if sys.platform == "win32":
    for dll in dlls:
        os.remove(os.path.join("pyo", os.path.basename(dll)))
