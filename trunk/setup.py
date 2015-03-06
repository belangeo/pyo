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

from distutils.core import setup, Extension
import os, sys, getopt
import time

pyo_version = "0.7.5"
build_osx_with_jack_support = False
compile_externals = False

macros = []
extension_names = ['_pyo']
main_modules = ['pyo']
extra_macros_per_extension = [[]]
if '--only-double' in sys.argv:
    sys.argv.remove('--only-double') 
    extension_names = ['_pyo64']
    main_modules = ['pyo64']
    extra_macros_per_extension = [[('USE_DOUBLE',None)]]
    
if '--use-double' in sys.argv and not '--only-double' in sys.argv: 
    sys.argv.remove('--use-double') 
    extension_names.append('_pyo64')
    main_modules.append('pyo64')
    extra_macros_per_extension.append([('USE_DOUBLE',None)])
    
if '--use-jack' in sys.argv: 
    sys.argv.remove('--use-jack') 
    if sys.platform == "darwin":
        build_osx_with_jack_support = True
    macros.append(('USE_JACK',None))

if '--use-coreaudio' in sys.argv: 
    sys.argv.remove('--use-coreaudio') 
    macros.append(('USE_COREAUDIO',None))

if '--no-messages' in sys.argv:    
    sys.argv.remove('--no-messages') 
    macros.append(('NO_MESSAGES',None))

if '--compile-externals' in sys.argv:
    compile_externals = True
    sys.argv.remove('--compile-externals') 
    macros.append(('COMPILE_EXTERNALS',None))

if sys.platform == "darwin":
    macros.append(('_OSX_', None))

path = 'src/engine/'
files = ['pyomodule.c', 'servermodule.c', 'pvstreammodule.c', 'streammodule.c', 'dummymodule.c', 
        'mixmodule.c', 'inputfadermodule.c', 'interpolation.c', 'fft.c', "wind.c"]
source_files = [path + f for f in files]

path = 'src/objects/'
files = ['granulatormodule.c', 'tablemodule.c', 'wgverbmodule.c', 'freeverbmodule.c', 'phasevocmodule.c', 'fftmodule.c', 
        'oscilmodule.c', 'randommodule.c', 'oscmodule.c','analysismodule.c', 
        'sfplayermodule.c', 'oscbankmodule.c', 'lfomodule.c', 
         'matrixmodule.c', 'filtremodule.c', 'noisemodule.c', 'distomodule.c',
        'inputmodule.c', 'fadermodule.c', 'midimodule.c', 'delaymodule.c','recordmodule.c', 
        'metromodule.c', 'trigmodule.c', 'patternmodule.c', 'bandsplitmodule.c', 'hilbertmodule.c', 'panmodule.c',
        'selectmodule.c', 'compressmodule.c', 'utilsmodule.c',
        'convolvemodule.c', 'arithmeticmodule.c', 'sigmodule.c',
        'matrixprocessmodule.c', 'harmonizermodule.c', 'chorusmodule.c']

if compile_externals:
    source_files = source_files + ["externals/externalmodule.c"] + [path + f for f in files]
else:
    source_files = source_files + [path + f for f in files]

# Platform-specific build settings for the pyo extension(s).  
if sys.platform == "win32":
    include_dirs = ['C:\portaudio\include', 'C:\Program Files\Mega-Nerd\libsndfile\include',
                    'C:\portmidi\pm_common', 'C:\liblo', 'C:\pthreads\include', 'include',
                    'C:\portmidi\porttime']
    library_dirs = ['C:\portaudio', 'C:/Program Files/Mega-Nerd/libsndfile/bin', 'C:\portmidi', 'C:\liblo', 'C:\pthreads\lib']
    libraries = ['portaudio', 'portmidi', 'porttime', 'libsndfile-1', 'lo', 'pthreadVC2']
else:
    tsrt = time.strftime('"%d %b %Y %H:%M:%S"', time.localtime())
    macros.append(('TIMESTAMP', tsrt))
    include_dirs = ['include', '/usr/local/include']
    if sys.platform == "darwin":
        include_dirs.append('/opt/local/include')
    library_dirs = []
    libraries = ['portaudio', 'portmidi', 'sndfile', 'lo']
    if build_osx_with_jack_support:
        libraries.append('jack')

extensions = []
for extension_name, extra_macros in zip(extension_names, extra_macros_per_extension):
    extensions.append(Extension(extension_name, source_files, include_dirs=include_dirs, library_dirs=library_dirs,
                                libraries=libraries, extra_compile_args=['-Wno-strict-prototypes', '-O3', '-Wno-strict-aliasing'],
                                define_macros=macros + extra_macros))

if compile_externals:
    include_dirs.append('externals')
    os.system('cp externals/external.py pyolib')

setup(  name = "pyo",
        author = "Olivier Belanger",
        author_email = "belangeo@gmail.com",
        version = pyo_version,
        description = "Python dsp module.",
        long_description = "pyo is a Python module written in C to help digital signal processing script creation.",
        url = "http://code.google.com/p/pyo/",
        license = "GPLv3",
        packages = ['pyolib', 'pyolib.snds'],
        py_modules = main_modules,
        package_data = {'pyolib.snds': [f for f in os.listdir('pyolib/snds') if f.endswith('aif') or f.endswith('wav')]},
        ext_modules = extensions )

if compile_externals:
    os.system('rm pyolib/external.py')

