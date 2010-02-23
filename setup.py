from distutils.core import setup, Extension
from distutils.sysconfig import get_python_lib
import os
        
path = 'src/engine/'
files = ['pyomodule.c', 'servermodule.c', 'streammodule.c', 'dummymodule.c', 'mixmodule.c', 'inputfadermodule.c']
source_files = [path + f for f in files]
path = 'src/objects/'
files = ['tablemodule.c', 'oscilmodule.c', 'filtremodule.c', 'noisemodule.c', 'distomodule.c', 'sigmodule.c',
        'inputmodule.c', 'fadermodule.c', 'midimodule.c', 'oscmodule.c', 'delaymodule.c', 'sfplayermodule.c',
        'metromodule.c', 'trigmodule.c', 'patternmodule.c', 'bandsplitmodule.c', 'hilbertmodule.c', 'panmodule.c',
        'selectmodule.c', 'freeverbmodule.c', 'granulatormodule.c', 'compressmodule.c', 'analysismodule.c', 
        'convolvemodule.c']
source_files = source_files + [path + f for f in files]

include_dirs = ['include']
libraries = ['portaudio', 'portmidi', 'sndfile', 'lo']

extension = [Extension("_pyo", source_files, include_dirs=include_dirs, libraries=libraries, 
             extra_compile_args=["-Wno-strict-prototypes"], extra_link_args=["-mmacosx-version-min=10.4"])]
       
setup(  name = "pyo",
        author = "Olivier Belanger",
        author_email = "belangeo@gmail.com",
        version = "0.01",
        description = "Python dsp module.",
        long_description = "pyo is a Python module written in C to help digital signal processing script creation.",
        url = "http://code.google.com/p/pyo/",
        license="GPLv3",
        packages=['pyolib', 'pyodemos'],
        data_files=[(get_python_lib(), ['pyo.py']),
        (os.path.join(get_python_lib(), 'pyodemos'), ['pyodemos/'+ f for f in os.listdir('pyodemos') if f.endswith('aif')])],
        ext_modules = extension )
         
os.system('rm -rf build')
      
