#! /usr/bin/env python
# encoding: utf-8
"""
E-Pyo is a simple text editor especially configured to edit pyo audio programs.

You can do absolutely everything you want with this piece of software.

Olivier Belanger - 2012

TODO:
    - Fix printing to pdf
    - Output panel close button on OSX (display only) 
"""
from __future__ import with_statement
import sys
import __builtin__
__builtin__.EPYO_APP_OPENED = True
        
if sys.platform == "linux2":
    import wxversion
    if wxversion.checkInstalled("3.0"):
        wxversion.select("3.0")
    elif wxversion.checkInstalled("2.8"):
        wxversion.select("2.8")

import os, string, inspect, keyword, wx, codecs, subprocess, unicodedata
import contextlib, StringIO, shutil, copy, pprint, random, time, threading
from types import UnicodeType, MethodType, ListType
from wx.lib.wordwrap import wordwrap
from wx.lib.embeddedimage import PyEmbeddedImage
import wx.lib.colourselect as csel
import wx.lib.scrolledpanel as scrolled
import wx.lib.dialogs
import wx.combo
import wx.stc  as stc
import wx.lib.agw.flatnotebook as FNB
from pyo import *
from PyoDoc import ManualFrame

reload(sys)
sys.setdefaultencoding("utf-8")

################## SETUP ##################
PLATFORM = sys.platform
DEFAULT_ENCODING = sys.getdefaultencoding()
ENCODING = sys.getfilesystemencoding()
ENCODING_LIST = ["utf_8", "latin_1", "mac_roman", "cp1252", "cp1250", "utf_16"]
ENCODING_DICT = {'cp-1250': 'cp1250', 'cp-1251': 'cp1251', 'cp-1252': 'cp1252', 
                 'latin-1': 'latin_1', 'mac-roman': 'mac_roman', 
                 'utf-8': 'utf_8', 'utf-16': 'utf_16', 
                 'utf-16 (Big Endian)': 'utf_16_be', 
                 'utf-16 (Little Endian)': 'utf_16_le', 'utf-32': 'utf_32', 
                 'utf-32 (Big Endian)': 'utf_32_be', 
                 'utf-32 (Little Endian)': 'utf_32_le'}

APP_NAME = 'E-Pyo'
APP_VERSION = PYO_VERSION
OSX_APP_BUNDLED = False
WIN_APP_BUNDLED = False

################## Utility Functions ##################
@contextlib.contextmanager
def stdoutIO(stdout=None):
    old = sys.stdout
    if stdout is None:
        stdout = StringIO.StringIO()
    sys.stdout = stdout
    yield stdout
    sys.stdout = old

def convert_line_endings(temp, mode):
    #modes:  0 - Unix, 1 - Mac, 2 - DOS
    if mode == 0:
        temp = string.replace(temp, '\r\n', '\n')
        temp = string.replace(temp, '\r', '\n')
    elif mode == 1:
        temp = string.replace(temp, '\r\n', '\r')
        temp = string.replace(temp, '\n', '\r')
    elif mode == 2:
        import re
        temp = re.sub("\r(?!\n)|(?<!\r)\n", "\r\n", temp)
    return temp

def ensureNFD(unistr):
    if PLATFORM in ['linux2', 'win32']:
        encodings = [DEFAULT_ENCODING, ENCODING,
                     'cp1252', 'iso-8859-1', 'utf-16']
        format = 'NFC'
    else:
        encodings = [DEFAULT_ENCODING, ENCODING,
                     'macroman', 'iso-8859-1', 'utf-16']
        format = 'NFC'
    decstr = unistr
    if type(decstr) != UnicodeType:
        for encoding in encodings:
            try:
                decstr = decstr.decode(encoding)
                break
            except UnicodeDecodeError:
                continue
            except:
                decstr = "UnableToDecodeString"
                print "Unicode encoding not in a recognized format..."
                break
    if decstr == "UnableToDecodeString":
        return unistr
    else:
        return unicodedata.normalize(format, decstr)

def toSysEncoding(unistr):
    try:
        if PLATFORM == "win32":
            unistr = unistr.encode(ENCODING)
        else:
            unistr = unicode(unistr)
    except:
        pass
    return unistr

def hex_to_rgb(value):
    value = value.lstrip('#')
    lv = len(value)
    return tuple(int(value[i:i+lv/3], 16) for i in range(0, lv, lv/3))

################## Paths ##################
TEMP_PATH = os.path.join(os.path.expanduser('~'), '.epyo')
if not os.path.isdir(TEMP_PATH):
    os.mkdir(TEMP_PATH)

PREFERENCES_PATH = os.path.join(TEMP_PATH, "epyo-prefs.py")
if not os.path.isfile(PREFERENCES_PATH):
    with open(PREFERENCES_PATH, "w") as f:
        f.write("epyo_prefs = {}")

epyo_prefs = {}
with open(PREFERENCES_PATH, "r") as f:
    text = f.read()
exec text in locals()
PREFERENCES = copy.deepcopy(epyo_prefs)

tmp_version = PREFERENCES.get("version", "no_version_pref")
if tmp_version != APP_VERSION:
    tmp_version = APP_VERSION
    print "Erasing preferences..."
    shutil.rmtree(os.path.join(TEMP_PATH, "doc"), True)
    PREFERENCES = {}
PREFERENCES["version"] = tmp_version

RESOURCES_PATH = PREFERENCES.get("resources_path", TEMP_PATH)

TEMP_FILE = os.path.join(TEMP_PATH, 'epyo_tempfile.py')

if PLATFORM == "win32" and sys.executable.endswith("E-Pyo.exe"):
    os.chdir(os.path.dirname(sys.executable))
    WIN_APP_BUNDLED = True

if PLATFORM == "darwin" and '/%s.app' % APP_NAME in os.getcwd():
    OSX_APP_BUNDLED = True
    
# Check for which Python to use #
if PLATFORM == "win32":
    WHICH_PYTHON = PREFERENCES.get("which_python", 
                            "C:\Python%d%d\python.exe" % sys.version_info[:2])
else:
    WHICH_PYTHON = PREFERENCES.get("which_python", "")
INSTALLATION_ERROR_MESSAGE = ""
CALLER_NEED_TO_INVOKE_32_BIT = False
SET_32_BIT_ARCH = "export VERSIONER_PYTHON_PREFER_32_BIT=yes;"
if WHICH_PYTHON == "":
    if OSX_APP_BUNDLED:
        proc = subprocess.Popen(["export PATH=/usr/local/bin:$PATH;which python"], 
                    shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        WHICH_PYTHON = proc.communicate()[0][:-1]
    elif PLATFORM == "darwin":
        proc = subprocess.Popen(["which python"], shell=True, 
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        WHICH_PYTHON = proc.communicate()[0][:-1]
    elif PLATFORM == "linux2":
        proc = subprocess.Popen(["which python"], shell=True, 
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        WHICH_PYTHON = proc.communicate()[0][:-1]
    else:
        ### No more used on Windows ###
        if "Python" in os.getenv("Path"):
            pos = os.getenv("Path").index("Python")
            ver = os.getenv("Path")[pos+6:pos+8]
            WHICH_PYTHON = "C:\Python%s\python.exe" % ver
        else:
            for i in reversed(range(5, 8)):
                if os.path.isfile("C:\Python2%d\python.exe" % i):
                    WHICH_PYTHON = "C:\Python2%d\python.exe" % i
                    break
        if WHICH_PYTHON == "":
            INSTALLATION_ERROR_MESSAGE = "Python 2.x doesn't seem to be installed on this computer. Check your Python installation and try again."

# Check for WxPython / Pyo installation and architecture #
if OSX_APP_BUNDLED:
    tmphome = os.environ["PYTHONHOME"]
    tmppath = os.environ["PYTHONPATH"]
    tmpexecpath = os.environ["EXECUTABLEPATH"]
    tmprscpath = os.environ["RESOURCEPATH"]
    tmpargv0 = os.environ["ARGVZERO"]
    cmd = 'export -n PYTHONHOME PYTHONPATH EXECUTABLEPATH RESOURCEPATH ARGVZERO;env;%s -c "import pyo";export PYTHONHOME=%s;export PYTHONPATH=%s;export EXECUTABLEPATH=%s;export RESOURCEPATH=%s;export ARGVZERO=%s' % (WHICH_PYTHON, tmphome, tmppath, tmpexecpath, tmprscpath, tmpargv0)
    cmd2 = 'export -n PYTHONHOME PYTHONPATH EXECUTABLEPATH RESOURCEPATH ARGVZERO;env;%s -c "import wx";export PYTHONHOME=%s;export PYTHONPATH=%s;export EXECUTABLEPATH=%s;export RESOURCEPATH=%s;export ARGVZERO=%s' % (WHICH_PYTHON, tmphome, tmppath, tmpexecpath, tmprscpath, tmpargv0)
else:
    cmd = '%s -c "import pyo"' % WHICH_PYTHON
    cmd2 = '%s -c "import wx"' % WHICH_PYTHON
proc = subprocess.Popen([cmd], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
IMPORT_PYO_STDOUT, IMPORT_PYO_STDERR = proc.communicate()
if "ImportError" in IMPORT_PYO_STDERR:
    if "No module named" in IMPORT_PYO_STDERR:
        INSTALLATION_ERROR_MESSAGE = "Pyo is not installed in the current Python installation. Audio programs won't run.\n\nCurrent Python path: %s\n" % WHICH_PYTHON
    elif "no appropriate 64-bit architecture" in IMPORT_PYO_STDERR or "but wrong architecture" in IMPORT_PYO_STDERR:
        CALLER_NEED_TO_INVOKE_32_BIT = True
        INSTALLATION_ERROR_MESSAGE = "The current Python installation is running in 64-bit mode but pyo installation is 32-bit.\n\n"
        if PLATFORM == "darwin":
            INSTALLATION_ERROR_MESSAGE += "'VERSIONER_PYTHON_PREFER_32_BIT=yes' will be invoked before calling python executable.\n\n"
        INSTALLATION_ERROR_MESSAGE += "Current Python path: %s\n" % WHICH_PYTHON
else:
    proc = subprocess.Popen([cmd2], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    IMPORT_WX_STDOUT, IMPORT_WX_STDERR = proc.communicate()
    if "ImportError" in IMPORT_WX_STDERR:
        if "No module named" in IMPORT_WX_STDERR:
            INSTALLATION_ERROR_MESSAGE = "WxPython is not installed in the current Python installation. It is needed by pyo to show graphical display.\n\nCurrent Python path: %s\n" % WHICH_PYTHON
        elif "no appropriate 64-bit architecture" in IMPORT_WX_STDERR:
            CALLER_NEED_TO_INVOKE_32_BIT = True
            INSTALLATION_ERROR_MESSAGE = "The current Python installation is running in 64-bit mode but WxPython installation is 32-bit.\n\n"
            if PLATFORM == "darwin":
                INSTALLATION_ERROR_MESSAGE += "'VERSIONER_PYTHON_PREFER_32_BIT=yes' will be invoked before calling python executable.\n\n"
            INSTALLATION_ERROR_MESSAGE += "Current Python path: %s\n" % WHICH_PYTHON
        
if OSX_APP_BUNDLED:
    EXAMPLE_PATH = os.path.join(os.getcwd(), "examples")
elif WIN_APP_BUNDLED:
    EXAMPLE_PATH = os.path.join(os.getcwd(), "Resources", "examples")
else:
    EXAMPLE_PATH = os.path.join(os.getcwd(), "../examples")
EXAMPLE_FOLDERS = [folder.capitalize() for folder in os.listdir(EXAMPLE_PATH) if folder[0] != "." and folder not in ["snds", "fft"]]
EXAMPLE_FOLDERS.append("FFT")
EXAMPLE_FOLDERS.sort()

SNIPPET_BUILTIN_CATEGORIES = ['Audio', 'Control', 'Interface', 'Utilities']
SNIPPETS_PATH = os.path.join(RESOURCES_PATH, 'snippets')
if not os.path.isdir(SNIPPETS_PATH):
    os.mkdir(SNIPPETS_PATH)
    for rep in SNIPPET_BUILTIN_CATEGORIES:
        if not os.path.isdir(os.path.join(SNIPPETS_PATH, rep)):
            os.mkdir(os.path.join(SNIPPETS_PATH, rep))
            if WIN_APP_BUNDLED:
                files = [f for f in os.listdir(os.path.join(os.getcwd(), "Resources", "snippets", rep)) if f[0] != "."]
                for file in files:
                    shutil.copy(os.path.join(os.getcwd(), "Resources", "snippets", rep, file), os.path.join(SNIPPETS_PATH, rep))
            else:
                files = [f for f in os.listdir(os.path.join(os.getcwd(), "snippets", rep)) if f[0] != "."]
                for file in files:
                    shutil.copy(os.path.join(os.getcwd(), "snippets", rep, file), os.path.join(SNIPPETS_PATH, rep))
SNIPPETS_CATEGORIES = [rep for rep in os.listdir(SNIPPETS_PATH) if os.path.isdir(os.path.join(SNIPPETS_PATH, rep))]
SNIPPET_DEL_FILE_ID = 30
SNIPPET_ADD_FOLDER_ID = 31

FILTERS_FILE = os.path.join(RESOURCES_PATH, 'filters.py')
if not os.path.isfile(FILTERS_FILE):
    f = open(FILTERS_FILE, "w")
    f.write('##################### E-Pyo filters file #####################\n')
    f.write('# A filter is a function taking a chunk of text in argument, #\n')
    f.write('# apply some text processing and return the new text chunk.  #\n')
    f.write('# When you select one of these functions in the Filters menu #\n')
    f.write('# the filter will be applied to the current selected text.   #\n')
    f.write('##############################################################\n')
    f.write('def example_filter(text):\n')
    f.write('    """Adds a "#" character in front of selected lines."""\n')
    f.write('    out = ""\n')
    f.write('    for line in text.splitlines(True):\n')
    f.write('        out += "# " + line\n')
    f.write('    return out\n')

STYLES_PATH = os.path.join(RESOURCES_PATH, "styles")
if not os.path.isdir(STYLES_PATH):
    os.mkdir(STYLES_PATH)
    if WIN_APP_BUNDLED:
        files = [f for f in os.listdir(os.path.join(os.getcwd(), "Resources", "styles")) if f[0] != "."]
        for file in files:
            shutil.copy(os.path.join(os.getcwd(), "Resources", "styles", file), os.path.join(STYLES_PATH, file))
    else:
        files = [f for f in os.listdir(os.path.join(os.getcwd(), "styles")) if f[0] != "."]
        for file in files:
            shutil.copy(os.path.join(os.getcwd(), "styles", file), os.path.join(STYLES_PATH, file))
DEFAULT_STYLE = os.path.join(STYLES_PATH, "Default")
if not os.path.isfile(os.path.join(STYLES_PATH, "Default")):
    shutil.copy(os.path.join(os.getcwd(), "styles", "Default"), DEFAULT_STYLE)
if PREFERENCES.has_key("pref_style"):
    PREF_STYLE = os.path.join(ensureNFD(STYLES_PATH), PREFERENCES["pref_style"])
else:
    PREF_STYLE = DEFAULT_STYLE

MARKERS_PATH = os.path.join(TEMP_PATH, 'markers')
MARKERS_FILE = os.path.join(MARKERS_PATH, 'markers_file_list')
if not os.path.isdir(MARKERS_PATH):
    os.mkdir(MARKERS_PATH)
if not os.path.isfile(MARKERS_FILE):
    with open(MARKERS_FILE, "w") as f:
        f.write("=\n")

BACKGROUND_SERVER_DEFAULT_ARGS = 'sr=44100, nchnls=2, buffersize=256, duplex=1, audio="portaudio", jackname="pyo"'
BACKGROUND_SERVER_ARGS = PREFERENCES.get("background_server_args", BACKGROUND_SERVER_DEFAULT_ARGS)

################## TEMPLATES ##################
HEADER_TEMPLATE = """#!/usr/bin/env python
# encoding: utf-8
"""

PYO_TEMPLATE = """#!/usr/bin/env python
# encoding: utf-8
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=1).boot()




s.gui(locals())
"""

CECILIA5_TEMPLATE = '''class Module(BaseModule):
    """
    Module's documentation
    
    """
    def __init__(self):
        BaseModule.__init__(self)
        self.snd = self.addSampler("snd")
        self.out = Mix(self.snd, voices=self.nchnls, mul=self.env)


Interface = [
    csampler(name="snd"),
    cgraph(name="env", label="Overall Amplitude", func=[(0,1),(1,1)], col="blue"),
    cpoly()
]
'''

ZYNE_TEMPLATE = '''class MySynth(BaseSynth):
    """
    Synth's documentation

    """
    def __init__(self, config):
        # `mode` handles pitch conversion : 1 for hertz, 2 for transpo, 3 for midi
        BaseSynth.__init__(self, config, mode=1)
        self.fm1 = FM(self.pitch, ratio=self.p1, index=self.p2, mul=self.amp*self.panL).mix(1)
        self.fm2 = FM(self.pitch*0.997, ratio=self.p1, index=self.p2, mul=self.amp*self.panR).mix(1)
        self.filt1 = Biquad(self.fm1, freq=self.p3, q=1, type=0)
        self.filt2 = Biquad(self.fm2, freq=self.p3, q=1, type=0)
        self.out = Mix([self.filt1, self.filt2], voices=2)


MODULES = {
            "MySynth": { "title": "- Generic module -", "synth": MySynth, 
                    "p1": ["Ratio", 0.5, 0, 10, False, False],
                    "p2": ["Index", 5, 0, 20, False, False],
                    "p3": ["LP cutoff", 4000, 100, 15000, False, True]
                    },
          }
'''

AUDIO_INTERFACE_TEMPLATE = '''#!/usr/bin/env python
# encoding: utf-8
import wx
from pyo import *

s = Server().boot()

class MyFrame(wx.Frame):
    def __init__(self, parent, title, pos, size):
        wx.Frame.__init__(self, parent, -1, title, pos, size)
        self.panel = wx.Panel(self)
        self.panel.SetBackgroundColour("#DDDDDD")

        self.freqPort = SigTo(value=250, time=0.05, init=250)
        self.sine = Sine(freq=self.freqPort, mul=0.3).mix(2).out()

        self.onOffText = wx.StaticText(self.panel, id=-1, label="Audio", 
                                       pos=(28,10), size=wx.DefaultSize)
        self.onOff = wx.ToggleButton(self.panel, id=-1, label="on / off", 
                                     pos=(10,28), size=wx.DefaultSize)
        self.onOff.Bind(wx.EVT_TOGGLEBUTTON, self.handleAudio)

        self.frTxt = wx.StaticText(self.panel, id=-1, label="Freq: 250.00", 
                                      pos=(140,60), size=(250,50))
        self.freq = wx.Slider(self.panel, id=-1, value=25000, minValue=5000, 
                              maxValue=1000000, pos=(140,82), size=(250,50))
        self.freq.Bind(wx.EVT_SLIDER, self.changeFreq)
        
    def handleAudio(self, evt):
        if evt.GetInt() == 1:
            s.start()
        else:
            s.stop()

    def changeFreq(self, evt):
        x = evt.GetInt() * 0.01
        self.frTxt.SetLabel("Freq: %.2f" % x)
        self.freqPort.value = x
        
app = wx.App(False)
mainFrame = MyFrame(None, title='Simple App', pos=(100,100), size=(500,300))
mainFrame.Show()
app.MainLoop()
'''

WXPYTHON_TEMPLATE = '''#!/usr/bin/env python
# encoding: utf-8
import wx

class MyFrame(wx.Frame):
    def __init__(self, parent, title, pos, size):
        wx.Frame.__init__(self, parent, -1, title, pos, size)
        self.panel = wx.Panel(self)
        self.panel.SetBackgroundColour("#DDDDDD")


if __name__ == "__main__":
    app = wx.App(False)
    mainFrame = MyFrame(None, title='Simple App', pos=(100,100), size=(500,300))
    mainFrame.Show()
    app.MainLoop()
'''

RADIOPYO_TEMPLATE = '''#!/usr/bin/env python
# encoding: utf-8
"""
Template for a RadioPyo song (version 1.0).

A RadioPyo song is a musical python script using the python-pyo 
module to create the audio processing chain. You can connect to
the radio here : http://radiopyo.acaia.ca/ 

There is only a few rules:
    1 - It must be a one-page script.
    2 - No soundfile, only synthesis.
    3 - The script must be finite in time, with fade-in and fade-out 
        to avoid clicks between pieces. Use the DURATION variable.

belangeo - 2014

"""
from pyo import *

################### USER-DEFINED VARIABLES ###################
### READY is used to manage the server behaviour depending ###
### of the context. Set this variable to True when the     ###
### music is ready for the radio. TITLE and ARTIST are the ###
### infos shown by the radio player. DURATION set the      ###
### duration of the audio file generated for the streaming.###
##############################################################
READY = False           # Set to True when ready for the radio
TITLE = "Song Title"    # The title of the music
ARTIST = "Artist Name"  # Your artist name
DURATION = 300          # The duration of the music in seconds
##################### These are optional #####################
GENRE = "Electronic"    # Kind of your music, if there is any
DATE = 2014             # Year of creation

####################### SERVER CREATION ######################
if READY:
    s = Server(duplex=0, audio="offline").boot()
    s.recordOptions(dur=DURATION, filename="radiopyo.ogg", fileformat=7)
else:
    s = Server(duplex=0).boot()


##################### PROCESSING SECTION #####################
# global volume (should be used to control the overall sound)
fade = Fader(fadein=0.001, fadeout=10, dur=DURATION).play()


###
### Insert your algorithms here...
###


#################### START THE PROCESSING ###################
s.start()
if not READY:
    s.gui(locals())
'''

TEMPLATE_NAMES = {98: "Header", 97: "Pyo", 96: "WxPython", 95: "Cecilia5", 
                  94: "Zyne", 93: "Audio Interface", 92: "RadioPyo"}
TEMPLATE_DICT = {98: HEADER_TEMPLATE, 97: PYO_TEMPLATE, 96: WXPYTHON_TEMPLATE, 
                 95: CECILIA5_TEMPLATE, 94: ZYNE_TEMPLATE, 
                 93: AUDIO_INTERFACE_TEMPLATE, 92: RADIOPYO_TEMPLATE}

TEMPLATE_PATH = os.path.join(RESOURCES_PATH, "templates")
if not os.path.isdir(TEMPLATE_PATH):
    os.mkdir(TEMPLATE_PATH)

templateid = 91
template_files = sorted([f for f in os.listdir(TEMPLATE_PATH) if f.endswith(".py")])   
for f in template_files:
    try:
        with open(os.path.join(TEMPLATE_PATH, f)) as ftemp:
            ftext = ftemp.read()
        TEMPLATE_NAMES[templateid] = f.replace(".py", "")
        TEMPLATE_DICT[templateid] = ftext
        templateid -= 1
    except:
        pass

################## BUILTIN KEYWORDS COMPLETION ##################
FROM_COMP = ''' `module` import `*`
'''
EXEC_COMP = ''' "`expression`" in `self.locals`
'''
RAISE_COMP = ''' Exception("`An exception occurred...`")
'''
TRY_COMP = ''':
    `expression`
except:
    `print "Ouch!"`
'''
IF_COMP = ''' `expression1`:
    `pass`
elif `expression2`:
    `pass`
else:
    `pass`
'''
DEF_COMP = ''' `fname`():
    `"""Doc string for fname function."""`
    `pass`
'''
CLASS_COMP = ''' `Cname`:
    `"""Doc string for Cname class."""`
    def __init__(self):
        `"""Doc string for __init__ function."""`
        `pass`
'''
FOR_COMP = """ i in range(`10`):
    `print i`
"""
WHILE_COMP = """ `i` `>` `0`:
    `i -= 1`
    `print i`
"""
ASSERT_COMP = ''' `expression` `>` `0`, "`expression should be positive`"
'''
BUILTINS_DICT = {"from": FROM_COMP, "try": TRY_COMP, "if": IF_COMP, 
                 "def": DEF_COMP, "class": CLASS_COMP, "for": FOR_COMP, 
                 "while": WHILE_COMP, "exec": EXEC_COMP, "raise": RAISE_COMP, 
                 "assert": ASSERT_COMP}

################## Interface Bitmaps ##################
catalog = {}
close_panel_icon_png = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAAA29JREFU"
    "eJzcVclrE1EcfvPem8zQRRNrumQS6lYRFYLUgxZBccWToODeoIKiCC4HL/UfKPTgxVBsD1Lc"
    "ULSYUlxA8KaiVDGut5auMdWkaTozmeW98TdxoTRNerEXB14mZN77vt/3/b7fhDqOg+bzovOK"
    "/n8SCIKQv3swDofKygLjudzTSdvmRc7LtbK8GztOT8IwmLtpZk+LKfA2+v29B1esCD7v7488"
    "Gh29Yc4CXuXxvGpuaAgzTbt7Z3DwyJhlsTkVQOXrALznUmNjsJZztHTJklZu25ln37/3apz/"
    "USLXyHL05KpV4Z2Vlcik9ICkKInrIyMXXRElCZZVVKw5unJlHtzUNOTDOBAJhWLwKPIkmXSV"
    "5Ct3wbeUlSEhm0UY9m5dsGDTkK5j2MdKEiR1/e6LgYG65fX1LRVgFYHD1YSgE4rSijiffK9p"
    "e1xbdpWXIwTgonvIceLfLOvsF1Wd26KUZVkPhofbEGPp/YrS6YOm27aNFglCoLm29uEOStFq"
    "SULCxASyGHNDEe9nrCk6Nqa+nZqaCTd7k3OQhFgi0QWJ8B2tq2up4NxLLQst1nVUA3c7l0PO"
    "b/ABjE9FVVXtAztneycUnYMpzq3uRCKvZK/H01mZySAbwCEEiMPChHwYIKSpPZtVX7uERXBK"
    "DhookV9OTjZt8PvRWlCFAVgGixyXgNLFfZxv+2QYvQBebE5KEsiLRPHVgVAovAzkMyBwK3d+"
    "L5GQuh2SFBtiLPIok5ltTkoSyDWSFD0B4FsAHKVSSIIkcUI+UlEMUoy9rpJ6+O1CVVUrISTz"
    "eGJi+pyUJMhXfhzAN6oqctJpxN0kQUMHwfPPnB/aDj2pAQIBlkJp4Hx1dQzsi8R+/LgxJ0FA"
    "kvYdVpTwZogch8rFX02Nj1B6ql3T1K+m2ZXwen1nfL4Wvyh6yz0e1ABKLodC56Bnt9CMfhQQ"
    "wOB0k0ymz8hmGwVQbBMSH4LKO3RdffMrLda9dLqNYJw+4/d3loMKLIpooSBEKymd26Jhw9Bv"
    "M7ZZkqRr6xlbM0LI6Q7DUN8axt8ounNyP5XqAu99l4LBYxLGbVdHR2/2jI8X+F1A4M76kG2r"
    "1x2neVCWadw0rXemWZDzLMzJzWSyTXOcK1QQ7G4AnyrscSHBtPe5+8UqOFF42e5HV5GH/+Ff"
    "5r++fgIAAP//AwDHcZZetNGOQQAAAABJRU5ErkJggg==")
catalog['close_panel_icon.png'] = close_panel_icon_png

file_add_icon_png = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAABDBJREFU"
    "eJyMVV1IHFcU/mZ29ifZXbNutMbUxgSaJooiURT1IVQKUvpSpNifvEiwr6GB5kEoNIVSDAmS"
    "UmglkFCaH2xIU6k15MG0EmkwQpq4lfiTmG5rR+Ouu2bH/d/Zmem9E2c7M7trc4bD3pl77vfd"
    "891zz3L4z5hNfzGzw4q96EEHerkGrsFqszLpRHpa9skXMIZvsQQRCsDpltRUVFR86vF41BdF"
    "UcAwTMExNb6Eb5Helva1HWpD+yvt8Ng8CCQCLeO7x1t8Fb6PpJ+lLszgkZ5g28cnTrzX3d2t"
    "gqkPy+SAn397bpOrkzg2fQwddR1oq2zGNtGOStvLKLHZUdV0BGXestpb3lvD8lX5kJ4ANqsN"
    "drtDBWM3wc0upAWcenQKZdVlqCh1YT07j+rMa3hjTye+njuJDfcG9lfuwez+3bX8Yb7XQEA3"
    "y7JsUXBqd1bvwJf1ocF5EOHMHGxEsnjMRbVDPO5H3LGKsAg4PXaQM/rQRGDcOasBb46p/lOh"
    "uxAdIpJ/J7BrZwvcHINy16skhEW5WAss78CujIiNZ7OAF/UGAnp8rA6QIdlo2tOxLEkIZ8Nq"
    "adRbGnHy8FdqHMtYwLEcjrb2EQwFyUwSt3+6j6f2MPIyyIHqMtBcIV7KlarFPJEaw/s321Fu"
    "A5pcb6K39TN88UsPlpQFrKYV/Mk9pnFPjAQaSSEnxBYy3+hsgjVqRcgVwfz2CDJOIrVco+48"
    "6FjAPw4fnsSAWJIEhzCqJ1C0emdMEuXOwmJBW3krGoINuCfew0oEkAnQwR0SUlKK7FxWwQNp"
    "QLJgGb/hUr5EOkCzRNSrXqrCcf44+mJ94DM8lgjJtcgYJoOvYzH+GAJJU3IigFl8gh+MF009"
    "3KISbbrD4UBnbSfS99M4J5/DjHcGK8kIVrIRMG4GLocb7AT7ufCN8CM2kDIS6CQqdg+o0XbS"
    "1dKFAwsHMMVPYS41h7gSR2m2FPXkufHwxu+jwmiUxhaU6P8I6LikpATNjc2oq6lDNBqFKIqk"
    "C9jhdrvx4OYDUYs1ZrC52CyVYU73y3GcSkRBtSZoKGvyLe+iaT1bD7yVZbNZSOQCyrKc67qS"
    "rOQIX0gi/Zw2pgBUllQqRXpQHMlkUiUCa5ngl5dXCkrE6Ii2MgpOd06BA8EggoEg0pkMnM7t"
    "fwwPD39we/zXpwUJtLTM38yEVA6687W1EHh+Gfv2VoP8o4W/HxrquXD+/ArNpnAGuu5ZSB7t"
    "nUqRSCQQXFtTL6PX6/3rypXL7549++W0IAiGzeRVkV4qDdyckZZBLBZD2U7v/MWL3711+vQZ"
    "f5DIZVYhTyLNi5k2LxESq9W6ODIy8s7g4KB/fX294Do9QTIUCl31+/158uizoSAZUj2hZxHh"
    "2vXrZ4YuX1qkF62Y6Qnm+/v7jwwMDBQNNmdBKkfOiuKWsf8CAAD//wMAHNbcbWLj1ggAAAAA"
    "SUVORK5CYII=")
catalog['file_add_icon.png'] = file_add_icon_png

folder_add_icon_png = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAABShJREFU"
    "eJykVW1sU1UYfu5H721vv7t1DFbXDZmb2UR0QNAYJTELkR9AJBKJ8SMx0R8mZiRojAkJMUSj"
    "EBMkasSEH2ZO8IcIGWYh8ZuJgwg4gS3LFCcg+7Dt7bp1be+X7zltZ8cIkHhvT07v7Tnv8z7P"
    "+7ynMm7z8njc0VDA11RT7W9sbY7fWV8XXtbWeldj/dKWZZo/WiuJib2PPvZyZzKZnrdPrnwQ"
    "BKGuKhLsiIS0hrracCy2OHRHdViJN8Qi9N7nC4cjCEaWIBCJQwvGoHoCkOVZ+DwzSOo5jyA4"
    "CxKbB7CmvXn/h29uWr84qsCluuEIXphSFFDqIHuWQNWqILs0ONYszNxlyNIkFDcgCoCetm/I"
    "fB5AbJEWamr0wkAQqHocbi0CSXKV2fHZLMwilz6FYFiCQ7dAt0OJO45F4xYMwDbwQDLRDxN9"
    "BYVCgf9iWxZ0XUc2k0Q0lKU3GnhkCKWtdvnbzQAWXuPj43xjIBjkQ3QIkLKFbRYBSsz4860A"
    "OMXip7RXQCQchk0PhmHwwWRhwRw2KgDmnm8GwALywe5SYvl8HpZtcVQGJAgiZyDYBs9CqGAg"
    "3ECjm0rEMpJdVGSKJcsyCsSAgcNgDKR5DODcpkR8wJmja9tF+5lUZEmSkJ3JwSWSY4jB/5RI"
    "4BIY5CKRAovlQCU5LEsm6SRuAEk0ixLdisGCqxS0QHVgEjG5XIoHutGCgWQel6dTIGg0eGvg"
    "t/0dTc0t7cn+M79UMlkoUWlm0pQlYAy4TJTl96kL+Pj8UVyZnkQ5DANpi8QbO9/Z8W3vgS/X"
    "dXV1nTRNcyEAaSwXpRFgmhZE0toi7Zk9XaqCT//oxYHBHgRUD5qiteiofxCp/DSOj/6Ekezf"
    "2PX7Z/7tL2w5lkql1hw5cmS4EkBraarvfmLjAyt5/FJRBRrsuyy7cDYzhO6RHtQHPfDKApYG"
    "fXhx+WaM6FdxdqIPllvCVEHAB5cOh7e/9Oye/v6fN4yNjRcB3KqyeveOzRs7Hq6D4BTdYVBw"
    "ZkfLIquqIo6OHkXcryCsUjbk3KhHZKcQH1EPs4UNv0vAWHYWw+ro+kfWrg0dOnhI5wCiKKiR"
    "oMxmckOxsBZJRDyQJxfNSHkE5RHUVscQ8y+DKskIKiFI1HR+RcPKRfdCof6bKuQxmh5A2rog"
    "ta9qbyWAvrJE9vVtaHB5mP4mZpUpBFx5rKqNo6Oxs9jNKB5z9f7FJFUnZ53ITWH/uW3IFHS4"
    "varG6imX3FM8hkr2YjMrskPdaZGb3JJKXheQzo3iwuTXBCDD66pCPNiGGTOHS/qvJG0WGSNL"
    "fspAk9y0XpqYKzIh2eUmKx+/DMB2RH48RBAgWcIYSXyH4X++4etqvC14bkU3Lmeuoft8J3xS"
    "motgOwpqtA0Tl0b+vMiPmjmJrutD5iKR7GqQnzOpHFqr1uHU1d9olcmTsJ1CRf+YPITD1jsa"
    "GoT7d79/bKdRYVNhhvuhqFPJRSYvOntlEPzd/kcxERrAqN7LA5p2HmPTI0jOTpIsVvFfTdCw"
    "onrryXM9w3sHBwf/a7SCYZ4ZGEq+3tYced6jYikB5omBWySJPF6fQV19+toV/fh9gWfWh9Tq"
    "1UOJHui5a+gaeJr3iSab8CqN1M1bvvqrz3pqz+5dRvmQlIt6m9YrOz9560T/msNbN92TSmSD"
    "7zattKMz04lDw0ODX5w53Z/qO/EDFJfyxvZXtz257qG3X0vYF5fruavE0lUIKw196nTso96D"
    "P36+7719TjKZnJPvXwAAAP//AwDmNHbvm7mEowAAAABJRU5ErkJggg==")
catalog['folder_add_icon.png'] = folder_add_icon_png

refresh_tree_icon_png = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAABmJLR0QA/wD/AP+gvaeTAAAA"
    "CXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3wIHETgo92WyxQAABKJJREFUSMfdlVtsVFUY"
    "hb9zmTNnZtoOLRel3ATkASiYQMI1YkTxQVMaxMQbCGJCAiY8KAY0aIQYb5CoJEo0RlEgaUwD"
    "clOUWwABBZFIgVKQi3RaZnqdmXbOOXPO2Wf7gFTRQcFH19NO9v7X+ve/VvLDP+C56qP/dM3U"
    "dfX8GxRuAh/Xtox3jGhFxqNnxlXcDtu9KOpPH/hk3qR2gNtX15FcNJzZH+1n3fwpACzdepI3"
    "KysKEy7bcwGAlYcb7n73XL72pTqRmfeT8KoO+fKefV4wZqdnD9/htg6qblv1zsSrTQ5+eccf"
    "9VtrhxX8weI9F1k1dTAvbj0e0vv0W5qP9VhxNguXuyBpSbI5sC0QdgCuABmgOdaRUGvrDOe9"
    "iibK3jImfDpvX7mae3tj5R2bCo5o+f7LPbqKSjekstaDZxqa6xoz+UxrVkbcrDZM2mYUoYMS"
    "BTUGgQAEqmefCucS89yS0hfkbUWPxPymys4VE7cB6NeI+wwZw1MffmEmUsnX6r6srj935MBb"
    "ttQbLMu2AmGEVE/cpvSf9ozoMWqBVDXQwlDcDzSTgNBIO9J/F64fI2HjZdqUgiPqXzE5nk6c"
    "G9CVbq4DRKFQGKPnTveGzfxM6uE4KlBUDpEykBIUAYog3Fhbla+ZvgVA/XN14uTBTFe6+eQN"
    "yAGke2LtZr1p3yxFc9JENJApIAU9NCg1oWcUQlp34yr/Ad7hVdvMY2umKyUh6F0MPRUozkJf"
    "A8ojEOue/B8e3Az6DRlJ44VT9L5nTrh93KxXZWkcYhEwQxDSIepDvARKQ9f/4KHH59yUQOOF"
    "U5RULYt33FVVE8SLpyphUMij+A6K70C+k7iaJQjcgiarQPgBwAYMIPS7GT7QXoSWGzreuZhs"
    "N2WvgZNkJKqg6QI9JAgZgpAhZMgQpWUxmbnYWh8c/Cx9ncAr247NzI8bU7MrKchmBOm0oLMj"
    "T77NQqa7UFsbftXPfTPJ/XZl062MtduDEyk3+LEZMi0BVspDpFxosSDroLZdOqEdWT3XPb+9"
    "6VYD0S1Q36XK9mawExqyKYCOALpAcXzM0xsWWue3H/8vieuOacLWpNXgI68EkDHAVsGXSFTs"
    "0YtqYlWfDwKIzFhfkCg+4dnu84BR9/1dwGsLIOVChwOWA0KA9EFYyEDcbkf6fxWdsX6UvWnW"
    "VcKqDwDoe+8iADLfv8/AKQuifcfNDjfU7v67AHYg6XAgZ6PkLfByELigKBDkCQJvhKWXHQo/"
    "XL0EILN5IQBX9q4GwJy8+Mlkp7eqvbHOLLhwzPnfVTrGgC1qrvOwmdz/ut1nYrUUfowgfzWs"
    "0gPhgCJRpe+o0tmJyDfj5gYGrjVN5tqv6Gc3VXotZ44VNFn6gaH57bVm8uijua8XNpgPvHe/"
    "WzJ0XRCIO8EHLQRGFIwwgaabAVSiBODbaIkfdoZ+2fR8vuVM7V+90a4dSkY8KGTnlTXW1qeT"
    "AP75HQkjHN2oGGaRDPJjCRxQFTAMMIuguAxN11v1hsNLOL52uZequ3TDndzrsXW0Vs8GoOfw"
    "qbTV7bnuUUnvQeX5itlPiOLysUTiUUXTknrq573O7jd2SEjzv8ZvVwomHyvQ89AAAAAASUVO"
    "RK5CYII=")
catalog['refresh_tree_icon.png'] = refresh_tree_icon_png

file_delete_icon_png = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAABEZJREFU"
    "eJyMVG1MW1UYfm57b1vogNLSdbotdJNkyAp+TOrYogZd4uJ+6GaYyg81auLHj7lEEpdFjfqH"
    "ZQTHDxP2g5m5jbC5MJQxZthgmWEMo5IOEBCJMNPC1hbsJy29H/Wcjou3X7i3edNzz8fzvO9z"
    "3vOy+M+YZb8v0wLcZuDN54G3trDsIxqWZRZjMcctSWq5Apz8G+DjZB+rOPOwxWL5zGAwJD7i"
    "8TgYhsk4plbgdD75oihaH62qwsadO6El58J37tgf7OuzW4aHP7wointHgD+VBDkf1dW9UlNT"
    "kwBL/FTMCvC9uXs2NzCAWwcOwPZsNdZtr4RazSIuxZFrtaKwthbGoqKtxt7ejnOS9LiSABpO"
    "A61WlwBTLYOn+pLPh8kjR1BsMaGgYjM463oEfr0GVpAQFbQwVFXDcPs2SsbGtj7tcr2dRECD"
    "ValUWcET0ff3QxoZxppntkAw58G8aw8i8QhcV87ioVfrYCp/AjO9PcjXamEF3kkhSI5cJQMv"
    "j6n+84ODyJV4aKJ/IHS1AaM6DmUvvQtDRRVyjGvxc9NhxG+0At4wjIAtiYBen0oByJBsZO3p"
    "WBJFCPPz0KjpFh4GjRdj3V/hAfsemDeVYOJ6D4K/nMRGUxjhBVJpATBpGayAKjKQPU6cKyyE"
    "IABLQcAfMsH2fhMK11kwN9yLku1Pgd//CULtX4DJiVLIv5IJZJJMTohJ4NBv24agioXHJWD9"
    "6++h2P4cpr77GJHJVojVn6Ns7wcYGh+Eb/AH+IGLSoK4XO9MikQrd6FWw7xjBzzlFRAcQ3B+"
    "cwL8jAOC60esKRTh/f5TBB0D8PVcRdSD2RvA6XSJFICpElFfu2EDnAcPInzoEJZ+n8WM8xI0"
    "ZsBXQM6Taop1XQAzC/e4iMPngckkAmSTR+E6nQ5lu3djKBYDd/w4dKOjiExFESbHOSKyLleP"
    "fr3qy68DgQsBIJpMoJAo2zugRtuJfd8+TJaWwnnzJqLj45DCYfBGUpjl5Rjp7v7N39UVpHsz"
    "SvR/BHScn5+PxyorUWqzIRgMgud50gW0yMvLw2WHg5f3JmewfDhVqqQ1xT/LsgkiCio3waSy"
    "JnNpD03u2Urg1Uwgj0IkD1CSpJWuK5LGJxPel0TKNXlMAags0Si5YKJ/JBJJEEGl/snpcs1m"
    "lIhREK1mFJxGToHvut1w33VjiVSVXp873NHR8dr1a31zGQnktFLnUgmpHDRyj8cLp9OFTdZi"
    "cBpu/mxb2xsnWlpmaTaZM1B0z0zyyN9UisXFRbg9nsRjNBqNM62tZ/YfO9bk8Pv9ScGkVZFS"
    "Khk8NSM5g1AohCKTceLUqW9fOHq0YdpN5EpVIU0i2bOZvC4SEo7jpjo7O19ubm6eXlhYyHhO"
    "SRDxer3npqen0+RRZkNBYqR6vP/4/Ofb2xvazpyeog8tmykJJurr62sbGxuzbk7NglSOJPD8"
    "qnv/BQAA//8DAPE93uTkTcJiAAAAAElFTkSuQmCC")
catalog['file_delete_icon.png'] = file_delete_icon_png

left_arrow_png = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABQAAAAUCAYAAACNiR0NAAAAAXNSR0IArs4c6QAAAppJREFU"
    "eJzsVFtLVFEUXnvObS5NTSRqFzANDHNC6sGXCKdA6qEL0T+I3opepJeCegjDl8goKLqAQb2n"
    "JVpBvoR0I6zUQLSxcsYZnWluOueyz967dc6MkYSV5GMbPtY++6zz7W/t9e0jwwoP+T/hb4fH"
    "4wkKIQqIlSFsaN7V2bQ7YvTdvXU2M5PI/jMhCQTVluOnjoVbDx55dP1y+5v+7juWXjQWEVZv"
    "aTxfsaEuQiQPwA9IQCTi1IhzjKSE6vpww7TJgKzbtL61rf1afeRw27Pbl058HXnbJzgrEdbs"
    "iDRubzkaIZoK4NWAIIRPxSiD0BQADckVCQRuRBiHmG4DmDYQg4NS11S79/SV3k/PH/cO9XSd"
    "ySc+f5BzRQti6Xn8EBP9uHtAgLAFEJu7EZgCRBMlQmcNFYJBkZCC0C0gpkQC2/Yc2FmxdX90"
    "sOecPJEowJfhaQCfBhD0A6wOIDCu8iJwzY/KfahUxvIpkukUoIiYMxE6QL7oRplLslKzLyxT"
    "tADlqITjAThucCyx4IoFe4gynByGE1S6ALQSaASmzJncTWO+2CWrlaGMEt4cdxrhNgQThIcA"
    "0U0QBqr4VgCCbAI35YoUMipDfrfjMgFVApMkM/dZKn+VW8YYS08UZVtRLnCft9NVgIdOLIZn"
    "h6W54OXI3HJJUOuAqrWHVI+gUjzdzydnb3DDfMHyiSyLDmDHXmLps/EpPv6+VBpHArd8h8Au"
    "zXETsLi7pm6syvmTa0bYx3gHy+tPxXwuxSYHGR9/go3Klo396gHA6+6/Mraobb7HrZMXha5H"
    "eWzYZKMPQWSi+IL/dFOch6Wv5uJBzQGeitr0XbcQ8SFUT39JWdbVo9OjFJJjaOy5JXOW9/ui"
    "xh9TVvx/+B0AAP//AwDd4UcxPpGF3gAAAABJRU5ErkJggg==")
catalog['left_arrow.png'] = left_arrow_png

left_arrow_png = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABQAAAAUCAYAAACNiR0NAAAAAXNSR0IArs4c6QAAAppJREFU"
    "eJzsVFtLVFEUXnvObS5NTSRqFzANDHNC6sGXCKdA6qEL0T+I3opepJeCegjDl8goKLqAQb2n"
    "JVpBvoR0I6zUQLSxcsYZnWluOueyz967dc6MkYSV5GMbPtY++6zz7W/t9e0jwwoP+T/hb4fH"
    "4wkKIQqIlSFsaN7V2bQ7YvTdvXU2M5PI/jMhCQTVluOnjoVbDx55dP1y+5v+7juWXjQWEVZv"
    "aTxfsaEuQiQPwA9IQCTi1IhzjKSE6vpww7TJgKzbtL61rf1afeRw27Pbl058HXnbJzgrEdbs"
    "iDRubzkaIZoK4NWAIIRPxSiD0BQADckVCQRuRBiHmG4DmDYQg4NS11S79/SV3k/PH/cO9XSd"
    "ySc+f5BzRQti6Xn8EBP9uHtAgLAFEJu7EZgCRBMlQmcNFYJBkZCC0C0gpkQC2/Yc2FmxdX90"
    "sOecPJEowJfhaQCfBhD0A6wOIDCu8iJwzY/KfahUxvIpkukUoIiYMxE6QL7oRplLslKzLyxT"
    "tADlqITjAThucCyx4IoFe4gynByGE1S6ALQSaASmzJncTWO+2CWrlaGMEt4cdxrhNgQThIcA"
    "0U0QBqr4VgCCbAI35YoUMipDfrfjMgFVApMkM/dZKn+VW8YYS08UZVtRLnCft9NVgIdOLIZn"
    "h6W54OXI3HJJUOuAqrWHVI+gUjzdzydnb3DDfMHyiSyLDmDHXmLps/EpPv6+VBpHArd8h8Au"
    "zXETsLi7pm6syvmTa0bYx3gHy+tPxXwuxSYHGR9/go3Klo396gHA6+6/Mraobb7HrZMXha5H"
    "eWzYZKMPQWSi+IL/dFOch6Wv5uJBzQGeitr0XbcQ8SFUT39JWdbVo9OjFJJjaOy5JXOW9/ui"
    "xh9TVvx/+B0AAP//AwDd4UcxPpGF3gAAAABJRU5ErkJggg==")
catalog['left_arrow.png'] = left_arrow_png

delete_all_markers_png = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAABUVJREFU"
    "eJykVWlsVFUYPW+Z5b2ZznSmnUI7pQt0KKZojCUNmIgVUgmNEYIaFxJXMP4SQlwaImokYhSj"
    "gagoQWOFCG2oplE0khDUQKAsDSFCC0hYytJ1OoXZ3+Z378zUliKQ+N7cvGXu/c53zne++2Tc"
    "4aEozkC+xx0qKsyrrKkun1YW9FXNrJleWTZ1RpWaF5gsiUMb5i98dWU4PDJunTz2QRCEYIHf"
    "2+DPVyuCk32lpcX5Uwp99vKKUj+9d7t9Pj+8/hJ4/OVQvaVwKB7IcgJuJYZwJKkIgjUhsXEA"
    "s2urN29at7ixOGCHzeGEJbigSwHAHoSslMChFkC2qbCMBPRkD2RpAHYnIApAZMS8KfNxAMFJ"
    "an6o0gUNXqBgCZyqH5Jky7HjVz2dQHLkELw+CRadAp0WJW5ZBo3bMBDYAh5IJvo+om9HOp3m"
    "/5mGgUgkgvj1MAL5cXqjgkdGBhiWmbv7b4CbHX19fXyhx+vlQ7QIkLKFqWcAssz48+0AOMXM"
    "L7tWgN/ng0kPmqbxwWRhwSw2xgCMPt9SIprMBzuziaVSKRimwVEZkCCInIFgajwLYQwD4SYa"
    "3VIilpFsoyJTLFmWkSYGDBwaYyCNYwDrDiXiA9YoXdPM2E+nIkuShHgsCZtIjiEG/1MigUug"
    "kYtECizmAmXlMAyZpJN4fEnQMxLdjsGEIxs0TXVgEjG5bHYFyVg1ot0xJHoHKboIR3EREnZ3"
    "Q2j6jNrwoc6jY5lMlCh7ZdLkJGAMMolrSB05gkst25Hs683yoSREEa6qUOXGN5r2bvpp14Kt"
    "27Yd0HV9IgBpLGekEaDrBq0zSAqD29NJ2Ufa23BlZysceXnw0BCyCZi0RO+5iNjmL/JWvLBs"
    "V3h4eHZ7e/vpsQDqjFDZ908smjOLx88WVaDB7mXZBuOv4xhoa4WvvAzBZ57F0K8/I3X2bwi0"
    "EXnmPgS5uASXW7cjuq3Zt/L55R93dBx8tLe3LwPgdNjr1q95bFHD3CBllXGHRsGZHQ3DgpN0"
    "HmjbAVVRMGn+wyiY1wD3XTXo+3Q9HGVlCCx7Bcxrw0cPI3GsE94zpxrrH6zP39HSEuEAoig4"
    "/F6ZXcH50mGQRMQDKXKRMxWH7dJ52mEVxCnzaOkUuCnrktVvQ1JUmFSD81u+BE53w6M6YZ48"
    "LtXV1tYQwP6cROaNbahxeZj+VKyRCGSyoU2xoF8+i4G1r0He8A2cNffyuVe2N0P79nOo/kLI"
    "Pi+S1yJw2x0qq6ecdU9mG8rai11ZkS3qToPcJNC3gcGb1wcgyINwzVsCe8U0ai6T90vR/Q9A"
    "2FMF7cwJWHERolJA86T+0SLTJDPXZLntlwGYlsi3B93rIyno43P1EtyNj6BwxVqYkh0Xt25E"
    "QSiEvDmNKHrnM1xtWg791HlI9ff1n7lw4STfakYluqEPmYtEsqtGfh5MpjB5/gJc+7oLRiQB"
    "7XoUPTubYf62CcOuPNLThBGoQHSE7AzaTmrr1u9a96E2xqZCzOJfJq5T1kU6Lzp7lSZ4a+Fi"
    "OI8dRPz3PYidexJGuAc2MoFBeg+ua0LCcsHR3w/H40sPtJ27uKGrq+vfRktreufx7vDqmdX+"
    "lxQHphJgihg4RZJIcbk16urD5yLR3YGXVzW62rbUxf/Yy2ZQP7IEWONcg+Kx4HjuxV92K4Gl"
    "H721RsttknJGb914/d3vPtjXMfvHpxffPTwU934SmmUGYtGhltPdXT90Hu4Y3r/vT9ht9vfe"
    "XLXiqfp5jU22E4fu0a/0UDFtaaG8an+4YuZXWw8ea934/horHA6PSv0PAAAA//8DAO+NgvJ9"
    "Mnw5AAAAAElFTkSuQmCC")
catalog['delete_all_markers.png'] = delete_all_markers_png

############## Editor Key Commands Dict ##############
KEY_COMMANDS = {
"01. DOWN ARROW": "Move caret down one line",
"02. Shift + DOWN ARROW": "Move caret down one line extending selection to new caret position",
"03. UP ARROW": "Move caret up one line",
"04. Shift + UP ARROW": "Move caret up one line extending selection to new caret position",
"05. LEFT ARROW": "Move caret left one character",
"06. Shift + LEFT ARROW": "Move caret left one character extending selection to new caret position",
"07. RIGHT ARROW": "Move caret right one character",
"08. Shift + RIGHT ARROW": "Move caret right one character extending selection to new caret position",
"09. Alt + LEFT ARROW": "Move caret left one word",
"10. Alt + Shift + LEFT ARROW": "Move caret left one word extending selection to new caret position",
"11. Alt + RIGHT ARROW": "Move caret right one word",
"12. Alt + Shift + RIGHT ARROW": "Move caret right one word extending selection to new caret position",
"13. Ctrl/Cmd + LEFT ARROW": "Move caret to first position on display line",
"14. Ctrl/Cmd + Shift + LEFT ARROW": "Move caret to first position on display line extending selection to new caret position",
"15. Ctrl/Cmd + RIGHT ARROW": "Move caret to last position on line",
"16. Ctrl/Cmd + Shift + RIGHT ARROW": "Move caret to last position on line extending selection to new caret position",
"17. PRIOR": "Move caret one page up",
"18. Shift + PRIOR": "Move caret one page up extending selection to new caret position",
"19. NEXT": "Move caret one page down",
"20. Shift + NEXT": "Move caret one page down extending selection to new caret position",
"21. ESCAPE": "Cancel any modes such as call tip or auto-completion list display",
"22. Alt + BACK": "Delete the word to the left of the caret",
"23. Alt + Shift + BACK": "Delete the word to the right of the caret",
"24. Ctrl/Cmd + BACK": "Delete back from the current position to the start of the line",
"25. Ctrl/Cmd + Shift + BACK": "Delete forwards from the current position to the end of the line",
"26. TAB": "If selection is empty or all on one line replace the selection with a tab character. If more than one line selected, indent the lines. In the middle of a word, trig the AutoCompletion of pyo keywords. Just after an open brace following a pyo keyword, insert its default arguments. Just after a complete python builtin keyword, insert a default structure snippet. Just after a variable name, representing a pyo object, followed by a dot, trig the AutoCompletion of the object's attributes.",
"27. Shift + TAB": "Dedent the selected lines",
"28. Alt + 'C'": "Line Copy",
"29. Alt + 'D'": "Line Duplicate",
"30. Alt + 'X'": "Line Cut",
"31. Alt + 'V'": "Line Paste",
"32. Alt + CLICK + DRAG": "Rectangular selection",
"33. Shit + Return": "Show the init line of a pyo object in a tooltip",
"34. Ctrl/Cmd + Return": "Show the __doc__ string of a python object, module or function",
"35. CLICK in the most left margin": "Add a marker to the corresponding line",
"36. Shift + CLICK on a marker": "Delete the marker"
}

############## Allowed Extensions ##############
ALLOWED_EXT = PREFERENCES.get("allowed_ext", 
                              ["py", "c5", "txt", "", "c", "h", "cpp", "hpp", "zy",
                               "sh", "rst", "iss", "sg", "md", "jsfx-inc", "lua", "css"])

############## Pyo keywords ##############
tree = OBJECTS_TREE
PYO_WORDLIST = []
for k1 in tree.keys():
    if type(tree[k1]) == type({}):
        for k2 in tree[k1].keys():
            if type(tree[k1][k2]) == type({}):
                for k3 in tree[k1][k2].keys():
                    for val in tree[k1][k2][k3]:
                        PYO_WORDLIST.append(val)
            else:
                for val in tree[k1][k2]:
                    PYO_WORDLIST.append(val)
    else:
        for val in tree[k1]:
            PYO_WORDLIST.append(val)
PYO_WORDLIST.append("PyoObject")
PYO_WORDLIST.append("PyoTableObject")
PYO_WORDLIST.append("PyoMatrixObject")
PYO_WORDLIST.append("PyoPVObject")
PYO_WORDLIST.append("Server")

############## Styles Constants ##############
if wx.Platform == '__WXMSW__':
    FONT_SIZE = 8
    FONT_SIZE2 = 6
    DEFAULT_FONT_FACE = 'Verdana'
elif wx.Platform == '__WXMAC__':
    FONT_SIZE = 12
    FONT_SIZE2 = 9
    DEFAULT_FONT_FACE = 'Monaco'
else:
    FONT_SIZE = 8
    FONT_SIZE2 = 7
    DEFAULT_FONT_FACE = 'Monospace'


STYLES_GENERALS = ['default', 'background', 'selback', 'caret']
STYLES_TEXT_COMP = ['comment', 'commentblock', 'number', 'operator', 'string', 
                    'triple', 'keyword', 'pyokeyword', 'class', 'function', 
                    'linenumber']
STYLES_INTER_COMP = ['marginback', 'foldmarginback', 'markerfg', 'markerbg', 
                     'bracelight', 'bracebad', 'lineedge']
STYLES_LABELS = {'default': 'Foreground', 'background': 'Background', 
                 'selback': 'Selection', 'caret': 'Caret', 'comment': 'Comment', 
                 'commentblock': 'Comment Block', 'number': 'Number', 
                 'string': 'String', 'triple': 'Triple String', 
                 'keyword': 'Python Keyword', 'pyokeyword': 'Pyo Keyword', 
                 'class': 'Class Name', 'function': 'Function Name', 
                 'linenumber': 'Line Number', 'operator': 'Operator', 
                 'foldmarginback': 'Folding Margin Background',
                 'marginback': 'Number Margin Background', 
                 'markerfg': 'Marker Foreground', 'markerbg': 'Marker Background', 
                 'bracelight': 'Brace Match', 'bracebad': 'Brace Mismatch', 
                 'lineedge': 'Line Edge'}

with open(PREF_STYLE) as f:
    text = f.read()
exec text in locals()
try:
    STYLES = copy.deepcopy(style)
except:
    STYLES = {'background': {'colour': '#FFFFFF'},
 'bracebad': {'colour': '#DD0000'},
 'bracelight': {'colour': '#AABBDD'},
 'caret': {'colour': '#000000'},
 'class': {'bold': 1, 'colour': '#000097', 'italic': 0, 'underline': 0},
 'comment': {'bold': 0, 'colour': '#0066FF', 'italic': 1, 'underline': 0},
 'commentblock': {'bold': 0, 'colour': u'#468EFF', 'italic': 1, 'underline': 0},
 'default': {'bold': 0, 'colour': '#000000', 'italic': 0, 'underline': 0},
 'foldmarginback': {'colour': '#D0D0D0'},
 'function': {'bold': 1, 'colour': '#0000A2', 'italic': 0, 'underline': 0},
 'keyword': {'bold': 1, 'colour': '#0000FF', 'italic': 0, 'underline': 0},
 'lineedge': {'colour': '#DDDDDD'},
 'linenumber': {'bold': 0, 'colour': '#000000', 'italic': 0, 'underline': 0},
 'marginback': {'colour': '#B0B0B0'},
 'markerbg': {'colour': '#000000'},
 'markerfg': {'colour': '#CCCCCC'},
 'number': {'bold': 1, 'colour': '#0000CD', 'italic': 0, 'underline': 0},
 'operator': {'bold': 1, 'colour': '#000000', 'italic': 0, 'underline': 0},
 'pyokeyword': {'bold': 1, 'colour': '#5555FF', 'italic': 0, 'underline': 0},
 'selback': {'colour': '#C0DFFF'},
 'string': {'bold': 0, 'colour': '#036A07', 'italic': 0, 'underline': 0},
 'triple': {'bold': 0, 'colour': '#03BA07', 'italic': 0, 'underline': 0}}
if not STYLES.has_key('face'):
    STYLES['face'] = DEFAULT_FONT_FACE
if not STYLES.has_key('size'):
    STYLES['size'] = FONT_SIZE
if not STYLES.has_key('size2'):
    STYLES['size2'] = FONT_SIZE2

STYLES_PREVIEW_TEXT = '''# Comment
## Comment block
from pyo import *
class Bidule:
    """
    Tripe string.
    """
    def __init__(self):
        "Single string"
        self.osc = Sine(freq=100, mul=0.2)
'''

snip_faces = {'face': DEFAULT_FONT_FACE, 'size': FONT_SIZE}

##### Data Event for processing events from the running thread #####
wxDATA_EVENT = wx.NewEventType()

def EVT_DATA_EVENT(win, func):
    win.Connect(-1, -1, wxDATA_EVENT, func)

class DataEvent(wx.PyEvent):
    def __init__(self, data):
        wx.PyEvent.__init__(self)
        self.SetEventType(wxDATA_EVENT)
        self.data = data

    def Clone (self): 
        self.__class__ (self.GetId())

class RunningThread(threading.Thread):
    def __init__(self, path, cwd, event_receiver):
        threading.Thread.__init__(self)
        self.path = path
        self.cwd = cwd
        self.event_receiver = event_receiver
        self.terminated = False
        self.pid = None
    
    def setFileName(self, filename):
        self.filename = filename

    def setPID(self, pid):
        self.pid = pid

    def kill(self):
        self.terminated = True
        if PLATFORM == "win32":
            try:
                os.system("tskill %d" % self.proc.pid)
            except:
                print "'tskill' doesn't seem to be installed on the system. It is needed to be able to kill a process."
        else:
            self.proc.terminate()
        if self.proc.poll() == None:
            self.proc.kill()

    def run(self):
        if OSX_APP_BUNDLED:
            vars_to_remove = "PYTHONHOME PYTHONPATH EXECUTABLEPATH RESOURCEPATH ARGVZERO PYTHONOPTIMIZE"
            prelude = "export -n %s;export PATH=/usr/local/bin:/usr/local/lib:$PATH;" % vars_to_remove
            if CALLER_NEED_TO_INVOKE_32_BIT:
                self.proc = subprocess.Popen(['%s%s%s -u "%s"' % (prelude, SET_32_BIT_ARCH, WHICH_PYTHON, self.path)], 
                                shell=True, cwd=self.cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            else:
                self.proc = subprocess.Popen(['%s%s -u "%s"' % (prelude, WHICH_PYTHON, self.path)], cwd=self.cwd, 
                                    shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        elif PLATFORM == "darwin":
            if CALLER_NEED_TO_INVOKE_32_BIT:
                self.proc = subprocess.Popen(['%s%s -u "%s"' % (SET_32_BIT_ARCH, WHICH_PYTHON, self.path)], 
                                shell=True, cwd=self.cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            else:
                self.proc = subprocess.Popen(['%s -u "%s"' % (WHICH_PYTHON, self.path)], cwd=self.cwd, 
                                shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        elif PLATFORM == "win32":
            self.proc = subprocess.Popen([WHICH_PYTHON, "-u", self.path], cwd=self.cwd, shell=False, 
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        else:
            self.proc = subprocess.Popen([WHICH_PYTHON, "-u", self.path], cwd=self.cwd, 
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

        header = '=== Output log of process "%s", launched: %s ===\n' % (self.filename, time.strftime('"%d %b %Y %H:%M:%S"', time.localtime()))
        data_event = DataEvent({"log": header, "pid": self.pid, "filename": self.filename, "active": True})
        wx.PostEvent(self.event_receiver, data_event)
        while self.proc.poll() == None and not self.terminated:
            log = ""
            for line in self.proc.stdout.readline():
                log = log + line
            log = log.replace(">>> ", "").replace("... ", "")
            data_event = DataEvent({"log": log, "pid": self.pid, "filename": self.filename, "active": True})
            wx.PostEvent(self.event_receiver, data_event)            
            sys.stdout.flush()
            time.sleep(.025)
        stdout, stderr = self.proc.communicate()
        output = ""
        if stdout is not None:
            output = output + stdout
        if stderr is not None:
            output = output + stderr
        output = output.replace(">>> ", "").replace("... ", "")
        if "StartNotification name = default" in output:
            output = output.replace("StartNotification name = default", "")
        if "epyo_tempfile.py" in output:
            try:
                findpos = output.find("epyo_tempfile.py")
                pos = findpos
                while (output[pos] != '"'):
                    pos -= 1
                startpos = pos + 1
                pos = findpos
                while (output[pos] != '"'):
                    pos += 1
                endpos = pos
                output = output[:startpos] + self.filename + output[endpos:]
                pos = startpos + len(self.filename)
                slinepos = pos + 8
                pos = slinepos
                while (output[pos] != ',' and output[pos] != '\n'):
                    pos += 1
                elinepos = pos
                linenum = int(output[slinepos:elinepos].strip())
                output = output[:slinepos] + str(linenum-3) + output[elinepos:]
            except:
                pass
        if self.terminated:
            output = output + "\n=== Process killed. ==="
        data_event = DataEvent({"log": output, "pid": self.pid, 
                                "filename": self.filename, "active": False})
        wx.PostEvent(self.event_receiver, data_event)

class BackgroundServerThread(threading.Thread):
    def __init__(self, cwd, event_receiver):
        threading.Thread.__init__(self)
        self.cwd = cwd
        self.event_receiver = event_receiver
        self.terminated = False
        self.pid = None

    def setPID(self, pid):
        self.pid = pid

    def kill(self):
        self.terminated = True
        self.proc.stdin.write("_quit_()\n")
        if self.proc.poll() == None:
            self.proc.kill()

    def sendText(self, text):
        for line in text.splitlines():
            self.proc.stdin.write(line + "\n")
        self.proc.stdin.write("\n")

    def run(self):
        if PLATFORM == "win32":
            self.proc = subprocess.Popen(
                    [WHICH_PYTHON, '-i', os.path.join(TEMP_PATH, "background_server.py")], 
                    shell=True, cwd=self.cwd, stdout=subprocess.PIPE,
                    stdin=subprocess.PIPE, stderr=subprocess.STDOUT)
        else:
            self.proc = subprocess.Popen(
                    ["%s -i -u %s" % (WHICH_PYTHON, os.path.join(TEMP_PATH, "background_server.py"))], 
                    cwd=self.cwd, shell=True, stdout=subprocess.PIPE, 
                    stderr=subprocess.STDOUT, stdin=subprocess.PIPE)

        header = '=== Output log of background server, launched: %s ===\n' % time.strftime('"%d %b %Y %H:%M:%S"', time.localtime())
        data_event = DataEvent({"log": header, "pid": self.pid, 
                                "filename": 'background_server.py', 
                                "active": True})
        wx.PostEvent(self.event_receiver, data_event)
        while self.proc.poll() == None and not self.terminated:
            log = ""
            for line in self.proc.stdout.readline():
                log = log + line
            log = log.replace(">>> ", "").replace("... ", "")
            data_event = DataEvent({"log": log, "pid": self.pid, 
                                    "filename": 'background_server.py', 
                                    "active": True})
            wx.PostEvent(self.event_receiver, data_event)            
            sys.stdout.flush()
            time.sleep(.025)
        stdout, stderr = self.proc.communicate()
        output = ""
        if stdout is not None:
            output = output + stdout
        if stderr is not None:
            output = output + stderr
        output = output.replace(">>> ", "").replace("... ", "")
        if "StartNotification name = default" in output:
            output = output.replace("StartNotification name = default", "")
        if "background_server.py" in output:
            try:
                findpos = output.find("background_server.py")
                pos = findpos
                while (output[pos] != '"'):
                    pos -= 1
                startpos = pos + 1
                pos = findpos
                while (output[pos] != '"'):
                    pos += 1
                endpos = pos
                output = output[:startpos] + self.filename + output[endpos:]
                pos = startpos + len(self.filename)
                slinepos = pos + 8
                pos = slinepos
                while (output[pos] != ',' and output[pos] != '\n'):
                    pos += 1
                elinepos = pos
                linenum = int(output[slinepos:elinepos].strip())
                output = output[:slinepos] + str(linenum-3) + output[elinepos:]
            except:
                pass
        if self.terminated:
            output = output + "\n=== Process killed. ==="
        data_event = DataEvent({"log": output, "pid": self.pid, 
                                "filename": 'background_server.py', 
                                "active": False})
        wx.PostEvent(self.event_receiver, data_event)

class KeyCommandsFrame(wx.Frame):
    def __init__(self, parent):
        wx.Frame.__init__(self, parent, wx.ID_ANY, 
                          title="Editor Key Commands List", size=(650,550))
        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(351, "Close\tCtrl+W")
        self.menuBar.Append(menu1, 'File')
        self.SetMenuBar(self.menuBar)

        self.Bind(wx.EVT_MENU, self.close, id=351)
        self.Bind(wx.EVT_CLOSE, self.close)
        panel = scrolled.ScrolledPanel(self)
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.FlexGridSizer(len(KEY_COMMANDS), 2, 15, 15)
        if PLATFORM == "darwin": accel = "Cmd"
        else: accel = "Ctrl"
        for key, value in sorted(KEY_COMMANDS.items()):
            short = key.replace("Ctrl/Cmd", accel)
            command = wx.StaticText(panel, wx.ID_ANY, label=short)
            box.Add(command, 0, wx.ALIGN_LEFT)
            action = wx.StaticText(panel, wx.ID_ANY, label=wordwrap(value, 350, wx.ClientDC(self)))
            box.Add(action, 1, wx.ALIGN_LEFT)

        mainSizer.Add(box, 1, wx.ALL, 15)
        panel.SetSizer(mainSizer)
        panel.SetAutoLayout(1)
        panel.SetupScrolling()

    def close(self, evt):
        self.Hide()

class EditorPreview(stc.StyledTextCtrl):
    def __init__(self, parent, ID, pos=wx.DefaultPosition, size=wx.DefaultSize, 
                 style= wx.SUNKEN_BORDER | wx.WANTS_CHARS):
        stc.StyledTextCtrl.__init__(self, parent, ID, pos, size, style)

        self.SetSTCCursor(2)
        self.panel = parent

        self.Colourise(0, -1)
        self.SetCurrentPos(0)

        self.SetText(STYLES_PREVIEW_TEXT)

        self.SetIndent(4)
        self.SetBackSpaceUnIndents(True)
        self.SetTabIndents(True)
        self.SetTabWidth(4)
        self.SetUseTabs(False)
        self.SetEOLMode(wx.stc.STC_EOL_LF)
        self.SetUseHorizontalScrollBar(False)
        self.SetReadOnly(True)
        self.SetProperty("fold", "1")
        self.SetProperty("tab.timmy.whinge.level", "1")
        self.SetMargins(5,5)
        self.SetUseAntiAliasing(True)
        self.SetEdgeColour(STYLES["lineedge"]['colour'])
        self.SetEdgeMode(stc.STC_EDGE_LINE)
        self.SetEdgeColumn(60)
        self.SetMarginType(0, stc.STC_MARGIN_SYMBOL)
        self.SetMarginWidth(0, 12)
        self.SetMarginMask(0, ~wx.stc.STC_MASK_FOLDERS)
        self.SetMarginSensitive(0, True)
        
        self.SetMarginType(1, stc.STC_MARGIN_NUMBER)
        self.SetMarginWidth(1, 28)
        self.SetMarginMask(1, 0)
        self.SetMarginSensitive(1, False)
        
        self.SetMarginType(2, stc.STC_MARGIN_SYMBOL)
        self.SetMarginWidth(2, 12)
        self.SetMarginMask(2, stc.STC_MASK_FOLDERS)
        self.SetMarginSensitive(2, True)

        self.setStyle()

        self.MarkerAdd(2, 0)

        wx.CallAfter(self.SetAnchor, 0)
        self.Refresh()

    def setStyle(self):
        def buildStyle(forekey, backkey=None, smallsize=False):
            if smallsize:
                st = "face:%s,fore:%s,size:%s" % (STYLES['face'], STYLES[forekey]['colour'], STYLES['size2'])
            else:
                st = "face:%s,fore:%s,size:%s" % (STYLES['face'], STYLES[forekey]['colour'], STYLES['size'])
            if backkey:
                st += ",back:%s" % STYLES[backkey]['colour']
            if STYLES[forekey].has_key('bold'):
                if STYLES[forekey]['bold']:
                    st += ",bold"
                if STYLES[forekey]['italic']:
                    st += ",italic"
                if STYLES[forekey]['underline']:
                    st += ",underline"
            return st

        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, buildStyle('default', 'background'))
        self.StyleClearAll()  # Reset all to be like the default
        self.MarkerDefine(0, stc.STC_MARK_SHORTARROW, STYLES['markerbg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPEN, stc.STC_MARK_BOXMINUS, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDER, stc.STC_MARK_BOXPLUS, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERSUB, stc.STC_MARK_VLINE, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERTAIL, stc.STC_MARK_LCORNERCURVE, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEREND, stc.STC_MARK_ARROW, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPENMID, stc.STC_MARK_ARROWDOWN, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERMIDTAIL, stc.STC_MARK_LCORNERCURVE, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, buildStyle('default', 'background'))
        self.StyleSetSpec(stc.STC_STYLE_LINENUMBER, buildStyle('linenumber', 'marginback', True))
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, buildStyle('default'))
        self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT, buildStyle('default', 'bracelight') + ",bold")
        self.StyleSetSpec(stc.STC_STYLE_BRACEBAD, buildStyle('default', 'bracebad') + ",bold")
        self.SetLexer(stc.STC_LEX_PYTHON)
        self.SetKeyWords(0, " ".join(keyword.kwlist) + " None True False ")
        self.SetKeyWords(1, " ".join(PYO_WORDLIST))
        self.StyleSetSpec(stc.STC_P_DEFAULT, buildStyle('default'))
        self.StyleSetSpec(stc.STC_P_COMMENTLINE, buildStyle('comment'))
        self.StyleSetSpec(stc.STC_P_NUMBER, buildStyle('number'))
        self.StyleSetSpec(stc.STC_P_STRING, buildStyle('string'))
        self.StyleSetSpec(stc.STC_P_CHARACTER, buildStyle('string'))
        self.StyleSetSpec(stc.STC_P_WORD, buildStyle('keyword'))
        self.StyleSetSpec(stc.STC_P_WORD2, buildStyle('pyokeyword'))
        self.StyleSetSpec(stc.STC_P_TRIPLE, buildStyle('triple'))
        self.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, buildStyle('triple'))
        self.StyleSetSpec(stc.STC_P_CLASSNAME, buildStyle('class'))
        self.StyleSetSpec(stc.STC_P_DEFNAME, buildStyle('function'))
        self.StyleSetSpec(stc.STC_P_OPERATOR, buildStyle('operator'))
        self.StyleSetSpec(stc.STC_P_IDENTIFIER, buildStyle('default'))
        self.StyleSetSpec(stc.STC_P_COMMENTBLOCK, buildStyle('commentblock'))
        self.SetEdgeColour(STYLES["lineedge"]['colour'])
        self.SetCaretForeground(STYLES['caret']['colour'])
        self.SetSelBackground(1, STYLES['selback']['colour'])
        self.SetFoldMarginColour(True, STYLES['foldmarginback']['colour'])
        self.SetFoldMarginHiColour(True, STYLES['foldmarginback']['colour'])
        self.SetEdgeColumn(60)

        # WxPython 3 needs the lexer to be set before folding property
        self.SetProperty("fold", "1")

class ComponentPanel(scrolled.ScrolledPanel):
    def __init__(self, parent, size):
        scrolled.ScrolledPanel.__init__(self, parent, wx.ID_ANY, pos=(0,0), size=size, style=wx.SUNKEN_BORDER)
        self.SetBackgroundColour("#FFFFFF")
        self.buttonRefs = {}
        self.bTogRefs = {}
        self.iTogRefs = {}
        self.uTogRefs = {}
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        for component in STYLES_TEXT_COMP:
            box = wx.BoxSizer(wx.HORIZONTAL)
            label = wx.StaticText(self, wx.ID_ANY, label=STYLES_LABELS[component])
            box.Add(label, 1, wx.EXPAND|wx.TOP|wx.LEFT, 3)
            btog = wx.ToggleButton(self, wx.ID_ANY, label="B", size=(24,20))
            btog.SetValue(STYLES[component]['bold'])
            box.Add(btog, 0, wx.TOP|wx.ALIGN_RIGHT, 1)
            btog.Bind(wx.EVT_TOGGLEBUTTON, self.OnBToggleButton)
            self.bTogRefs[btog] = component          
            itog = wx.ToggleButton(self, wx.ID_ANY, label="I", size=(24,20))
            itog.SetValue(STYLES[component]['italic'])
            box.Add(itog, 0, wx.TOP|wx.ALIGN_RIGHT, 1)            
            itog.Bind(wx.EVT_TOGGLEBUTTON, self.OnIToggleButton)
            self.iTogRefs[itog] = component          
            utog = wx.ToggleButton(self, wx.ID_ANY, label="U", size=(24,20))
            utog.SetValue(STYLES[component]['underline'])
            box.Add(utog, 0, wx.TOP|wx.ALIGN_RIGHT, 1)  
            utog.Bind(wx.EVT_TOGGLEBUTTON, self.OnUToggleButton)
            self.uTogRefs[utog] = component          
            box.AddSpacer(20)          
            selector = csel.ColourSelect(self, -1, "", hex_to_rgb(STYLES[component]['colour']), size=(20,20))
            box.Add(selector, 0, wx.TOP|wx.ALIGN_RIGHT, 1)
            selector.Bind(csel.EVT_COLOURSELECT, self.OnSelectColour)
            self.buttonRefs[selector] = component
            mainSizer.Add(box, 1, wx.LEFT|wx.RIGHT|wx.EXPAND, 10)
            mainSizer.Add(wx.StaticLine(self), 0, wx.LEFT|wx.RIGHT|wx.EXPAND, 1)

        for component in STYLES_INTER_COMP:
            box = wx.BoxSizer(wx.HORIZONTAL)
            label = wx.StaticText(self, wx.ID_ANY, label=STYLES_LABELS[component])
            box.Add(label, 1, wx.EXPAND|wx.TOP|wx.LEFT, 3)
            selector = csel.ColourSelect(self, -1, "", hex_to_rgb(STYLES[component]['colour']), size=(20,20))
            box.Add(selector, 0, wx.TOP|wx.ALIGN_RIGHT, 1)
            selector.Bind(csel.EVT_COLOURSELECT, self.OnSelectColour)
            self.buttonRefs[selector] = component
            mainSizer.Add(box, 1, wx.LEFT|wx.RIGHT|wx.EXPAND, 10)
            if component != STYLES_INTER_COMP[-1]:
                mainSizer.Add(wx.StaticLine(self), 0, wx.LEFT|wx.RIGHT|wx.EXPAND, 1)

        self.SetSizer(mainSizer)
        self.SetAutoLayout(1)
        self.SetupScrolling()
        h = label.GetSize()[1]+6
        num_rows = len(STYLES_TEXT_COMP) + len(STYLES_INTER_COMP)
        self.SetMaxSize((-1, h*num_rows))

    def reset(self):
        for but, name in self.buttonRefs.items():
            but.SetColour(hex_to_rgb(STYLES[name]['colour']))
        for tog, name in self.bTogRefs.items():
            tog.SetValue(STYLES[name]['bold'])
        for tog, name in self.iTogRefs.items():
            tog.SetValue(STYLES[name]['italic'])
        for tog, name in self.uTogRefs.items():
            tog.SetValue(STYLES[name]['underline'])

    def OnSelectColour(self, event):
        col = wx.Colour(*event.GetValue())
        col = col.GetAsString(wx.C2S_HTML_SYNTAX)
        key = self.buttonRefs[event.GetEventObject()]
        STYLES[key]['colour'] = col
        self.GetParent().GetParent().editorPreview.setStyle()

    def OnBToggleButton(self, event):
        value = event.GetInt()
        key = self.bTogRefs[event.GetEventObject()]
        STYLES[key]['bold'] = value
        self.GetParent().GetParent().editorPreview.setStyle()

    def OnIToggleButton(self, event):
        value = event.GetInt()
        key = self.iTogRefs[event.GetEventObject()]
        STYLES[key]['italic'] = value
        self.GetParent().GetParent().editorPreview.setStyle()

    def OnUToggleButton(self, event):
        value = event.GetInt()
        key = self.uTogRefs[event.GetEventObject()]
        STYLES[key]['underline'] = value
        self.GetParent().GetParent().editorPreview.setStyle()

class ColourEditor(wx.Frame):
    def __init__(self, parent, title, pos, size):
        wx.Frame.__init__(self, parent, -1, title, pos, size)
        self.SetMinSize((500,550))
        self.SetMaxSize((500,-1))

        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(350, "Close\tCtrl+W")
        self.menuBar.Append(menu1, 'File')
        self.SetMenuBar(self.menuBar)

        self.Bind(wx.EVT_MENU, self.close, id=350)
        self.Bind(wx.EVT_CLOSE, self.close)

        self.cur_style = ""

        self.panel = wx.Panel(self)
        self.panel.SetAutoLayout(True)
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        self.panel.SetSizer(mainSizer)

        toolbar = wx.ToolBar(self.panel, -1)
        saveButton = wx.Button(toolbar, wx.ID_ANY, label="Save Style")
        saveButton.Bind(wx.EVT_BUTTON, self.OnSave)
        toolbar.AddControl(saveButton)
        toolbar.AddSeparator()
        toolbar.AddControl(wx.StaticText(toolbar, wx.ID_ANY, label="Edit Style:"))
        choices = [f for f in os.listdir(STYLES_PATH) if f[0] != "."]
        self.choiceMenu = wx.Choice(toolbar, wx.ID_ANY, choices=choices)
        self.choiceMenu.SetStringSelection(PREFERENCES.get("pref_style", "Default"))
        self.choiceMenu.Bind(wx.EVT_CHOICE, self.OnStyleChoice)
        toolbar.AddControl(self.choiceMenu)
        toolbar.AddSeparator()
        deleteButton = wx.Button(toolbar, wx.ID_ANY, label="Delete Style")
        deleteButton.Bind(wx.EVT_BUTTON, self.OnDelete)
        toolbar.AddControl(deleteButton)
        toolbar.Realize()

        mainSizer.Add(toolbar, 0, wx.EXPAND)

        enum = wx.FontEnumerator()
        enum.EnumerateFacenames(fixedWidthOnly=True)
        facelist = enum.GetFacenames()
        facelist.sort()

        buttonData = [  (STYLES_GENERALS[0], STYLES['default']['colour'], (50, 24), STYLES_LABELS['default']),
                        (STYLES_GENERALS[1], STYLES['background']['colour'], (50, 24), STYLES_LABELS['background']),
                        (STYLES_GENERALS[2], STYLES['selback']['colour'], (50, 24), STYLES_LABELS['selback']),
                        (STYLES_GENERALS[3], STYLES['caret']['colour'], (50, 24), STYLES_LABELS['caret']) ]

        self.buttonRefs = {}

        section1Sizer = wx.BoxSizer(wx.HORIZONTAL)
        buttonSizer1 = wx.FlexGridSizer(0, 2, 25, 5)
        for name, color, size, label in buttonData[:2]:
            b = csel.ColourSelect(self.panel, -1, "", hex_to_rgb(color), size=size)
            b.Bind(csel.EVT_COLOURSELECT, self.OnSelectColour)
            self.buttonRefs[b] = name
            buttonSizer1.AddMany([(wx.StaticText(self.panel, -1, label+":"), 0, wx.ALIGN_RIGHT|wx.ALIGN_CENTER_VERTICAL),
                                (b, 0, wx.LEFT|wx.RIGHT, 5)])
        section1Sizer.Add(buttonSizer1, 0, wx.EXPAND|wx.LEFT|wx.RIGHT|wx.TOP|wx.ALIGN_LEFT, 10)
        section1Sizer.AddSpacer(110)
        buttonSizer2 = wx.FlexGridSizer(0, 2, 25, 5)
        for name, color, size, label in buttonData[2:4]:
            b = csel.ColourSelect(self.panel, -1, "", hex_to_rgb(color), size=size)
            b.Bind(csel.EVT_COLOURSELECT, self.OnSelectColour)
            self.buttonRefs[b] = name
            buttonSizer2.AddMany([(wx.StaticText(self.panel, -1, label+":"), 0, wx.ALIGN_RIGHT|wx.ALIGN_CENTER_VERTICAL),
                                (b, 0, wx.LEFT|wx.RIGHT, 5)])
        section1Sizer.Add(buttonSizer2, 0, wx.LEFT|wx.RIGHT|wx.TOP|wx.ALIGN_RIGHT, 10)
        mainSizer.Add(section1Sizer, 0, wx.LEFT|wx.RIGHT|wx.TOP, 10)

        self.components = ComponentPanel(self.panel, size=(480, 100))
        mainSizer.Add(self.components, 1, wx.EXPAND|wx.LEFT|wx.RIGHT|wx.BOTTOM, 10)

        mainSizer.Add(wx.StaticLine(self.panel), 0, wx.LEFT|wx.RIGHT|wx.EXPAND, 10)
        
        faceBox = wx.BoxSizer(wx.HORIZONTAL)
        faceLabel = wx.StaticText(self.panel, wx.ID_ANY, "Font Face:")
        faceBox.Add(faceLabel, 0, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 5)
        self.facePopup = wx.ComboBox(self.panel, wx.ID_ANY, STYLES['face'], size=(250, -1), choices=facelist, style=wx.CB_READONLY)
        faceBox.Add(self.facePopup, 1, wx.ALL|wx.EXPAND, 5)
        self.faceView = wx.StaticText(self.panel, wx.ID_ANY, STYLES['face'])
        self.font = self.faceView.GetFont()
        self.font.SetFaceName(STYLES['face'])
        self.faceView.SetFont(self.font)
        faceBox.Add(self.faceView, 1, wx.ALL|wx.ALIGN_CENTER_VERTICAL, 5)
        self.facePopup.Bind(wx.EVT_COMBOBOX, self.OnFaceSelected)
        mainSizer.Add(faceBox, 0, wx.ALL|wx.EXPAND, 10)

        mainSizer.Add(wx.StaticLine(self.panel), 0, wx.LEFT|wx.RIGHT|wx.EXPAND, 10)

        mainSizer.Add(wx.StaticText(self.panel, wx.ID_ANY, label="Preview"), 0, wx.TOP|wx.CENTER, 10)
        self.editorPreview = EditorPreview(self.panel, wx.ID_ANY, size=(400, 180))
        mainSizer.Add(self.editorPreview, 0, wx.ALL|wx.EXPAND, 10)

        self.panel.Layout()

    def setCurrentStyle(self, st):
        self.cur_style = st
        self.choiceMenu.SetStringSelection(st)
        self.editorPreview.setStyle()

    def close(self, evt):
        self.Hide()
        if self.cur_style != "":
            self.GetParent().setStyle(self.cur_style)

    def OnDelete(self, event):
        if self.cur_style != "":
            os.remove(os.path.join(STYLES_PATH, self.cur_style))
        choices = [f for f in os.listdir(STYLES_PATH) if f[0] != "."]
        self.choiceMenu.SetItems(choices)
        self.choiceMenu.SetSelection(0)
        evt = wx.CommandEvent(10006, self.choiceMenu.GetId())
        evt.SetInt(0)
        evt.SetString(choices[0])
        self.choiceMenu.ProcessEvent(evt)
        self.GetParent().rebuildStyleMenu()

    def OnSave(self, event):
        dlg = wx.TextEntryDialog(self, "Enter the Style's name:", 'Save Style')
        dlg.CenterOnParent()
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            if name != "":
                self.cur_style = name
                with open(os.path.join(STYLES_PATH, name), "w") as f:
                    texttosave = pprint.pformat(STYLES, width=120)
                    f.write("style = " + texttosave)
                choices = [f for f in os.listdir(STYLES_PATH) if f[0] != "."]
                self.choiceMenu.SetItems(choices)
                self.choiceMenu.SetStringSelection(name)
                self.GetParent().rebuildStyleMenu()

    def OnStyleChoice(self, event):
        global STYLES
        stl = event.GetString()
        self.cur_style = stl
        with open(os.path.join(ensureNFD(STYLES_PATH), stl)) as f:
            text = f.read()
        exec text in locals()
        STYLES = copy.deepcopy(style)
        if not STYLES.has_key('face'):
            STYLES['face'] = DEFAULT_FONT_FACE
        if not STYLES.has_key('size'):
            STYLES['size'] = FONT_SIZE
        if not STYLES.has_key('size2'):
            STYLES['size2'] = FONT_SIZE2
        self.editorPreview.setStyle()
        for but, name in self.buttonRefs.items():
            but.SetColour(hex_to_rgb(STYLES[name]['colour']))
        self.facePopup.SetStringSelection(STYLES['face'])
        self.font.SetFaceName(STYLES['face'])
        self.faceView.SetFont(self.font)
        self.faceView.SetLabel(STYLES['face'])
        self.components.reset()

    def OnFaceSelected(self, event):
        face = event.GetString()
        self.font.SetFaceName(face)
        self.faceView.SetFont(self.font)
        self.faceView.SetLabel(face)
        STYLES['face'] = face
        self.editorPreview.setStyle()

    def OnSelectColour(self, event):
        col = wx.Colour(*event.GetValue())
        col = col.GetAsString(wx.C2S_HTML_SYNTAX)
        key = self.buttonRefs[event.GetEventObject()]
        STYLES[key]['colour'] = col
        self.editorPreview.setStyle()

class SearchProjectPanel(scrolled.ScrolledPanel):
    def __init__(self, parent, root, dict, size):
        scrolled.ScrolledPanel.__init__(self, parent, wx.ID_ANY, pos=(0,0), size=size, style=wx.SUNKEN_BORDER)
        self.SetBackgroundColour("#FFFFFF")
        self.root = root
        self.dict = dict
        self.files = sorted(self.dict.keys())
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        BUTID = 25000
        textX = self.GetTextExtent("open")[0]
        textY = self.GetTextExtent("open")[1]
        for file in self.files:
            box = wx.BoxSizer(wx.HORIZONTAL)
            but = wx.Button(self, BUTID, label="open", size=(textX+16, textY+10))
            box.Add(but, 0, wx.ALL, 1)
            self.Bind(wx.EVT_BUTTON, self.onOpenFile, id=BUTID)
            BUTID += 1
            fileText = wx.StaticText(self, wx.ID_ANY, label="File : %s" % file)
            off = (but.GetSize()[1] - textY) / 2
            box.Add(fileText, 1, wx.EXPAND|wx.LEFT|wx.RIGHT|wx.TOP, off)
            mainSizer.Add(box, 1, wx.LEFT|wx.RIGHT|wx.EXPAND, 10)
            for i in range(len(self.dict[file][0])):
                box2 = wx.BoxSizer(wx.HORIZONTAL)
                label = "    line %d : %s" % (self.dict[file][0][i], self.dict[file][1][i])
                fileText = wx.StaticText(self, wx.ID_ANY, label=label, size=(-1, textY))
                box2.Add(fileText, 1, wx.EXPAND|wx.LEFT|wx.RIGHT|wx.TOP, off)
                mainSizer.Add(box2, 1, wx.LEFT|wx.RIGHT|wx.EXPAND, 10)
            mainSizer.Add(wx.StaticLine(self), 0, wx.LEFT|wx.RIGHT|wx.EXPAND, 1)

        self.SetSizer(mainSizer)
        self.SetAutoLayout(1)
        self.SetupScrolling()
        h = but.GetSize()[1]+2
        num_rows = 0
        for f in self.files:
            num_rows += len(self.dict[f][0])
        self.SetMaxSize((-1, max(h*num_rows, self.GetSize()[1])))
    
    def onOpenFile(self, evt):
        filename = self.root + self.files[evt.GetId() - 25000]
        self.GetParent().GetParent().panel.addPage(filename)

class SearchProjectFrame(wx.Frame):
    def __init__(self, parent, root, dict, size=(500,500)):
        wx.Frame.__init__(self, parent, wx.ID_ANY, size=size)
        self.SetTitle('Search Results in Project "%s"' % os.path.split(root)[1])
        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(351, "Close\tCtrl+W")
        self.menuBar.Append(menu1, 'File')
        self.SetMenuBar(self.menuBar)
        self.Bind(wx.EVT_MENU, self.close, id=351)
        panel = SearchProjectPanel(self, root, dict, size=size)
        self.Show()

    def close(self, evt):
        self.Destroy()

class SnippetTree(wx.Panel):
    def __init__(self, parent, size):
        wx.Panel.__init__(self, parent, -1, size=size, style=wx.WANTS_CHARS | wx.SUNKEN_BORDER | wx.EXPAND)
        self.SetMinSize((100, -1))

        self.selected = None

        tsize = (24, 24)
        file_add_bmp = catalog['file_delete_icon.png'].GetBitmap()
        folder_add_bmp = catalog['folder_add_icon.png'].GetBitmap()

        self.sizer = wx.BoxSizer(wx.VERTICAL)

        toolbarbox = wx.BoxSizer(wx.HORIZONTAL)
        self.toolbar = wx.ToolBar(self, -1)
        self.toolbar.SetToolBitmapSize(tsize)
        self.toolbar.AddLabelTool(SNIPPET_ADD_FOLDER_ID, "Add Category", folder_add_bmp, shortHelp="Add a New Category")
        self.toolbar.AddLabelTool(SNIPPET_DEL_FILE_ID, "Delete", file_add_bmp, shortHelp="Delete Snippet or Category")
        self.toolbar.EnableTool(SNIPPET_DEL_FILE_ID, False)
        self.toolbar.Realize()
        toolbarbox.Add(self.toolbar, 1, wx.ALIGN_LEFT | wx.EXPAND, 0)

        wx.EVT_TOOL(self, SNIPPET_ADD_FOLDER_ID, self.onAdd)
        wx.EVT_TOOL(self, SNIPPET_DEL_FILE_ID, self.onDelete)

        self.sizer.Add(toolbarbox, 0, wx.EXPAND)

        self.tree = wx.TreeCtrl(self, -1, (0, 26), size, wx.TR_DEFAULT_STYLE|wx.TR_HIDE_ROOT|wx.SUNKEN_BORDER|wx.EXPAND)

        if wx.Platform == '__WXMAC__':
            self.tree.SetFont(wx.Font(11, wx.ROMAN, wx.NORMAL, wx.NORMAL, face=snip_faces['face']))
        else:
            self.tree.SetFont(wx.Font(8, wx.ROMAN, wx.NORMAL, wx.NORMAL, face=snip_faces['face']))

        self.sizer.Add(self.tree, 1, wx.EXPAND)
        self.SetSizer(self.sizer)

        isz = (12,12)
        self.il = wx.ImageList(isz[0], isz[1])
        self.fldridx     = self.il.Add(wx.ArtProvider_GetBitmap(wx.ART_FOLDER,      wx.ART_OTHER, isz))
        self.fldropenidx = self.il.Add(wx.ArtProvider_GetBitmap(wx.ART_FILE_OPEN,   wx.ART_OTHER, isz))
        self.fileidx     = self.il.Add(wx.ArtProvider_GetBitmap(wx.ART_NORMAL_FILE, wx.ART_OTHER, isz))

        self.tree.SetImageList(self.il)
        self.tree.SetSpacing(12)
        self.tree.SetIndent(6)

        self.root = self.tree.AddRoot("EPyo_Snippet_tree", self.fldridx, self.fldropenidx, None)

        self.tree.Bind(wx.EVT_LEFT_DOWN, self.OnLeftClick)
        self.tree.Bind(wx.EVT_LEFT_DCLICK, self.OnLeftDClick)

        self.load()

    def load(self):
        categories = [d for d in os.listdir(SNIPPETS_PATH) if os.path.isdir(os.path.join(SNIPPETS_PATH, d)) and d[0] != "."]
        for category in categories:
            child = self.tree.AppendItem(self.root, category, self.fldridx, self.fldropenidx, None)
            files = [f for f in os.listdir(os.path.join(SNIPPETS_PATH, category)) if f[0] != "."]
            for file in files:
                item = self.tree.AppendItem(child, file, self.fileidx, self.fileidx, None)
            self.tree.SortChildren(child)
        self.tree.SortChildren(self.root)

    def addItem(self, name, category):
        child, cookie = self.tree.GetFirstChild(self.root)
        while child.IsOk():
            if self.tree.GetItemText(child) == category:
                break
            child, cookie = self.tree.GetNextChild(self.root, cookie)
        subchild, subcookie = self.tree.GetFirstChild(child)
        while subchild.IsOk():
            if self.tree.GetItemText(subchild) == name:
                return
            subchild, subcookie = self.tree.GetNextChild(child, subcookie)
        item = self.tree.AppendItem(child, name, self.fileidx, self.fileidx, None)
        self.tree.SortChildren(child)
        self.tree.SortChildren(self.root)

    def onAdd(self, evt):
        dlg = wx.TextEntryDialog(self, "Enter the Category's name:", 'New Category')
        dlg.CenterOnParent()
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            if name != "" and name not in os.listdir(SNIPPETS_PATH):
                os.mkdir(os.path.join(SNIPPETS_PATH, name))
                child = self.tree.AppendItem(self.root, name, self.fldridx, self.fldropenidx, None)
                self.tree.SortChildren(self.root)
                SNIPPETS_CATEGORIES.append(name)

    def onDelete(self, evt):
        item = self.tree.GetSelection()
        if item.IsOk():
            name = self.tree.GetItemText(item)
            if self.tree.GetItemParent(item) == self.tree.GetRootItem():
                files = os.listdir(os.path.join(SNIPPETS_PATH, name))
                for file in files:
                    os.remove(os.path.join(SNIPPETS_PATH, name, file))
                os.rmdir(os.path.join(SNIPPETS_PATH, name))
                SNIPPETS_CATEGORIES.remove(name)
                if self.tree.ItemHasChildren(item):
                    self.tree.DeleteChildren(item)
            else:
                category = self.tree.GetItemText(self.tree.GetItemParent(item))
                os.remove(os.path.join(SNIPPETS_PATH, category, name))
            self.tree.Delete(item)
            self.GetParent().GetParent().GetParent().reloadSnippetMenu()

    def OnLeftClick(self, event):
        pt = event.GetPosition()
        item, flags = self.tree.HitTest(pt)
        if item:
            self.select(item)
        else:
            self.unselect()
        event.Skip()

    def OnLeftDClick(self, event):
        pt = event.GetPosition()
        item, flags = self.tree.HitTest(pt)
        if item:
            self.select(item)
            self.openPage(item)
        else:
            self.unselect()
        event.Skip()

    def openPage(self, item):
        if self.tree.GetItemParent(item) != self.tree.GetRootItem():
            name = self.tree.GetItemText(item)
            ritem = self.tree.GetItemParent(item)
            category = self.tree.GetItemText(ritem)
            self.GetParent().GetParent().onLoad(name, category)

    def select(self, item):
        self.tree.SelectItem(item)
        self.selected = self.tree.GetItemText(item)
        self.toolbar.EnableTool(SNIPPET_DEL_FILE_ID, True)

    def unselect(self):
        self.tree.UnselectAll()
        self.selected = None
        self.toolbar.EnableTool(SNIPPET_DEL_FILE_ID, False)

class SnippetEditor(stc.StyledTextCtrl):
    def __init__(self, parent, id=-1, pos=wx.DefaultPosition, size=wx.DefaultSize, style=wx.SUNKEN_BORDER):
        stc.StyledTextCtrl.__init__(self, parent, id, pos, size, style)
        self.SetViewWhiteSpace(False)
        self.SetIndent(4)
        self.SetBackSpaceUnIndents(True)
        self.SetTabIndents(True)
        self.SetTabWidth(4)
        self.SetUseTabs(False)
        self.SetViewWhiteSpace(False)
        self.SetEOLMode(wx.stc.STC_EOL_LF)
        self.SetViewEOL(False)
        self.SetMarginWidth(1, 0)
        self.SetLexer(stc.STC_LEX_PYTHON)
        self.SetKeyWords(0, " ".join(keyword.kwlist) + " None True False " + " ".join(PYO_WORDLIST))
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, "fore:#000000,face:%(face)s,size:%(size)d,back:#FFFFFF" % snip_faces)
        self.StyleClearAll()
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, "fore:#000000,face:%(face)s,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "fore:#000000,face:%(face)s" % snip_faces)
        self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT, "fore:#000000,back:#AABBDD,bold" % snip_faces)
        self.StyleSetSpec(stc.STC_STYLE_BRACEBAD, "fore:#000000,back:#DD0000,bold" % snip_faces)
        self.StyleSetSpec(stc.STC_P_DEFAULT, "fore:#000000,face:%(face)s,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_COMMENTLINE, "fore:#0066FF,face:%(face)s,italic,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_NUMBER, "fore:#0000CD,face:%(face)s,bold,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_STRING, "fore:#036A07,face:%(face)s,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_CHARACTER, "fore:#036A07,face:%(face)s,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_WORD, "fore:#0000FF,face:%(face)s,bold,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_TRIPLE, "fore:#038A07,face:%(face)s,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, "fore:#038A07,face:%(face)s,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_CLASSNAME, "fore:#000097,face:%(face)s,bold,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_DEFNAME, "fore:#0000A2,face:%(face)s,bold,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_OPERATOR, "fore:#000000,face:%(face)s,bold,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_IDENTIFIER, "fore:#000000,face:%(face)s,size:%(size)d" % snip_faces)
        self.StyleSetSpec(stc.STC_P_COMMENTBLOCK, "fore:#0066FF,face:%(face)s,size:%(size)d" % snip_faces)
        self.SetSelBackground(1, "#C0DFFF")
        self.Bind(stc.EVT_STC_UPDATEUI, self.OnUpdateUI)

    def OnUpdateUI(self, evt):
        if self.GetSelectedText():
            self.GetParent().GetParent().GetParent().tagButton.Enable()
            self.GetParent().GetParent().GetParent().tagItem.Enable()
        else:
            self.GetParent().GetParent().GetParent().tagButton.Enable(False)
            self.GetParent().GetParent().GetParent().tagItem.Enable(False)

class SnippetFrame(wx.Frame):
    def __init__(self, parent, title, pos, size):
        wx.Frame.__init__(self, parent, -1, title, pos, size)
        self.parent = parent

        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        self.tagItem = menu1.Append(249, "Tag Selection\tCtrl+T")
        menu1.AppendSeparator()
        menu1.Append(250, "Close\tCtrl+W")
        self.menuBar.Append(menu1, 'File')
        self.SetMenuBar(self.menuBar)

        self.Bind(wx.EVT_MENU, self.onTagSelection, id=249)
        self.Bind(wx.EVT_MENU, self.close, id=250)
        self.Bind(wx.EVT_CLOSE, self.close)

        self.splitter = wx.SplitterWindow(self, -1, style=wx.SP_LIVE_UPDATE|wx.SP_3DSASH)

        self.snippet_tree = SnippetTree(self.splitter, (-1, -1))

        self.panel = wx.Panel(self.splitter)
        self.panel.SetBackgroundColour("#DDDDDD")

        self.splitter.SplitVertically(self.snippet_tree, self.panel, 150)

        self.box = wx.BoxSizer(wx.VERTICAL)

        self.category_name = ""
        self.snippet_name = ""

        toolbarBox = wx.BoxSizer(wx.HORIZONTAL)

        self.toolbar = wx.ToolBar(self.panel, -1)

        saveButton = wx.Button(self.toolbar, wx.ID_ANY, label="Save Snippet")
        self.toolbar.AddControl(saveButton)
        self.Bind(wx.EVT_BUTTON, self.onSave, id=saveButton.GetId())
        self.toolbar.Realize()
		
        toolbarBox.Add(self.toolbar, 1, wx.ALIGN_LEFT|wx.EXPAND|wx.LEFT, 5)

        toolbar2 = wx.ToolBar(self.panel, -1)
        self.tagButton = wx.Button(toolbar2, wx.ID_ANY, label="Tag Selection")
        toolbar2.AddControl(self.tagButton)
        self.Bind(wx.EVT_BUTTON, self.onTagSelection, id=self.tagButton.GetId())
        toolbar2.Realize()

        toolbarBox.Add(toolbar2, 0, wx.ALIGN_RIGHT|wx.RIGHT, 5)

        self.box.Add(toolbarBox, 0, wx.EXPAND|wx.ALL, 5)

        self.entry = SnippetEditor(self.panel)
        self.box.Add(self.entry, 1, wx.EXPAND|wx.ALL, 10)

        activateBox = wx.BoxSizer(wx.HORIZONTAL)
        activateLabel = wx.StaticText(self.panel, wx.ID_ANY, label="Activation :")
        activateBox.Add(activateLabel, 0, wx.LEFT|wx.TOP, 10)

        self.short = wx.TextCtrl(self.panel, wx.ID_ANY, size=(170,-1))
        activateBox.Add(self.short, 1, wx.EXPAND|wx.ALL, 8)
        self.short.SetValue("Type your shortcut...")
        self.short.SetForegroundColour("#AAAAAA")
        self.short.Bind(wx.EVT_KEY_DOWN, self.onKey)
        self.short.Bind(wx.EVT_LEFT_DOWN, self.onShortLeftClick)
        self.short.Bind(wx.EVT_KILL_FOCUS, self.onShortLooseFocus)
        self.box.Add(activateBox, 0, wx.EXPAND)

        self.panel.SetSizer(self.box)

    def close(self, evt):
        self.Hide()

    def onTagSelection(self, evt):
        select = self.entry.GetSelection()
        if select:
            self.entry.InsertText(select[1], "`")
            self.entry.InsertText(select[0], "`")

    def onLoad(self, name, category):
        if os.path.isfile(os.path.join(ensureNFD(SNIPPETS_PATH), category, name)):
            self.snippet_name = name
            self.category_name = category
            with codecs.open(os.path.join(ensureNFD(SNIPPETS_PATH), self.category_name, self.snippet_name), "r", encoding="utf-8") as f:
                text = f.read()
            exec text in locals()
            try:
                self.entry.SetTextUTF8(snippet["value"])
            except:
                self.entry.SetText(snippet["value"])
            if snippet["shortcut"]:
                self.short.SetValue(snippet["shortcut"])
                self.short.SetForegroundColour("#000000")
            else:
                self.short.SetValue("Type your shortcut...")
                self.short.SetForegroundColour("#AAAAAA")

    def onSave(self, evt):
        dlg = wx.SingleChoiceDialog(self, 'Choose the Snippet Category', 
                                    'Snippet Category', SNIPPETS_CATEGORIES, wx.OK)
        dlg.SetSize((250,300))
        dlg.CenterOnParent()
        if dlg.ShowModal() == wx.ID_OK:
            category = dlg.GetStringSelection()
        dlg.Destroy()

        dlg = wx.TextEntryDialog(self, "Enter the Snippet's name:", 'Save Snippet', self.snippet_name)
        dlg.CenterOnParent()
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            if name != "":
                self.category_name = category
                self.snippet_name = name
                short = self.short.GetValue()
                if short == "Type your shortcut...":
                    short = ""
                dic = {'shortcut': short, 'value': self.entry.GetTextUTF8()}
                with codecs.open(os.path.join(SNIPPETS_PATH, category, name), "w", encoding="utf-8") as f:
                    f.write("snippet = %s" % pprint.pformat(dic))
                self.snippet_tree.addItem(name, category)
                self.parent.reloadSnippetMenu()
        dlg.Destroy()

    def onShortLooseFocus(self, evt):
        short = self.short.GetValue()
        if short == "":
            self.short.SetValue("Type your shortcut...")
            self.short.SetForegroundColour("#AAAAAA")

    def onShortLeftClick(self, evt):
        self.short.SetValue("")
        evt.Skip()

    def onKey(self, evt):
        key = evt.GetKeyCode()
        if key < 256 and key != wx.WXK_TAB:
            id = evt.GetEventObject().GetId()
            txt = ""
            if evt.ShiftDown():
                txt += "Shift-"
            if evt.ControlDown():
                if sys.platform == "darwin":
                    txt += "XCtrl-"
                else:
                    txt += "Ctrl-"
            if evt.AltDown():
                txt += "Alt-"
            if sys.platform == "darwin" and evt.CmdDown():
                txt += "Ctrl-"
            if txt == "":
                return
            ch = chr(key)
            if ch in string.lowercase:
                ch = ch.upper()
            txt += ch
            self.short.SetValue(txt)
            self.short.SetForegroundColour("#000000")
            self.entry.SetFocus()
        else:
            evt.Skip()

class FileSelectorCombo(wx.combo.ComboCtrl):
    def __init__(self, *args, **kw):
        wx.combo.ComboCtrl.__init__(self, *args, **kw)
        w, h = 12, 14
        bmp = wx.EmptyBitmap(w,h)
        dc = wx.MemoryDC(bmp)

        bgcolor = wx.Colour(255,254,255)
        dc.SetBackground(wx.Brush(bgcolor))
        dc.Clear()

        dc.SetBrush(wx.Brush("#444444"))
        dc.SetPen(wx.Pen("#444444"))
        dc.DrawPolygon([wx.Point(4,h/2-2), wx.Point(w/2,2), wx.Point(w-4,h/2-2)])
        dc.DrawPolygon([wx.Point(4,h/2+2), wx.Point(w/2,h-2), wx.Point(w-4,h/2+2)])
        del dc

        bmp.SetMaskColour(bgcolor)
        self.SetButtonBitmaps(bmp, True)

class TreeCtrlComboPopup(wx.combo.ComboPopup):
    def Init(self):
        self.value = None
        self.curitem = None

    def Create(self, parent):
        self.tree = wx.TreeCtrl(parent, style=wx.TR_HIDE_ROOT
                                |wx.TR_HAS_BUTTONS
                                |wx.TR_SINGLE
                                |wx.TR_LINES_AT_ROOT
                                |wx.SIMPLE_BORDER)
        font, psize = self.tree.GetFont(), self.tree.GetFont().GetPointSize()
        if PLATFORM == "darwin":
            font.SetPointSize(psize-2)
        else:
            font.SetPointSize(psize-1)
        self.tree.SetFont(font)
        self.tree.Bind(wx.EVT_MOTION, self.OnMotion)
        self.tree.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)

    def GetControl(self):
        return self.tree

    def GetStringValue(self):
        if self.value:
            return self.tree.GetItemText(self.value)
        return ""

    def OnPopup(self):
        self.tree.DeleteAllItems()
        editor = self.GetCombo().GetParent().GetParent().panel.editor
        count = editor.GetLineCount()
        for i in range(count):
            text = editor.GetLineUTF8(i)
            if text.startswith("class "):
                text = text.replace("class ", "")
                text = text[0:text.find(":")]
                if len(text) > 50:
                    text = text[:50] + "...)"
                item = self.AddItem(text, None, wx.TreeItemData(i))
            elif text.startswith("def "):
                text = text.replace("def ", "")
                text = text[0:text.find(":")]
                if len(text) > 50:
                    text = text[:50] + "...)"
                item = self.AddItem(text, None, wx.TreeItemData(i))
            elif text.lstrip().startswith("def "):
                indent = editor.GetLineIndentation(i)
                text = text.lstrip().replace("def ", "")
                text = " "*indent + text[0:text.find(":")]
                if len(text) > 50:
                    text = text[:50] + "...)"
                item = self.AddItem(text, None, wx.TreeItemData(i))
        self.tree.SetSize((400, 500))

    def SetStringValue(self, value):
        root = self.tree.GetRootItem()
        if not root:
            return
        found = self.FindItem(root, value)
        if found:
            self.value = found
            self.tree.SelectItem(found)

    def GetAdjustedSize(self, minWidth, prefHeight, maxHeight):
        return wx.Size(minWidth, min(200, maxHeight))

    def FindItem(self, parentItem, text):
        item, cookie = self.tree.GetFirstChild(parentItem)
        while item:
            if self.tree.GetItemText(item) == text:
                return item
            if self.tree.ItemHasChildren(item):
                item = self.FindItem(item, text)
            item, cookie = self.tree.GetNextChild(parentItem, cookie)
        return wx.TreeItemId();

    def AddItem(self, value, parent=None, data=None):
        if not parent:
            root = self.tree.GetRootItem()
            if not root:
                root = self.tree.AddRoot("<hidden root>")
            parent = root
        item = self.tree.AppendItem(parent, value, data=data)
        return item

    def OnMotion(self, evt):
        item, flags = self.tree.HitTest(evt.GetPosition())
        if item and flags & wx.TREE_HITTEST_ONITEMLABEL:
            self.tree.SelectItem(item)
            self.curitem = item
        evt.Skip()

    def OnLeftDown(self, evt):
        item, flags = self.tree.HitTest(evt.GetPosition())
        if item and flags & wx.TREE_HITTEST_ONITEMLABEL:
            self.curitem = item
            self.value = item
            self.Dismiss()
            editor = self.GetCombo().GetParent().GetParent().panel.editor
            line = self.tree.GetPyData(item)
            editor.GotoLine(line)
            halfNumLinesOnScreen = editor.LinesOnScreen() / 2
            editor.ScrollToLine(line - halfNumLinesOnScreen)
            wx.CallAfter(editor.SetFocus)
        evt.Skip()

class MainFrame(wx.Frame):
    def __init__(self, parent, ID, title, pos=wx.DefaultPosition, size=wx.DefaultSize, style=wx.DEFAULT_FRAME_STYLE):
        wx.Frame.__init__(self, parent, ID, title, pos, size, style)

        self.Bind(wx.EVT_CLOSE, self.OnClose)

        EVT_DATA_EVENT(self, self.format_outputLog)

        if sys.platform == "darwin":
            accel_ctrl = wx.ACCEL_CMD
        else:
            accel_ctrl = wx.ACCEL_CTRL
        # To set up an accelerator key
        #aEntry = wx.AcceleratorEntry(accel_ctrl|wx.ACCEL_SHIFT, wx.WXK_UP, 602)
        # "\t%s" % aEntry.ToString()

        self.snippet_frame = SnippetFrame(self, title='Snippet Editor', pos=(25,25), size=(700,450))
        self.style_frame = ColourEditor(self, title='Style Editor', pos=(100,100), size=(500,550))
        self.style_frame.setCurrentStyle(PREF_STYLE)
        self.keyCommandsFrame = KeyCommandsFrame(self)

        self.server_pipe = None
        self.back_server_started = False

        self.master_document = None
        self.processID = 0
        self.processes = {}
        self.filters = {}

        self.print_data = wx.PrintData()
        self.print_data.SetPaperId(wx.PAPER_LETTER)

        self.pastingList = []
        self.panel = MainPanel(self, size=size)

        self.menuBar = wx.MenuBar()

        menu1 = wx.Menu()
        menu1.Append(wx.ID_NEW, "New\tCtrl+N")
        self.Bind(wx.EVT_MENU, self.new, id=wx.ID_NEW)
        self.submenu1 = wx.Menu()
        for key, name in sorted(TEMPLATE_NAMES.items(), reverse=True):
            self.submenu1.Append(key, "%s Template" % name)
        menu1.AppendMenu(99, "New From Template", self.submenu1)
        self.Bind(wx.EVT_MENU, self.newFromTemplate, id=min(TEMPLATE_NAMES.keys()), id2=max(TEMPLATE_NAMES.keys()))
        menu1.Append(wx.ID_OPEN, "Open\tCtrl+O")
        self.Bind(wx.EVT_MENU, self.open, id=wx.ID_OPEN)
        menu1.Append(160, "Open With Encoding")
        self.Bind(wx.EVT_MENU, self.openWithEncoding, id=160)
        menu1.Append(112, "Open Folder\tShift+Ctrl+O")
        self.Bind(wx.EVT_MENU, self.openFolder, id=112)
        self.submenu2 = wx.Menu()
        ID_OPEN_RECENT = 2000
        recentFiles = []
        filename = ensureNFD(os.path.join(TEMP_PATH,'.recent.txt'))
        if os.path.isfile(filename):
            f = codecs.open(filename, "r", encoding="utf-8")
            for line in f.readlines():
                recentFiles.append(line.replace("\n", ""))
            f.close()
        if recentFiles:
            for file in recentFiles:
                self.submenu2.Append(ID_OPEN_RECENT, file)
                ID_OPEN_RECENT += 1
        if ID_OPEN_RECENT > 2000:
            for i in range(2000, ID_OPEN_RECENT):
                self.Bind(wx.EVT_MENU, self.openRecent, id=i)
        menu1.AppendMenu(1999, "Open Recent...", self.submenu2)
        menu1.AppendSeparator()
        menu1.Append(wx.ID_CLOSE, "Close\tCtrl+W")
        self.Bind(wx.EVT_MENU, self.close, id=wx.ID_CLOSE)
        menu1.Append(wx.ID_CLOSE_ALL, "Close All Tabs\tShift+Ctrl+W")
        self.Bind(wx.EVT_MENU, self.closeAll, id=wx.ID_CLOSE_ALL)
        menu1.Append(wx.ID_SAVE, "Save\tCtrl+S")
        self.Bind(wx.EVT_MENU, self.save, id=wx.ID_SAVE)
        menu1.Append(wx.ID_SAVEAS, "Save As...\tShift+Ctrl+S")
        self.Bind(wx.EVT_MENU, self.saveas, id=wx.ID_SAVEAS)
        menu1.Append(100, "Save As Template...")
        self.Bind(wx.EVT_MENU, self.saveasTemplate, id=100)
        # TODO : printing not working well enough
        #menu1.AppendSeparator()
        #menu1.Append(wx.ID_PREVIEW, "Print Preview")
        #self.Bind(wx.EVT_MENU, self.OnPrintPreview, id=wx.ID_PREVIEW)
        #menu1.Append(wx.ID_PRINT, "Print\tCtrl+P")
        #self.Bind(wx.EVT_MENU, self.OnPrint, id=wx.ID_PRINT)
        if sys.platform != "darwin":
            menu1.AppendSeparator()
        prefItem = menu1.Append(wx.ID_PREFERENCES, "Preferences...\tCtrl+;")
        self.Bind(wx.EVT_MENU, self.openPrefs, prefItem)
        if sys.platform != "darwin":
            menu1.AppendSeparator()
        quitItem = menu1.Append(wx.ID_EXIT, "Quit\tCtrl+Q")
        self.Bind(wx.EVT_MENU, self.OnClose, quitItem)
        self.menuBar.Append(menu1, 'File')

        menu2 = wx.Menu()
        menu2.Append(wx.ID_UNDO, "Undo\tCtrl+Z")
        menu2.Append(wx.ID_REDO, "Redo\tShift+Ctrl+Z")
        self.Bind(wx.EVT_MENU, self.undo, id=wx.ID_UNDO, id2=wx.ID_REDO)
        menu2.AppendSeparator()
        menu2.Append(wx.ID_CUT, "Cut\tCtrl+X")
        self.Bind(wx.EVT_MENU, self.cut, id=wx.ID_CUT)
        menu2.Append(wx.ID_COPY, "Copy\tCtrl+C")
        self.Bind(wx.EVT_MENU, self.copy, id=wx.ID_COPY)
        menu2.Append(wx.ID_PASTE, "Paste\tCtrl+V")
        self.Bind(wx.EVT_MENU, self.paste, id=wx.ID_PASTE)
        menu2.Append(wx.ID_SELECTALL, "Select All\tCtrl+A")
        self.Bind(wx.EVT_MENU, self.selectall, id=wx.ID_SELECTALL)
        menu2.AppendSeparator()
        menu2.Append(200, "Add to Pasting List\tShift+Ctrl+C")
        self.Bind(wx.EVT_MENU, self.listCopy, id=200)
        menu2.Append(201, "Paste From List\tShift+Ctrl+V")
        self.Bind(wx.EVT_MENU, self.listPaste, id=201)
        menu2.Append(202, "Save Pasting List")
        self.Bind(wx.EVT_MENU, self.saveListPaste, id=202)
        menu2.Append(203, "Load Pasting List")
        self.Bind(wx.EVT_MENU, self.loadListPaste, id=203)
        menu2.Append(204, "Edit Pasting List")
        self.Bind(wx.EVT_MENU, self.editPastingList, id=204)
        menu2.AppendSeparator()
        menu2.Append(107, "Remove Trailing White Space")
        self.Bind(wx.EVT_MENU, self.removeTrailingWhiteSpace, id=107)
        menu2.AppendSeparator()
        menu2.Append(103, "Fold All\tCtrl+I")
        self.Bind(wx.EVT_MENU, self.fold, id=103)
        menu2.Append(104, "Expand All\tShift+Ctrl+I")
        self.Bind(wx.EVT_MENU, self.fold, id=104)
        menu2.Append(105, "Fold/Expand Current Scope\tCtrl+8")
        self.Bind(wx.EVT_MENU, self.foldExpandScope, id=105)
        menu2.Append(108, "Un/Comment Selection\tCtrl+J")
        self.Bind(wx.EVT_MENU, self.OnComment, id=108)
        menu2.Append(121, "Insert File Path...\tShift+Ctrl+P")
        self.Bind(wx.EVT_MENU, self.insertPath, id=121)
        menu2.AppendSeparator()
        submenublk = wx.Menu()
        submenublk.Append(400, "Insert Code Block Head\tCtrl+B")
        submenublk.Append(401, "Insert Code Block Tail\tShift+Ctrl+B")
        submenublk.Append(402, "Select Code Block\tCtrl+,")
        self.Bind(wx.EVT_MENU, self.onCodeBlock, id=400, id2=402)
        menu2.AppendMenu(-1, "Code Blocks", submenublk)
        menu2.AppendSeparator()
        menu2.Append(114, "Auto Complete container syntax", kind=wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.autoCompContainer, id=114)
        menu2.Check(114, PREFERENCES.get("auto_comp_container", 0))
        menu2.AppendSeparator()
        submenu2 = wx.Menu()
        submenu2.Append(170, "Convert Selection to Uppercase\tCtrl+U")
        submenu2.Append(171, "Convert Selection to Lowercase\tShift+Ctrl+U")
        self.Bind(wx.EVT_MENU, self.upperLower, id=170, id2=171)
        submenu2.Append(172, "Convert Tabs to Spaces")
        self.Bind(wx.EVT_MENU, self.tabsToSpaces, id=172)
        menu2.AppendMenu(-1, "Text Converters", submenu2)
        menu2.AppendSeparator()
        menu2.Append(140, "Goto line...\tCtrl+L")
        self.Bind(wx.EVT_MENU, self.gotoLine, id=140)
        menu2.Append(141, "Quick Search\tCtrl+F")
        self.Bind(wx.EVT_MENU, self.quickSearch, id=141)
        menu2.Append(142, "Quick Search Word Under Caret\tShift+Ctrl+J")
        self.Bind(wx.EVT_MENU, self.quickSearchWordUnderCaret, id=142)
        menu2.Append(143, "Search Again Next...\tCtrl+G")
        menu2.Append(144, "Search Again Previous...\tShift+Ctrl+G")
        self.Bind(wx.EVT_MENU, self.searchAgain, id=143, id2=144)
        menu2.Append(146, "Search in Project Files\tShift+Ctrl+H")
        self.Bind(wx.EVT_MENU, self.searchInProject, id=146)
        menu2.Append(wx.ID_FIND, "Find/Replace\tShift+Ctrl+F")
        self.Bind(wx.EVT_MENU, self.showFind, id=wx.ID_FIND)
        self.menuBar.Append(menu2, 'Code')

        self.menu3 = wx.Menu()
        self.menu3.Append(299, "Set Current Document as Master")
        self.Bind(wx.EVT_MENU, self.setMasterDocument, id=299)
        self.menu3.AppendSeparator()
        self.menu3.Append(300, "Run\tCtrl+R")
        self.Bind(wx.EVT_MENU, self.runner, id=300)
        self.menu3.Append(301, "Run Selection\tShift+Ctrl+R")
        self.Bind(wx.EVT_MENU, self.runSelection, id=301)
        self.menu3.Append(302, "Run Line/Selection as Pyo\tCtrl+E")
        self.Bind(wx.EVT_MENU, self.runSelectionAsPyo, id=302)
        self.menu3.Append(303, "Execute Line/Selection as Python\tShift+Ctrl+E")
        self.Bind(wx.EVT_MENU, self.execSelection, id=303)
        self.menu3.AppendSeparator()
        self.backServerItem = self.menu3.Append(304, "Start Pyo Background Server")
        self.Bind(wx.EVT_MENU, self.startStopBackgroundServer, id=304)
        self.sendToServerItem = self.menu3.Append(305, "Send Line/Selection to Pyo Background Server\tCtrl+.")
        self.sendToServerItem.Enable(False)
        self.Bind(wx.EVT_MENU, self.sendSelectionToBackgroundServer, id=305)
        self.menuBar.Append(self.menu3, 'Process')

        menu4 = wx.Menu()
        menu4.Append(wx.ID_ZOOM_IN, "Zoom in\tCtrl+=")
        menu4.Append(wx.ID_ZOOM_OUT, "Zoom out\tCtrl+-")
        self.Bind(wx.EVT_MENU, self.zoom, id=wx.ID_ZOOM_IN, id2=wx.ID_ZOOM_OUT)
        menu4.AppendSeparator()
        menu4.Append(130, "Show Invisibles", kind=wx.ITEM_CHECK)
        menu4.Check(130, PREFERENCES.get("show_invisibles", 0))
        self.Bind(wx.EVT_MENU, self.showInvisibles, id=130)
        menu4.Append(131, "Show Edge Line", kind=wx.ITEM_CHECK)
        menu4.Check(131, PREFERENCES.get("show_edge_line", 0))
        self.Bind(wx.EVT_MENU, self.showEdge, id=131)
        menu4.Append(132, "Wrap Text Line", kind=wx.ITEM_CHECK)
        menu4.Check(132, PREFERENCES.get("wrap_text_line", 0))
        self.Bind(wx.EVT_MENU, self.wrapMode, id=132)
        menu4.AppendSeparator()
        self.showProjItem = menu4.Append(50, "Show Folder Panel", kind=wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.showHideFolderPanel, id=50)
        self.showMarkItem = menu4.Append(49, "Show Markers Panel", kind=wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.showHideMarkersPanel, id=49)
        self.showOutputItem = menu4.Append(48, "Show Output Panel", kind=wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.showHideOutputPanel, id=48)
        menu4.AppendSeparator()
        menu4.Append(190, "Open Documentation Frame\tShift+Ctrl+D")
        self.Bind(wx.EVT_MENU, self.showDocFrame, id=190)
        menu4.Append(180, "Open Documentation for Pyo Object Under Caret\tCtrl+D")
        self.Bind(wx.EVT_MENU, self.showDoc, id=180)
        menu4.Append(181, "Show args for Pyo Object Under Caret\tShift+Return")
        self.Bind(wx.EVT_MENU, self.showArgs, id=181)
        menu4.Append(182, "Show __doc__ String for Word Under Caret\tCtrl+Return")
        self.Bind(wx.EVT_MENU, self.showDocString, id=182)
        menu4.AppendSeparator()
        menu4.Append(185, "Rebuild Documentation")
        self.Bind(wx.EVT_MENU, self.rebuildDoc, id=185)
        self.menuBar.Append(menu4, 'View')

        self.menu5 = wx.Menu()
        ID_STYLE = 500
        for st in [f for f in os.listdir(STYLES_PATH) if f[0] != "."]:
            self.menu5.Append(ID_STYLE, st, "", wx.ITEM_RADIO)
            if st == PREFERENCES.get("pref_style", "Default"): self.menu5.Check(ID_STYLE, True)
            ID_STYLE += 1
        self.menu5.AppendSeparator()
        self.menu5.Append(499, "Open Style Editor")
        self.Bind(wx.EVT_MENU, self.openStyleEditor, id=499)
        self.menuBar.Append(self.menu5, 'Styles')
        for i in range(500, ID_STYLE):
            self.Bind(wx.EVT_MENU, self.changeStyle, id=i)

        self.menu7 = wx.Menu()
        self.makeSnippetMenu()
        self.menuBar.Append(self.menu7, "Snippets")

        menu8 = wx.Menu()
        menu8.Append(600, "Add Marker to Current Line\tShift+Ctrl+M")
        self.Bind(wx.EVT_MENU, self.addMarker, id=600)
        menu8.Append(601, "Delete Current Line Marker\tShift+Ctrl+K")
        self.Bind(wx.EVT_MENU, self.deleteMarker, id=601)
        menu8.Append(604, "Delete All Markers")
        self.Bind(wx.EVT_MENU, self.deleteAllMarkers, id=604)
        menu8.AppendSeparator()
        menu8.Append(602, 'Navigate Markers Upward\tCtrl+9')
        menu8.Append(603, 'Navigate Markers Downward\tCtrl+0')
        self.Bind(wx.EVT_MENU, self.navigateMarkers, id=602, id2=603)
        self.menuBar.Append(menu8, "Markers")

        self.menu6 = wx.Menu()
        ID_EXAMPLE = 1000
        for folder in EXAMPLE_FOLDERS:
            exmenu = wx.Menu(folder.lower())
            for ex in sorted([exp for exp in os.listdir(os.path.join(EXAMPLE_PATH, folder.lower())) if exp[0] != "." and not exp.endswith("pyc")]):
                exmenu.Append(ID_EXAMPLE, ex)
                ID_EXAMPLE += 1
            self.menu6.AppendMenu(-1, folder, exmenu)
            ID_EXAMPLE += 1
        self.Bind(wx.EVT_MENU, self.openExample, id=1000, id2=ID_EXAMPLE)
        self.menuBar.Append(self.menu6, "Pyo Examples")

        self.ID_FILTERS = 12000
        self.filters_menu = wx.Menu()
        self.filters_menu.Append(11998, "Open Filters File")
        self.Bind(wx.EVT_MENU, self.openFilters, id=11998)
        self.filters_menu.Append(11999, "Rebuild Filters Menu")
        self.Bind(wx.EVT_MENU, self.buildFilterMenu, id=11999)
        self.filters_menu.AppendSeparator() 
        self.buildFilterMenu()
        self.menuBar.Append(self.filters_menu, "Filters")
            
        windowMenu = wx.Menu()
        aEntry = wx.AcceleratorEntry(wx.ACCEL_CTRL, wx.WXK_TAB, 10001)
        windowMenu.Append(10001, 'Navigate Tabs Forward\t%s' % aEntry.ToString())
        aEntry = wx.AcceleratorEntry(wx.ACCEL_CTRL|wx.ACCEL_SHIFT, wx.WXK_TAB, 10002)
        windowMenu.Append(10002, 'Navigate Tabs Backward\t%s' % aEntry.ToString())
        self.Bind(wx.EVT_MENU, self.onSwitchTabs, id=10001, id2=10002)
        self.menuBar.Append(windowMenu, '&Window')

        helpmenu = wx.Menu()
        helpItem = helpmenu.Append(wx.ID_ABOUT, '&About %s %s' % (APP_NAME, APP_VERSION), 'wxPython RULES!!!')
        self.Bind(wx.EVT_MENU, self.onHelpAbout, helpItem)
        helpmenu.Append(999, 'Show Editor Key Commands')
        self.Bind(wx.EVT_MENU, self.onShowEditorKeyCommands, id=999)
        helpmenu.Append(998, 'Tutorial: How to Create a Custom PyoObject - RingMod')
        self.Bind(wx.EVT_MENU, self.openTutorial, id=998)
        helpmenu.Append(997, 'Tutorial: How to Create a Custom PyoObject - Flanger')
        self.Bind(wx.EVT_MENU, self.openTutorial, id=997)
        helpmenu.Append(996, 'Tutorial: How to Create a Custom PyoTableObject - TriTable')
        self.Bind(wx.EVT_MENU, self.openTutorial, id=996)
        self.menuBar.Append(helpmenu, '&Help')

        self.SetMenuBar(self.menuBar)

        self.status = self.CreateStatusBar()
        self.status.Bind(wx.EVT_SIZE, self.StatusOnSize)
        self.status.SetFieldsCount(3)
        
        if PLATFORM == "darwin":
            ststyle = wx.TE_PROCESS_ENTER|wx.NO_BORDER
            sth = self.status.GetSize()[1] #16
            cch = -1
        elif PLATFORM == "linux2":
            ststyle = wx.TE_PROCESS_ENTER|wx.SIMPLE_BORDER
            sth = self.status.GetSize()[1]+1 #20
            cch = self.status.GetSize()[1] #21
        elif PLATFORM == "win32":
            ststyle = wx.TE_PROCESS_ENTER|wx.SIMPLE_BORDER
            sth = 20
            cch = 20

        self.field1X, field1Y = self.status.GetTextExtent("Quick Search:")
        self.status.SetStatusWidths([self.field1X+9,-1,-2])
        self.status.SetStatusText("Quick Search:", 0)
        self.status_search = wx.TextCtrl(self.status, wx.ID_ANY, size=(150,sth), style=ststyle)
        self.status_search.Bind(wx.EVT_TEXT_ENTER, self.onQuickSearchEnter)

        self.cc = FileSelectorCombo(self.status, size=(250, cch), style=wx.CB_READONLY)
        self.tcp = TreeCtrlComboPopup()
        self.cc.SetPopupControl(self.tcp)
        self.Reposition()

        self.showProjectTree(PREFERENCES.get("show_folder_panel", 0))
        self.showMarkersPanel(PREFERENCES.get("show_markers_panel", 0))
        self.showOutputPanel(PREFERENCES.get("show_output_panel", 1))

        if INSTALLATION_ERROR_MESSAGE != "":
            report = wx.MessageDialog(self, INSTALLATION_ERROR_MESSAGE, "Installation Report", wx.OK|wx.ICON_INFORMATION|wx.STAY_ON_TOP)
            report.ShowModal()
            report.Destroy()

        if foldersToOpen:
            for p in foldersToOpen:
                self.panel.project.loadFolder(p)
                sys.path.append(p)

        if filesToOpen:
            for f in filesToOpen:
                self.panel.addPage(f)

        wx.CallAfter(self.buildDoc)

    def Reposition(self):
        if PLATFORM == "darwin":
            yoff1 = -1
            yoff2 = -5
        elif PLATFORM == "linux2":
            yoff1 = -2
            yoff2 = -1
        elif PLATFORM == "win32":
            yoff1 = 0
            yoff2 = -1

        self.status.SetStatusText("Quick Search:", 0)
        rect = self.status.GetFieldRect(1)
        self.status_search.SetPosition((self.field1X+12, rect.y+yoff1))
        rect = self.status.GetFieldRect(2)
        if rect.x > self.field1X+160:
            self.cc.SetPosition((rect.x, rect.y+yoff2))

    def onCodeBlock(self, evt):
        if evt.GetId() == 400:
            self.panel.editor.insertBlockHead()
        elif evt.GetId() == 401:
            self.panel.editor.insertBlockTail()
        elif evt.GetId() == 402:
            self.panel.editor.selectCodeBlock()

    def setMasterDocument(self, evt):
        if self.master_document == None:
            self.master_document = self.panel.editor.path
            self.menu3.SetLabel(299, "Revert Master Document to None")
        else:
            self.master_document = None
            self.menu3.SetLabel(299, "Set Current Document as Master")                

    def StatusOnSize(self, evt):
        self.Reposition()

    def rebuildStyleMenu(self):
        items = self.menu5.GetMenuItems()
        for item in items:
            self.menu5.DeleteItem(item)
        ID_STYLE = 500
        for st in [f for f in os.listdir(STYLES_PATH) if f[0] != "."]:
            self.menu5.Append(ID_STYLE, st, "", wx.ITEM_RADIO)
            ID_STYLE += 1
        self.menu5.AppendSeparator()
        self.menu5.Append(499, "Open Style Editor")
        self.Bind(wx.EVT_MENU, self.openStyleEditor, id=499)
        for i in range(500, ID_STYLE):
            self.Bind(wx.EVT_MENU, self.changeStyle, id=i)

    def reloadSnippetMenu(self):
        items = self.menu7.GetMenuItems()
        for item in items:
            self.menu7.DeleteItem(item)
        self.makeSnippetMenu()

    def makeSnippetMenu(self):
        itemId = 30000
        accel_entries = []
        for cat in SNIPPETS_CATEGORIES:
            submenu = wx.Menu(title=cat)
            files = [f for f in os.listdir(os.path.join(SNIPPETS_PATH, cat))]
            for file in files:
                with open(os.path.join(SNIPPETS_PATH, cat, file), "r") as f:
                    text = f.read()
                exec text in locals()
                short = snippet["shortcut"]
                accel = 0
                if "Shift" in short:
                    accel |= wx.ACCEL_SHIFT
                    short = short.replace("Shift", "")
                if "XCtrl" in short:
                    accel |= wx.ACCEL_CTRL
                    short = short.replace("XCtrl", "")
                if "Ctrl" in short:
                    if PLATFORM == "darwin":
                        accel |= wx.ACCEL_CMD
                    else:
                        accel |= wx.ACCEL_CTRL
                    short = short.replace("Ctrl", "")
                if "Alt" in short:
                    accel |= wx.ACCEL_ALT
                    short = short.replace("Alt", "")
                if accel == 0:
                    accel = wx.ACCEL_NORMAL
                short = short.replace("-", "")
                if short != "":
                    accel_tuple = wx.AcceleratorEntry(accel, ord(short), itemId)
                    accel_entries.append(accel_tuple)
                    short = accel_tuple.ToString()
                    submenu.Append(itemId, "%s\t%s" % (file, short))
                else:
                    submenu.Append(itemId, file)
                self.Bind(wx.EVT_MENU, self.insertSnippet, id=itemId)
                itemId += 1
            self.menu7.AppendMenu(itemId, cat, submenu)
            itemId += 1
        if accel_entries != []:            
            accel_table  = wx.AcceleratorTable(accel_entries)
            self.SetAcceleratorTable(accel_table)
        
        self.menu7.AppendSeparator()
        self.menu7.Append(51, "Open Snippet Editor")
        self.Bind(wx.EVT_MENU, self.showSnippetEditor, id=51)

    ### Editor functions ###
    def cut(self, evt):
        self.panel.editor.Cut()

    def copy(self, evt):
        self.panel.editor.Copy()

    def listCopy(self, evt):
        text = self.panel.editor.GetSelectedTextUTF8()
        self.pastingList.append(toSysEncoding(text))

    def paste(self, evt):
        if self.FindFocus() == self.status_search:
            self.status_search.Paste()
        else:
            self.panel.editor.Paste()

    def listPaste(self, evt):
        self.panel.editor.listPaste(self.pastingList)

    def saveListPaste(self, evt):
        if self.pastingList != []:
            dlg = wx.FileDialog(self, message="Save file as ...", 
                defaultDir=os.path.expanduser('~'), style=wx.SAVE)
            if dlg.ShowModal() == wx.ID_OK:
                path = ensureNFD(dlg.GetPath())
                with open(path, "w") as f:
                    f.write(str(self.pastingList))

    def loadListPaste(self, evt):
        dlg = wx.FileDialog(self, message="Choose a file", 
            defaultDir=os.path.expanduser("~"), style=wx.OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.pastingList = []
            with open(path, "r") as f:
                try:
                    pastingList = eval(f.read())
                    if type(pastingList) == ListType:
                        self.pastingList = pastingList
                except:
                    f.seek(0)
                    for line in f:
                        if line.replace("\n", "").strip() != "":
                            self.pastingList.append(line)

    def editPastingList(self, evt):
        f = PastingListEditorFrame(self, self.pastingList)
        f.Show()

    def selectall(self, evt):
        self.panel.editor.SelectAll()

    def upperLower(self, evt):
        if evt.GetId() == 170:
            self.panel.editor.UpperCase()
        else:
            self.panel.editor.LowerCase()

    def tabsToSpaces(self, evt):
        self.panel.editor.tabsToSpaces()

    def buildFilterMenu(self, evt=None):
        if self.ID_FILTERS != 12000:
            for i in range(12000, self.ID_FILTERS):
                self.filters_menu.Delete(i)
        ID_FILTERS = 12000       
        with open(FILTERS_FILE, "r") as f:
            for line in f.readlines():
                if line.startswith("def "):
                    ppos = line.find("(")
                    name = line[4:ppos]
                    self.filters_menu.Append(ID_FILTERS, name)
                    self.Bind(wx.EVT_MENU, self.applyFilter, id=ID_FILTERS)
                    self.filters[ID_FILTERS] = name
                    ID_FILTERS += 1
        self.ID_FILTERS = ID_FILTERS

    def openFilters(self, evt):
        self.panel.addPage(FILTERS_FILE)

    def applyFilter(self, evt):
        execfile(FILTERS_FILE, {}, locals())
        filter = self.filters[evt.GetId()]
        try:
            text = self.panel.editor.GetSelectedTextUTF8()
        except:
            text = self.panel.editor.GetSelectedText()
        if text == "":
            dlg = wx.MessageDialog(self, "You must select some text to apply a filter...", "No selected text!", style=wx.OK|wx.STAY_ON_TOP)
            dlg.ShowModal()
            dlg.Destroy()
        else:
            self.panel.editor.ReplaceSelection(locals()[filter](text))
        
    def undo(self, evt):
        if evt.GetId() == wx.ID_UNDO:
            self.panel.editor.Undo()
        else:
            self.panel.editor.Redo()

    def zoom(self, evt):
        if evt.GetId() == wx.ID_ZOOM_IN:
            self.panel.editor.SetZoom(self.panel.editor.GetZoom() + 1)
        else:
            self.panel.editor.SetZoom(self.panel.editor.GetZoom() - 1)

    def showInvisibles(self, evt):
        state = evt.GetInt()
        PREFERENCES["show_invisibles"] = state
        for i in range(self.panel.notebook.GetPageCount()):
            ed = self.panel.notebook.GetPage(i)
            ed.showInvisibles(state)

    def showEdge(self, evt):
        state = evt.GetInt()
        PREFERENCES["show_edge_line"] = state
        for i in range(self.panel.notebook.GetPageCount()):
            ed = self.panel.notebook.GetPage(i)
            ed.showEdge(state)

    def wrapMode(self, evt):
        state = evt.GetInt()
        PREFERENCES["wrap_text_line"] = state
        mode = {0: stc.STC_WRAP_NONE, 1: stc.STC_WRAP_WORD}[state]
        for i in range(self.panel.notebook.GetPageCount()):
            ed = self.panel.notebook.GetPage(i)
            ed.SetWrapMode(mode)

    def removeTrailingWhiteSpace(self, evt):
        self.panel.editor.removeTrailingWhiteSpace()

    def addMarker(self, evt):
        line = self.panel.editor.GetCurrentLine()
        self.panel.editor.addMarker(line)

    def deleteMarker(self, evt):
        line = self.panel.editor.GetCurrentLine()
        self.panel.editor.deleteMarker(line)

    def deleteAllMarkers(self, evt):
        self.panel.editor.deleteAllMarkers()

    def navigateMarkers(self, evt):
        if evt.GetId() == 602:
            self.panel.editor.navigateMarkers(down=False)
        else:
            self.panel.editor.navigateMarkers(down=True)

    def gotoLine(self, evt):
        dlg = wx.TextEntryDialog(self, "Enter a line number:", "Go to Line")
        val = -1
        if dlg.ShowModal() == wx.ID_OK:
            try:
                val = int(dlg.GetValue())
            except:
                val = -1
        dlg.Destroy()
        if val != -1:
            val -= 1
            pos = self.panel.editor.FindColumn(val, 0)
            self.panel.editor.GotoLine(val)
            first = self.panel.editor.GetFirstVisibleLine()
            if val == first:
                self.panel.editor.LineScroll(0, -self.panel.editor.LinesOnScreen()/2)
            else:
                self.panel.editor.LineScroll(0, self.panel.editor.LinesOnScreen()/2)
            #self.panel.editor.SetCurrentPos(pos)
            #self.panel.editor.EnsureVisible(val)
            #self.panel.editor.EnsureCaretVisible()
            wx.CallAfter(self.panel.editor.SetAnchor, pos)

    def OnComment(self, evt):
        self.panel.editor.OnComment()

    def fold(self, event):
        if event.GetId() == 103:
            self.panel.editor.FoldAll()
        else:
            self.panel.editor.ExpandAll()

    def foldExpandScope(self, evt):
        self.panel.editor.foldExpandCurrentScope()

    def autoCompContainer(self, evt):
        state = evt.GetInt()
        PREFERENCES["auto_comp_container"] = state
        self.panel.editor.showAutoCompContainer(state)

    def showFind(self, evt):
        self.panel.editor.OnShowFindReplace()

    def quickSearch(self, evt):
        self.status.SetStatusText("Quick Search:", 0)
        self.status_search.SetFocus()
        self.status_search.SelectAll()

    def quickSearchWordUnderCaret(self, evt):
        self.status.SetStatusText("Quick Search:", 0)
        word = self.panel.editor.getWordUnderCaret()
        self.status_search.SetValue(word)
        self.onQuickSearchEnter(None)
        
    def onQuickSearchEnter(self, evt):
        str = self.status_search.GetValue()
        self.panel.editor.SetFocus()
        self.panel.editor.OnQuickSearch(str)

    def searchAgain(self, evt):
        if evt.GetId() == 143:
            next = True
        else:
            next = False
        str = self.status_search.GetValue()
        self.panel.editor.OnQuickSearch(str, next)

    def searchInProject(self, evt):
        ok = False
        search = ""
        choices = self.panel.project.projectDict.keys()
        if len(choices) == 0:
            dlg = wx.MessageDialog(self, 'You must load at least one folder to use the "Search in Project Files" option.',
                                    'No project folder', wx.OK | wx.ICON_INFORMATION)
            dlg.ShowModal()
            dlg.Destroy()
        elif len(choices) == 1:
            rootdir = self.panel.project.projectDict[choices[0]]
            ok = True
        else:
            dlg = wx.SingleChoiceDialog(self, 'Choose a project folder...', 'Search in project files',
                                        choices, wx.CHOICEDLG_STYLE)
            if dlg.ShowModal() == wx.ID_OK:
                root = dlg.GetStringSelection()
                rootdir = self.panel.project.projectDict[root]
                ok = True
        if ok:
            dlg = wx.TextEntryDialog(self, 'Enter a search term...', 'Search in Project Files')
            if dlg.ShowModal() == wx.ID_OK:
                search = dlg.GetValue()
            dlg.Destroy()
            if search:
                wx.CallAfter(self.doSearchInProject, rootdir, search)
    
    def doSearchInProject(self, rootdir, search):
        result = {}
        filters = ["build"]
        for root, dirs, files in os.walk(rootdir):
            if os.path.split(root)[1].startswith("."):
                filters.append(os.path.split(root)[1])
                continue
            filter_detect = False
            for filter in filters:
                if filter in root:
                    filter_detect = True
                    break
            if filter_detect:
                continue
            for file in files:
                filepath = os.path.join(root, file).replace(rootdir, "")
                if filepath.endswith("~"):
                    continue
                with open(os.path.join(root, file), "r") as f:
                    for i, line in enumerate(f.readlines()):
                        if "\0" in line:
                            # binary file detected
                            break
                        if search.encode("utf-8").lower() in line.lower():
                            if not result.has_key(filepath):
                                result[filepath] = ([], [])
                            result[filepath][0].append(i+1)
                            if len(line) < 50:
                                result[filepath][1].append(line.strip().replace("\n", ""))
                            else:
                                pos = line.lower().find(search.encode("utf-8").lower())
                                p1 = pos - 25
                                if p1 < 0:
                                    p1, pre = 0, ""
                                else:
                                    pre = "... "
                                p2 = pos + 25
                                if p2 >= len(line):
                                    p2, post = len(line), ""
                                else:
                                    post = " ..."
                                result[filepath][1].append(pre + line[p1:p2].strip().replace("\n", "") + post)
        if result:
            f = SearchProjectFrame(self, rootdir, result)

    def insertPath(self, evt):
        dlg = wx.FileDialog(self, message="Choose a file", 
                            defaultDir=PREFERENCES.get("insert_path", os.path.expanduser("~")),
                            defaultFile="", style=wx.OPEN | wx.MULTIPLE)
        if dlg.ShowModal() == wx.ID_OK:
            paths = dlg.GetPaths()
            if len(paths) == 1:
                text = ensureNFD(paths[0])
                if PLATFORM == "win32":
                    text = text.replace("\\", "/")
                self.panel.editor.ReplaceSelection("'" + text + "'")
            else:
                text = ", ".join(["'"+ensureNFD(path)+"'" for path in paths])
                if PLATFORM == "win32":
                    text = text.replace("\\", "/")
                self.panel.editor.ReplaceSelection("[" + text + "]")
            PREFERENCES["insert_path"] = os.path.split(paths[0])[0]
        dlg.Destroy()

    def insertSnippet(self, evt):
        id = evt.GetId()
        menu = self.menu7
        item = menu.FindItemById(id)
        name = item.GetLabel()
        category = item.GetMenu().GetTitle()
        with codecs.open(os.path.join(ensureNFD(SNIPPETS_PATH), category, name), "r", encoding="utf-8") as f:
            text = f.read()
        exec text in locals()
        self.panel.editor.insertSnippet(snippet["value"])

    def openStyleEditor(self, evt):
        self.style_frame.Show()

    def changeStyle(self, evt):
        menu = self.GetMenuBar()
        id = evt.GetId()
        st = menu.FindItemById(id).GetLabel()
        self.setStyle(st, fromMenu=True)
        self.style_frame.setCurrentStyle(st)
        
    def setStyle(self, st, fromMenu=False):
        global STYLES
        with open(os.path.join(ensureNFD(STYLES_PATH), st)) as f:
            text = f.read()
        exec text in locals()
        STYLES = copy.deepcopy(style)
        if not STYLES.has_key('face'):
            STYLES['face'] = DEFAULT_FONT_FACE
        if not STYLES.has_key('size'):
            STYLES['size'] = FONT_SIZE
        if not STYLES.has_key('size2'):
            STYLES['size2'] = FONT_SIZE2

        for i in range(self.panel.notebook.GetPageCount()):
            ed = self.panel.notebook.GetPage(i)
            ed.setStyle()
        self.panel.project.setStyle()
        self.panel.markers.scroll.setStyle()
        self.panel.outputlog.editor.setStyle()

        PREFERENCES["pref_style"] = st

        if not fromMenu:
            itemList = self.menu5.GetMenuItems()
            for item in itemList:
                if self.menu5.GetLabelText(item.GetId()) == st:
                    self.menu5.Check(item.GetId(), True)
                    break

    def onSwitchTabs(self, evt):
        if evt.GetId() == 10001:
            forward = True
        else:
            forward = False
        self.panel.notebook.AdvanceSelection(forward)

    ### Open Prefs ang Logs ###
    def openPrefs(self, evt):
        dlg = PreferencesDialog()
        if dlg.ShowModal() == wx.ID_OK:
            dlg.writePrefs()
        dlg.Destroy()

    def showSnippetEditor(self, evt):
        self.snippet_frame.Show()

    def showHideFolderPanel(self, evt):
        state = evt.GetInt()
        self.showProjectTree(state)

    def showHideMarkersPanel(self, evt):
        state = evt.GetInt()
        self.showMarkersPanel(state)

    def showHideOutputPanel(self, evt):
        state = evt.GetInt()
        self.showOutputPanel(state)

    def showProjectTree(self, state):
        self.showProjItem.Check(state)
        PREFERENCES["show_folder_panel"] = state
        if state:
            if self.panel.project.IsShownOnScreen():
                return
            if not self.panel.splitter.IsSplit():
                self.panel.splitter.SplitVertically(self.panel.left_splitter, self.panel.right_splitter, 175)
                h = self.panel.GetSize()[1]
                self.panel.left_splitter.SplitHorizontally(self.panel.project, self.panel.markers, h*3/4)
                self.panel.left_splitter.Unsplit(self.panel.markers)
            else:
                h = self.panel.GetSize()[1]
                self.panel.left_splitter.SplitHorizontally(self.panel.project, self.panel.markers, h*3/4)
        else:
            if self.panel.markers.IsShown():
                self.panel.left_splitter.Unsplit(self.panel.project)
            else:
                self.panel.splitter.Unsplit(self.panel.left_splitter)

    def showMarkersPanel(self, state):
        self.showMarkItem.Check(state)
        PREFERENCES["show_markers_panel"] = state
        if state:
            if self.panel.markers.IsShownOnScreen():
                return
            if not self.panel.splitter.IsSplit():
                self.panel.splitter.SplitVertically(self.panel.left_splitter, self.panel.right_splitter, 175)
                h = self.panel.GetSize()[1]
                self.panel.left_splitter.SplitHorizontally(self.panel.project, self.panel.markers, h*3/4)
                self.panel.left_splitter.Unsplit(self.panel.project)
            else:
                h = self.panel.GetSize()[1]
                self.panel.left_splitter.SplitHorizontally(self.panel.project, self.panel.markers, h*3/4)
        else:
            if self.panel.project.IsShown():
                self.panel.left_splitter.Unsplit(self.panel.markers)
            else:
                self.panel.splitter.Unsplit(self.panel.left_splitter)

    def showOutputPanel(self, state):
        self.showOutputItem.Check(state)
        PREFERENCES["show_output_panel"] = state
        if state:
            if self.panel.outputlog.IsShownOnScreen():
                return
            h = self.panel.GetSize()[1]
            self.panel.right_splitter.SplitHorizontally(self.panel.notebook, self.panel.outputlog, h*4/5 - h)
        else:
            if not self.panel.outputlog.IsShownOnScreen():
                return
            self.panel.right_splitter.Unsplit(self.panel.outputlog)

    ### New / Open / Save / Delete ###
    def new(self, event):
        self.panel.addNewPage()

    def newFromTemplate(self, event):
        self.panel.addNewPage()
        temp = TEMPLATE_DICT[event.GetId()]
        self.panel.editor.setText(temp)

    def newRecent(self, file):
        filename = ensureNFD(os.path.join(TEMP_PATH,'.recent.txt'))
        try:
            f = codecs.open(filename, "r", encoding="utf-8")
            lines = [line.replace("\n", "") for line in f.readlines()]
            f.close()
        except:
            lines = []
        if not file in lines:
            f = codecs.open(filename, "w", encoding="utf-8")
            lines.insert(0, file)
            if len(lines) > 20:
                lines = lines[0:20]
            for line in lines:
                f.write(line + '\n')
            f.close()
        subId2 = 2000
        if lines != []:
            for item in self.submenu2.GetMenuItems():
                self.submenu2.DeleteItem(item)
            for file in lines:
                self.submenu2.Append(subId2, toSysEncoding(file))
                subId2 += 1

    def openRecent(self, event):
        menu = self.GetMenuBar()
        id = event.GetId()
        file = menu.FindItemById(id).GetLabel()
        self.panel.addPage(ensureNFD(file))

    def open(self, event, encoding=None):
        dlg = wx.FileDialog(self, message="Choose a file", 
            defaultDir=PREFERENCES.get("open_file_path", os.path.expanduser("~")),
            defaultFile="", style=wx.OPEN | wx.MULTIPLE)
        if dlg.ShowModal() == wx.ID_OK:
            paths = dlg.GetPaths()
            for path in paths:
                filename = ensureNFD(path)
                self.panel.addPage(filename, encoding=encoding)
                self.newRecent(filename)
            PREFERENCES["open_file_path"] = os.path.split(paths[0])[0]
        dlg.Destroy()

    def openWithEncoding(self, event):
        ok = False
        dlg = wx.SingleChoiceDialog(self, 'Choose the encoding:', 'Encoding',
                sorted(ENCODING_DICT.keys()), wx.CHOICEDLG_STYLE)
        dlg.SetSize((-1, 370))
        if dlg.ShowModal() == wx.ID_OK:
            encoding = ENCODING_DICT[dlg.GetStringSelection()]
            ok = True
        dlg.Destroy()

        if ok:
            self.open(event, encoding=encoding)

    def openExample(self, event):
        id = event.GetId()
        menu = self.menu6
        item = menu.FindItemById(id)
        filename = item.GetLabel()
        folder = item.GetMenu().GetTitle()
        path = os.path.join(ensureNFD(EXAMPLE_PATH), folder, filename)
        self.panel.addPage(ensureNFD(path))

    def openTutorial(self, event):
        filename = {998: "Tutorial_01_RingMod.py", 997: "Tutorial_02_Flanger.py", 996: "Tutorial_03_TriTable.py"}[event.GetId()]
        if WIN_APP_BUNDLED:
            self.panel.addPage(os.path.join(os.getcwd(), "Resources", filename))
        else:
            self.panel.addPage(os.path.join(os.getcwd(), filename))
        
    def openFolder(self, event):
        dlg = wx.DirDialog(self, message="Choose a folder", 
            defaultPath=PREFERENCES.get("open_folder_path", os.path.expanduser("~")),
            style=wx.DD_DEFAULT_STYLE)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.folder = path
            self.panel.project.loadFolder(self.folder)
            sys.path.append(path)
            PREFERENCES["open_folder_path"] = os.path.split(path)[0]
        dlg.Destroy()

    def save(self, event):
        path = self.panel.editor.path
        if not path or "Untitled-" in path:
            self.saveas(None)
        else:
            self.panel.editor.saveMyFile(path)
            self.SetTitle(path)
            tab = self.panel.notebook.GetSelection()
            self.panel.notebook.SetPageText(tab, os.path.split(path)[1].split('.')[0])

    def saveas(self, event):
        deffile = os.path.split(self.panel.editor.path)[1]
        dlg = wx.FileDialog(self, message="Save file as ...", 
            defaultDir=PREFERENCES.get("save_file_path", os.path.expanduser("~")),
            defaultFile=deffile, style=wx.SAVE)
        dlg.SetFilterIndex(0)
        if dlg.ShowModal() == wx.ID_OK:
            path = ensureNFD(dlg.GetPath())
            self.panel.editor.path = path
            self.panel.editor.setStyle()
            self.panel.editor.SetCurrentPos(0)
            self.panel.editor.addText(" ", False)
            self.panel.editor.DeleteBackNotLine()
            self.panel.editor.saveMyFile(path)
            self.SetTitle(path)
            tab = self.panel.notebook.GetSelection()
            self.panel.notebook.SetPageText(tab, os.path.split(path)[1].split('.')[0])
            self.newRecent(path)
            PREFERENCES["save_file_path"] = os.path.split(path)[0]
        dlg.Destroy()

    def saveasTemplate(self, event):
        dlg = wx.TextEntryDialog(self, 'Give a name to your template:', 'Save file as template...')
        if dlg.ShowModal() == wx.ID_OK:
            fname = dlg.GetValue()
            if not fname.endswith(".py"):
                fname = fname + ".py"
            try:
                text = self.panel.editor.GetTextUTF8()
            except:
                text = self.panel.editor.GetText()
            with open(os.path.join(TEMPLATE_PATH, fname), "w") as f:
                f.write(text)
        dlg.Destroy()

    def close(self, event):
        action = self.panel.editor.close()
        if action == 'delete':
            self.panel.close_from_menu = True
            self.panel.deletePage()
        else:
            pass
        self.panel.close_from_menu = False

    def closeAll(self, event):
        count = self.panel.notebook.GetPageCount()
        while count > 0:
            count -= 1
            self.panel.setPage(count)
            self.close(None)

    ### Run actions ###
    def getCurrentWorkingDirectory(self):
        if self.master_document != None:
            path = ensureNFD(self.master_document)
        else:
            path = ensureNFD(self.panel.editor.path)
        cwd = os.path.expanduser("~")
        if os.path.isfile(path):
            try:
                cwd = toSysEncoding(os.path.split(path)[0])
            except:
                pass
        return cwd

    def addCwdToSysPath(self, text):
        cwd = self.getCurrentWorkingDirectory()
        check = True
        newtext = ""
        for line in text.splitlines():
            if check and not line.startswith("#"):
                newtext += '# encoding: utf-8\nimport sys\nsys.path.append("%s")\n' % cwd
                check = False
            newtext += line + "\n"
        return newtext

    def format_outputLog(self, evt):
        data = evt.data
        self.panel.outputlog.appendToLog(data["log"])
        if not data["active"]:
            self.panel.outputlog.removeProcess(data["pid"], data["filename"])

    def run(self, path):
        cwd = self.getCurrentWorkingDirectory()
        th = RunningThread(path, cwd, self)
        if self.master_document != None:
            filename = os.path.split(self.master_document)[1]
        elif "Untitled-" in self.panel.editor.path:
            filename = self.panel.editor.path
        else:
            filename = os.path.split(self.panel.editor.path)[1]
        th.setFileName(filename)
        th.setPID(self.processID)
        self.processes[self.processID] = [th, filename]
        self.panel.outputlog.addProcess(self.processID, filename)
        self.processID += 1
        th.start()

    def runner(self, event):
        if self.master_document != None:
            with open(self.master_document, "r") as f:
                text = f.read()
        else:
            text = self.panel.editor.GetTextUTF8()
        if text != "":
            text = self.addCwdToSysPath(text)
            with open(TEMP_FILE, "w") as f:
                f.write(text)
            self.run(TEMP_FILE)

    def runSelection(self, event):
        text = self.panel.editor.GetSelectedTextUTF8()
        if text != "":
            text = self.addCwdToSysPath(text)
            with open(TEMP_FILE, "w") as f:
                f.write(text)
            self.run(TEMP_FILE)

    def runSelectionAsPyo(self, event):
        text = self.panel.editor.GetSelectedTextUTF8()
        if text == "":
            pos = self.panel.editor.GetCurrentPos()
            line = self.panel.editor.LineFromPosition(pos)
            text = self.panel.editor.GetLineUTF8(line)
        text = self.addCwdToSysPath(text)
        with open(TEMP_FILE, "w") as f:
            f.write("from pyo import *\ns = Server().boot()\n")
            f.write(text)
            f.write("\ns.gui(locals())\n")
        self.run(TEMP_FILE)

    def execSelection(self, event):
        text = self.panel.editor.GetSelectedTextUTF8()
        if text == "":
            pos = self.panel.editor.GetCurrentPos()
            line = self.panel.editor.LineFromPosition(pos)
            text = self.panel.editor.GetLineUTF8(line)
            if not text.startswith("print"):
                text = "print " + text
        else:
            pos = self.panel.editor.GetSelectionEnd()
        line = self.panel.editor.LineFromPosition(pos)
        pos = self.panel.editor.GetLineEndPosition(line)
        self.panel.editor.SetCurrentPos(pos)
        self.panel.editor.addText("\n", False)
        with stdoutIO() as s:
            exec text
        self.panel.editor.addText(s.getvalue())

    def prepareBackgroundServer(self):
        outDriverIndex = -1
        preferedDriver = PREFERENCES.get("background_server_out_device", "")
        if preferedDriver != "":
            driverList, driverIndexes = pa_get_output_devices()
            driverList = [ensureNFD(driver) for driver in driverList]
            if preferedDriver and preferedDriver in driverList:
                outDriverIndex = driverIndexes[driverList.index(preferedDriver)]

        inDriverIndex = -1
        preferedDriver = PREFERENCES.get("background_server_in_device", "")
        if preferedDriver != "":
            driverList, driverIndexes = pa_get_input_devices()
            driverList = [ensureNFD(driver) for driver in driverList]
            if preferedDriver and preferedDriver in driverList:
                inDriverIndex = driverIndexes[driverList.index(preferedDriver)]

        midiOutDriverIndex = -1
        preferedDriver = PREFERENCES.get("background_server_midiout_device", "")
        if preferedDriver != "":
            driverList, driverIndexes = pm_get_output_devices()
            driverList = [ensureNFD(driver) for driver in driverList]
            if preferedDriver and preferedDriver in driverList:
                midiOutDriverIndex = driverIndexes[driverList.index(preferedDriver)]

        midiInDriverIndex = -1
        preferedDriver = PREFERENCES.get("background_server_midiin_device", "")
        if preferedDriver != "":
            driverList, driverIndexes = pm_get_input_devices()
            driverList = [ensureNFD(driver) for driver in driverList]
            if preferedDriver and preferedDriver in driverList:
                midiInDriverIndex = driverIndexes[driverList.index(preferedDriver)]

        with open(os.path.join(TEMP_PATH, "background_server.py"), "w") as f:
            f.write("print 'Starting background server...'\nimport time\nfrom pyo import *\n")
            f.write("s = Server(%s)\n" % BACKGROUND_SERVER_ARGS)
            if outDriverIndex != -1:
                f.write("s.setOutputDevice(%d)\n" % outDriverIndex)
            if inDriverIndex != -1:
                f.write("s.setInputDevice(%d)\n" % inDriverIndex)
            if midiOutDriverIndex != -1:
                f.write("s.setMidiOutputDevice(%d)\n" % midiOutDriverIndex)
            if midiInDriverIndex != -1:
                f.write("s.setMidiInputDevice(%d)\n" % midiInDriverIndex)
            f.write("s.boot()\ns.start()\n\n")
            f.write("def _quit_():\n    s.stop()\n    time.sleep(0.25)\n    exit()\n")

    def resetBackgroundServerMenu(self):
        self.back_server_started = False
        self.backServerItem.SetItemLabel("Start Pyo Background Server")
        self.sendToServerItem.Enable(False)
        
    def startStopBackgroundServer(self, evt):
        if not self.back_server_started:
            self.prepareBackgroundServer()
            cwd = self.getCurrentWorkingDirectory()
            th = BackgroundServerThread(cwd, self)
            th.setPID(1000)
            self.processes[1000] = [th, 'background_server.py']
            self.panel.outputlog.addProcess(1000, 'background_server.py')
            th.start()
            self.back_server_started = True
            self.backServerItem.SetItemLabel("Stop Pyo Background Server")
            self.sendToServerItem.Enable(True)
        else:
            self.processes[1000][0].kill()
            self.back_server_started = False
            self.backServerItem.SetItemLabel("Start Pyo Background Server")
            self.sendToServerItem.Enable(False)

    def sendSelectionToBackgroundServer(self, evt):
        end = None
        text = self.panel.editor.GetSelectedTextUTF8()
        if text == "":
            pos = self.panel.editor.GetCurrentPos()
            line = self.panel.editor.LineFromPosition(pos)
            text = self.panel.editor.GetLineUTF8(line)
        else:
            end = self.panel.editor.GetSelectionEnd()
        if self.back_server_started:
            self.processes[1000][0].sendText(text)
        if end != None:
            self.panel.editor.SetCurrentPos(end)
        self.panel.editor.LineDown()
        line = self.panel.editor.GetCurrentLine()
        pos = self.panel.editor.PositionFromLine(line)
        self.panel.editor.SetCurrentPos(pos)
        self.panel.editor.SetSelectionEnd(pos)

    def buildDoc(self):
        self.doc_frame = ManualFrame(osx_app_bundled=OSX_APP_BUNDLED, which_python=WHICH_PYTHON,
                                    caller_need_to_invoke_32_bit=CALLER_NEED_TO_INVOKE_32_BIT,
                                    set_32_bit_arch=SET_32_BIT_ARCH)

    def showDoc(self, evt):
        if not self.doc_frame.IsShown():
            self.doc_frame.Show()
        word = self.panel.editor.getWordUnderCaret()
        if word:
            self.doc_frame.doc_panel.getPage(word)

    def showDocFrame(self, evt):
        if not self.doc_frame.IsShown():
            self.doc_frame.Show()

    def rebuildDoc(self, evt):
        shutil.rmtree(os.path.join(TEMP_PATH, "doc"), True)
        try:
            self.doc_frame.Destroy()
        except:
            pass
        self.buildDoc()

    def showArgs(self, evt):
        self.panel.editor.onShowTip()

    def showDocString(self, evt):
        self.panel.editor.onShowDocString()
        
    def onShowEditorKeyCommands(self, evt):
        if not self.keyCommandsFrame.IsShown():
            self.keyCommandsFrame.CenterOnParent()
            self.keyCommandsFrame.Show()

    def onHelpAbout(self, evt):
        info = wx.AboutDialogInfo()
        info.Name = APP_NAME
        info.Version = APP_VERSION
        info.Copyright = u"(C) 2012 Olivier Belanger"
        info.Description = "E-Pyo is a text editor especially configured to edit pyo audio programs.\n\n"
        wx.AboutBox(info)

    def OnClose(self, event):
        if self.back_server_started == True:
            try:
                self.startStopBackgroundServer(None)
                time.sleep(0.5)
            except:
                pass
        with open(PREFERENCES_PATH, "w") as f:
            f.write("epyo_prefs = %s" % str(PREFERENCES))
        try:
            self.snippet_frame.Destroy()
        except:
            pass
        try:
            self.doc_frame.Destroy()
        except:
            pass
        try:
            self.keyCommandsFrame.Destroy()
        except:
            pass
        self.panel.OnQuit()
        self.Destroy()

    def getPrintData(self):
        return self.print_data

    def OnPrintPreview(self, evt):
        wx.CallAfter(self.showPrintPreview)

    def showPrintPreview(self):
        printout = STCPrintout(self.panel.editor, title="", border=False, output_point_size=None)
        printout2 = STCPrintout(self.panel.editor, title="", border=False, output_point_size=None)
        preview = wx.PrintPreview(printout, printout2, self.getPrintData())
        preview.SetZoom(100)
        if preview.IsOk():
            pre_frame = wx.PreviewFrame(preview, self, "Print Preview")
            dsize = wx.GetDisplaySize()
            pre_frame.SetInitialSize((self.GetSize()[0], dsize.GetHeight() - 100))
            pre_frame.Initialize()
            pre_frame.Show()
        else:
            wx.MessageBox("Failed to create print preview",
                          "Print Error", style=wx.ICON_ERROR|wx.OK)

    def OnPrint(self, evt):
        wx.CallAfter(self.showPrint)

    def showPrint(self):
        pdd = wx.PrintDialogData(self.getPrintData())
        printer = wx.Printer(pdd)
        printout = STCPrintout(self.panel.editor, title="", border=False, output_point_size=None)
        result = printer.Print(self.panel.editor, printout)
        if result:
            data = printer.GetPrintDialogData()
            self.print_data = wx.PrintData(data.GetPrintData())
        elif printer.GetLastError() == wx.PRINTER_ERROR:
            wx.MessageBox("There was an error when printing.\n"
                            "Check that your printer is properly connected.",
                          "Printer Error", style=wx.ICON_ERROR|wx.OK)
        printout.Destroy()

class MainPanel(wx.Panel):
    def __init__(self, parent, size=(1200,800), style=wx.SUNKEN_BORDER):
        wx.Panel.__init__(self, parent, size=size, style=wx.SUNKEN_BORDER)

        self.new_inc = 0
        self.close_from_menu = False
        self.mainFrame = parent
        mainBox = wx.BoxSizer(wx.HORIZONTAL)

        self.splitter = wx.SplitterWindow(self, -1, style=wx.SP_LIVE_UPDATE|wx.SP_3DSASH)
        self.splitter.SetMinimumPaneSize(150)

        self.left_splitter = wx.SplitterWindow(self.splitter, -1, style=wx.SP_LIVE_UPDATE|wx.SP_3DSASH)
        self.right_splitter = wx.SplitterWindow(self.splitter, -1, style=wx.SP_LIVE_UPDATE|wx.SP_3DSASH)

        self.project = ProjectTree(self.left_splitter, self, (-1, -1))
        self.markers = MarkersPanel(self.left_splitter, self, (-1, -1))

        self.notebook = FNB.FlatNotebook(self.right_splitter, size=(-1,-1))
        self.notebook.SetAGWWindowStyleFlag(FNB.FNB_FF2|FNB.FNB_X_ON_TAB|FNB.FNB_NO_X_BUTTON|FNB.FNB_DROPDOWN_TABS_LIST|FNB.FNB_HIDE_ON_SINGLE_TAB)
        self.addNewPage()
        self.outputlog = OutputLogPanel(self.right_splitter, self, size=(-1,150))

        self.right_splitter.SplitHorizontally(self.notebook, self.outputlog, (self.GetSize()[1]*4/5) - self.GetSize()[1])

        self.splitter.SplitVertically(self.left_splitter, self.right_splitter, 175)
        self.splitter.Unsplit(self.left_splitter)

        mainBox.Add(self.splitter, 1, wx.EXPAND)
        self.SetSizer(mainBox)

        self.notebook.Bind(FNB.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.onPageChange)
        self.notebook.Bind(FNB.EVT_FLATNOTEBOOK_PAGE_CLOSING, self.onClosingPage)

    def addNewPage(self):
        title = "Untitled-%i.py" % self.new_inc
        self.new_inc += 1
        editor = Editor(self.notebook, -1, size=(0, -1), setTitle=self.SetTitle, getTitle=self.GetTitle)
        editor.path = title
        editor.setStyle()
        self.notebook.AddPage(editor, title, True)
        self.editor = editor

    def addPage(self, file, encoding=None):
        editor = Editor(self.notebook, -1, size=(0, -1), setTitle=self.SetTitle, getTitle=self.GetTitle)
        label = os.path.split(file)[1].split('.')[0]
        self.notebook.AddPage(editor, label, True)
        text = ""
        if encoding != None:
            with codecs.open(file, "r", encoding=encoding) as f:
                text = f.read()
        else:
            for enc in ENCODING_LIST:
                try:
                    with codecs.open(file, "r", encoding=enc) as f:
                        text = f.read()
                    break
                except:
                    continue
        editor.setText(ensureNFD(text))

        # Scan the entire document (needed for FoldAll to fold everything)
        editor.GotoLine(editor.GetLineCount())
        wx.CallAfter(editor.GotoLine, 0)

        editor.path = file
        editor.saveMark = True
        editor.EmptyUndoBuffer()
        editor.SetSavePoint()
        editor.setStyle()
        self.editor = editor
        self.SetTitle(file)
        with open(MARKERS_FILE, "r") as f:
            lines = [line.replace("\n", "").split("=") for line in f.readlines()]
        founded = False
        for line in lines:
            if line[1] == editor.path:
                marker_file = line[0]
                founded = True
                break
        if founded:
            with open(os.path.join(MARKERS_PATH, marker_file), "r") as f:
                text = f.read()
            exec text in locals()
            self.editor.setMarkers(copy.deepcopy(markers))

    def onClosingPage(self, evt):
        if not self.close_from_menu:
            action = self.editor.close()
            if action == "keep":
                evt.Veto()

    def deletePage(self):
        select = self.notebook.GetSelection()
        self.notebook.DeletePage(select)
        if self.notebook.GetPageCount() == 0:
            self.addNewPage()

    def setPage(self, pageNum):
        totalNum = self.notebook.GetPageCount()
        if pageNum < totalNum:
            self.notebook.SetSelection(pageNum)

    def onPageChange(self, event):
        self.markers.setDict({})
        self.editor = self.notebook.GetPage(self.notebook.GetSelection())
        self.editor.SetFocus()
        if not self.editor.path:
            if self.editor.GetModify():
                self.SetTitle("*** E-Pyo Editor ***")
            else:
                self.SetTitle("E-Pyo Editor")
        else:
            if self.editor.GetModify():
                self.SetTitle('*** ' + self.editor.path + ' ***')
            else:
                self.SetTitle(self.editor.path)
        self.markers.setDict(self.editor.markers_dict)

    def SetTitle(self, title):
        self.mainFrame.SetTitle(title)

    def GetTitle(self):
        return self.mainFrame.GetTitle()

    def OnQuit(self):
        for i in range(self.notebook.GetPageCount()):
            ed = self.notebook.GetPage(i)
            ed.Close()

#######################################################
### The idea of EditorPanel is to allow multiple views
### at the same time in a single notebook page. 
### Also: A tree view of classes and functions of the file
### Not yet implemented... ( TODO )
#######################################################
class EditorPanel(wx.Panel):
    def __init__(self, parent):
        wx.Panel.__init__(self, parent, -1)
        self.editor = Editor(parent, -1, size=(0, -1))
        self.editor2 = Editor(parent, -1, size=(0, -1))
        box = wx.BoxSizer(wx.HORIZONTAL)
        box.Add(self.editor, 1, wx.ALL|wx.EXPAND, 5)
        box.Add(self.editor2, 1, wx.ALL|wx.EXPAND, 5)
        self.SetSizerAndFit(box)

class Editor(stc.StyledTextCtrl):
    def __init__(self, parent, ID, pos=wx.DefaultPosition, size=wx.DefaultSize, 
                 style= wx.NO_BORDER | wx.WANTS_CHARS, setTitle=None, getTitle=None):
        stc.StyledTextCtrl.__init__(self, parent, ID, pos, size, style)

        dt = MyFileDropTarget(self)
        self.SetDropTarget(dt)

        self.SetSTCCursor(2)
        self.panel = parent

        self.path = ''
        self.setTitle = setTitle
        self.getTitle = getTitle
        self.saveMark = False
        self.inside = False
        self.anchor1 = self.anchor2 = 0
        self.args_buffer = []
        self.snip_buffer = []
        self.args_line_number = [0,0]
        self.quit_navigate_args = False
        self.quit_navigate_snip = False
        self.markers_dict = {}
        self.current_marker = -1
        self.objs_attr_dict = {}
        self.auto_comp_container = PREFERENCES.get("auto_comp_container", 0)


        self.alphaStr = string.lowercase + string.uppercase + '0123456789'

        self.Colourise(0, -1)
        self.SetCurrentPos(0)

        self.SetIndent(4)
        self.SetBackSpaceUnIndents(True)
        self.SetTabIndents(True)
        self.SetTabWidth(4)
        self.SetUseTabs(False)
        self.AutoCompSetChooseSingle(True)
        self.SetEOLMode(wx.stc.STC_EOL_LF)
        self.SetPasteConvertEndings(True)
        self.SetControlCharSymbol(32)
        self.SetLayoutCache(True)

        self.SetViewWhiteSpace(PREFERENCES.get("show_invisibles", 0))
        self.SetViewEOL(PREFERENCES.get("show_invisibles", 0))
        self.SetEdgeMode(PREFERENCES.get("show_edge_line", 0))
        mode = {0: stc.STC_WRAP_NONE, 1: stc.STC_WRAP_WORD}[PREFERENCES.get("wrap_text_line", 0)]
        self.SetWrapMode(mode)

        self.SetProperty("fold", "1")
        self.SetProperty("tab.timmy.whinge.level", "1")
        self.SetMargins(5, 5)
        self.SetUseAntiAliasing(True)
        self.SetEdgeColour(STYLES["lineedge"]['colour'])
        self.SetEdgeColumn(78)

        self.SetMarginType(0, stc.STC_MARGIN_SYMBOL)
        self.SetMarginWidth(0, 12)
        self.SetMarginMask(0, ~wx.stc.STC_MASK_FOLDERS)
        self.SetMarginSensitive(0, True)

        self.SetMarginType(1, stc.STC_MARGIN_NUMBER)
        self.SetMarginWidth(1, 28)
        self.SetMarginMask(1, 0)
        self.SetMarginSensitive(1, False)

        self.SetMarginType(2, stc.STC_MARGIN_SYMBOL)
        self.SetMarginWidth(2, 12)
        self.SetMarginMask(2, stc.STC_MASK_FOLDERS)
        self.SetMarginSensitive(2, True)

        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        self.Bind(wx.EVT_CHAR, self.OnChar)
        self.Bind(stc.EVT_STC_UPDATEUI, self.OnUpdateUI)
        self.Bind(stc.EVT_STC_MARGINCLICK, self.OnMarginClick)
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.Bind(wx.EVT_LEFT_DCLICK, self.OnDoubleClick)

        self.EmptyUndoBuffer()
        self.SetFocus()
        self.setStyle()

        # Remove unwanted KeyCommands
        self.CmdKeyClear(stc.STC_KEY_RIGHT, stc.STC_SCMOD_ALT)
        self.CmdKeyClear(stc.STC_KEY_LEFT, stc.STC_SCMOD_ALT)
        self.CmdKeyClear(stc.STC_KEY_RIGHT, stc.STC_SCMOD_SHIFT | stc.STC_SCMOD_ALT)
        self.CmdKeyClear(stc.STC_KEY_LEFT, stc.STC_SCMOD_SHIFT | stc.STC_SCMOD_ALT)
        self.CmdKeyClear(stc.STC_KEY_RIGHT, stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(stc.STC_KEY_LEFT, stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(stc.STC_KEY_RIGHT, stc.STC_SCMOD_SHIFT | stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(stc.STC_KEY_LEFT, stc.STC_SCMOD_SHIFT | stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(stc.STC_KEY_DELETE, 0)
        self.CmdKeyClear(stc.STC_KEY_DELETE, stc.STC_SCMOD_SHIFT)
        self.CmdKeyClear(stc.STC_KEY_DELETE, stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(stc.STC_KEY_DELETE, stc.STC_SCMOD_SHIFT | stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(stc.STC_KEY_BACK, stc.STC_SCMOD_ALT)
        self.CmdKeyClear(stc.STC_KEY_BACK, stc.STC_SCMOD_SHIFT)
        self.CmdKeyClear(stc.STC_KEY_BACK, stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(stc.STC_KEY_BACK, stc.STC_SCMOD_CTRL | stc.STC_SCMOD_SHIFT)
        self.CmdKeyClear(stc.STC_KEY_INSERT, 0)
        self.CmdKeyClear(stc.STC_KEY_INSERT, stc.STC_SCMOD_SHIFT)
        self.CmdKeyClear(stc.STC_KEY_INSERT, stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(ord('Y'), stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(ord('D'), stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(ord('L'), stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(ord('T'), stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(ord('L'), stc.STC_SCMOD_CTRL | stc.STC_SCMOD_SHIFT)
        self.CmdKeyClear(stc.STC_KEY_RETURN, stc.STC_SCMOD_SHIFT)
        self.CmdKeyClear(stc.STC_KEY_ADD, stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(stc.STC_KEY_SUBTRACT, stc.STC_SCMOD_CTRL)
        self.CmdKeyClear(stc.STC_KEY_DIVIDE, stc.STC_SCMOD_CTRL)

        self.CmdKeyAssign(ord('U'), stc.STC_SCMOD_CTRL, stc.STC_CMD_UPPERCASE)
        self.CmdKeyAssign(ord('U'), stc.STC_SCMOD_CTRL | stc.STC_SCMOD_SHIFT, stc.STC_CMD_LOWERCASE)

        wx.CallAfter(self.SetAnchor, 0)
        self.Refresh()

    def setStyle(self):
        def buildStyle(forekey, backkey=None, smallsize=False):
            if smallsize:
                st = "face:%s,fore:%s,size:%s" % (STYLES['face'], STYLES[forekey]['colour'], STYLES['size2'])
            else:
                st = "face:%s,fore:%s,size:%s" % (STYLES['face'], STYLES[forekey]['colour'], STYLES['size'])
            if backkey:
                st += ",back:%s" % STYLES[backkey]['colour']
            if STYLES[forekey].has_key('bold'):
                if STYLES[forekey]['bold']:
                    st += ",bold"
                if STYLES[forekey]['italic']:
                    st += ",italic"
                if STYLES[forekey]['underline']:
                    st += ",underline"
            return st

        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, buildStyle('default', 'background'))
        self.StyleClearAll()  # Reset all to be like the default

        self.MarkerDefine(0, stc.STC_MARK_SHORTARROW, STYLES['markerbg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPEN, stc.STC_MARK_BOXMINUS, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDER, stc.STC_MARK_BOXPLUS, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERSUB, stc.STC_MARK_VLINE, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERTAIL, stc.STC_MARK_LCORNERCURVE, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEREND, stc.STC_MARK_ARROW, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPENMID, stc.STC_MARK_ARROWDOWN, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERMIDTAIL, stc.STC_MARK_LCORNERCURVE, STYLES['markerfg']['colour'], STYLES['markerbg']['colour'])
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, buildStyle('default', 'background'))
        self.StyleSetSpec(stc.STC_STYLE_LINENUMBER, buildStyle('linenumber', 'marginback', True))
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, buildStyle('default') + ",size:5")
        self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT, buildStyle('default', 'bracelight') + ",bold")
        self.StyleSetSpec(stc.STC_STYLE_BRACEBAD, buildStyle('default', 'bracebad') + ",bold")

        ext = os.path.splitext(self.path)[1].strip(".")
        if ext == "":
            try:
                with open(self.path, "r") as f:
                    fline = f.readline()
                    if fline.startswith("#!"):
                        fline = fline.replace("/", " ")
                        last = fline.split()[-1]
                        ext = {"python": "py", "bash": "sh", "sh": "sh"}.get(last, "")
                    else:
                        text = f.read()
                        if "desc:" in text:
                            ext = "jsfx"
            except:
                pass
        if ext in ["py", "pyw", "c5"]:
            self.SetLexer(stc.STC_LEX_PYTHON)
            self.SetStyleBits(self.GetStyleBitsNeeded())
            self.SetKeyWords(0, " ".join(keyword.kwlist) + " None True False ")
            self.SetKeyWords(1, " ".join(PYO_WORDLIST))
            self.StyleSetSpec(stc.STC_P_DEFAULT, buildStyle('default'))
            self.StyleSetSpec(stc.STC_P_COMMENTLINE, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_P_NUMBER, buildStyle('number'))
            self.StyleSetSpec(stc.STC_P_STRING, buildStyle('string'))
            self.StyleSetSpec(stc.STC_P_CHARACTER, buildStyle('string'))
            self.StyleSetSpec(stc.STC_P_WORD, buildStyle('keyword'))
            self.StyleSetSpec(stc.STC_P_WORD2, buildStyle('pyokeyword'))
            self.StyleSetSpec(stc.STC_P_TRIPLE, buildStyle('triple'))
            self.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, buildStyle('triple'))
            self.StyleSetSpec(stc.STC_P_CLASSNAME, buildStyle('class'))
            self.StyleSetSpec(stc.STC_P_DEFNAME, buildStyle('function'))
            self.StyleSetSpec(stc.STC_P_OPERATOR, buildStyle('operator'))
            self.StyleSetSpec(stc.STC_P_IDENTIFIER, buildStyle('default'))
            self.StyleSetSpec(stc.STC_P_COMMENTBLOCK, buildStyle('commentblock'))
        elif ext in ["c", "cc", "cpp", "cxx", "cs", "h", "hh", "hpp", "hxx"]:
            self.SetLexer(stc.STC_LEX_CPP)
            self.SetStyleBits(self.GetStyleBitsNeeded())
            self.SetProperty('fold.comment', '1')
            self.SetProperty('fold.preprocessor', '1')
            self.SetProperty('fold.compact', '1')
            self.SetProperty('styling.within.preprocessor', '0')
            self.SetKeyWords(0, "auto break case char const continue default do double else enum extern float for goto if int long \
            register return short signed sizeof static struct switch typedef union unsigned void volatile while ")
            self.StyleSetSpec(stc.STC_C_DEFAULT, buildStyle('default'))
            self.StyleSetSpec(stc.STC_C_COMMENT, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_C_COMMENTDOC, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_C_COMMENTLINE, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_C_COMMENTLINEDOC, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_C_NUMBER, buildStyle('number'))
            self.StyleSetSpec(stc.STC_C_STRING, buildStyle('string'))
            self.StyleSetSpec(stc.STC_C_CHARACTER, buildStyle('string'))
            self.StyleSetSpec(stc.STC_C_WORD, buildStyle('keyword'))
            self.StyleSetSpec(stc.STC_C_OPERATOR, buildStyle('operator'))
            self.StyleSetSpec(stc.STC_C_IDENTIFIER, buildStyle('default'))
            self.StyleSetSpec(stc.STC_C_PREPROCESSOR, buildStyle('commentblock'))
        elif ext == "sh":
            self.SetLexer(stc.STC_LEX_BASH)
            self.SetStyleBits(self.GetStyleBitsNeeded())
            self.SetKeyWords(0, "! [[ ]] case do done elif else esac fi for function if in select then time until while { } \
            alias bg bind break builtin caller cd command compgen complete compopt continue declare dirs disown echo enable \
            eval exec exit export fc fg getopts hash help history jobs kill let local logout mapfile popd printf pushd pwd \
            read readarray readonly return set shift shopt source suspend test times trap type typeset ulimit umask unalias unset wait")
            self.StyleSetSpec(stc.STC_SH_DEFAULT, buildStyle('default'))
            self.StyleSetSpec(stc.STC_SH_COMMENTLINE, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_SH_NUMBER, buildStyle('number'))
            self.StyleSetSpec(stc.STC_SH_STRING, buildStyle('string'))
            self.StyleSetSpec(stc.STC_SH_CHARACTER, buildStyle('string'))
            self.StyleSetSpec(stc.STC_SH_WORD, buildStyle('keyword'))
            self.StyleSetSpec(stc.STC_SH_OPERATOR, buildStyle('default'))
            self.StyleSetSpec(stc.STC_SH_IDENTIFIER, buildStyle('default'))
            self.StyleSetSpec(stc.STC_SH_PARAM, buildStyle('default'))
            self.StyleSetSpec(stc.STC_SH_SCALAR, buildStyle('function'))
        elif ext in ["jsfx", "jsfx-inc"]:
            self.SetLexer(stc.STC_LEX_CPP)
            self.SetStyleBits(self.GetStyleBitsNeeded())
            self.SetProperty('fold.comment', '1')
            self.SetProperty('fold.preprocessor', '1')
            self.SetProperty('fold.compact', '1')
            self.SetProperty('styling.within.preprocessor', '0')
            self.SetKeyWords(1, "abs acos asin atan atan2 atexit ceil convolve_c cos defer eval exp fclose feof fflush \
                                 fft fft_ipermute fft_permute fgetc floor fopen fprintf fread freembuf fseek ftell fwrite \
                                 gfx_aaaaa gfx_arc gfx_blit gfx_blit gfx_blitext gfx_blurto gfx_circle gfx_deltablit \
                                 gfx_drawchar gfx_drawnumber gfx_drawstr gfx_getchar gfx_getfont gfx_getimgdim gfx_getpixel \
                                 gfx_gradrect gfx_init gfx_line gfx_lineto gfx_loadimg gfx_measurestr gfx_muladdrect gfx_printf \
                                 gfx_quit gfx_rect gfx_rectto gfx_roundrect gfx_setfont gfx_setimgdim gfx_setpixel gfx_transformblit \
                                 gfx_update ifft invsqrt log log10 match matchi max memcpy memset min pow printf rand sign sin sleep \
                                 sprintf sqr sqrt stack_exch stack_peek stack_pop stack_push str_delsub str_getchar str_insert \
                                 str_setchar str_setlen strcat strcmp strcpy strcpy_from strcpy_substr stricmp strlen strncat strncmp \
                                 strncpy strnicmp tan tcp_close tcp_connect tcp_listen tcp_listen_end tcp_recv tcp_send tcp_set_block \
                                 time time_precise")
            self.SetKeyWords(0, "loop while function local static instance this global globals _global gfx_r gfx_g gfx_b gfx_a gfx_w \
                                 gfx_h gfx_x gfx_y gfx_mode gfx_clear gfx_dest gfx_texth mouse_x mouse_y mouse_cap mouse_wheel mouse_hwheel \
                                 @init @slider @sample @block @serialize @gfx import desc slider1 slider2 slider3 slider4 slider5 \
                                 slider6 slider7 slider8 slider9 slider10 slider11 slider12 slider13 slider14 slider15 slider16 in_pin \
                                 out_pin filename ")
            self.StyleSetSpec(stc.STC_C_DEFAULT, buildStyle('default'))
            self.StyleSetSpec(stc.STC_C_COMMENT, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_C_COMMENTDOC, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_C_COMMENTLINE, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_C_COMMENTLINEDOC, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_C_NUMBER, buildStyle('number'))
            self.StyleSetSpec(stc.STC_C_STRING, buildStyle('string'))
            self.StyleSetSpec(stc.STC_C_CHARACTER, buildStyle('string'))
            self.StyleSetSpec(stc.STC_C_WORD, buildStyle('keyword'))
            self.StyleSetSpec(stc.STC_C_WORD2, buildStyle('pyokeyword'))
            self.StyleSetSpec(stc.STC_C_OPERATOR, buildStyle('operator'))
            self.StyleSetSpec(stc.STC_C_IDENTIFIER, buildStyle('default'))
            self.StyleSetSpec(stc.STC_C_PREPROCESSOR, buildStyle('commentblock'))
        elif ext == "md":
            self.SetLexer(stc.STC_LEX_MARKDOWN)
            self.SetStyleBits(self.GetStyleBitsNeeded())
            self.StyleSetSpec(stc.STC_MARKDOWN_DEFAULT, buildStyle('default'))
            self.StyleSetSpec(stc.STC_MARKDOWN_LINE_BEGIN, buildStyle('default'))
            self.StyleSetSpec(stc.STC_MARKDOWN_STRONG1, buildStyle('default') + ",italic,bold")
            self.StyleSetSpec(stc.STC_MARKDOWN_STRONG2, buildStyle('default') + ",italic,bold")
            self.StyleSetSpec(stc.STC_MARKDOWN_EM1, buildStyle('default') + ",italic")
            self.StyleSetSpec(stc.STC_MARKDOWN_EM2, buildStyle('default') + ",italic")
            self.StyleSetSpec(stc.STC_MARKDOWN_HEADER1, buildStyle('comment') + ", bold")
            self.StyleSetSpec(stc.STC_MARKDOWN_HEADER2, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_MARKDOWN_HEADER3, buildStyle('commentblock') + ", bold")
            self.StyleSetSpec(stc.STC_MARKDOWN_HEADER4, buildStyle('commentblock') + ", bold")
            self.StyleSetSpec(stc.STC_MARKDOWN_HEADER5, buildStyle('commentblock'))
            self.StyleSetSpec(stc.STC_MARKDOWN_HEADER6, buildStyle('commentblock'))
            self.StyleSetSpec(stc.STC_MARKDOWN_PRECHAR, buildStyle('default'))
            self.StyleSetSpec(stc.STC_MARKDOWN_ULIST_ITEM, buildStyle('string'))
            self.StyleSetSpec(stc.STC_MARKDOWN_OLIST_ITEM, buildStyle('triple'))
            self.StyleSetSpec(stc.STC_MARKDOWN_BLOCKQUOTE, buildStyle('string'))
            self.StyleSetSpec(stc.STC_MARKDOWN_STRIKEOUT, buildStyle('string'))
            self.StyleSetSpec(stc.STC_MARKDOWN_HRULE, buildStyle('triple'))
            self.StyleSetSpec(stc.STC_MARKDOWN_LINK, buildStyle('function'))
            self.StyleSetSpec(stc.STC_MARKDOWN_CODE, buildStyle('default') + ",italic,bold")
            self.StyleSetSpec(stc.STC_MARKDOWN_CODE2, buildStyle('default') + ",italic,bold")
            self.StyleSetSpec(stc.STC_MARKDOWN_CODEBK, buildStyle('default') + ",italic,bold")
        elif ext == "lua":
            self.SetLexer(stc.STC_LEX_LUA)
            self.SetStyleBits(self.GetStyleBitsNeeded())
            self.SetKeyWords(0, "and break do else elseif for if in nil not or \
                        repeat then until while function local end return true false ")
            self.StyleSetSpec(stc.STC_LUA_DEFAULT, buildStyle('default'))
            self.StyleSetSpec(stc.STC_LUA_COMMENT, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_LUA_COMMENTLINE, buildStyle('comment'))
            self.StyleSetSpec(stc.STC_LUA_COMMENTDOC, buildStyle('commentblock'))
            self.StyleSetSpec(stc.STC_LUA_NUMBER, buildStyle('number'))
            self.StyleSetSpec(stc.STC_LUA_WORD, buildStyle('keyword'))
            self.StyleSetSpec(stc.STC_LUA_STRING, buildStyle('string'))
            self.StyleSetSpec(stc.STC_LUA_CHARACTER, buildStyle('string'))
            self.StyleSetSpec(stc.STC_LUA_LITERALSTRING, buildStyle('triple'))
            self.StyleSetSpec(stc.STC_LUA_PREPROCESSOR, buildStyle('default') + ",italic")
            self.StyleSetSpec(stc.STC_LUA_OPERATOR, buildStyle('operator'))
            self.StyleSetSpec(stc.STC_LUA_IDENTIFIER, buildStyle('default'))
            self.StyleSetSpec(stc.STC_LUA_STRINGEOL, buildStyle('default') + ",bold")
            self.StyleSetSpec(stc.STC_LUA_WORD2, buildStyle('pyokeyword'))
            self.StyleSetSpec(stc.STC_LUA_WORD3, buildStyle('pyokeyword'))
            self.StyleSetSpec(stc.STC_LUA_WORD4, buildStyle('pyokeyword'))
            self.StyleSetSpec(stc.STC_LUA_WORD5, buildStyle('pyokeyword'))
            self.StyleSetSpec(stc.STC_LUA_WORD6, buildStyle('pyokeyword'))
            self.StyleSetSpec(stc.STC_LUA_WORD7, buildStyle('pyokeyword'))
            self.StyleSetSpec(stc.STC_LUA_WORD8, buildStyle('pyokeyword'))
            self.StyleSetSpec(stc.STC_LUA_LABEL, buildStyle('default') + ",italic,bold")

        self.SetEdgeColour(STYLES["lineedge"]['colour'])
        self.SetCaretForeground(STYLES['caret']['colour'])
        self.SetSelBackground(1, STYLES['selback']['colour'])
        self.SetFoldMarginColour(True, STYLES['foldmarginback']['colour'])
        self.SetFoldMarginHiColour(True, STYLES['foldmarginback']['colour'])
        self.CallTipSetForeground(STYLES['default']['colour'])
        self.CallTipSetBackground(STYLES['background']['colour'])

        # WxPython 3 needs the lexer to be set before folding property
        self.SetProperty("fold", "1")

    def insertBlockHead(self):
        pos = self.PositionFromLine(self.GetCurrentLine())
        if self.GetLine(self.GetCurrentLine()).strip() == "":
            self.InsertText(pos, "#-->")
        else:
            self.InsertText(pos, "#-->\n")

    def insertBlockTail(self):
        pos = self.GetLineEndPosition(self.GetCurrentLine())
        if self.GetLine(self.GetCurrentLine()).strip() == "":
            self.InsertText(pos, "#<--")
        else:
            self.InsertText(pos, "\n#<--")

    def selectCodeBlock(self):
        self.OnDoubleClick(None)

    def OnDoubleClick(self, evt):
        """
        Double-click used to select chunk of code between #--> and #<--
        """
        if "#-->" in self.GetLine(self.GetCurrentLine()):
            first = self.GetCurrentLine()
            last = self.GetLineCount()
            self.LineDown()
            while (self.GetCurrentLine() < self.GetLineCount()):
                if "#<--" in self.GetLine(self.GetCurrentLine()):
                    last = self.GetCurrentLine() - 1
                    break
                self.LineDown()
            self.SetSelection(self.GetLineEndPosition(first)+1, 
                              self.GetLineEndPosition(last)+1)
            if evt is not None:
                evt.StopPropagation()
        elif "#<--" in self.GetLine(self.GetCurrentLine()):
            first = 0
            last = self.GetCurrentLine() - 1
            self.LineUp()
            while (self.GetCurrentLine() > 0):
                if "#-->" in self.GetLine(self.GetCurrentLine()):
                    first = self.GetCurrentLine()
                    break
                self.LineUp()
            self.SetSelection(self.GetLineEndPosition(first)+1, 
                              self.GetLineEndPosition(last)+1)
            if evt is not None:
                evt.StopPropagation()
        else:
            if evt is not None:
                evt.Skip()

    def OnQuickSearch(self, str, next=True):
        if self.GetSelection() != (0,0):
            self.SetSelection(self.GetSelectionEnd()-1, self.GetSelectionEnd())
        self.SearchAnchor()
        if next:
            res = self.SearchNext(stc.STC_FIND_MATCHCASE, str)
        else:
            res = self.SearchPrev(stc.STC_FIND_MATCHCASE, str)
        if res == -1:
            if next:
                self.SetCurrentPos(0)
                self.SetAnchor(0)
                self.SearchAnchor()
                res = self.SearchNext(stc.STC_FIND_MATCHCASE, str)
            else:
                pos = self.GetTextLength()
                self.SetCurrentPos(pos)
                self.SetAnchor(pos)
                self.SearchAnchor()
                res = self.SearchPrev(stc.STC_FIND_MATCHCASE, str)
        line = self.GetCurrentLine()
        halfNumLinesOnScreen = self.LinesOnScreen() / 2
        self.ScrollToLine(line - halfNumLinesOnScreen)

    def OnShowFindReplace(self):
        self.data = wx.FindReplaceData()
        dlg = wx.FindReplaceDialog(self, self.data, "Find & Replace", wx.FR_REPLACEDIALOG | wx.FR_NOUPDOWN)
        dlg.Bind(wx.EVT_FIND, self.OnFind)
        dlg.Bind(wx.EVT_FIND_NEXT, self.OnFind)
        dlg.Bind(wx.EVT_FIND_REPLACE, self.OnFind)
        dlg.Bind(wx.EVT_FIND_REPLACE_ALL, self.OnFind)
        dlg.Bind(wx.EVT_FIND_CLOSE, self.OnFindClose)
        dlg.Show(True)

    def OnFind(self, evt):
        map = { wx.wxEVT_COMMAND_FIND : "FIND",
                wx.wxEVT_COMMAND_FIND_NEXT : "FIND_NEXT",
                wx.wxEVT_COMMAND_FIND_REPLACE : "REPLACE",
                wx.wxEVT_COMMAND_FIND_REPLACE_ALL : "REPLACE_ALL" }

        evtType = evt.GetEventType()
        findTxt = evt.GetFindString()
        newTxt = evt.GetReplaceString()
        findStrLen = len(findTxt)
        newStrLen = len(newTxt)
        diffLen = newStrLen - findStrLen

        selection = self.GetSelection()
        if selection[0] == selection[1]:
            selection = (0, self.GetLength())

        if map[evtType] == 'FIND':
            startpos = self.FindText(selection[0], selection[1], findTxt, evt.GetFlags())
            endpos = startpos+len(findTxt)
            self.anchor1 = endpos
            self.anchor2 = selection[1]
            self.SetSelection(startpos, endpos)
        elif map[evtType] == 'FIND_NEXT':
            startpos = self.FindText(self.anchor1, self.anchor2, findTxt, evt.GetFlags())
            endpos = startpos+len(findTxt)
            self.anchor1 = endpos
            self.SetSelection(startpos, endpos)
        elif map[evtType] == 'REPLACE':
            startpos = self.FindText(selection[0], selection[1], findTxt, evt.GetFlags())
            if startpos != -1:
                endpos = startpos+len(findTxt)
                self.SetSelection(startpos, endpos)
                self.ReplaceSelection(newTxt)
                self.anchor1 = startpos + newStrLen + 1
                self.anchor2 += diffLen
        elif map[evtType] == 'REPLACE_ALL':
            self.anchor1 = startpos = selection[0]
            self.anchor2 = selection[1]
            while startpos != -1:
                startpos = self.FindText(self.anchor1, self.anchor2, findTxt, evt.GetFlags())
                if startpos != -1:
                    endpos = startpos+len(findTxt)
                    self.SetSelection(startpos, endpos)
                    self.ReplaceSelection(newTxt)
                    self.anchor1 = startpos + newStrLen + 1
                    self.anchor2 += diffLen
        line = self.GetCurrentLine()
        halfNumLinesOnScreen = self.LinesOnScreen() / 2
        self.ScrollToLine(line - halfNumLinesOnScreen)

    def OnFindClose(self, evt):
        evt.GetDialog().Destroy()

    def showInvisibles(self, x):
        self.SetViewWhiteSpace(x)
        self.SetViewEOL(x)

    def showEdge(self, x):
        if x:
            self.SetEdgeMode(stc.STC_EDGE_LINE)
        else:
            self.SetEdgeMode(stc.STC_EDGE_NONE)
        
    def removeTrailingWhiteSpace(self):
        text = self.GetTextUTF8()
        lines = [line.rstrip() for line in text.splitlines(False)]
        text= "\n".join(lines)
        self.setText(text, False)

    def tabsToSpaces(self):
        text = self.GetTextUTF8()
        text = text.replace("\t", "    ")
        self.setText(text, False)

    ### Save and Close file ###
    def saveMyFile(self, file):
        #with codecs.open(file, "w", encoding="utf-8") as f:
         #   f.write(self.GetTextUTF8())
        self.SaveFile(file)
        self.path = file
        self.saveMark = False
        marker_file = os.path.split(self.path)[1].split(".")[0]
        marker_file += "%04d" % random.randint(0,1000)
        with open(MARKERS_FILE, "r") as f:
            lines = [line.replace("\n", "").split("=") for line in f.readlines()]

        founded = False
        for line in lines:
            if line[1] == self.path:
                marker_file = line[0]
                founded = True
                break

        if self.markers_dict != {}:
            with open(os.path.join(MARKERS_PATH, marker_file), "w") as f:
                f.write("markers = " + str(self.markers_dict))
            if not founded:
                lines.append([marker_file, self.path])
        else:
            if founded:
                os.remove(os.path.join(MARKERS_PATH, marker_file))
                lines.remove(line)

        with open(MARKERS_FILE, "w") as f:
            for line in lines:
                f.write("%s=%s\n" % (line[0], line[1]))

    def close(self):
        if self.GetModify():
            if not self.path: f = "Untitled"
            else: f = self.path
            dlg = wx.MessageDialog(None, 'file ' + f + ' has been modified. Do you want to save?', 
                                   'Warning!', wx.YES | wx.NO | wx.CANCEL)
            but = dlg.ShowModal()
            if but == wx.ID_YES:
                dlg.Destroy()
                if not self.path or "Untitled-" in self.path:
                    dlg2 = wx.FileDialog(None, message="Save file as ...", defaultDir=os.getcwd(), 
                                         defaultFile="", style=wx.SAVE|wx.FD_OVERWRITE_PROMPT)
                    dlg2.SetFilterIndex(0)
                    if dlg2.ShowModal() == wx.ID_OK:
                        path = dlg2.GetPath()
                        self.SaveFile(path)
                        dlg2.Destroy()
                    else:
                        dlg2.Destroy()
                        return 'keep'
                else:
                    self.SaveFile(self.path)
                return 'delete'
            elif but == wx.ID_NO:
                dlg.Destroy()
                return 'delete'
            elif but == wx.ID_CANCEL:
                dlg.Destroy()
                return 'keep'
        else:
            return 'delete'

    def OnClose(self, event):
        if self.GetModify():
            if not self.path: f = "Untitled"
            else: f = os.path.split(self.path)[1]
            dlg = wx.MessageDialog(None, 'file ' + f + ' has been modified. Do you want to save?', 
                                   'Warning!', wx.YES | wx.NO)
            if dlg.ShowModal() == wx.ID_YES:
                dlg.Destroy()
                if not self.path or "Untitled-" in self.path:
                    dlg2 = wx.FileDialog(None, message="Save file as ...", defaultDir=os.getcwd(),
                                         defaultFile="", style=wx.SAVE|wx.FD_OVERWRITE_PROMPT)
                    dlg2.SetFilterIndex(0)

                    if dlg2.ShowModal() == wx.ID_OK:
                        path = dlg2.GetPath()
                        self.SaveFile(path)
                        dlg2.Destroy()
                else:
                    self.SaveFile(self.path)
            else:
                dlg.Destroy()

    def OnModified(self):
        title = self.getTitle()
        if self.GetModify() and not "***" in title:
            str = '*** ' + title + ' ***'
            self.setTitle(str)
            tab = self.panel.GetSelection()
            tabtitle = self.panel.GetPageText(tab)
            self.panel.SetPageText(tab, "*" + tabtitle)
            self.saveMark = True

    ### Text Methods ###
    def addText(self, text, update=True):
        try:
            self.AddTextUTF8(text)
        except:
            self.AddText(text)
        if update:
            count = self.GetLineCount()
            for i in range(count):
                self.updateVariableDict(i)

    def insertText(self, pos, text, update=True):
        try:
            self.InsertTextUTF8(pos, text)
        except:
            self.InsertText(pos, text)
        if update:
            count = self.GetLineCount()
            for i in range(count):
                self.updateVariableDict(i)

    def setText(self, text, update=True):
        try:
            self.SetTextUTF8(text)
        except:
            self.SetText(text)
        if update:
            count = self.GetLineCount()
            for i in range(count):
                self.updateVariableDict(i)

    ### Editor functions ###
    def listPaste(self, pastingList):
        if pastingList != []:
            self.popupmenu = wx.Menu()
            for item in pastingList:
                item = self.popupmenu.Append(-1, item)
                self.Bind(wx.EVT_MENU, self.onPasteFromList, item)
            self.PopupMenu(self.popupmenu, self.PointFromPosition(self.GetCurrentPos()))
            self.popupmenu.Destroy()

    def onPasteFromList(self, evt):
        item = self.popupmenu.FindItemById(evt.GetId())
        text = item.GetText()
        self.insertText(self.GetCurrentPos(), text)
        self.SetCurrentPos(self.GetCurrentPos() + len(text))
        wx.CallAfter(self.SetAnchor, self.GetCurrentPos())

    def getWordUnderCaret(self):
        caretPos = self.GetCurrentPos()
        startpos = self.WordStartPosition(caretPos, True)
        endpos = self.WordEndPosition(caretPos, True)
        currentword = self.GetTextRangeUTF8(startpos, endpos)
        return currentword

    def showAutoCompContainer(self, state):
        self.auto_comp_container = state
        
    def showAutoComp(self):
        propagate = True
        charBefore = " "
        caretPos = self.GetCurrentPos()
        if caretPos > 0:
            charBefore = self.GetTextRangeUTF8(caretPos - 1, caretPos)
        currentword = self.getWordUnderCaret()
        if charBefore in self.alphaStr:
            list = ''
            for word in PYO_WORDLIST:
                if word.startswith(currentword) and word != currentword and word != "class_args":
                    list = list + word + ' '
            if list:
                self.AutoCompShow(len(currentword), list)
                propagate = False
        return propagate

    def insertDefArgs(self, currentword, charat):
        propagate = True
        braceend = True
        currentword = ""
        if charat == ord("("):
            pos = self.GetCurrentPos()
            if chr(self.GetCharAt(pos)) == ')':
                braceend = False
            startpos = self.WordStartPosition(pos-2, True)
            endpos = self.WordEndPosition(pos-2, True)
            currentword = self.GetTextRangeUTF8(startpos, endpos)
        for word in PYO_WORDLIST:
            if word == currentword:
                text = class_args(eval(word)).replace(word, "")
                self.args_buffer = text.replace("(", "").replace(")", "").split(",")
                self.args_buffer = [arg.strip() for arg in self.args_buffer]
                self.args_line_number = [self.GetCurrentLine(), self.GetCurrentLine()+1]
                if braceend:
                    self.insertText(self.GetCurrentPos(), text[1:], False)
                else:
                    self.insertText(self.GetCurrentPos(), text[1:-1], False)
                self.selection = self.GetSelectedText()
                wx.CallAfter(self.navigateArgs)
                propagate = False
                break
        return propagate

    def navigateArgs(self):
        if self.selection != "":
            self.SetCurrentPos(self.GetSelectionEnd())
        arg = self.args_buffer.pop(0)
        if len(self.args_buffer) == 0:
            self.quit_navigate_args = True
        if "=" in arg:
            search = arg.split("=")[1].strip()
        else:
            search = arg
        self.SearchAnchor()
        self.SearchNext(stc.STC_FIND_MATCHCASE, search)

    def quitNavigateArgs(self):
        pos = self.GetLineEndPosition(self.GetCurrentLine()) + 1
        self.SetCurrentPos(pos)
        wx.CallAfter(self.SetAnchor, self.GetCurrentPos())

    def formatBuiltinComp(self, text, indent=0):
        self.snip_buffer = []
        a1 = text.find("`", 0)
        while a1 != -1:
            a2 = text.find("`", a1+1)
            if a2 != -1:
                self.snip_buffer.append(ensureNFD(text[a1+1:a2]))
            a1 = text.find("`", a2+1)
        text = text.replace("`", "")
        lines = text.splitlines(True)
        text = lines[0]
        for i in range(1, len(lines)):
            text += " "*indent + lines[i]
        return text, len(text)

    def checkForBuiltinComp(self):
        text, pos = self.GetCurLine()
        if text.strip() in BUILTINS_DICT.keys():
            indent = self.GetLineIndentation(self.GetCurrentLine())
            text, tlen = self.formatBuiltinComp(BUILTINS_DICT[text.strip()], indent)
            self.args_line_number = [self.GetCurrentLine(), self.GetCurrentLine()+len(text.splitlines())]
            self.insertText(self.GetCurrentPos(), text)
            if len(self.snip_buffer) == 0:
                pos = self.GetCurrentPos() + len(text) + 1
                self.SetCurrentPos(pos)
                wx.CallAfter(self.SetAnchor, self.GetCurrentPos())
            else:
                self.selection = self.GetSelectedText()
                pos = self.GetSelectionStart()
                wx.CallAfter(self.navigateSnips, pos)
            return False
        else:
            return True

    def insertSnippet(self, text):
        indent = self.GetLineIndentation(self.GetCurrentLine())
        text, tlen = self.formatBuiltinComp(text, 0)
        self.args_line_number = [self.GetCurrentLine(), self.GetCurrentLine()+len(text.splitlines())]
        self.insertText(self.GetCurrentPos(), text)
        if len(self.snip_buffer) == 0:
            pos = self.GetCurrentPos() + len(text) + 1
            self.SetCurrentPos(pos)
            wx.CallAfter(self.SetAnchor, self.GetCurrentPos())
        else:
            self.selection = self.GetSelectedTextUTF8()
            pos = self.GetSelectionStart()
            wx.CallAfter(self.navigateSnips, pos)

    def navigateSnips(self, pos):
        if self.selection != "":
            self.SetCurrentPos(self.GetSelectionEnd())
        arg = self.snip_buffer.pop(0)
        if len(self.snip_buffer) == 0:
            self.quit_navigate_snip = True
        self.SearchAnchor()
        self.SearchNext(stc.STC_FIND_MATCHCASE, arg)

    def quitNavigateSnips(self, pos):
        pos = self.PositionFromLine(self.args_line_number[1])
        self.SetCurrentPos(pos)
        wx.CallAfter(self.SetAnchor, self.GetCurrentPos())

    def checkForAttributes(self, charat, pos):
        propagate = True
        currentword = ""
        while charat == ord("."):
            startpos = self.WordStartPosition(pos-2, True)
            endpos = self.WordEndPosition(pos-2, True)
            currentword = "%s.%s" % (self.GetTextRangeUTF8(startpos, endpos), currentword)
            pos = startpos - 1
            charat = self.GetCharAt(pos)
        if currentword != "":
            currentword = currentword[:-1]
            if currentword in self.objs_attr_dict.keys():
                pyokeyword = self.objs_attr_dict[currentword]
                obj = eval(pyokeyword)
                list = [word for word in dir(obj) if not word.startswith("_")]
                for i, word in enumerate(list):
                    if type(getattr(obj, word)) == MethodType:
                        args, varargs, varkw, defaults = inspect.getargspec(getattr(obj, word))
                        args = inspect.formatargspec(args, varargs, varkw, defaults, formatvalue=removeExtraDecimals)
                        args = args.replace('self, ', '').replace('self', '')
                        list[i] = word + args
                list = "/".join(list)
                if list:
                    self.AutoCompSetSeparator(ord("/"))
                    self.AutoCompShow(0, list)
                    self.AutoCompSetSeparator(ord(" "))
                    propagate = False
        return propagate

    def updateVariableDict(self, line):
        text = self.GetLineUTF8(line).replace(" ", "")
        egpos = text.find("=")
        brpos = text.find("(")
        if egpos != -1 and brpos != -1:
            if egpos < brpos:
                name = text[:egpos]
                obj = text[egpos+1:brpos]
                if obj in PYO_WORDLIST:
                    self.objs_attr_dict[name] = obj

    def processReturn(self):
        prevline = self.GetCurrentLine() - 1
        self.updateVariableDict(prevline)
        if self.GetLineUTF8(prevline).strip().endswith(":"):
            indent = self.GetLineIndentation(prevline)
            self.addText(" "*(indent+4), False)
        elif self.GetLineIndentation(prevline) != 0 and self.GetLineUTF8(prevline).strip() != "":
            indent = self.GetLineIndentation(prevline)
            self.addText(" "*indent, False)

    def processTab(self, currentword, autoCompActive, charat, pos):
        propagate = self.showAutoComp()
        if propagate:
            propagate = self.insertDefArgs(currentword, charat)
            if propagate:
                propagate = self.checkForBuiltinComp()
                if propagate:
                    propagate = self.checkForAttributes(charat, pos)
        return propagate

    def onShowTip(self):
        currentword = self.getWordUnderCaret()
        try:
            text = class_args(eval(currentword)).replace(currentword, "")
            self.CallTipShow(self.GetCurrentPos(), text)
        except:
            pass

    def onShowDocString(self):
        if self.GetSelectedText() != "":
            currentword = self.GetSelectedText()
        else:
            currentword = self.getWordUnderCaret()
            firstCaretPos = self.GetCurrentPos()
            caretPos = self.GetCurrentPos()
            startpos = self.WordStartPosition(caretPos, True)
            while chr(self.GetCharAt(startpos-1)) == ".":
                self.GotoPos(startpos-2)
                parent = self.getWordUnderCaret()
                currentword = parent + "." + currentword
                caretPos = self.GetCurrentPos()
                startpos = self.WordStartPosition(caretPos, True)
            self.GotoPos(firstCaretPos)
        lineCount = self.GetLineCount()
        text = ""
        for i in range(lineCount):
            line = self.GetLine(i)
            if "import " in line:
                text = text + line
        try:
            exec text in locals()
            docstr = eval(currentword).__doc__
            dlg = wx.lib.dialogs.ScrolledMessageDialog(self, docstr, "__doc__ string for %s" % currentword, size=(700,500))
            dlg.CenterOnParent()
            dlg.ShowModal()
        except:
            pass

    def OnChar(self, evt):
        propagate = True

        if chr(evt.GetKeyCode()) in ['[', '{', '(', '"', '`'] and self.auto_comp_container:
            if chr(evt.GetKeyCode()) == '[':
                self.AddText('[]')
            elif chr(evt.GetKeyCode()) == '{':
                self.AddText('{}')
            elif chr(evt.GetKeyCode()) == '(':
                self.AddText('()')
            elif chr(evt.GetKeyCode()) == '"':
                self.AddText('""')
            elif chr(evt.GetKeyCode()) == '`':
                self.AddText('``')
            self.CharLeft()
            propagate = False

        if propagate:
            evt.Skip()
        else:
            evt.StopPropagation()
        
    def OnKeyDown(self, evt):
        if PLATFORM == "darwin":
            ControlDown = evt.CmdDown
        else:
            ControlDown = evt.ControlDown
        
        propagate = True
        # Stop propagation on markers navigation --- Shift+Ctrl+Arrows up/down
        if evt.GetKeyCode() in [wx.WXK_DOWN,wx.WXK_UP] and evt.ShiftDown() and ControlDown():
            propagate = False
        # Stop propagation on Tip Show of pyo keyword --- Shift+Return
        elif evt.GetKeyCode() == wx.WXK_RETURN and evt.ShiftDown():
            self.onShowTip()
            propagate = False
        # Stop propagation on Tip Show of __doc__ string --- Ctrl+Return
        elif evt.GetKeyCode() == wx.WXK_RETURN and ControlDown():
            self.onShowDocString()
            propagate = False

        # Move and/or Select one word left or right --- (Shift+)Alt+Arrows left/right
        elif evt.GetKeyCode() == wx.WXK_LEFT and evt.AltDown() and evt.ShiftDown():
            self.CmdKeyExecute(stc.STC_CMD_WORDLEFTEXTEND)
        elif evt.GetKeyCode() == wx.WXK_LEFT and evt.AltDown():
            self.CmdKeyExecute(stc.STC_CMD_WORDLEFT)
        elif evt.GetKeyCode() == wx.WXK_RIGHT and evt.AltDown() and evt.ShiftDown():
            self.CmdKeyExecute(stc.STC_CMD_WORDRIGHTEXTEND)
        elif evt.GetKeyCode() == wx.WXK_RIGHT and evt.AltDown():
            self.CmdKeyExecute(stc.STC_CMD_WORDRIGHT)

        # Move and/or Select one line left or right --- (Shift+)Ctrl+Arrows left/right
        elif evt.GetKeyCode() == wx.WXK_LEFT and ControlDown() and evt.ShiftDown():
            self.CmdKeyExecute(stc.STC_CMD_HOMEDISPLAYEXTEND)
            propagate = False
        elif evt.GetKeyCode() == wx.WXK_LEFT and ControlDown():
            self.CmdKeyExecute(stc.STC_CMD_HOMEDISPLAY)
            propagate = False
        elif evt.GetKeyCode() == wx.WXK_RIGHT and ControlDown() and evt.ShiftDown():
            self.CmdKeyExecute(stc.STC_CMD_LINEENDEXTEND)
            propagate = False
        elif evt.GetKeyCode() == wx.WXK_RIGHT and ControlDown():
            self.CmdKeyExecute(stc.STC_CMD_LINEEND)
            propagate = False

        # Delete forward DELETE
        elif evt.GetKeyCode() == wx.WXK_DELETE:
            self.CmdKeyExecute(stc.STC_CMD_CHARRIGHT)
            self.CmdKeyExecute(stc.STC_CMD_DELETEBACK)
            propagate = False
        # Delete the word to the right of the caret --- Shift+Alt+BACK
        elif evt.GetKeyCode() == wx.WXK_BACK and evt.AltDown() and evt.ShiftDown():
            self.DelWordRight()
            propagate = False
        # Delete the word to the left of the caret --- Alt+BACK
        elif evt.GetKeyCode() == wx.WXK_BACK and evt.AltDown():
            self.DelWordLeft()
            propagate = False

        # Delete the line to the right of the caret --- Shift+Ctrl+BACK
        elif evt.GetKeyCode() == wx.WXK_BACK and ControlDown() and evt.ShiftDown():
            self.DelLineRight()
            propagate = False
        # Delete the line to the left of the caret --- Ctrl+BACK
        elif evt.GetKeyCode() == wx.WXK_BACK and ControlDown():
            self.DelLineLeft()
            propagate = False

        # Line Copy / Duplicate / Cut / Paste --- Alt+'C', Alt+'D', Alt+'C', Alt+'V'
        elif evt.GetKeyCode() in [ord('X'), ord('D'), ord('C'), ord('V')] and evt.AltDown():
            if evt.GetKeyCode() == ord('C'):
                self.LineCopy()
            elif evt.GetKeyCode() == ord('D'):
                self.LineDuplicate()
            elif evt.GetKeyCode() == ord('X'):
                self.LineCut()
            elif evt.GetKeyCode() == ord('V'):
                self.GotoPos(self.PositionFromLine(self.GetCurrentLine()))
                self.Paste()
            propagate = False

        # Show documentation for pyo object under the caret
        elif evt.GetKeyCode() == ord('D') and ControlDown():
            self.GetParent().GetParent().GetParent().GetParent().GetParent().showDoc(None)
            propagate = False
        # Goto line
        elif evt.GetKeyCode() == ord('L') and ControlDown():
            self.GetParent().GetParent().GetParent().GetParent().GetParent().gotoLine(None)
            propagate = False

        # Process Return key --- automatic indentation
        elif evt.GetKeyCode() == wx.WXK_RETURN:
            wx.CallAfter(self.processReturn)
        # Process Tab key --- AutoCompletion, Insert object's args, snippet for builtin keywords
        elif evt.GetKeyCode() == wx.WXK_TAB:
            autoCompActive =  self.AutoCompActive()
            currentword = self.getWordUnderCaret()
            currentline = self.GetCurrentLine()
            charat = self.GetCharAt(self.GetCurrentPos()-1)
            pos = self.GetCurrentPos()
            if len(self.args_buffer) > 0 and currentline in range(*self.args_line_number):
                self.selection = self.GetSelectedText()
                self.navigateArgs()
                propagate = False
            elif self.quit_navigate_args and currentline in range(*self.args_line_number):
                self.quit_navigate_args = False
                self.selection = self.GetSelectedText()
                self.quitNavigateArgs()
                propagate = False
            elif len(self.snip_buffer) > 0 and currentline in range(*self.args_line_number):
                self.selection = self.GetSelectedText()
                pos = self.GetSelectionStart()
                self.navigateSnips(pos)
                propagate = False
            elif self.quit_navigate_snip and currentline in range(*self.args_line_number):
                self.quit_navigate_snip = False
                self.selection = self.GetSelectedText()
                pos = self.GetSelectionStart()
                self.quitNavigateSnips(pos)
                propagate = False
            elif autoCompActive:
                propagate = True
            else:
                propagate = self.processTab(currentword, autoCompActive, charat, pos)

        if propagate:
            evt.Skip()
        else:
            evt.StopPropagation()

    def OnUpdateUI(self, evt):
        # check for matching braces
        braceAtCaret = -1
        braceOpposite = -1
        charBefore = None
        caretPos = self.GetCurrentPos()

        if caretPos > 0:
            charBefore = self.GetCharAt(caretPos - 1)
            styleBefore = self.GetStyleAt(caretPos - 1)

        # check before
        if charBefore and chr(charBefore) in "[]{}()" and styleBefore == stc.STC_P_OPERATOR:
            braceAtCaret = caretPos - 1

        # check after
        if braceAtCaret < 0:
            charAfter = self.GetCharAt(caretPos)
            styleAfter = self.GetStyleAt(caretPos)

            if charAfter and chr(charAfter) in "[]{}()" and styleAfter == stc.STC_P_OPERATOR:
                braceAtCaret = caretPos
        if braceAtCaret >= 0:
            braceOpposite = self.BraceMatch(braceAtCaret)

        if braceAtCaret != -1  and braceOpposite == -1:
            self.BraceBadLight(braceAtCaret)
        else:
            self.BraceHighlight(braceAtCaret, braceOpposite)

        if self.GetCurrentLine() not in range(*self.args_line_number):
            self.args_line_number = [0,0]
            self.args_buffer = []
            self.quit_navigate_args = False

        # if self.endOfLine:
        #     for i in range(self.GetLineCount()):
        #         pos = self.GetLineEndPosition(i)
        #         if self.GetCharAt(pos-1) != 172:
        #             self.InsertTextUTF8(pos, "�")
        self.moveMarkers()
        self.checkScrollbar()
        self.OnModified()
        evt.Skip()

    def checkScrollbar(self):
        lineslength = [self.LineLength(i)+1 for i in range(self.GetLineCount())]
        maxlength = max(lineslength)
        width = self.GetCharWidth() + (self.GetZoom() * 0.5)
        if (self.GetSize()[0]) < (maxlength * width):
            self.SetUseHorizontalScrollBar(True)
        else:
            self.SetUseHorizontalScrollBar(False)
            self.SetXOffset(0)

    def OnComment(self):
        selStartPos, selEndPos = self.GetSelection()
        self.firstLine = self.LineFromPosition(selStartPos)
        self.endLine = self.LineFromPosition(selEndPos)
        for i in range(self.firstLine, self.endLine+1):
            lineLen = len(self.GetLine(i))
            pos = self.PositionFromLine(i)
            if self.GetTextRangeUTF8(pos,pos+1) != '#' and lineLen > 2:
                self.insertText(pos, '#', False)
            elif self.GetTextRangeUTF8(pos,pos+1) == '#':
                self.GotoPos(pos+1)
                self.DelWordLeft()

    def navigateMarkers(self, down=True):
        if self.markers_dict != {}:
            llen = len(self.markers_dict)
            swap = [(x[1], x[0]) for x in self.markers_dict.items()]
            handles = [x[1] for x in sorted(swap)]
            if down:
                self.current_marker += 1
            else:
                self.current_marker -= 1
            if self.current_marker < 0:
                self.current_marker = llen - 1
            elif self.current_marker >= llen:
                self.current_marker = 0
            handle = handles[self.current_marker]
            line = self.markers_dict[handle][0]
            self.GotoLine(line)
            halfNumLinesOnScreen = self.LinesOnScreen() / 2
            self.ScrollToLine(line - halfNumLinesOnScreen)
            self.GetParent().GetParent().GetParent().GetParent().markers.setSelected(handle)

    def setMarkers(self, dic):
        try:
            key = dic.keys()[0]
        except:
            return
        if type(dic[key]) != ListType:
            return
        self.markers_dict = dic
        for handle in self.markers_dict.keys():
            line = self.markers_dict[handle][0]
            self.MarkerAdd(line, 0)
        self.GetParent().GetParent().GetParent().GetParent().markers.setDict(self.markers_dict)

    def moveMarkers(self):
        dict = {}
        for handle in self.markers_dict.keys():
            line = self.MarkerLineFromHandle(handle)
            comment = self.markers_dict[handle][1]
            dict[handle] = [line, comment]
        if dict != self.markers_dict:
            self.markers_dict = dict
            self.GetParent().GetParent().GetParent().GetParent().markers.setDict(self.markers_dict)
            
    def addMarker(self, line):
        if not self.MarkerGet(line):
            handle = self.MarkerAdd(line, 0)
            self.markers_dict[handle] = [line, ""]            
        comment = ""
        dlg = wx.TextEntryDialog(self, 'Enter a comment for that marker:', 'Marker Comment')
        if dlg.ShowModal() == wx.ID_OK:
            comment = dlg.GetValue()
            dlg.Destroy()
        else:
            dlg.Destroy()
            return
        self.markers_dict[handle][1] = comment
        self.GetParent().GetParent().GetParent().GetParent().markers.setDict(self.markers_dict)

    def deleteMarker(self, line):
        for handle in self.markers_dict.keys():
            if line == self.markers_dict[handle][0]:
                del self.markers_dict[handle]
                self.MarkerDeleteHandle(handle)
                self.GetParent().GetParent().GetParent().GetParent().markers.setDict(self.markers_dict)

    def deleteAllMarkers(self):
        self.markers_dict = {}
        self.MarkerDeleteAll(0)
        self.GetParent().GetParent().GetParent().GetParent().markers.setDict(self.markers_dict)

    def OnMarginClick(self, evt):
        if evt.GetMargin() == 0:
            lineClicked = self.LineFromPosition(evt.GetPosition())
            if evt.GetShift():
                self.deleteMarker(lineClicked)
            else:
                self.addMarker(lineClicked)
        elif evt.GetMargin() == 2:
            if evt.GetShift() and evt.GetControl():
                self.ToggleFoldAll()
            else:
                lineClicked = self.LineFromPosition(evt.GetPosition())
                if self.GetFoldLevel(lineClicked) & stc.STC_FOLDLEVELHEADERFLAG:
                    self.ToggleFold(lineClicked)

    def FoldAll(self):
        lineCount = self.GetLineCount()
        lineNum = 0
        while lineNum < lineCount:
            level = self.GetFoldLevel(lineNum)
            if level & stc.STC_FOLDLEVELHEADERFLAG and \
               (level & stc.STC_FOLDLEVELNUMBERMASK) == stc.STC_FOLDLEVELBASE:
                lastChild = self.GetLastChild(lineNum, -1)
                self.SetFoldExpanded(lineNum, False)
                if lastChild > lineNum:
                    self.HideLines(lineNum+1, lastChild)
            lineNum = lineNum + 1

    def ExpandAll(self):
        lineCount = self.GetLineCount()
        lineNum = 0
        while lineNum < lineCount:
            level = self.GetFoldLevel(lineNum)
            if level & stc.STC_FOLDLEVELHEADERFLAG and \
               (level & stc.STC_FOLDLEVELNUMBERMASK) == stc.STC_FOLDLEVELBASE:
                self.SetFoldExpanded(lineNum, True)
                lineNum = self.Expand(lineNum, True)
                lineNum = lineNum - 1
            lineNum = lineNum + 1

    def ToggleFoldAll(self):
        lineCount = self.GetLineCount()
        expanding = True
        # find out if we are folding or unfolding
        for lineNum in range(lineCount):
            if self.GetFoldLevel(lineNum) & stc.STC_FOLDLEVELHEADERFLAG:
                expanding = not self.GetFoldExpanded(lineNum)
                break
        lineNum = 0
        while lineNum < lineCount:
            level = self.GetFoldLevel(lineNum)
            if level & stc.STC_FOLDLEVELHEADERFLAG and \
               (level & stc.STC_FOLDLEVELNUMBERMASK) == stc.STC_FOLDLEVELBASE:
                if expanding:
                    self.SetFoldExpanded(lineNum, True)
                    lineNum = self.Expand(lineNum, True)
                    lineNum = lineNum - 1
                else:
                    lastChild = self.GetLastChild(lineNum, -1)
                    self.SetFoldExpanded(lineNum, False)
                    if lastChild > lineNum:
                        self.HideLines(lineNum+1, lastChild)
            lineNum = lineNum + 1

    def foldExpandCurrentScope(self):
        line = self.GetCurrentLine()
        while (line >= 0):
            level = self.GetFoldLevel(line)
            if level & stc.STC_FOLDLEVELHEADERFLAG and (level & stc.STC_FOLDLEVELNUMBERMASK) == stc.STC_FOLDLEVELBASE:
                self.ToggleFold(line)
                self.GotoLine(line)
                break
            line -= 1

    def Expand(self, line, doExpand, force=False, visLevels=0, level=-1):
        lastChild = self.GetLastChild(line, level)
        line = line + 1

        while line <= lastChild:
            if force:
                if visLevels > 0:
                    self.ShowLines(line, line)
                else:
                    self.HideLines(line, line)
            else:
                if doExpand:
                    self.ShowLines(line, line)

            if level == -1:
                level = self.GetFoldLevel(line)

            if level & stc.STC_FOLDLEVELHEADERFLAG:
                if force:
                    if visLevels > 1:
                        self.SetFoldExpanded(line, True)
                    else:
                        self.SetFoldExpanded(line, False)
                    line = self.Expand(line, doExpand, force, visLevels-1)
                else:
                    if doExpand and self.GetFoldExpanded(line):
                        line = self.Expand(line, True, force, visLevels-1)
                    else:
                        line = self.Expand(line, False, force, visLevels-1)
            else:
                line = line + 1
        return line

class SimpleEditor(stc.StyledTextCtrl):
    def __init__(self, parent, ID=wx.ID_ANY, pos=wx.DefaultPosition, size=wx.DefaultSize, style= wx.NO_BORDER):
        stc.StyledTextCtrl.__init__(self, parent, ID, pos, size, style)

        self.panel = parent

        self.alphaStr = string.lowercase + string.uppercase + '0123456789'

        self.Colourise(0, -1)
        self.SetCurrentPos(0)
        self.SetIndent(4)
        self.SetBackSpaceUnIndents(True)
        self.SetTabIndents(True)
        self.SetTabWidth(4)
        self.SetUseTabs(False)
        self.SetEOLMode(wx.stc.STC_EOL_LF)
        self.SetPasteConvertEndings(True)
        self.SetControlCharSymbol(32)
        self.SetLayoutCache(True)

        self.SetViewWhiteSpace(0)
        self.SetViewEOL(0)
        self.SetEdgeMode(0)
        self.SetWrapMode(stc.STC_WRAP_WORD)

        self.SetUseAntiAliasing(True)
        self.SetEdgeColour(STYLES["lineedge"]['colour'])
        self.SetEdgeColumn(78)
        self.SetReadOnly(True)
        self.setStyle()

    def setStyle(self):
        def buildStyle(forekey, backkey=None, smallsize=False):
            if smallsize:
                st = "face:%s,fore:%s,size:%s" % (STYLES['face'], STYLES[forekey]['colour'], STYLES['size2'])
            else:
                st = "face:%s,fore:%s,size:%s" % (STYLES['face'], STYLES[forekey]['colour'], STYLES['size'])
            if backkey:
                st += ",back:%s" % STYLES[backkey]['colour']
            if STYLES[forekey].has_key('bold'):
                if STYLES[forekey]['bold']:
                    st += ",bold"
                if STYLES[forekey]['italic']:
                    st += ",italic"
                if STYLES[forekey]['underline']:
                    st += ",underline"
            return st
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, buildStyle('default', 'background'))
        self.StyleClearAll()  # Reset all to be like the default

        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, buildStyle('default', 'background'))
        self.StyleSetSpec(stc.STC_STYLE_LINENUMBER, buildStyle('linenumber', 'marginback', True))
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, buildStyle('default') + ",size:5")
        self.SetEdgeColour(STYLES["lineedge"]['colour'])
        self.SetCaretForeground(STYLES['caret']['colour'])
        self.SetSelBackground(1, STYLES['selback']['colour'])
        self.SetFoldMarginColour(True, STYLES['foldmarginback']['colour'])
        self.SetFoldMarginHiColour(True, STYLES['foldmarginback']['colour'])

class OutputLogEditor(SimpleEditor):
    def __init__(self, parent, ID=wx.ID_ANY, pos=wx.DefaultPosition, size=wx.DefaultSize, style= wx.NO_BORDER):
        SimpleEditor.__init__(self, parent=parent, ID=ID, pos=pos, size=size, style=style)

    def appendToLog(self, text):
        self.SetReadOnly(False)
        self.AppendText(text)
        self.GotoLine(self.GetLineCount())
        self.SetReadOnly(True)

    def setLog(self, text):
        self.SetReadOnly(False)
        self.SetText(text)
        self.SetReadOnly(True)

class OutputLogPanel(wx.Panel):
    def __init__(self, parent, mainPanel, size=(175,400)):
        wx.Panel.__init__(self, parent, wx.ID_ANY, size=size, style=wx.SUNKEN_BORDER)
        self.mainPanel = mainPanel

        self.running = 0
        tsize = (30, 30)
        close_panel_bmp = catalog['close_panel_icon.png'].GetBitmap()

        self.sizer = wx.BoxSizer(wx.VERTICAL)

        toolbarbox = wx.BoxSizer(wx.HORIZONTAL)
        self.toolbar = wx.ToolBar(self, -1)
        self.toolbar.SetMargins((5, 0))
        font, psize = self.toolbar.GetFont(), self.toolbar.GetFont().GetPointSize()
        if PLATFORM == "darwin":
            font.SetPointSize(psize-1)
        self.toolbar.SetToolBitmapSize(tsize)
        
        if PLATFORM == "win32":
            self.toolbar.AddSeparator()
        title = wx.StaticText(self.toolbar, -1, " Output panel")
        title.SetFont(font)
        self.toolbar.AddControl(title)
        self.toolbar.AddSeparator()
        if PLATFORM == "win32":
            self.toolbar.AddSeparator()
        self.processPopup = wx.Choice(self.toolbar, -1, choices=[])
        self.processPopup.SetFont(font)
        self.toolbar.AddControl(self.processPopup)
        if PLATFORM == "win32":
            self.toolbar.AddSeparator()
        self.processKill = wx.Button(self.toolbar, -1, label="Kill", size=(40,self.processPopup.GetSize()[1]))
        self.processKill.SetFont(font)        
        self.toolbar.AddControl(self.processKill)
        self.processKill.Bind(wx.EVT_BUTTON, self.killProcess)
        if PLATFORM == "win32":
            self.toolbar.AddSeparator()
        self.runningLabel = wx.StaticText(self.toolbar, -1, " Running: 0")
        self.runningLabel.SetFont(font)
        self.toolbar.AddControl(self.runningLabel)
        self.toolbar.AddSeparator()
        self.copyLog = wx.Button(self.toolbar, -1, label="Copy log", size=(70,self.processPopup.GetSize()[1]))
        self.copyLog.SetFont(font)
        self.toolbar.AddControl(self.copyLog)
        self.copyLog.Bind(wx.EVT_BUTTON, self.onCopy)
        self.toolbar.AddSeparator()
        zoomLabel = wx.StaticText(self.toolbar, -1, "Zoom:")
        zoomLabel.SetFont(font)
        self.toolbar.AddControl(zoomLabel)
        if PLATFORM == "win32":
            self.toolbar.AddSeparator()
        self.zoomer = wx.SpinCtrl(self.toolbar, -1, "0", size=(60, -1))
        self.zoomer.SetRange(-10,10)
        self.toolbar.AddControl(self.zoomer)
        self.zoomer.Bind(wx.EVT_SPINCTRL, self.onZoom)
        self.toolbar.Realize()
        toolbarbox.Add(self.toolbar, 1, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, 0)

        tb2 = wx.ToolBar(self, -1, size=(-1,32))
        if PLATFORM == "darwin":
            tb2.SetToolBitmapSize(tsize)
        tb2.AddSeparator()
        tb2.AddLabelTool(17, "Close Panel", close_panel_bmp, shortHelp="Close Panel")
        tb2.Realize()
        toolbarbox.Add(tb2, 0, wx.ALIGN_RIGHT, 0)

        wx.EVT_TOOL(self, 17, self.onCloseOutputPanel)

        self.sizer.Add(toolbarbox, 0, wx.EXPAND)

        self.editor = OutputLogEditor(self, size=(-1, -1))
        self.sizer.Add(self.editor, 1, wx.EXPAND|wx.ALL, 0)

        self.SetSizer(self.sizer)

    def onZoom(self, evt):
        self.editor.SetZoom(self.zoomer.GetValue())

    def onCopy(self, evt):
        self.editor.SelectAll()
        self.editor.Copy()
        self.editor.SetAnchor(0)

    def addProcess(self, procID, filename):
        self.processPopup.Append("%d :: %s" % (procID, filename))
        self.processPopup.SetStringSelection("%d :: %s" % (procID, filename))
        self.running += 1
        self.runningLabel.SetLabel(" Running: %d" % self.running)
        self.editor.setLog("")

    def removeProcess(self, procID, filename):
        str = "%d :: %s" % (procID, filename)
        del self.mainPanel.mainFrame.processes[procID]
        self.processPopup.Delete(self.processPopup.GetItems().index(str))
        self.running -= 1
        self.runningLabel.SetLabel(" Running: %d" % self.running)

    def killProcess(self, evt):
        str = self.processPopup.GetStringSelection()
        if str != "":
            procID = int(str.split("::")[0].strip())
            thread = self.mainPanel.mainFrame.processes[procID][0]
            thread.kill()
            if procID == 1000:
                self.mainPanel.mainFrame.resetBackgroundServerMenu()

    def appendToLog(self, text):
        self.editor.appendToLog(text)

    def setLog(self, text):
        self.editor.setLog(text)

    def onCloseOutputPanel(self, evt):
        self.mainPanel.mainFrame.showOutputPanel(False)

class PastingListEditorFrame(wx.Frame):
    def __init__(self, parent, pastingList):
        wx.Frame.__init__(self, parent, wx.ID_ANY, title="Pasting List Editor ", size=(700,500))
        self.parent = parent
        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(351, "Close\tCtrl+W")
        self.menuBar.Append(menu1, 'File')
        self.SetMenuBar(self.menuBar)

        self.Bind(wx.EVT_MENU, self.close, id=351)
        self.Bind(wx.EVT_CLOSE, self.close)

        mainSizer = wx.BoxSizer(wx.VERTICAL)
        panel = scrolled.ScrolledPanel(self)
        self.editors = []
        if PLATFORM == "darwin":
            heightSum = 22
        else:
            heightSum = 50
        if pastingList != []:
            for line in pastingList:
                editor = SimpleEditor(panel, style=wx.SUNKEN_BORDER)
                editor.SetReadOnly(False)
                height = editor.TextHeight(0) * len(line.splitlines()) + editor.TextHeight(0) * 2
                heightSum += height
                editor.SetMinSize((-1, height))
                editor.SetMaxSize((-1, height))
                mainSizer.Add(editor, 0, wx.EXPAND|wx.TOP|wx.BOTTOM, 0)
                self.editors.append(editor)
                if not line.endswith("\n"):
                    line = line + "\n"
                try:
                    editor.AddTextUTF8(line)
                except:
                    editor.AddText(line)

        Y = wx.SystemSettings.GetMetric(wx.SYS_SCREEN_Y)
        if heightSum > Y - 100:
            self.SetSize((-1, Y - 100))
        else:
            self.SetSize((-1, heightSum))
        panel.SetSizer(mainSizer)
        panel.SetAutoLayout(1)
        panel.SetupScrolling()

    def close(self, evt):
        pastingList = []
        for editor in self.editors:
            text = editor.GetTextUTF8()
            if text.replace("\n", "").strip() != "":
                pastingList.append(text)
        self.parent.pastingList = pastingList
        self.Destroy()
 
TOOL_ADD_FILE_ID = 10
TOOL_ADD_FOLDER_ID = 11
TOOL_REFRESH_TREE_ID = 12
class ProjectTree(wx.Panel):
    """Project panel"""
    def __init__(self, parent, mainPanel, size):
        wx.Panel.__init__(self, parent, -1, size=size, 
                          style=wx.WANTS_CHARS|wx.SUNKEN_BORDER|wx.EXPAND)
        self.SetMinSize((150, -1))
        self.mainPanel = mainPanel

        self.projectDict = {}
        self.selectedItem = None
        self.edititem = self.editfolder = self.itempath = self.scope = None

        tsize = (24, 24)
        file_add_bmp = catalog['file_add_icon.png'].GetBitmap()
        folder_add_bmp = catalog['folder_add_icon.png'].GetBitmap()
        close_panel_bmp = catalog['close_panel_icon.png'].GetBitmap()
        refresh_tree_bmp = catalog['refresh_tree_icon.png'].GetBitmap()

        self.sizer = wx.BoxSizer(wx.VERTICAL)

        toolbarbox = wx.BoxSizer(wx.HORIZONTAL)
        self.toolbar = wx.ToolBar(self, -1, size=(-1,36))
        self.toolbar.SetToolBitmapSize(tsize)
        self.toolbar.AddLabelTool(TOOL_ADD_FILE_ID, "Add File", 
                                  file_add_bmp, shortHelp="Add File")
        self.toolbar.AddLabelTool(TOOL_ADD_FOLDER_ID, "Add Folder", 
                                  folder_add_bmp, shortHelp="Add Folder")
        self.toolbar.AddLabelTool(TOOL_REFRESH_TREE_ID, "Refresh Tree", 
                                  refresh_tree_bmp, shortHelp="Refresh Tree")
        self.toolbar.EnableTool(TOOL_ADD_FILE_ID, False)
        self.toolbar.Realize()
        toolbarbox.Add(self.toolbar, 1, wx.ALIGN_LEFT | wx.EXPAND, 0)

        tb2 = wx.ToolBar(self, -1, size=(-1,36))
        tb2.SetToolBitmapSize(tsize)
        tb2.AddLabelTool(15, "Close Panel", close_panel_bmp, 
                         shortHelp="Close Panel")
        tb2.Realize()
        toolbarbox.Add(tb2, 0, wx.ALIGN_RIGHT, 0)

        wx.EVT_TOOL(self, TOOL_ADD_FILE_ID, self.onAdd)
        wx.EVT_TOOL(self, TOOL_ADD_FOLDER_ID, self.onAdd)
        wx.EVT_TOOL(self, TOOL_REFRESH_TREE_ID, self.onRefresh)
        wx.EVT_TOOL(self, 15, self.onCloseProjectPanel)

        self.sizer.Add(toolbarbox, 0, wx.EXPAND)

        stls = wx.TR_DEFAULT_STYLE|wx.TR_HIDE_ROOT|wx.SUNKEN_BORDER|wx.EXPAND
        self.tree = wx.TreeCtrl(self, -1, (0, 26), size, stls)
        self.tree.SetBackgroundColour(STYLES['background']['colour'])

        if PLATFORM == 'darwin':
            pt = 11
        else:
            pt = 8
        fnt = wx.Font(pt, wx.ROMAN, wx.NORMAL, wx.NORMAL, face=STYLES['face'])
        self.tree.SetFont(fnt)

        self.sizer.Add(self.tree, 1, wx.EXPAND)
        self.SetSizer(self.sizer)

        isz = (12,12)
        self.il = wx.ImageList(isz[0], isz[1])
        bmp = wx.ArtProvider_GetBitmap(wx.ART_FOLDER, wx.ART_OTHER, isz)
        self.fldridx = self.il.Add(bmp)
        bmp = wx.ArtProvider_GetBitmap(wx.ART_FILE_OPEN, wx.ART_OTHER, isz)
        self.fldropenidx = self.il.Add(bmp)
        bmp = wx.ArtProvider_GetBitmap(wx.ART_NORMAL_FILE, wx.ART_OTHER, isz)
        self.fileidx = self.il.Add(bmp)

        self.tree.SetImageList(self.il)
        self.tree.SetSpacing(12)
        self.tree.SetIndent(6)

        self.root = self.tree.AddRoot("EPyo_Project_tree", self.fldridx, 
                                      self.fldropenidx, None)
        self.tree.SetItemTextColour(self.root, STYLES['default']['colour'])

        self.tree.Bind(wx.EVT_TREE_END_LABEL_EDIT, self.OnEndEdit)
        self.tree.Bind(wx.EVT_RIGHT_DOWN, self.OnRightDown)
        self.tree.Bind(wx.EVT_LEFT_DOWN, self.OnLeftClick)
        self.tree.Bind(wx.EVT_LEFT_DCLICK, self.OnLeftDClick)

    def _tree_analyze(self, item, expanded_list):
        if self.tree.ItemHasChildren(item):
            it, cookie = self.tree.GetFirstChild(item)
            while it.IsOk():
                if self.tree.ItemHasChildren(it) and self.tree.IsExpanded(it):
                    parent = self.tree.GetItemText(self.tree.GetItemParent(it))
                    expanded_list.append((parent, self.tree.GetItemText(it)))
                    self._tree_analyze(it, expanded_list)
                it, cookie = self.tree.GetNextChild(item, cookie)

    def _tree_restore(self, item, expanded_list):
        if self.tree.ItemHasChildren(item):
            it, cookie = self.tree.GetFirstChild(item)
            while it.IsOk():
                if self.tree.ItemHasChildren(it):
                    parent = self.tree.GetItemText(self.tree.GetItemParent(it))
                    if (parent, self.tree.GetItemText(it)) in expanded_list:
                        self.tree.Expand(it)
                    self._tree_restore(it, expanded_list)
                it, cookie = self.tree.GetNextChild(item, cookie)

    def onRefresh(self, evt):
        expanded = []
        self._tree_analyze(self.root, expanded)
        self.tree.DeleteAllItems()
        self.root = self.tree.AddRoot("EPyo_Project_tree", self.fldridx, 
                                      self.fldropenidx, None)
        for folder, path in self.projectDict.items():
            self.loadFolder(path)
        self._tree_restore(self.root, expanded)

    def loadFolder(self, dirPath):
        folderName = os.path.split(dirPath)[1]
        self.projectDict[folderName] = dirPath
        self.mainPanel.mainFrame.showProjectTree(True)
        item = self.tree.AppendItem(self.root, folderName, self.fldridx, 
                                    self.fldropenidx, None)
        self.tree.SetPyData(item, dirPath)
        self.tree.SetItemTextColour(item, STYLES['default']['colour'])
        self.buildRecursiveTree(dirPath, item)

    def buildRecursiveTree(self, dir, item):
        elems = [f for f in os.listdir(dir) if f[0] != "."]
        for elem in sorted(elems):
            child = None
            path = os.path.join(dir, elem)
            if os.path.isfile(path):
                if not path.endswith("~") and \
                       os.path.splitext(path)[1].strip(".") in ALLOWED_EXT:
                    child = self.tree.AppendItem(item, elem, self.fileidx, 
                                                 self.fileidx)
                    self.tree.SetPyData(child, os.path.join(dir, path))
            elif os.path.isdir(path):
                if elem != "build":
                    child = self.tree.AppendItem(item, elem, self.fldridx, 
                                                 self.fldropenidx)
                    self.tree.SetPyData(child, os.path.join(dir, path))
                    self.buildRecursiveTree(path, child)
            if child is not None:
                self.tree.SetItemTextColour(child, STYLES['default']['colour'])

    def onAdd(self, evt):
        id = evt.GetId()
        treeItemId = self.tree.GetSelection()
        if self.selectedItem != None:
            selPath = self.tree.GetPyData(self.selectedItem)
            if os.path.isdir(selPath):
                self.scope = selPath
            elif os.path.isfile(selPath):
                treeItemId = self.tree.GetItemParent(treeItemId)
                self.scope = self.tree.GetPyData(treeItemId)
        elif self.selectedItem == None and id == TOOL_ADD_FOLDER_ID:
            dlg = wx.DirDialog(self, 
                               "Choose directory where to save your folder:",
                               defaultPath=os.path.expanduser("~"), 
                               style=wx.DD_DEFAULT_STYLE)
            if dlg.ShowModal() == wx.ID_OK:
                self.scope = dlg.GetPath()
                dlg.Destroy()
            else:
                dlg.Destroy()
                return
            treeItemId = self.tree.GetRootItem()
        if id == TOOL_ADD_FILE_ID:
            item = self.tree.AppendItem(treeItemId, "Untitled", self.fileidx, 
                                        self.fileidx, None)
            self.edititem = item
        else:
            item = self.tree.AppendItem(treeItemId, "Untitled", self.fldridx, 
                                        self.fldropenidx, None)
            self.editfolder = item
        self.tree.SetPyData(item, os.path.join(self.scope, "Untitled"))
        self.tree.SetItemTextColour(item, STYLES['default']['colour'])
        self.tree.EnsureVisible(item)
        if PLATFORM == "darwin":
            self.tree.ScrollTo(item)
            self.tree.EditLabel(item)
            txtctrl = self.tree.GetEditControl()
            txtctrl.SetSize((self.GetSize()[0], 22))
            txtctrl.SelectAll()
        else:
            self.tree.EditLabel(item)

    def setStyle(self):
        def set_item_style(root_item, colour):
            self.tree.SetItemTextColour(root_item, colour)
            item, cookie = self.tree.GetFirstChild(root_item)
            while item.IsOk():
                self.tree.SetItemTextColour(item, colour)
                if self.tree.ItemHasChildren(item):
                    set_item_style(item, colour)
                item, cookie = self.tree.GetNextChild(root_item, cookie)

        if not self.tree.IsEmpty():
            self.tree.SetBackgroundColour(STYLES['background']['colour'])
            set_item_style(self.tree.GetRootItem(), STYLES['default']['colour'])

    def OnRightDown(self, event):
        pt = event.GetPosition();
        self.edititem, flags = self.tree.HitTest(pt)
        if self.edititem:
            self.itempath = self.tree.GetPyData(self.edititem)
            self.select(self.edititem)
            self.tree.EditLabel(self.edititem)
        else:
            self.unselect()

    def OnEndEdit(self, event):
        if self.edititem and self.itempath:
            self.select(self.edititem)
            head, tail = os.path.split(self.itempath)
            newlabel = event.GetLabel()
            if newlabel != "":
                newpath = os.path.join(head, event.GetLabel())
                os.rename(self.itempath, newpath)
                self.tree.SetPyData(self.edititem, newpath)
        elif self.edititem and self.scope:
            newitem = event.GetLabel()
            if not newitem:
                newitem = "Untitled"
                wx.CallAfter(self.tree.SetItemText, self.edititem, newitem)
            newpath = os.path.join(self.scope, newitem)
            self.tree.SetPyData(self.edititem, newpath)
            f = open(newpath, "w")
            f.close()
            self.mainPanel.addPage(newpath)
        elif self.editfolder and self.scope:
            newitem = event.GetLabel()
            if not newitem:
                newitem = "Untitled"
                wx.CallAfter(self.tree.SetItemText, self.editfolder, newitem)
            newpath = os.path.join(self.scope, newitem)
            self.tree.SetPyData(self.editfolder, newpath)
            os.mkdir(newpath)
            if self.selectedItem == None:
                self.projectDict[newitem] = self.scope
        self.edititem = self.editfolder = self.itempath = self.scope = None

    def OnLeftClick(self, event):
        pt = event.GetPosition()
        item, flags = self.tree.HitTest(pt)
        if item:
            self.select(item)
        else:
            self.unselect()
        event.Skip()

    def OnLeftDClick(self, event):
        pt = event.GetPosition()
        item, flags = self.tree.HitTest(pt)
        if item:
            self.select(item)
            self.openPage(item)
        else:
            self.unselect()
        event.Skip()

    def openPage(self, item):
        hasChild = self.tree.ItemHasChildren(item)
        if not hasChild:
            path = self.tree.GetPyData(item)
            self.mainPanel.addPage(path)

    def select(self, item):
        self.tree.SelectItem(item)
        self.selectedItem = item
        self.toolbar.EnableTool(TOOL_ADD_FILE_ID, True)

    def unselect(self):
        self.tree.UnselectAll()
        self.selectedItem = None
        self.toolbar.EnableTool(TOOL_ADD_FILE_ID, False)

    def onCloseProjectPanel(self, evt):
        self.mainPanel.mainFrame.showProjectTree(False)

class MarkersListScroll(scrolled.ScrolledPanel):
    def __init__(self, parent, id=-1, pos=(25,25), size=(500,400)):
        scrolled.ScrolledPanel.__init__(self, parent, wx.ID_ANY, pos=(0,0), 
                                        size=size, style=wx.SUNKEN_BORDER)
        self.parent = parent
        self.SetBackgroundColour(STYLES['background']['colour'])
        self.arrow_bit = catalog['left_arrow.png'].GetBitmap()
        self.row_dict = {}

        self.box = wx.FlexGridSizer(0, 3, 0, 10)

        self.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.selected = None
        self.selected2 = None

        if wx.Platform == '__WXMAC__':
            self.font = wx.Font(11, wx.ROMAN, wx.NORMAL, wx.NORMAL, face=STYLES['face'])
        else:
            self.font = wx.Font(8, wx.ROMAN, wx.NORMAL, wx.NORMAL, face=STYLES['face'])

        self.SetSizer(self.box)
        self.SetAutoLayout(1)
        self.SetupScrolling()

    def setDict(self, dic):
        self.row_dict = dic
        self.box.Clear(True)
        swap = [(x[1], x[0]) for x in self.row_dict.items()]
        handles = [x[1] for x in sorted(swap)]
        for i in handles:
            label = wx.StaticBitmap(self, wx.ID_ANY)
            label.SetBitmap(self.arrow_bit)
            self.box.Add(label, 0, wx.LEFT|wx.ALIGN_CENTER_VERTICAL, 2, userData=(i,self.row_dict[i][0]))
            line = wx.StaticText(self, wx.ID_ANY, label=str(self.row_dict[i][0]+1))
            line.SetFont(self.font)
            self.box.Add(line, 0, wx.ALIGN_LEFT|wx.TOP, 3, userData=(i,self.row_dict[i][0]))
            comment = wx.StaticText(self, wx.ID_ANY, label=self.row_dict[i][1])
            comment.SetFont(self.font)
            self.box.Add(comment, 1, wx.EXPAND|wx.ALIGN_LEFT|wx.TOP, 3, userData=(i,self.row_dict[i][0]))
            self.box.Layout()
            label.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
            line.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
            comment.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.setStyle()

    def OnLeftDown(self, evt):
        evtobj = evt.GetEventObject()
        editor = self.parent.mainPanel.editor
        if not evt.ShiftDown():
            self.selected = None
            self.selected2 = None
        for item in self.box.GetChildren():
            obj = item.GetWindow()
            if obj == evtobj:
                if not evt.ShiftDown() or self.selected == None:
                    self.selected = item.GetUserData()[0]
                    line = item.GetUserData()[1]
                    editor.GotoLine(line)
                    halfNumLinesOnScreen = editor.LinesOnScreen() / 2
                    editor.ScrollToLine(line - halfNumLinesOnScreen)
                else:
                    line = self.row_dict[self.selected][0]
                    self.selected2 = item.GetUserData()[0]
                    line2 = item.GetUserData()[1]
                    l1, l2 = min(line, line2), max(line, line2)
                    editor.GotoLine(l1)
                    halfNumLinesOnScreen = editor.LinesOnScreen() / 2
                    editor.ScrollToLine(l1 - halfNumLinesOnScreen)
                    editor.SetSelection(editor.PositionFromLine(l1), editor.PositionFromLine(l2+1))
                break
        self.setColour()

    def setColour(self):
        for item in self.box.GetChildren():
            obj = item.GetWindow()
            data = item.GetUserData()[0]
            if self.selected == data or self.selected2 == data:
                obj.SetForegroundColour(STYLES['comment']['colour'])
            else:
                obj.SetForegroundColour(STYLES['default']['colour'])
        self.Refresh()

    def setStyle(self):
        self.SetBackgroundColour(STYLES['background']['colour'])
        self.setColour()

    def setSelected(self, mark):
        self.selected = mark
        self.selected2 = None
        self.setColour()

TOOL_DELETE_ALL_MARKERS_ID = 12
class MarkersPanel(wx.Panel):
    def __init__(self, parent, mainPanel, size=(175,400)):
        wx.Panel.__init__(self, parent, wx.ID_ANY, size=size, style=wx.SUNKEN_BORDER)
        self.mainPanel = mainPanel

        tsize = (24, 24)
        delete_all_markers = catalog['delete_all_markers.png'].GetBitmap()
        close_panel_bmp = catalog['close_panel_icon.png'].GetBitmap()

        self.sizer = wx.BoxSizer(wx.VERTICAL)

        toolbarbox = wx.BoxSizer(wx.HORIZONTAL)
        self.toolbar = wx.ToolBar(self, -1, size=(-1,36))
        self.toolbar.SetToolBitmapSize(tsize)
        self.toolbar.AddLabelTool(TOOL_DELETE_ALL_MARKERS_ID, "Delete All Markers", delete_all_markers, shortHelp="Delete All Markers")
        self.toolbar.Realize()
        toolbarbox.Add(self.toolbar, 1, wx.ALIGN_LEFT | wx.EXPAND, 0)

        tb2 = wx.ToolBar(self, -1, size=(-1,36))
        tb2.SetToolBitmapSize(tsize)
        tb2.AddLabelTool(16, "Close Panel", close_panel_bmp, shortHelp="Close Panel")
        tb2.Realize()
        toolbarbox.Add(tb2, 0, wx.ALIGN_RIGHT, 0)

        wx.EVT_TOOL(self, TOOL_DELETE_ALL_MARKERS_ID, self.onDeleteAll)
        wx.EVT_TOOL(self, 16, self.onCloseMarkersPanel)

        self.sizer.Add(toolbarbox, 0, wx.EXPAND)

        self.scroll = MarkersListScroll(self, size=(-1, -1))
        self.sizer.Add(self.scroll, 1, wx.EXPAND|wx.LEFT|wx.RIGHT|wx.BOTTOM, 0)

        self.SetSizer(self.sizer)

    def setSelected(self, mark):
        self.scroll.setSelected(mark)

    def setDict(self, dic):
        self.row_dict = copy.deepcopy(dic)
        self.scroll.setDict(dic)

    def onDeleteAll(self, evt):
        self.mainPanel.mainFrame.deleteAllMarkers(evt)

    def onCloseMarkersPanel(self, evt):
        self.mainPanel.mainFrame.showMarkersPanel(False)

class PreferencesDialog(wx.Dialog):
    def __init__(self):
        wx.Dialog.__init__(self, None, wx.ID_ANY, 'E-Pyo Preferences')
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.AddSpacer((-1,10))
        font, entryfont, pointsize = self.GetFont(), self.GetFont(), self.GetFont().GetPointSize()
        
        font.SetWeight(wx.BOLD)
        if PLATFORM == "linux2":
            entryfont.SetPointSize(pointsize)
        elif PLATFORM == "win32":
            entryfont.SetPointSize(pointsize)
        else:
            font.SetPointSize(pointsize-1)
            entryfont.SetPointSize(pointsize-2)

        lbl = wx.StaticText(self, label="Python Executable:")
        lbl.SetFont(font)
        mainSizer.Add(lbl, 0, wx.LEFT|wx.RIGHT, 10)
        ctrlSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.entry_exe = wx.TextCtrl(self, size=(500,-1), value=WHICH_PYTHON)
        self.entry_exe.SetFont(entryfont)
        if PLATFORM == "win32":
            self.entry_exe.SetEditable(False)
        ctrlSizer.Add(self.entry_exe, 0, wx.ALL|wx.EXPAND, 5)
        but = wx.Button(self, id=wx.ID_ANY, label="Choose...")
        but.Bind(wx.EVT_BUTTON, self.setExecutable)
        ctrlSizer.Add(but, 0, wx.ALL, 5)            
        but2 = wx.Button(self, id=wx.ID_ANY, label="Revert")
        but2.Bind(wx.EVT_BUTTON, self.revertExecutable)
        ctrlSizer.Add(but2, 0, wx.ALL, 5)
        if PLATFORM == "win32":
            but.Disable()
            but2.Disable()
        mainSizer.Add(ctrlSizer, 0, wx.BOTTOM|wx.LEFT|wx.RIGHT, 5)

        lbl = wx.StaticText(self, label="Resources Folder:")
        lbl.SetFont(font)
        mainSizer.Add(lbl, 0, wx.LEFT|wx.RIGHT, 10)
        ctrlSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.entry_res = wx.TextCtrl(self, size=(500,-1), value=RESOURCES_PATH)
        self.entry_res.SetFont(entryfont)
        ctrlSizer.Add(self.entry_res, 0, wx.ALL|wx.EXPAND, 5)
        but = wx.Button(self, id=wx.ID_ANY, label="Choose...")
        but.Bind(wx.EVT_BUTTON, self.setResourcesFolder)
        ctrlSizer.Add(but, 0, wx.ALL, 5)            
        but2 = wx.Button(self, id=wx.ID_ANY, label="Revert")
        but2.Bind(wx.EVT_BUTTON, self.revertResourcesFolder)
        ctrlSizer.Add(but2, 0, wx.ALL, 5)            
        mainSizer.Add(ctrlSizer, 0, wx.BOTTOM|wx.LEFT|wx.RIGHT, 5)

        mainSizer.Add(wx.StaticLine(self, -1), 0, wx.EXPAND|wx.BOTTOM|wx.LEFT|wx.RIGHT, 5)

        lbl = wx.StaticText(self, label="=== Background Pyo Server ===")
        lbl.SetFont(font)
        mainSizer.Add(lbl, 0, wx.BOTTOM|wx.LEFT|wx.RIGHT, 10)

        lbl = wx.StaticText(self, label="Arguments:")
        lbl.SetFont(font)
        mainSizer.Add(lbl, 0, wx.LEFT|wx.RIGHT, 10)
        ctrlSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.server_args = wx.TextCtrl(self, size=(500,-1), value=BACKGROUND_SERVER_ARGS)
        self.server_args.SetFont(entryfont)
        ctrlSizer.Add(self.server_args, 0, wx.ALL|wx.EXPAND, 5)
        but = wx.Button(self, id=wx.ID_ANY, label=" Restore default args ")
        but.Bind(wx.EVT_BUTTON, self.setServerDefaultArgs)
        ctrlSizer.Add(but, 0, wx.ALL, 5)            
        mainSizer.Add(ctrlSizer, 0, wx.BOTTOM|wx.LEFT|wx.RIGHT, 5)

        popupSizer = wx.FlexGridSizer(2, 4, 5, 10)
        for label in ["Input Driver", "Output Driver", "Midi Input", "Midi Output"]:
            lbl = wx.StaticText(self, label=label)
            lbl.SetFont(font)
            popupSizer.Add(lbl)

        cX = 160
        preferedDriver = PREFERENCES.get("background_server_in_device", "")
        driverList, driverIndexes = pa_get_input_devices()
        driverList = [ensureNFD(driver) for driver in driverList]
        defaultDriver = pa_get_default_input()
        self.popupInDriver = wx.Choice(self, choices=driverList, size=(cX,-1))
        popupSizer.Add(self.popupInDriver, 1, wx.EXPAND, 5)
        if preferedDriver and preferedDriver in driverList:
            driverIndex = driverIndexes[driverList.index(preferedDriver)]
            self.popupInDriver.SetStringSelection(preferedDriver)
        elif defaultDriver:
            self.popupInDriver.SetSelection(driverIndexes.index(defaultDriver))

        preferedDriver = PREFERENCES.get("background_server_out_device", "")
        driverList, driverIndexes = pa_get_output_devices()
        driverList = [ensureNFD(driver) for driver in driverList]
        defaultDriver = pa_get_default_output()
        self.popupOutDriver = wx.Choice(self, choices=driverList, size=(cX,-1))
        popupSizer.Add(self.popupOutDriver, 1, wx.EXPAND, 5)
        if preferedDriver and preferedDriver in driverList:
            driverIndex = driverIndexes[driverList.index(preferedDriver)]
            self.popupOutDriver.SetStringSelection(preferedDriver)
        elif defaultDriver:
            self.popupOutDriver.SetSelection(driverIndexes.index(defaultDriver))

        # TODO: Added "all" interfaces option in input and output
        preferedDriver = PREFERENCES.get("background_server_midiin_device", "")
        driverList, driverIndexes = pm_get_input_devices()
        driverList = [ensureNFD(driver) for driver in driverList]
        if driverList != []:
            defaultDriver = pm_get_default_input()
            self.popupMidiInDriver = wx.Choice(self, choices=driverList, size=(cX,-1))
            if preferedDriver and preferedDriver in driverList:
                driverIndex = driverIndexes[driverList.index(preferedDriver)]
                self.popupMidiInDriver.SetStringSelection(preferedDriver)
            elif defaultDriver >= 0:
                self.popupMidiInDriver.SetSelection(driverIndexes.index(defaultDriver))
        else:
            self.popupMidiInDriver = wx.Choice(self, choices=["No Interface"])
            self.popupMidiInDriver.SetSelection(0)
        popupSizer.Add(self.popupMidiInDriver, 1, wx.EXPAND, 5)

        preferedDriver = PREFERENCES.get("background_server_midiout_device", "")
        driverList, driverIndexes = pm_get_output_devices()
        driverList = [ensureNFD(driver) for driver in driverList]
        if driverList != []:
            defaultDriver = pm_get_default_output()
            self.popupMidiOutDriver = wx.Choice(self, choices=driverList, size=(cX,-1))
            if preferedDriver and preferedDriver in driverList:
                driverIndex = driverIndexes[driverList.index(preferedDriver)]
                self.popupMidiOutDriver.SetStringSelection(preferedDriver)
            elif defaultDriver >= 0:
                self.popupMidiOutDriver.SetSelection(driverIndexes.index(defaultDriver))
        else:
            self.popupMidiOutDriver = wx.Choice(self, choices=["No Interface"])
            self.popupMidiOutDriver.SetSelection(0)
        popupSizer.Add(self.popupMidiOutDriver, 1, wx.EXPAND, 5)

        mainSizer.Add(popupSizer, 0, wx.EXPAND|wx.BOTTOM|wx.LEFT|wx.RIGHT, 10)

        mainSizer.Add(wx.StaticLine(self, -1), 0, wx.EXPAND|wx.BOTTOM|wx.LEFT|wx.RIGHT, 5)

        lbl = wx.StaticText(self, label="Allowed File Types in Folder Panel (file extension):")
        lbl.SetFont(font)
        mainSizer.Add(lbl, 0, wx.LEFT|wx.RIGHT, 10)
        ctrlSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.entry_ext = wx.TextCtrl(self, size=(500,-1), value=", ".join(ALLOWED_EXT))
        self.entry_ext.SetFont(entryfont)
        ctrlSizer.Add(self.entry_ext, 1, wx.ALL|wx.EXPAND, 5)
        mainSizer.Add(ctrlSizer, 0, wx.EXPAND|wx.BOTTOM|wx.LEFT|wx.RIGHT, 5)

        btnSizer = self.CreateButtonSizer(wx.CANCEL|wx.OK)
 
        mainSizer.AddSpacer((-1,5))
        mainSizer.Add(wx.StaticLine(self), 1, wx.EXPAND|wx.ALL, 2)
        mainSizer.Add(btnSizer, 0, wx.ALL | wx.ALIGN_RIGHT, 5)
        self.SetSizer(mainSizer)
        self.SetClientSize(self.GetBestSize())

    def setExecutable(self, evt):
        dlg = wx.FileDialog(self, "Choose your python executable", os.path.expanduser("~"), style=wx.FD_OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.entry_exe.SetValue(path)
        dlg.Destroy()

    def setResourcesFolder(self, evt):
        dlg = wx.DirDialog(self, "Choose your resources folder", os.path.expanduser("~"), style=wx.FD_OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.entry_res.SetValue(path)
        dlg.Destroy()

    def revertExecutable(self, evt):
        self.entry_exe.SetValue(WHICH_PYTHON)

    def revertResourcesFolder(self, evt):
        self.entry_res.SetValue(RESOURCES_PATH)

    def setServerDefaultArgs(self, evt):
        self.server_args.SetValue(BACKGROUND_SERVER_DEFAULT_ARGS)
    
    def writePrefs(self):
        global ALLOWED_EXT, WHICH_PYTHON, RESOURCES_PATH, SNIPPETS_PATH, STYLES_PATH, BACKGROUND_SERVER_ARGS

        which_python = self.entry_exe.GetValue()
        if os.path.isfile(which_python) and "python" in which_python:
            WHICH_PYTHON = PREFERENCES["which_python"] = which_python

        res_folder = self.entry_res.GetValue()
        if os.path.isdir(res_folder):
            if res_folder != ensureNFD(RESOURCES_PATH):
                RESOURCES_PATH = PREFERENCES["resources_path"] = res_folder
                # snippets
                old_snippets_path = SNIPPETS_PATH
                SNIPPETS_PATH = os.path.join(RESOURCES_PATH, 'snippets')
                if not os.path.isdir(SNIPPETS_PATH):
                    os.mkdir(SNIPPETS_PATH)
                for rep in SNIPPETS_CATEGORIES:
                    if not os.path.isdir(os.path.join(SNIPPETS_PATH, rep)):
                        os.mkdir(os.path.join(SNIPPETS_PATH, rep))
                    files = [f for f in os.listdir(os.path.join(old_snippets_path, rep)) if f[0] != "."]
                    for file in files:
                        try:
                            shutil.copy(os.path.join(old_snippets_path, rep, file), os.path.join(SNIPPETS_PATH, rep))
                        except:
                            pass
                # styles
                old_styles_path = STYLES_PATH
                STYLES_PATH = os.path.join(RESOURCES_PATH, 'styles')
                if not os.path.isdir(STYLES_PATH):
                    os.mkdir(STYLES_PATH)
                files = [f for f in os.listdir(old_styles_path) if f[0] != "."]
                for file in files:
                    try:
                        shutil.copy(os.path.join(old_styles_path, file), STYLES_PATH)
                    except:
                        pass

        extensions = [ext.strip() for ext in self.entry_ext.GetValue().split(",")]
        ALLOWED_EXT = PREFERENCES["allowed_ext"] = extensions
        
        server_args = self.server_args.GetValue()
        BACKGROUND_SERVER_ARGS = PREFERENCES["background_server_args"] = server_args
        
        PREFERENCES["background_server_out_device"] = self.popupOutDriver.GetStringSelection()
        PREFERENCES["background_server_in_device"] = self.popupInDriver.GetStringSelection()
        midiDevice = self.popupMidiInDriver.GetStringSelection()
        if midiDevice != "No Interface":
            PREFERENCES["background_server_midiin_device"] = midiDevice
        midiDevice = self.popupMidiOutDriver.GetStringSelection()
        if midiDevice != "No Interface":
            PREFERENCES["background_server_midiout_device"] = midiDevice

class STCPrintout(wx.Printout):
    """Specific printing support of the wx.StyledTextCtrl for the wxPython
    framework

    This class can be used for both printing to a printer and for print preview
    functions.  Unless otherwise specified, the print is scaled based on the
    size of the current font used in the STC so that specifying a larger font
    produces a larger font in the printed output (and correspondingly fewer
    lines per page).  Alternatively, you can eihdec specify the number of
    lines per page, or you can specify the print font size in points which
    produces a constant number of lines per inch regardless of the paper size.

    Note that line wrapping in the source STC is currently ignored and lines
    will be truncated at the right margin instead of wrapping.  The STC doesn't
    provide a convenient method for determining where line breaks occur within
    a wrapped line, so it may be a difficult task to ever implement printing
    with line wrapping using the wx.StyledTextCtrl.FormatRange method.
    """
    debuglevel = 1

    def __init__(self, stc, page_setup_data=None, print_mode=None, title=None, 
                 border=False, lines_per_page=None, output_point_size=None):
        """Constructor.

        @param stc: wx.StyledTextCtrl to print

        @kwarg page_setup_data: optional wx.PageSetupDialogData instance that
        is used to determine the margins of the page.

        @kwarg print_mode: optional; of the wx.stc.STC_PRINT_*
        flags indicating how to render color text.  Defaults to
        wx.stc.STC_PRINT_COLOURONWHITEDEFAULTBG

        @kwarg title: optional text string to use as the title which will be
        centered above the first line of text on each page

        @kwarg border: optional flag indicating whether or not to draw a black
        border around the text on each page

        @kwarg lines_per_page: optional integer that will force the page to
        contain the specified number of lines.  Either of C{output_point_size}
        and C{lines_per_page} fully specifies the page, so if both are
        specified, C{lines_per_page} will be used.

        @kwarg output_point_size: optional integer that will force the output
        text to be drawn in the specified point size.  (Note that there are
        72 points per inch.) If not specified, the point size of the text in
        the STC will be used unless C{lines_per_page} is specified.  Either of
        C{output_point_size} and C{lines_per_page} fully specifies the page,
        so if both are specified, C{lines_per_page} will be used.
        """
        wx.Printout.__init__(self)
        self.stc = stc
        if print_mode:
            self.print_mode = print_mode
        else:
            self.print_mode = wx.stc.STC_PRINT_COLOURONWHITEDEFAULTBG
        if title is not None:
            self.title = title
        else:
            self.title = ""
        if page_setup_data is None:
            self.top_left_margin = wx.Point(15,15)
            self.bottom_right_margin = wx.Point(15,15)
        else:
            self.top_left_margin = page_setup_data.GetMarginTopLeft()
            self.bottom_right_margin = page_setup_data.GetMarginBottomRight()

        try:
            value = float(output_point_size)
            if value > 0.0:
                self.output_point_size = value
        except (TypeError, ValueError):
            self.output_point_size = None

        try:
            value = int(lines_per_page)
            if value > 0:
                self.user_lines_per_page = value
        except (TypeError, ValueError):
            self.user_lines_per_page = None

        self.page_count = 2
        self.border_around_text = border

        self.setHeaderFont()

    def OnPreparePrinting(self):
        """Called once before a print job is started to set up any defaults.

        """
        dc = self.GetDC()
        self._calculateScale(dc)
        self._calculatePageCount()

    def _calculateScale(self, dc):
        """Scale the DC

        This routine scales the DC based on the font size, determines the
        number of lines on a page, and saves some useful pixel locations like
        the top left corner and the width and height of the drawing area in
        logical coordinates.
        """
        if self.debuglevel > 0:
            print

        dc.SetFont(self.stc.GetFont())

        # Calculate pixels per inch of the various devices.  The dc_ppi will be
        # equivalent to the page or screen PPI if the target is the printer or
        # a print preview, respectively.
        page_ppi_x, page_ppi_y = self.GetPPIPrinter()
        screen_ppi_x, screen_ppi_y = self.GetPPIScreen()
        dc_ppi_x, dc_ppi_y = dc.GetPPI()
        if self.debuglevel > 0:
            print("printer ppi: %dx%d" % (page_ppi_x, page_ppi_y))
            print("screen ppi: %dx%d" % (screen_ppi_x, screen_ppi_y))
            print("dc ppi: %dx%d" % (dc_ppi_x, dc_ppi_y))

        # Calculate paper size.  Note that this is the size in pixels of the
        # entire paper, which may be larger than the printable range of the
        # printer.  We need to use the entire paper size because we calculate
        # margins ourselves.  Note that GetPageSizePixels returns the
        # dimensions of the printable area.
        px, py, pw, ph = self.GetPaperRectPixels()
        page_width_inch = float(pw) / page_ppi_x
        page_height_inch = float(ph) / page_ppi_y
        if self.debuglevel > 0:
            print("page pixels: %dx%d" % (pw, ph))
            print("page size: %fx%f in" % (page_width_inch, page_height_inch))

        dw, dh = dc.GetSizeTuple()
        dc_pixels_per_inch_x = float(dw) / page_width_inch
        dc_pixels_per_inch_y = float(dh) / page_height_inch
        if self.debuglevel > 0:
            print("device pixels: %dx%d" % (dw, dh))
            print("device pixels per inch: %fx%f" % (dc_pixels_per_inch_x, dc_pixels_per_inch_y))

        # Calculate usable page size
        page_height_mm = page_height_inch * 25.4
        margin_mm = self.top_left_margin[1] + self.bottom_right_margin[1]
        usable_page_height_mm = page_height_mm - margin_mm

        # Lines per page is then the number of lines (based on the point size
        # reported by wx) that will fit into the usable page height
        self.lines_pp = self._calculateLinesPerPage(dc, usable_page_height_mm)

        # The final DC scale factor is then the ratio of the total height in
        # pixels inside the margins to the number of pixels that it takes to
        # represent the number of lines
        dc_margin_pixels = float(dc_pixels_per_inch_y) * margin_mm / 25.4
        dc_usable_pixels = dh - dc_margin_pixels
        page_to_dc = self._calculateScaleFactor(dc, dc_usable_pixels, self.lines_pp)

        dc.SetUserScale(page_to_dc, page_to_dc)

        if self.debuglevel > 0:
            print("Usable page height: %f in" % (usable_page_height_mm / 25.4))
            print("Usable page pixels: %d" % dc_usable_pixels)
            print("lines per page: %d" % self.lines_pp)
            print("page_to_dc: %f" % page_to_dc)

        self.x1 = dc.DeviceToLogicalXRel(float(self.top_left_margin[0]) / 25.4 * dc_pixels_per_inch_x)
        self.y1 = dc.DeviceToLogicalXRel(float(self.top_left_margin[1]) / 25.4 * dc_pixels_per_inch_y)
        self.x2 = dc.DeviceToLogicalXRel(dw) - dc.DeviceToLogicalXRel(float(self.bottom_right_margin[0]) / 25.4 * dc_pixels_per_inch_x)
        self.y2 = dc.DeviceToLogicalYRel(dh) - dc.DeviceToLogicalXRel(float(self.bottom_right_margin[1]) / 25.4 * dc_pixels_per_inch_y)
        page_height = self.y2 - self.y1

        #self.lines_pp = int(page_height / dc_pixels_per_line)

        if self.debuglevel > 0:
            print("page size: %d,%d -> %d,%d, height=%d" % (int(self.x1), int(self.y1), int(self.x2), int(self.y2), page_height))

    def _calculateLinesPerPage(self, dc, usable_page_height_mm):
        """Calculate the number of lines that will fit on the page.

        @param dc: the Device Context

        @param usable_page_height_mm: height in mm of the printable part of the
        page (i.e.  with the border height removed)

        @returns: the number of lines on the page
        """
        if self.user_lines_per_page is not None:
            return self.user_lines_per_page

        font = dc.GetFont()
        if self.output_point_size is not None:
            points_per_line = self.output_point_size
        else:
            points_per_line = font.GetPointSize()

        # desired lines per mm based on point size.  Note: printer points are
        # defined as 72 points per inch
        lines_per_inch = 72.0 / float(points_per_line)

        if self.debuglevel > 0:
            print("font: point size per line=%d" % points_per_line)
            print("font: lines per inch=%f" % lines_per_inch)

        # Lines per page is then the number of lines (based on the point size
        # reported by wx) that will fit into the usable page height
        return float(usable_page_height_mm) / 25.4 * lines_per_inch

    def _calculateScaleFactor(self, dc, dc_usable_pixels, lines_pp):
        """Calculate the scale factor for the DC to fit the number of lines
        onto the printable area

        @param dc: the Device Context

        @param dc_usable_pixels: the number of pixels that defines usable
        height of the printable area

        @param lines_pp: the number of lines to fit into the printable area

        @returns: the scale facter to be used in wx.DC.SetUserScale
        """
        # actual line height in pixels according to the DC
        dc_pixels_per_line = dc.GetCharHeight()

        # actual line height in pixels according to the STC.  This can be
        # different from dc_pixels_per_line even though it is the same font.
        # Don't know why this is the case; maybe because the STC takes into
        # account additional spacing?
        stc_pixels_per_line = self.stc.TextHeight(0)
        if self.debuglevel > 0:
            print("font: dc pixels per line=%d" % dc_pixels_per_line)
            print("font: stc pixels per line=%d" % stc_pixels_per_line)

        # Platform dependency alert: I don't know why this works, but through
        # experimentation it seems like the scaling factor depends on
        # different font heights depending on the platform.
        if wx.Platform == "__WXMSW__":
            # On windows, the important font height seems to be the number of
            # pixels reported by the STC
            page_to_dc = float(dc_usable_pixels) / (stc_pixels_per_line * lines_pp)
        else:
            # Linux and Mac: the DC font height seems to be the correct height
            page_to_dc = float(dc_usable_pixels) / (dc_pixels_per_line * lines_pp)
        return page_to_dc

    def _calculatePageCount(self, attempt_wrap=False):
        """Calculates offsets into the STC for each page

        This pre-calculates the page offsets for each page to support print
        preview being able to seek backwards and forwards.
        """
        page_offsets = []
        page_line_start = 0
        lines_on_page = 0
        num_lines = self.stc.GetLineCount()

        line = 0
        while line < num_lines:
            if attempt_wrap:
                wrap_count = self.stc.WrapCount(line)
                if wrap_count > 1 and self.debuglevel > 0:
                    print("found wrapped line %d: %d" % (line, wrap_count))
            else:
                wrap_count = 1

            # If the next line pushes the count over the edge, mark a page and
            # start the next page
            if lines_on_page + wrap_count > self.lines_pp:
                start_pos = self.stc.PositionFromLine(page_line_start)
                end_pos = self.stc.GetLineEndPosition(page_line_start + lines_on_page - 1)
                if self.debuglevel > 0:
                    print("Page: line %d - %d" % (page_line_start, page_line_start + lines_on_page))
                page_offsets.append((start_pos, end_pos))
                page_line_start = line
                lines_on_page = 0
            lines_on_page += wrap_count
            line += 1

        if lines_on_page > 0:
            start_pos = self.stc.PositionFromLine(page_line_start)
            end_pos = self.stc.GetLineEndPosition(page_line_start + lines_on_page)
            page_offsets.append((start_pos, end_pos))

        self.page_count = len(page_offsets)
        self.page_offsets = page_offsets
        if self.debuglevel > 0:
            print("page offsets: %s" % self.page_offsets)

    def _getPositionsOfPage(self, page):
        """Get the starting and ending positions of a page

        @param page: page number

        @returns: tuple containing the start and end positions that can be
        passed to FormatRange to render a page
        """
        page -= 1
        start_pos, end_pos = self.page_offsets[page]
        return start_pos, end_pos

    def GetPageInfo(self):
        """Return the valid page ranges.

        Note that pages are numbered starting from one.
        """
        return (1, self.page_count, 1, self.page_count)

    def HasPage(self, page):
        """Returns True if the specified page is within the page range

        """
        return page <= self.page_count

    def OnPrintPage(self, page):
        """Draws the specified page to the DC

        @param page: page number to render
        """
        dc = self.GetDC()
        self._calculateScale(dc)

        self._drawPageContents(dc, page)
        self._drawPageHeader(dc, page)
        self._drawPageBorder(dc)

        return True

    def _drawPageContents(self, dc, page):
        """Render the STC window into a DC for printing.

        Force the right margin of the rendered window to be huge so the STC
        won't attempt word wrapping.

        @param dc: the device context representing the page

        @param page: page number
        """
        start_pos, end_pos = self._getPositionsOfPage(page)
        render_rect = wx.Rect(self.x1, self.y1, 32000, self.y2)
        page_rect = wx.Rect(self.x1, self.y1, self.x2, self.y2)

        self.stc.SetPrintColourMode(self.print_mode)
        edge_mode = self.stc.GetEdgeMode()
        margin_width_0 = self.stc.GetMarginWidth(0)
        margin_width_1 = self.stc.GetMarginWidth(1)
        margin_width_2 = self.stc.GetMarginWidth(2)
        self.stc.SetEdgeMode(wx.stc.STC_EDGE_NONE)
        self.stc.SetMarginWidth(0, 0)
        self.stc.SetMarginWidth(1, 0)
        self.stc.SetMarginWidth(2, 0)
        end_point = self.stc.FormatRange(True, start_pos, end_pos, dc, dc,
                                        render_rect, page_rect)
        self.stc.SetEdgeMode(edge_mode)
        self.stc.SetMarginWidth(0, margin_width_0)
        self.stc.SetMarginWidth(1, margin_width_1)
        self.stc.SetMarginWidth(2, margin_width_2)

    def _drawPageHeader(self, dc, page):
        """Draw the page header into the DC for printing

        @param dc: the device context representing the page

        @param page: page number
        """
        # Set font for title/page number rendering
        dc.SetFont(self.getHeaderFont())
        dc.SetTextForeground ("black")
        dum, yoffset = dc.GetTextExtent(".")
        yoffset /= 2
        if self.title:
            title_w, title_h = dc.GetTextExtent(self.title)
            dc.DrawText(self.title, self.x1, self.y1 - title_h - yoffset)

        # Page Number
        page_lbl = "%d" % page
        pg_lbl_w, pg_lbl_h = dc.GetTextExtent(page_lbl)
        dc.DrawText(page_lbl, self.x2 - pg_lbl_w, self.y1 - pg_lbl_h - yoffset)

    def setHeaderFont(self, point_size=10, family=wx.FONTFAMILY_SWISS,
                      style=wx.FONTSTYLE_NORMAL, weight=wx.FONTWEIGHT_NORMAL):
        """Set the font to be used as the header font

        @param point_size: point size of the font

        @param family: one of the wx.FONTFAMILY_* values, e.g.
        wx.FONTFAMILY_SWISS, wx.FONTFAMILY_ROMAN, etc.

        @param style: one of the wx.FONTSTYLE_* values, e.g.
        wxFONTSTYLE_NORMAL, wxFONTSTYLE_ITALIC, etc.

        @param weight: one of the wx.FONTWEIGHT_* values, e.g.
        wx.FONTWEIGHT_NORMAL, wx.FONTWEIGHT_LIGHT, etc.
        """
        self.header_font_point_size = point_size
        self.header_font_family = family
        self.header_font_style = style
        self.header_font_weight = weight

    def getHeaderFont(self):
        """Returns the font to be used to draw the page header text

        @returns: wx.Font instance
        """
        point_size = self.header_font_point_size
        font = wx.Font(point_size, self.header_font_family,
                       self.header_font_style, self.header_font_weight)
        return font

    def _drawPageBorder(self, dc):
        """Draw the page border into the DC for printing

        @param dc: the device context representing the page
        """
        if self.border_around_text:
            dc.SetPen(wx.BLACK_PEN)
            dc.SetBrush(wx.TRANSPARENT_BRUSH)
            dc.DrawRectangle(self.x1, self.y1, self.x2 - self.x1 + 1, 
                             self.y2 - self.y1 + 1)

class MyFileDropTarget(wx.FileDropTarget):
    def __init__(self, window):
        wx.FileDropTarget.__init__(self)
        self.window = window

    def OnDropFiles(self, x, y, filenames):
        for file in filenames:
            if os.path.isdir(file):
                self.window.GetTopLevelParent().panel.project.loadFolder(file)
                sys.path.append(file)
            elif os.path.isfile(file):
                self.window.GetTopLevelParent().panel.addPage(file)
            else:
                pass

class EPyoApp(wx.App):
    def __init__(self, *args, **kwargs):
        wx.App.__init__(self, *args, **kwargs)

    def OnInit(self):
        X = wx.SystemSettings.GetMetric(wx.SYS_SCREEN_X)
        Y = wx.SystemSettings.GetMetric(wx.SYS_SCREEN_Y)
        if X < 850: X -= 50
        else: X = 850
        if Y < 750: Y -= 50
        else: Y = 750
        self.frame = MainFrame(None, -1, title='E-Pyo Editor', 
                               pos=(10,25), size=(X, Y))
        self.frame.Show()
        return True

    def MacOpenFiles(self, filenames):
        if type(filenames) != ListType:
            filenames = [filenames]
        for filename in filenames:
            if os.path.isdir(filename):
                self.frame.panel.project.loadFolder(filename)
                sys.path.append(filename)
            elif os.path.isfile(filename):
                self.frame.panel.addPage(filename)

    def MacReopenApp(self):
        try:
            self.frame.Raise()
        except:
            pass

if __name__ == '__main__':
    filesToOpen = []
    foldersToOpen = []
    if len(sys.argv) > 1:
        for f in sys.argv[1:]:
            if os.path.isdir(f):
                if f[-1] == '/': f = f[:-1]
                foldersToOpen.append(f)
            elif os.path.isfile(f):
                filesToOpen.append(f)
            else:
                pass

    app = EPyoApp(redirect=False)
    app.MainLoop()
