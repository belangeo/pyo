# -*- coding: utf-8 -*-
"""
Copyright 2009-2015 Olivier Belanger

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
from distutils.sysconfig import get_python_lib
from distutils.core import setup, Extension
import os, sys, py_compile, subprocess

if sys.version_info[0] < 3:
    def tobytes(strng, encoding=None):
        "Convert unicode string to bytes."
        return bytes(strng)
else:
    def tobytes(strng, encoding="utf-8"):
        "Convert unicode string to bytes."
        return bytes(strng, encoding=encoding)

def get_jack_api():
    try:
        output = subprocess.Popen(["jackd", "-V"], stdout=subprocess.PIPE)
    except FileNotFoundError:
        # jack2-dbus is probably installed instead of jackd.
        return "JACK_OLD_API"

    text = output.communicate()[0]
    if text != "":
        line = text.splitlines()[0]
        if tobytes("0.124") in line or tobytes("1.9.10") in line:
            return "JACK_OLD_API"
        else:
            return "JACK_NEW_API"
    else:
        return "JACK_NEW_API"

def get_hrtf_file_names(folder):
    path = os.path.join("pyolib", "snds", "hrtf_compact", folder)
    files = [f for f in os.listdir(path) if f.endswith(".wav")]
    return files

pyo_version = "0.9.1"
build_with_jack_support = False
compile_externals = False

macros = []
extension_names = ['_pyo']
main_modules = ['pyo']
extra_macros_per_extension = [[]]

if '--use-double' in sys.argv:
    sys.argv.remove('--use-double') 
    if not '--only-double' in sys.argv: 
        extension_names.append('_pyo64')
        main_modules.append('pyo64')
        extra_macros_per_extension.append([('USE_DOUBLE',None)])

if '--only-double' in sys.argv:
    sys.argv.remove('--only-double') 
    extension_names = ['_pyo64']
    main_modules = ['pyo64']
    extra_macros_per_extension = [[('USE_DOUBLE',None)]]

if '--no-messages' in sys.argv:    
    sys.argv.remove('--no-messages') 
    macros.append(('NO_MESSAGES',None))

if '--compile-externals' in sys.argv:
    compile_externals = True
    sys.argv.remove('--compile-externals') 
    macros.append(('COMPILE_EXTERNALS',None))

if '--debug' in sys.argv:
    sys.argv.remove('--debug')
    gflag = ["-g3", "-UNDEBUG"]
else:
    gflag = ["-g0", "-DNDEBUG"]

if '--fast-compile' in sys.argv:
    sys.argv.remove('--fast-compile')
    oflag = ["-O0"]
else:
    oflag = ["-O3"]

# Specific audio drivers source files to compile
ad_files = []
obj_files = []

# Special flag to build without portaudio, portmidi and liblo deps.
if '--minimal' in sys.argv:
    sys.argv.remove('--minimal') 
    libraries = []
else:
    # portaudio
    macros.append(('USE_PORTAUDIO', None))
    ad_files.append("ad_portaudio.c")
    libraries = ["portaudio"]
    # portmidi
    macros.append(('USE_PORTMIDI', None))
    ad_files.append("md_portmidi.c")
    ad_files.append("midilistenermodule.c")
    libraries += ["portmidi"]
    # liblo
    macros.append(('USE_OSC', None))
    ad_files.append("osclistenermodule.c")
    obj_files.append("oscmodule.c")
    if sys.platform == "win32":
        libraries += ["liblo-7"]
    else:
        libraries += ["lo"]

# Optional Audio / Midi drivers
if '--use-jack' in sys.argv: 
    sys.argv.remove('--use-jack') 
    build_with_jack_support = True
    macros.append(('USE_JACK', None))
    macros.append((get_jack_api(), None))
    ad_files.append("ad_jack.c")

if '--use-coreaudio' in sys.argv: 
    sys.argv.remove('--use-coreaudio') 
    macros.append(('USE_COREAUDIO', None))
    ad_files.append("ad_coreaudio.c")

if sys.platform == "darwin":
    macros.append(('_OSX_', None))

path = 'src/engine'
files = ['pyomodule.c', 'streammodule.c', 'servermodule.c', 'pvstreammodule.c', 
         'dummymodule.c', 'mixmodule.c', 'inputfadermodule.c', 'interpolation.c', 
         'fft.c', "wind.c"] + ad_files
source_files = [os.path.join(path, f) for f in files]

path = 'src/objects'
files = ['hrtfmodule.c', 'filtremodule.c', 'arithmeticmodule.c', 'oscilmodule.c', 
         'randommodule.c', 'analysismodule.c', 'sfplayermodule.c', 
         'oscbankmodule.c', 'lfomodule.c', 'exprmodule.c', 'utilsmodule.c', 
         'granulatormodule.c', 'matrixmodule.c', 'noisemodule.c', 'distomodule.c', 
         'tablemodule.c', 'wgverbmodule.c', 'inputmodule.c', 'fadermodule.c', 
         'midimodule.c', 'delaymodule.c','recordmodule.c', 'metromodule.c', 
         'trigmodule.c', 'patternmodule.c', 'bandsplitmodule.c', 'hilbertmodule.c', 
         'panmodule.c', 'selectmodule.c', 'compressmodule.c',  'freeverbmodule.c', 
         'phasevocmodule.c', 'fftmodule.c', 'convolvemodule.c', 'sigmodule.c',
         'matrixprocessmodule.c', 'harmonizermodule.c', 'chorusmodule.c'] + obj_files

if compile_externals:
    source_files = source_files + \
                   ["externals/externalmodule.c"] + \
                   [os.path.join(path, f) for f in files]
else:
    source_files = source_files + [os.path.join(path, f) for f in files]

# Platform-specific build settings for the pyo extension(s).  
if sys.platform == "win32":
    include_dirs = ['C:\portaudio\include', 'C:\portmidi\pm_common', 'include',
                    'C:\Program Files (x86)\Mega-Nerd\libsndfile\include',
                    'C:\liblo-0.28', 'C:\pthreads\include', 'C:\portmidi\porttime']
    library_dirs = ['C:\portaudio', 'C:\portmidi', 'C:\liblo-0.28\src\.libs', 'C:\pthreads\lib', 
                    'C:/Program Files (x86)/Mega-Nerd/libsndfile/bin']
    libraries += ['libsndfile-1', 'pthreadGC2']
    if 'portmidi' in libraries:
        libraries.append('porttime')
else:
    include_dirs = ['include', '/usr/local/include']
    if sys.platform == "darwin":
        include_dirs.append('/opt/local/include')
    library_dirs = ['/usr/local/lib']
    libraries += ['sndfile']
    if build_with_jack_support:
        libraries.append('jack')

libraries += ['m']
extra_compile_args = ['-Wno-strict-prototypes', '-Wno-strict-aliasing'] + oflag + gflag

extensions = []
for extension_name, extra_macros in zip(extension_names, extra_macros_per_extension):
    extensions.append(Extension(extension_name, source_files, libraries=libraries, 
                                library_dirs=library_dirs, include_dirs=include_dirs, 
                                extra_compile_args=extra_compile_args,
                                define_macros=macros + extra_macros))

if compile_externals:
    include_dirs.append('externals')
    os.system('cp externals/external.py pyolib')

soundfiles = [f for f in os.listdir(os.path.join('pyolib', 'snds')) if f[-3:] in ['aif', 'wav']]
ldesc = "Python module written in C to help digital signal processing script creation."
setup(  name = "pyo",
        author = "Olivier Belanger",
        author_email = "belangeo@gmail.com",
        version = pyo_version,
        description = "Python dsp module.",
        long_description = ldesc,
        url = "https://github.com/belangeo/pyo",
        license = "LGPLv3+",
        packages = ['pyolib', 'pyolib.snds', 'pyolib.snds.hrtf_compact', 
                    'pyolib.snds.hrtf_compact.elev0', 'pyolib.snds.hrtf_compact.elev10',
                    'pyolib.snds.hrtf_compact.elev20', 'pyolib.snds.hrtf_compact.elev30',
                    'pyolib.snds.hrtf_compact.elev40', 'pyolib.snds.hrtf_compact.elev50',
                    'pyolib.snds.hrtf_compact.elev60', 'pyolib.snds.hrtf_compact.elev70',
                    'pyolib.snds.hrtf_compact.elev80', 'pyolib.snds.hrtf_compact.elev90',
                    'pyolib.snds.hrtf_compact.elev-10', 'pyolib.snds.hrtf_compact.elev-20',
                    'pyolib.snds.hrtf_compact.elev-30', 'pyolib.snds.hrtf_compact.elev-40'],
        py_modules = main_modules,
        package_data = {'pyolib.snds': soundfiles,
                        'pyolib.snds.hrtf_compact.elev0': get_hrtf_file_names("elev0"),
                        'pyolib.snds.hrtf_compact.elev10': get_hrtf_file_names("elev10"),
                        'pyolib.snds.hrtf_compact.elev20': get_hrtf_file_names("elev20"),
                        'pyolib.snds.hrtf_compact.elev30': get_hrtf_file_names("elev30"),
                        'pyolib.snds.hrtf_compact.elev40': get_hrtf_file_names("elev40"),
                        'pyolib.snds.hrtf_compact.elev50': get_hrtf_file_names("elev50"),
                        'pyolib.snds.hrtf_compact.elev60': get_hrtf_file_names("elev60"),
                        'pyolib.snds.hrtf_compact.elev70': get_hrtf_file_names("elev70"),
                        'pyolib.snds.hrtf_compact.elev80': get_hrtf_file_names("elev80"),
                        'pyolib.snds.hrtf_compact.elev90': get_hrtf_file_names("elev90"),
                        'pyolib.snds.hrtf_compact.elev-10': get_hrtf_file_names("elev-10"),
                        'pyolib.snds.hrtf_compact.elev-20': get_hrtf_file_names("elev-20"),
                        'pyolib.snds.hrtf_compact.elev-30': get_hrtf_file_names("elev-30"),
                        'pyolib.snds.hrtf_compact.elev-40': get_hrtf_file_names("elev-40")},
        ext_modules = extensions)

if compile_externals:
    os.system('rm pyolib/external.py')

