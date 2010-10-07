# -*- coding: utf-8 -*-
"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""

from distutils.core import setup, Extension
from distutils.sysconfig import get_python_lib
import os, sys, getopt
import time
    
macros = []
if '--use-double' in sys.argv: 
    sys.argv.remove('--use-double') 
    macros.append(('USE_DOUBLE',None))
    extension_name = "_pyo64"
    main_files = ['pyo.py', 'pyo64.py']
else:
    extension_name = "_pyo"
    main_files = ['pyo.py']
    
if '--use-jack' in sys.argv: 
    sys.argv.remove('--use-jack') 
    macros.append(('USE_JACK',None))

if '--use-coreaudio' in sys.argv: 
    sys.argv.remove('--use-coreaudio') 
    macros.append(('USE_COREAUDIO',None))
    
path = 'src/engine/'
files = ['pyomodule.c', 'servermodule.c', 'streammodule.c', 'dummymodule.c', 'mixmodule.c', 'inputfadermodule.c',
        'interpolation.c']
source_files = [path + f for f in files]

path = 'src/objects/'
files = ['tablemodule.c', 'matrixmodule.c', 'oscilmodule.c', 'filtremodule.c', 'noisemodule.c', 'distomodule.c',
        'inputmodule.c', 'fadermodule.c', 'midimodule.c', 'oscmodule.c', 'delaymodule.c', 'sfplayermodule.c',
        'metromodule.c', 'trigmodule.c', 'patternmodule.c', 'bandsplitmodule.c', 'hilbertmodule.c', 'panmodule.c',
        'selectmodule.c', 'freeverbmodule.c', 'granulatormodule.c', 'compressmodule.c', 'analysismodule.c',
        'convolvemodule.c', 'randommodule.c', 'wgverbmodule.c', 'utilsmodule.c', 'arithmeticmodule.c', 'sigmodule.c',
        'matrixprocessmodule.c', 'harmonizermodule.c', 'recordmodule.c', 'chorusmodule.c']
source_files = source_files + [path + f for f in files]
    
if sys.platform == "win32":
    include_dirs = ['C:\portaudio\include', 'C:\Program Files\Mega-Nerd\libsndfile\include',
                    'C:\portmidi\pm_common', 'C:\liblo', 'C:\pthreads\include', 'include']
    library_dirs = ['C:\portaudio', 'C:\Program Files\Mega-Nerd\libsndfile', 'C:\portmidi', 'C:\liblo']
    libraries = ['portaudio', 'portmidi', 'sndfile-1', 'lo']
    extension = [Extension(extension_name, source_files, include_dirs=include_dirs, libraries=libraries, 
                library_dirs=library_dirs, extra_compile_args=["-Wno-strict-prototypes"], define_macros=macros)]
else:
    tsrt = time.strftime('"%d %b %Y %H:%M:%S"', time.gmtime())
    macros.append(('TIMESTAMP', tsrt))
    include_dirs = ['include', '/usr/local/include']
    libraries = ['portaudio', 'portmidi', 'sndfile', 'lo']
    extension = [Extension(extension_name, source_files, include_dirs=include_dirs, libraries=libraries, 
                extra_compile_args=["-Wno-strict-prototypes"], define_macros=macros)]
       
setup(  name = "pyo",
        author = "Olivier Belanger",
        author_email = "belangeo@gmail.com",
        version = "0.01",
        description = "Python dsp module.",
        long_description = "pyo is a Python module written in C to help digital signal processing script creation.",
        url = "http://code.google.com/p/pyo/",
        license="GPLv3",
        packages=['pyolib', 'pyolib.snds'],
        data_files=[(get_python_lib(), main_files),
        (os.path.join(get_python_lib(), 'pyolib', 'snds'), ['pyolib/snds/'+ f for f in os.listdir('pyolib/snds') if f.endswith('aif')])],
        ext_modules = extension )
      
