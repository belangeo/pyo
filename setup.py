from distutils.core import setup, Extension
import os

path = 'src/engine/'
files = ['pyomodule.c', 'servermodule.c', 'streammodule.c', 'dummymodule.c', 'mixmodule.c']
source_files = [path + f for f in files]
path = 'src/objects/'
files = ['tablemodule.c', 'oscilmodule.c', 'filtremodule.c', 'noisemodule.c', 'distomodule.c', 
        'inputmodule.c', 'fadermodule.c', 'midictlmodule.c', 'oscmodule.c']
source_files = source_files + [path + f for f in files]

include_dirs = ['include']
libraries = ['portaudio', 'portmidi', 'sndfile', 'lo']

extension = [Extension("_pyo", source_files, include_dirs=include_dirs, libraries=libraries, extra_link_args=["-mmacosx-version-min=10.4"])]

setup(  name = "_pyo",
        version = "0.01",
        ext_modules = extension)
         
os.system('rm -rf build')
      
