from distutils.core import setup, Extension
import os

source_files = ["src/pyomodule.c", "src/servermodule.c", "src/streammodule.c", "src/tablemodule.c", "src/oscmodule.c",
                "src/sinemodule.c", "src/biquadmodule.c", "src/noisemodule.c", "src/distomodule.c", "src/dummymodule.c",
                "src/inputmodule.c", "src/fadermodule.c", "src/midictlmodule.c", "src/mixmodule.c", "src/oscsendmodule.c",
                "src/oscreceivemodule.c", "src/oscreceivermodule.c"]
include_dirs = ['include']
libraries = ['portaudio', 'portmidi', 'sndfile', 'lo']

extension = [Extension("_pyo", source_files, include_dirs=include_dirs, libraries=libraries, extra_link_args=["-mmacosx-version-min=10.4"])]

setup(  name = "_pyo",
        version = "0.01",
        ext_modules = extension)
         
os.system('rm -rf build')
      
