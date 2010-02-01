from distutils.sysconfig import get_python_lib
import os

DEMOS_PATH = os.path.join(get_python_lib(), "pyodemos")

from pyolib.server import *
from pyolib.control import *
from pyolib.effects import *
from pyolib.input import *
from pyolib.midi import *
from pyolib.OSC import *
from pyolib.sfplayer import *
from pyolib.table import *
from pyolib.trigger import *

from pyolib.pattern import *

from pyolib.bandsplitter import *

from pyolib.hilbert import *

from pyolib.pan import *
