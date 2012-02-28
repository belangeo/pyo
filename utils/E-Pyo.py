#! /usr/bin/env python
# encoding: utf-8
"""
E-Pyo is a simple text editor especially configured to edit pyo audio programs.

You can do absolutely everything you want to do with this piece of software.

Olivier Belanger - 2012

"""
import sys, os, string, inspect, keyword, wx, codecs, subprocess, unicodedata, contextlib, StringIO
from types import UnicodeType
from wx.lib.embeddedimage import PyEmbeddedImage
import wx.stc  as  stc
from wx.lib.stattext import GenStaticText
import wx.aui
from pyo import *
from PyoDoc import ManualFrame

reload(sys)
sys.setdefaultencoding("utf-8")

PLATFORM = sys.platform
DEFAULT_ENCODING = sys.getdefaultencoding()
ENCODING = sys.getfilesystemencoding()

APP_NAME = 'E-Pyo'
APP_VERSION = '0.6.1'
OSX_APP_BUNDLED = False
TEMP_PATH = os.path.join(os.path.expanduser('~'), '.epyo')
TEMP_FILE = os.path.join(TEMP_PATH, 'epyo_tempfile.py')
if not os.path.isdir(TEMP_PATH):
    os.mkdir(TEMP_PATH)

if '/%s.app' % APP_NAME in os.getcwd():
    EXAMPLE_PATH = os.path.join(os.getcwd(), "examples")
else:
    EXAMPLE_PATH = os.path.join(os.getcwd(), "../examples")
EXAMPLE_FOLDERS = [folder.capitalize() for folder in os.listdir(EXAMPLE_PATH) if folder[0] != "." and folder not in ["snds", "fft"]]
EXAMPLE_FOLDERS.append("FFT")
EXAMPLE_FOLDERS.sort()

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

if '/%s.app' % APP_NAME in os.getcwd():
    OSX_APP_BUNDLED = True
    # Use the same terminal window for each run
    terminal_close_server_script = """tell application "Terminal" 
    close window 1
end tell
    """
    terminal_close_server_script = convert_line_endings(terminal_close_server_script, 1)
    terminal_close_server_script_path = os.path.join(TEMP_PATH, "terminal_close_server_script.scpt")

    terminal_server_script = """tell application "Terminal"
    do script ""
    set a to get id of front window
    set custom title of window id a to "E-Pyo Output"
    set custom title of tab 1 of window id a to "E-Pyo Output"
    set current settings of first window to settings set "Homebrew"
    set the number of columns of window 1 to 80
    set the number of rows of window 1 to 30
    set the position of window 1 to {810, 25}
end tell
    """
    terminal_server_script = convert_line_endings(terminal_server_script, 1)
    terminal_server_script_path = os.path.join(TEMP_PATH, "terminal_server_script.scpt")
    f = open(terminal_server_script_path, "w")
    f.write(terminal_server_script)
    f.close()
    pid = subprocess.Popen(["osascript", terminal_server_script_path]).pid
    
    terminal_client_script = """set my_path to quoted form of POSIX path of "%s"
set my_file to quoted form of POSIX path of "%s"
tell application "System Events"
    tell application process "Terminal"
    set frontmost to true
    keystroke "clear"
    keystroke return
    delay 0.25
    keystroke "cd " & my_path
    keystroke return
    delay 0.25
    keystroke "python " & my_file
    keystroke return
    delay 0.25
    end tell
    tell application process "E-Pyo"
    set frontmost to true
    end tell
end tell
    """
    terminal_client_script_path = os.path.join(TEMP_PATH, "terminal_client_script.scpt")

################## TEMPLATES ##################
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

WXPYTHON_TEMPLATE = '''#!/usr/bin/env python
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
        
app = wx.PySimpleApp()
mainFrame = MyFrame(None, title='Simple App', pos=(100,100), size=(500,300))
mainFrame.Show()
app.MainLoop()
'''

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
BUILTINS_DICT = {"from": FROM_COMP, "try": TRY_COMP, "if": IF_COMP, "def": DEF_COMP, "class": CLASS_COMP, 
                "for": FOR_COMP, "while": WHILE_COMP, "exec": EXEC_COMP, "raise": RAISE_COMP, "assert": ASSERT_COMP}

# ***************** Catalog starts here *******************

catalog = {}

#----------------------------------------------------------------------
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

#----------------------------------------------------------------------
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

#----------------------------------------------------------------------
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

############## allowed extensions ##############
ALLOWED_EXT = ["py", "c5", "txt", "", "c", "h", "cpp", "hpp", "sh"]

############## Pyo keywords ##############
tree = OBJECTS_TREE
PYO_WORDLIST = []
for k1 in tree.keys():
    if type(tree[k1]) == type({}):
        for k2 in tree[k1].keys():
            for val in tree[k1][k2]:
                PYO_WORDLIST.append(val)
    else:
        for val in tree[k1]:
            PYO_WORDLIST.append(val)
PYO_WORDLIST.append("PyoObject")
PYO_WORDLIST.append("PyoTableObject")
PYO_WORDLIST.append("PyoMatrixObject")
PYO_WORDLIST.append("Server")

# Bitstream Vera Sans Mono, Corbel, Monaco, Envy Code R, MonteCarlo, Courier New
conf = {"preferedStyle": "Default"}
STYLES = {'Default': {'default': '#000000', 'comment': '#0066FF', 'commentblock': '#0066FF', 'selback': "#C0DFFF",
                    'number': '#0000CD', 'string': '#036A07', 'triple': '#038A07', 'keyword': '#0000FF',
                    'class': '#000097', 'function': '#0000A2', 'identifier': '#000000', 'caret': '#000000',
                    'background': '#FFFFFF', 'linenumber': '#000000', 'marginback': '#B0B0B0', 'markerfg': '#CCCCCC',
                      'markerbg': '#000000', 'bracelight': '#AABBDD', 'bracebad': '#DD0000', 'lineedge': '#DDDDDD'},

           'Custom': {'default': '#FFFFFF', 'comment': '#9FFF9F', 'commentblock': '#7F7F7F', 'selback': '#333333',
                      'number': '#90CB43', 'string': '#FF47D7', 'triple': '#FF3300', 'keyword': '#4A94FF',
                      'class': '#4AF3FF', 'function': '#00E0B6', 'identifier': '#FFFFFF', 'caret': '#DDDDDD',
                      'background': '#000000', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                      'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000', 'lineedge': '#222222'},

            'Soft': {'default': '#000000', 'comment': '#444444', 'commentblock': '#7F7F7F', 'selback': '#CBCBCB',
                     'number': '#222222', 'string': '#272727', 'triple': '#333333', 'keyword': '#000000',
                     'class': '#666666', 'function': '#555555', 'identifier': '#000000', 'caret': '#222222',
                     'background': '#EFEFEF', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                     'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000', 'lineedge': '#CDCDCD'},

            'Smooth': {'default': '#FFFFFF', 'comment': '#DD0000', 'commentblock': '#AF0000', 'selback': '#555555',
                       'number': '#FFFFFF', 'string': '#00EE00', 'triple': '#00AA00', 'keyword': '#9999FF',
                       'class': '#00FFA2', 'function': '#00FFD5', 'identifier': '#CCCCCC', 'caret': '#EEEEEE',
                       'background': '#222222', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                       'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000', 'lineedge': '#333333'},

            'Espresso': {'default': '#BDAE9C', 'comment': '#0066FF', 'commentblock': '#0044DD', 'selback': '#5D544F',
                         'number': '#44AA43', 'string': '#2FE420', 'triple': '#049B0A', 'keyword': '#43A8ED',
                         'class': '#E5757B', 'function': '#FF9358', 'identifier': '#BDAE9C', 'caret': '#999999',
                         'background': '#2A211C', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                         'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000', 'lineedge': '#3B322D'}
        }
if wx.Platform == '__WXMSW__':
    faces = {'face': 'Courier', 'size' : 10, 'size2': 8}
elif wx.Platform == '__WXMAC__':
    faces = {'face': 'Monaco', 'size' : 12, 'size2': 9}
else:
    faces = {'face': 'Courier New', 'size' : 8, 'size2': 7}

styles = STYLES.keys()

for key, value in STYLES[conf['preferedStyle']].items():
    faces[key] = value
faces2 = faces.copy()
faces2['size3'] = faces2['size2'] + 4
for key, value in STYLES['Default'].items():
    faces2[key] = value

class MainFrame(wx.Frame):
    def __init__(self, parent, ID, title, pos=wx.DefaultPosition, size=wx.DefaultSize, style=wx.DEFAULT_FRAME_STYLE):
        wx.Frame.__init__(self, parent, ID, title, pos, size, style)

        self.Bind(wx.EVT_CLOSE, self.OnClose)

        self.panel = MainPanel(self, size=size)

        if sys.platform == "darwin":
            accel = wx.ACCEL_CMD
        else:
            accel = wx.ACCEL_CTRL
        aTable = wx.AcceleratorTable([(accel, ord('1'), 10001),
                                      (accel, ord('2'), 10002),
                                      (accel, ord('3'), 10003),
                                      (accel, ord('4'), 10004),
                                      (accel, ord('5'), 10005),
                                      (accel, ord('6'), 10006),
                                      (accel, ord('7'), 10007),
                                      (accel, ord('8'), 10008),
                                      (accel, ord('9'), 10009),
                                      (accel, ord('0'), 10010)])
        self.SetAcceleratorTable(aTable)
        self.Bind(wx.EVT_MENU, self.onSwitchTabs, id=10001, id2=10010)

        self.menuBar = wx.MenuBar()

        menu1 = wx.Menu()
        menu1.Append(wx.ID_NEW, "New\tCtrl+N")
        self.Bind(wx.EVT_MENU, self.new, id=wx.ID_NEW)
        self.submenu1 = wx.Menu()
        self.submenu1.Append(98, "Pyo Template")
        self.submenu1.Append(97, "Cecilia5 Template")
        self.submenu1.Append(96, "Zyne Template")
        self.submenu1.Append(95, "WxPython Template")
        menu1.AppendMenu(99, "New From Template", self.submenu1)
        self.Bind(wx.EVT_MENU, self.newFromTemplate, id=95, id2=98)
        menu1.Append(wx.ID_OPEN, "Open\tCtrl+O")
        self.Bind(wx.EVT_MENU, self.open, id=wx.ID_OPEN)
        menu1.Append(112, "Open Folder\tShift+Ctrl+O")
        self.Bind(wx.EVT_MENU, self.openFolder, id=112)
        self.submenu2 = wx.Menu()
        ID_OPEN_RECENT = 2000
        recentFiles = []
        filename = ensureNFD(os.path.join(TEMP_PATH,'.recent.txt'))
        if os.path.isfile(filename):
            f = codecs.open(filename, "r", encoding="utf-8")
            for line in f.readlines():
                recentFiles.append(line)
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
        menu1.AppendSeparator()
        self.showProjItem = menu1.Append(50, "Show Folder Panel")
        self.Bind(wx.EVT_MENU, self.showHideFolderPanel, id=50)
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
        menu2.Append(wx.ID_ZOOM_IN, "Zoom in\tCtrl+=")
        menu2.Append(wx.ID_ZOOM_OUT, "Zoom out\tCtrl+-")
        self.Bind(wx.EVT_MENU, self.zoom, id=wx.ID_ZOOM_IN, id2=wx.ID_ZOOM_OUT)
        menu2.Append(130, "Show Invisibles", kind=wx.ITEM_CHECK)
        self.Bind(wx.EVT_MENU, self.showInvisibles, id=130)
        menu2.Append(131, "Remove Trailing White Space")
        self.Bind(wx.EVT_MENU, self.removeTrailingWhiteSpace, id=131)
        menu2.AppendSeparator()
        menu2.Append(103, "Collapse/Expand\tShift+Ctrl+F")
        self.Bind(wx.EVT_MENU, self.fold, id=103)
        menu2.Append(108, "Un/Comment Selection\tCtrl+J")
        self.Bind(wx.EVT_MENU, self.OnComment, id=108)
        menu2.Append(114, "Show AutoCompletion\tCtrl+K")
        self.Bind(wx.EVT_MENU, self.autoComp, id=114)
        menu2.Append(121, "Insert File Path...\tCtrl+P")
        self.Bind(wx.EVT_MENU, self.insertPath, id=121)
        menu2.AppendSeparator()
        menu2.Append(170, "Convert Selection to Uppercase\tCtrl+U")
        menu2.Append(171, "Convert Selection to Lowercase\tShift+Ctrl+U")
        self.Bind(wx.EVT_MENU, self.upperLower, id=170, id2=171)
        menu2.Append(172, "Convert Tabs to Spaces")
        self.Bind(wx.EVT_MENU, self.tabsToSpaces, id=172)
        menu2.AppendSeparator()
        menu2.Append(140, "Goto line...\tCtrl+L")
        self.Bind(wx.EVT_MENU, self.gotoLine, id=140)
        menu2.Append(wx.ID_FIND, "Find...\tCtrl+F")
        self.Bind(wx.EVT_MENU, self.showFind, id=wx.ID_FIND)
        menu2.AppendSeparator()
        menu2.Append(180, "Show Documentation for Current Object\tCtrl+D")
        self.Bind(wx.EVT_MENU, self.showDoc, id=180)
        self.menuBar.Append(menu2, 'Code')

        menu3 = wx.Menu()
        menu3.Append(104, "Run\tCtrl+R")
        self.Bind(wx.EVT_MENU, self.runner, id=104)
        menu3.Append(105, "Run Selection\tShift+Ctrl+R")
        self.Bind(wx.EVT_MENU, self.runSelection, id=105)
        menu3.Append(106, "Execute Line/Selection as Python\tCtrl+E")
        self.Bind(wx.EVT_MENU, self.execSelection, id=106)
        self.menuBar.Append(menu3, 'Process')

        menu5 = wx.Menu()
        ID_STYLE = 500
        for st in styles:
            menu5.Append(ID_STYLE, st, "", wx.ITEM_RADIO)
            if st == conf['preferedStyle']: menu5.Check(ID_STYLE, True)
            ID_STYLE += 1
        self.menuBar.Append(menu5, 'Styles')
        for i in range(500, ID_STYLE):
            self.Bind(wx.EVT_MENU, self.changeStyle, id=i)

        menu6 = wx.Menu()
        ID_EXAMPLE = 1000
        for folder in EXAMPLE_FOLDERS:
            exmenu = wx.Menu(title=folder.lower())
            for ex in sorted([exp for exp in os.listdir(os.path.join(EXAMPLE_PATH, folder.lower())) if exp[0] != "."]):
                exmenu.Append(ID_EXAMPLE, ex)
                ID_EXAMPLE += 1
            menu6.AppendMenu(-1, folder, exmenu)
            ID_EXAMPLE += 1
        self.Bind(wx.EVT_MENU, self.openExample, id=1000, id2=ID_EXAMPLE)
        self.menuBar.Append(menu6, "Pyo Examples")

        helpmenu = wx.Menu()
        helpItem = helpmenu.Append(wx.ID_ABOUT, '&About %s %s' % (APP_NAME, APP_VERSION), 'wxPython RULES!!!')
        self.Bind(wx.EVT_MENU, self.onHelpAbout, helpItem)
        helpmenu.Append(190, "Show Documentation Frame\tShift+Ctrl+D")
        self.Bind(wx.EVT_MENU, self.showDocFrame, id=190)
        self.menuBar.Append(helpmenu, '&Help')

        self.SetMenuBar(self.menuBar)

        if foldersToOpen:
            for p in foldersToOpen:
                self.panel.project.loadFolder(p)
                sys.path.append(p)

        if filesToOpen:
            for f in filesToOpen:
                self.panel.addPage(f)

        wx.CallAfter(self.buildDoc)

    ### Editor functions ###
    def cut(self, evt):
        self.panel.editor.Cut()

    def copy(self, evt):
        self.panel.editor.Copy()

    def paste(self, evt):
        self.panel.editor.Paste()

    def selectall(self, evt):
        self.panel.editor.SelectAll()

    def upperLower(self, evt):
        if evt.GetId() == 170:
            self.panel.editor.UpperCase()
        else:
            self.panel.editor.LowerCase()

    def tabsToSpaces(self, evt):
        self.panel.editor.tabsToSpaces()

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
        self.panel.editor.showInvisibles(evt.GetInt())

    def removeTrailingWhiteSpace(self, evt):
        self.panel.editor.removeTrailingWhiteSpace()

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
            pos = self.panel.editor.FindColumn(val-1, 0)
            self.panel.editor.SetCurrentPos(pos)
            self.panel.editor.EnsureVisible(val)
            self.panel.editor.EnsureCaretVisible()
            wx.CallAfter(self.panel.editor.SetAnchor, pos)

    def OnComment(self, evt):
        self.panel.editor.OnComment()

    def fold(self, event):
        self.panel.editor.FoldAll()

    def autoComp(self, evt):
        try:
            self.panel.editor.showAutoComp()
        except AttributeError:
            pass

    def showFind(self, evt):
        self.panel.editor.OnShowFindReplace()

    def insertPath(self, evt):
        dlg = wx.FileDialog(self, message="Choose a file", defaultDir=os.getcwd(),
                            defaultFile="", style=wx.OPEN | wx.MULTIPLE)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPaths()
            text = ensureNFD(path[0])
            self.panel.editor.ReplaceSelection("'" + text + "'")
        dlg.Destroy()

    def changeStyle(self, evt):
        menu = self.GetMenuBar()
        id = evt.GetId()
        st = menu.FindItemById(id).GetLabel()
        for key, value in STYLES[st].items():
            faces[key] = value
        for i in range(self.panel.notebook.GetPageCount()):
            ed = self.panel.notebook.GetPage(i)
            ed.setStyle()
        self.panel.project.setStyle()

    def onSwitchTabs(self, evt):
        page = evt.GetId() - 10001
        self.panel.setPage(page)

    ### Open Prefs ang Logs ###
    def openPrefs(self, evt):
        pass

    def showHideFolderPanel(self, evt):
        state = self.showProjItem.GetItemLabel() == "Show Folder Panel"
        if state:
            self.panel.splitter.SplitVertically(self.panel.project, self.panel.notebook, 175)
            self.showProjItem.SetItemLabel("Hide Folder Panel")
        else:
            self.panel.splitter.Unsplit(self.panel.project)
            self.showProjItem.SetItemLabel("Show Folder Panel")

    def showProjectTree(self, state):
        if state:
            self.panel.splitter.SplitVertically(self.panel.project, self.panel.notebook, 175)
            self.showProjItem.SetItemLabel("Hide Folder Panel")
        else:
            self.panel.splitter.Unsplit(self.panel.project)
            self.showProjItem.SetItemLabel("Show Folder Panel")

    ### New / Open / Save / Delete ###
    def new(self, event):
        self.panel.addNewPage()

    def newFromTemplate(self, event):
        self.panel.addNewPage()
        temp = {98: PYO_TEMPLATE, 97: CECILIA5_TEMPLATE, 96: ZYNE_TEMPLATE, 95: WXPYTHON_TEMPLATE}[event.GetId()]
        self.panel.editor.SetText(temp)

    def newRecent(self, file):
        filename = ensureNFD(os.path.join(TEMP_PATH,'.recent.txt'))
        try:
            f = codecs.open(filename, "r", encoding="utf-8")
            lines = [line[:-1] for line in f.readlines()]
            f.close()
        except:
            lines = []
        if not file in lines:
            f = codecs.open(filename, "w", encoding="utf-8")
            lines.insert(0, file)
            if len(lines) > 10:
                lines = lines[0:10]
            for line in lines:
                f.write(line + '\n')
            f.close()
        subId2 = 2000
        if lines != []:
            for item in self.submenu2.GetMenuItems():
                self.submenu2.DeleteItem(item)
            for file in lines:
                self.submenu2.Append(subId2, toSysEncoding(file + '\n'))
                subId2 += 1

    def openRecent(self, event):
        menu = self.GetMenuBar()
        id = event.GetId()
        file = menu.FindItemById(id).GetLabel()
        self.panel.addPage(ensureNFD(file[:-1]))

    def open(self, event):
        dlg = wx.FileDialog(self, message="Choose a file", 
            defaultDir=os.path.expanduser("~"), defaultFile="", style=wx.OPEN | wx.MULTIPLE)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPaths()
            for file in path:
                filename = ensureNFD(file)
                self.panel.addPage(filename)
                self.newRecent(filename)
        dlg.Destroy()

    def openExample(self, event):
        id = event.GetId()
        menu = event.GetEventObject()
        item = menu.FindItemById(id)
        filename = item.GetLabel()
        folder = menu.GetTitle()
        path = os.path.join(EXAMPLE_PATH, folder, filename)
        self.panel.addPage(ensureNFD(path))

    def openFolder(self, event):
        dlg = wx.DirDialog(self, message="Choose a folder", 
            defaultPath=os.path.expanduser("~"), style=wx.DD_DEFAULT_STYLE)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.folder = path
            self.panel.project.loadFolder(self.folder)
            sys.path.append(path)
        dlg.Destroy()

    def save(self, event):
        path = self.panel.editor.path
        if not path or path == "Untitled.py":
            self.saveas(None)
        else:
            self.panel.editor.saveMyFile(path)
            self.SetTitle(path)
            tab = self.panel.notebook.GetSelection()
            self.panel.notebook.SetPageText(tab, os.path.split(path)[1].split('.')[0])

    def saveas(self, event):
        deffile = os.path.split(self.panel.editor.path)[1]
        dlg = wx.FileDialog(self, message="Save file as ...", 
            defaultDir=os.path.expanduser('~'), defaultFile=deffile, style=wx.SAVE)
        dlg.SetFilterIndex(0)
        if dlg.ShowModal() == wx.ID_OK:
            path = ensureNFD(dlg.GetPath())
            self.panel.editor.path = path
            self.panel.editor.setStyle()
            self.panel.editor.SetCurrentPos(0)
            self.panel.editor.AddText(" ")
            self.panel.editor.DeleteBackNotLine()
            self.panel.editor.saveMyFile(path)
            self.SetTitle(path)
            tab = self.panel.notebook.GetSelection()
            self.panel.notebook.SetPageText(tab, os.path.split(path)[1].split('.')[0])
            self.newRecent(path)
        dlg.Destroy()

    def close(self, event):
        action = self.panel.editor.close()
        if action == 'delete':
            self.panel.deletePage()
        else:
            pass

    def closeAll(self, event):
        count = self.panel.notebook.GetPageCount()
        while count > 0:
            count -= 1
            self.panel.setPage(count)
            self.close(None)

    ### Run actions ###
    def run(self, path, cwd):
        if OSX_APP_BUNDLED:
            script = terminal_client_script % (cwd, path)
            script = convert_line_endings(script, 1)
            with codecs.open(terminal_client_script_path, "w", encoding="utf-8") as f:
                f.write(script)
            pid = subprocess.Popen(["osascript", terminal_client_script_path]).pid
        else:
            pid = subprocess.Popen(["python", path], cwd=cwd).pid

    def runner(self, event):
        # Need to determine which python to use...
        path = ensureNFD(self.panel.editor.path)
        if os.path.isfile(path):
            cwd = os.path.split(path)[0]
            self.run(path, cwd)
        else:
            text = self.panel.editor.GetTextUTF8()
            if text != "":
                with open(TEMP_FILE, "w") as f:
                    f.write(text)
                self.run(TEMP_FILE, os.path.expanduser("~"))

    def runSelection(self, event):
        text = self.panel.editor.GetSelectedTextUTF8()
        if text != "":
            with open(TEMP_FILE, "w") as f:
                f.write(text)
            self.run(TEMP_FILE, os.path.expanduser("~"))

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
        self.panel.editor.AddText("\n")
        with stdoutIO() as s:
            exec text
        self.panel.editor.AddText(s.getvalue())

    def buildDoc(self):
        self.doc_frame = ManualFrame(osx_app_bundled=OSX_APP_BUNDLED)

    def showDoc(self, evt):
        if not self.doc_frame.IsShown():
            self.doc_frame.Show()
        word = self.panel.editor.getWordUnderCaret()
        if word:
            self.doc_frame.doc_panel.getPage(word)

    def showDocFrame(self, evt):
        if not self.doc_frame.IsShown():
            self.doc_frame.Show()

    def onHelpAbout(self, evt):
        info = wx.AboutDialogInfo()
        info.Name = APP_NAME
        info.Version = APP_VERSION
        info.Copyright = u"(C) 2012 Olivier Bélanger"
        info.Description = "E-Pyo is a simple text editor especially configured to edit pyo audio programs.\n\n"
        wx.AboutBox(info)

    def OnClose(self, event):
        try:
            self.doc_frame.Destroy()
        except:
            pass
        self.panel.OnQuit()
        if OSX_APP_BUNDLED:
            with open(terminal_close_server_script_path, "w") as f:
                f.write(terminal_close_server_script)
            subprocess.Popen(["osascript", terminal_close_server_script_path])
        self.Destroy()

class MainPanel(wx.Panel):
    def __init__(self, parent, size=(1200,800), style=wx.SUNKEN_BORDER):
        wx.Panel.__init__(self, parent, size=(1200,800), style=wx.SUNKEN_BORDER)

        self.mainFrame = parent
        mainBox = wx.BoxSizer(wx.HORIZONTAL)

        self.splitter = wx.SplitterWindow(self, -1, style=wx.SP_LIVE_UPDATE)
        
        self.project = ProjectTree(self.splitter, self, (-1, -1))
        self.notebook = MyNotebook(self.splitter)
        self.editor = Editor(self.notebook, -1, size=(0, -1), setTitle=self.SetTitle, getTitle=self.GetTitle)
        
        self.splitter.SplitVertically(self.project, self.notebook, 175)
        self.splitter.Unsplit(self.project)
        
        mainBox.Add(self.splitter, 1, wx.EXPAND)
        self.SetSizer(mainBox)

        self.Bind(wx.aui.EVT_AUINOTEBOOK_PAGE_CHANGED, self.onPageChange)
        self.Bind(wx.aui.EVT_AUINOTEBOOK_PAGE_CLOSE, self.onClosePage)

    def addNewPage(self):
        editor = Editor(self.notebook, -1, size=(0, -1), setTitle=self.SetTitle, getTitle=self.GetTitle)
        editor.path = "Untitled.py"
        editor.setStyle()
        self.notebook.AddPage(editor, "Untitled.py", True)
        self.editor = editor

    def addPage(self, file):
        editor = Editor(self.notebook, -1, size=(0, -1), setTitle=self.SetTitle, getTitle=self.GetTitle)
        label = os.path.split(file)[1].split('.')[0]
        self.notebook.AddPage(editor, label, True)
        with codecs.open(file, "r", encoding="utf-8") as f:
            text = f.read()
        editor.SetText(ensureNFD(text))
        editor.path = file
        editor.saveMark = True
        editor.SetSavePoint()
        editor.setStyle()
        self.editor = editor
        self.SetTitle(file)

    def onClosePage(self, evt):
        ed = self.notebook.GetPage(self.notebook.GetSelection())
        ed.Close()

    def deletePage(self):
        ed = self.notebook.GetPage(self.notebook.GetSelection())
        self.notebook.DeletePage(self.notebook.GetSelection())

    def setPage(self, pageNum):
        totalNum = self.notebook.GetPageCount()
        if pageNum < totalNum:
            self.notebook.SetSelection(pageNum)

    def onPageChange(self, event):
        self.editor = self.notebook.GetPage(self.notebook.GetSelection())
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

    def SetTitle(self, title):
        self.mainFrame.SetTitle(title)

    def GetTitle(self):
        return self.mainFrame.GetTitle()

    def OnQuit(self):
        for i in range(self.notebook.GetPageCount()):
            ed = self.notebook.GetPage(i)
            ed.Close()

class Editor(stc.StyledTextCtrl):
    def __init__(self, parent, ID, pos=wx.DefaultPosition, size=wx.DefaultSize, style= wx.NO_BORDER | wx.WANTS_CHARS,
                 setTitle=None, getTitle=None):
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

        self.alphaStr = string.lowercase + string.uppercase + '0123456789'

        self.Colourise(0, -1)
        self.SetCurrentPos(0)

        self.SetIndent(4)
        self.SetBackSpaceUnIndents(True)
        self.SetTabIndents(True)
        self.SetTabWidth(4)
        self.SetUseTabs(False)
        self.AutoCompSetChooseSingle(True)
        self.SetViewWhiteSpace(False)
        self.SetEOLMode(wx.stc.STC_EOL_LF)
        self.SetViewEOL(False)

        self.SetProperty("fold", "1")
        self.SetProperty("tab.timmy.whinge.level", "1")
        self.SetMargins(5,5)
        self.SetUseAntiAliasing(False)
        self.SetEdgeColour(faces["lineedge"])
        self.SetEdgeMode(stc.STC_EDGE_LINE)
        self.SetEdgeColumn(78)

        self.SetMarginType(1, stc.STC_MARGIN_NUMBER)
        self.SetMarginWidth(1, 28)
        self.SetMarginType(2, stc.STC_MARGIN_SYMBOL)
        self.SetMarginMask(2, stc.STC_MASK_FOLDERS)
        self.SetMarginSensitive(2, True)
        self.SetMarginWidth(2, 12)

        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        self.Bind(stc.EVT_STC_UPDATEUI, self.OnUpdateUI)
        self.Bind(stc.EVT_STC_MARGINCLICK, self.OnMarginClick)
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.Bind(wx.EVT_FIND, self.OnFind)
        self.Bind(wx.EVT_FIND_NEXT, self.OnFind)
        self.Bind(wx.EVT_FIND_REPLACE, self.OnFind)
        self.Bind(wx.EVT_FIND_REPLACE_ALL, self.OnFind)
        self.Bind(wx.EVT_FIND_CLOSE, self.OnFindClose)

        self.EmptyUndoBuffer()
        self.SetFocus()
        self.setStyle()

        wx.CallAfter(self.SetAnchor, 0)
        self.Refresh()

    def setStyle(self):
        # Global default styles for all languages
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "fore:%(default)s,face:%(face)s,size:%(size)d,back:%(background)s" % faces)
        self.StyleClearAll()  # Reset all to be like the default

        ext = os.path.splitext(self.path)[1].strip(".")
        if ext in ["py", "c5"]:
            self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPEN, stc.STC_MARK_BOXMINUS, faces['markerfg'], faces['markerbg'])
            self.MarkerDefine(stc.STC_MARKNUM_FOLDER, stc.STC_MARK_BOXPLUS, faces['markerfg'], faces['markerbg'])
            self.MarkerDefine(stc.STC_MARKNUM_FOLDERSUB, stc.STC_MARK_VLINE, faces['markerfg'], faces['markerbg'])
            self.MarkerDefine(stc.STC_MARKNUM_FOLDERTAIL, stc.STC_MARK_LCORNERCURVE, faces['markerfg'], faces['markerbg'])
            self.MarkerDefine(stc.STC_MARKNUM_FOLDEREND, stc.STC_MARK_ARROW, faces['markerfg'], faces['markerbg'])
            self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPENMID, stc.STC_MARK_ARROWDOWN, faces['markerfg'], faces['markerbg'])
            self.MarkerDefine(stc.STC_MARKNUM_FOLDERMIDTAIL, stc.STC_MARK_LCORNERCURVE, faces['markerfg'], faces['markerbg'])

            self.StyleSetSpec(stc.STC_STYLE_DEFAULT, "fore:%(default)s,face:%(face)s,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_STYLE_LINENUMBER, "fore:%(linenumber)s,back:%(marginback)s,face:%(face)s,size:%(size2)d" % faces)
            self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "fore:%(default)s,face:%(face)s" % faces)
            self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT, "fore:#000000,back:%(bracelight)s,bold" % faces)
            self.StyleSetSpec(stc.STC_STYLE_BRACEBAD, "fore:#000000,back:%(bracebad)s,bold" % faces)

            self.SetLexer(stc.STC_LEX_PYTHON)
            self.SetKeyWords(0, " ".join(keyword.kwlist) + " None True False " + " ".join(PYO_WORDLIST))

            self.StyleSetSpec(stc.STC_P_DEFAULT, "fore:%(default)s,face:%(face)s,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_COMMENTLINE, "fore:%(comment)s,face:%(face)s,italic,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_NUMBER, "fore:%(number)s,face:%(face)s,bold,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_STRING, "fore:%(string)s,face:%(face)s,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_CHARACTER, "fore:%(string)s,face:%(face)s,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_WORD, "fore:%(keyword)s,face:%(face)s,bold,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_TRIPLE, "fore:%(triple)s,face:%(face)s,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, "fore:%(triple)s,face:%(face)s,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_CLASSNAME, "fore:%(class)s,face:%(face)s,bold,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_DEFNAME, "fore:%(function)s,face:%(face)s,bold,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_OPERATOR, "bold,size:%(size)d,face:%(face)s" % faces)
            self.StyleSetSpec(stc.STC_P_IDENTIFIER, "fore:%(identifier)s,face:%(face)s,size:%(size)d" % faces)
            self.StyleSetSpec(stc.STC_P_COMMENTBLOCK, "fore:%(commentblock)s,face:%(face)s,size:%(size)d" % faces)

        self.SetEdgeColour(faces["lineedge"])
        self.SetCaretForeground(faces['caret'])
        self.SetSelBackground(1, faces['selback'])

    def OnShowFindReplace(self):
        data = wx.FindReplaceData()
        self.findReplace = wx.FindReplaceDialog(self, data, "Find & Replace", wx.FR_REPLACEDIALOG | wx.FR_NOUPDOWN)
        self.findReplace.data = data  # save a reference to it...
        self.findReplace.Show(True)

    def OnFind(self, evt):
        map = { wx.wxEVT_COMMAND_FIND : "FIND",
                wx.wxEVT_COMMAND_FIND_NEXT : "FIND_NEXT",
                wx.wxEVT_COMMAND_FIND_REPLACE : "REPLACE",
                wx.wxEVT_COMMAND_FIND_REPLACE_ALL : "REPLACE_ALL" }

        et = evt.GetEventType()
        findTxt = evt.GetFindString()

        selection = self.GetSelection()
        if selection[0] == selection[1]:
            selection = (0, self.GetLength())

        if map[et] == 'FIND':
            startpos = self.FindText(selection[0], selection[1], findTxt, evt.GetFlags())
            endpos = startpos+len(findTxt)
            self.anchor1 = endpos
            self.anchor2 = selection[1]
            self.SetSelection(startpos, endpos)
        elif map[et] == 'FIND_NEXT':
            startpos = self.FindText(self.anchor1, self.anchor2, findTxt, evt.GetFlags())
            endpos = startpos+len(findTxt)
            self.anchor1 = endpos
            self.SetSelection(startpos, endpos)
        elif map[et] == 'REPLACE':
            startpos = self.FindText(selection[0], selection[1], findTxt)
            endpos = startpos+len(findTxt)
            if startpos != -1:
                self.SetSelection(startpos, endpos)
                self.ReplaceSelection(evt.GetReplaceString())
        elif map[et] == 'REPLACE_ALL':
            self.anchor1 = selection[0]
            self.anchor2 = selection[1]
            startpos = selection[0]
            while startpos != -1:
                startpos = self.FindText(self.anchor1, self.anchor2, findTxt)
                endpos = startpos+len(findTxt)
                self.anchor1 = endpos
                if startpos != -1:
                    self.SetSelection(startpos, endpos)
                    self.ReplaceSelection(evt.GetReplaceString())

    def OnFindClose(self, evt):
        evt.GetDialog().Destroy()

    def showInvisibles(self, x):
        self.SetViewWhiteSpace(x)

    def removeTrailingWhiteSpace(self):
        text = self.GetText()
        lines = [line.rstrip() for line in text.splitlines(False)]
        text= "\n".join(lines)
        self.SetText(text)

    def tabsToSpaces(self):
        text = self.GetText()
        text = text.replace("\t", "    ")
        self.SetText(text)

    ### Save and Close file ###
    def saveMyFile(self, file):
        self.SaveFile(file)
        self.path = file
        self.saveMark = False

    def close(self):
        if self.GetModify():
            if not self.path: f = "Untitled"
            else: f = self.path
            dlg = wx.MessageDialog(None, 'file ' + f + ' has been modified. Do you want to save?', 
                                   'Warning!', wx.YES | wx.NO | wx.CANCEL)
            but = dlg.ShowModal()
            if but == wx.ID_YES:
                dlg.Destroy()
                if not self.path:
                    dlg2 = wx.FileDialog(None, message="Save file as ...", defaultDir=os.getcwd(), 
                                         defaultFile="", style=wx.SAVE)
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
                if not self.path:
                    dlg2 = wx.FileDialog(None, message="Save file as ...", defaultDir=os.getcwd(),
                                         defaultFile="", style=wx.SAVE)
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

    ### Editor functions ###
    def deleteBackWhiteSpaces(self):
        count = self.GetCurrentPos()
        while self.GetCharAt(self.GetCurrentPos()-1) == 32:
            self.DeleteBack()
        count -= self.GetCurrentPos()
        return count

    def getWordUnderCaret(self):
        caretPos = self.GetCurrentPos()
        startpos = self.WordStartPosition(caretPos, True)
        endpos = self.WordEndPosition(caretPos, True)
        currentword = self.GetTextRange(startpos, endpos)
        return currentword

    def showAutoComp(self):
        ws = self.deleteBackWhiteSpaces()
        charBefore = " "
        caretPos = self.GetCurrentPos()
        if caretPos > 0:
            charBefore = self.GetTextRange(caretPos - 1, caretPos)
        currentword = self.getWordUnderCaret()
        if charBefore in self.alphaStr:
            list = ''
            for word in PYO_WORDLIST:
                if word.startswith(currentword) and word != currentword:
                    list = list + word + ' '
            if list:
                self.AutoCompShow(len(currentword), list)
            else:
                self.AddText(" "*ws)
        else:
            self.AddText(" "*ws)

    def insertDefArgs(self, currentword):
        for word in PYO_WORDLIST:
            if word == currentword:
                self.deleteBackWhiteSpaces()
                text = class_args(eval(word)).replace(word, "")
                self.args_buffer = text.replace("(", "").replace(")", "").split(",")
                self.args_line_number = [self.GetCurrentLine(), self.GetCurrentLine()+1]
                self.InsertText(self.GetCurrentPos(), text)
                self.selection = self.GetSelectedText()
                wx.CallAfter(self.navigateArgs)
                break

    def navigateArgs(self):
        self.deleteBackWhiteSpaces()
        if self.selection != "":
            self.AddText(self.selection)
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
        self.deleteBackWhiteSpaces()
        if self.selection != "":
            self.AddText(self.selection)
        pos = self.GetLineEndPosition(self.GetCurrentLine()) + 1
        self.SetCurrentPos(pos)
        wx.CallAfter(self.SetAnchor, self.GetCurrentPos())

    def formatBuiltinComp(self, text, indent=0):
        self.snip_buffer = []
        a1 = text.find("`", 0)
        while a1 != -1:
            a2 = text.find("`", a1+1)
            if a2 != -1:
                self.snip_buffer.append(text[a1+1:a2])
                print a1, a2
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
            self.deleteBackWhiteSpaces()
            indent = self.GetLineIndentation(self.GetCurrentLine())
            text, tlen = self.formatBuiltinComp(BUILTINS_DICT[text.strip()], indent)
            self.args_line_number = [self.GetCurrentLine(), self.GetCurrentLine()+len(text.splitlines())]
            self.InsertText(self.GetCurrentPos(), text)
            if len(self.snip_buffer) == 0:
                pos = self.GetCurrentPos() + len(text) + 1
                self.SetCurrentPos(pos)
                wx.CallAfter(self.SetAnchor, self.GetCurrentPos())
            else:
                self.selection = self.GetSelectedText()
                pos = self.GetSelectionStart()
                wx.CallAfter(self.navigateSnips, pos)

    def navigateSnips(self, pos):
        if self.selection != "":
            while self.GetCurrentPos() > pos:
                self.DeleteBack()
            self.AddText(self.selection)
        arg = self.snip_buffer.pop(0)
        if len(self.snip_buffer) == 0:
            self.quit_navigate_snip = True
        self.SearchAnchor()
        self.SearchNext(stc.STC_FIND_MATCHCASE, arg)

    def quitNavigateSnips(self, pos):
        if self.selection != "":
            while self.GetCurrentPos() > pos:
                self.DeleteBack()
            self.AddText(self.selection)
        pos = self.PositionFromLine(self.args_line_number[1])
        self.SetCurrentPos(pos)
        wx.CallAfter(self.SetAnchor, self.GetCurrentPos())

    def processReturn(self):
        prevline = self.GetCurrentLine() - 1
        if self.GetLineUTF8(prevline).strip().endswith(":"):
            indent = self.GetLineIndentation(prevline)
            self.AddText(" "*(indent+4))

    def OnKeyDown(self, evt):
        if evt.GetKeyCode() == wx.WXK_RETURN:
            wx.CallAfter(self.processReturn)
        elif evt.GetKeyCode() == wx.WXK_TAB:
            currentword = self.getWordUnderCaret()
            currentline = self.GetCurrentLine()
            wx.CallAfter(self.checkForBuiltinComp)
            wx.CallAfter(self.insertDefArgs, currentword)
            wx.CallAfter(self.showAutoComp)
            if len(self.args_buffer) > 0 and currentline in range(*self.args_line_number):
                self.selection = self.GetSelectedText()
                wx.CallAfter(self.navigateArgs)
            elif self.quit_navigate_args and currentline in range(*self.args_line_number):
                self.quit_navigate_args = False
                self.selection = self.GetSelectedText()
                wx.CallAfter(self.quitNavigateArgs)
            elif len(self.snip_buffer) > 0 and currentline in range(*self.args_line_number):
                self.selection = self.GetSelectedText()
                pos = self.GetSelectionStart()
                wx.CallAfter(self.navigateSnips, pos)
            elif self.quit_navigate_snip and currentline in range(*self.args_line_number):
                self.quit_navigate_snip = False
                self.selection = self.GetSelectedText()
                pos = self.GetSelectionStart()
                wx.CallAfter(self.quitNavigateSnips, pos)
        evt.Skip()

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
            if self.GetTextRange(pos,pos+1) != '#' and lineLen > 2:
                self.InsertText(pos, '#')
            elif self.GetTextRange(pos,pos+1) == '#':
                self.GotoPos(pos+1)
                self.DelWordLeft()

    def OnMarginClick(self, evt):
        # fold and unfold as needed
        if evt.GetMargin() == 2:
            if evt.GetShift() and evt.GetControl():
                self.FoldAll()
            else:
                lineClicked = self.LineFromPosition(evt.GetPosition())

                if self.GetFoldLevel(lineClicked) & stc.STC_FOLDLEVELHEADERFLAG:
                    if evt.GetShift():
                        self.SetFoldExpanded(lineClicked, True)
                        self.Expand(lineClicked, True, True, 1)
                    elif evt.GetControl():
                        if self.GetFoldExpanded(lineClicked):
                            self.SetFoldExpanded(lineClicked, False)
                            self.Expand(lineClicked, False, True, 0)
                        else:
                            self.SetFoldExpanded(lineClicked, True)
                            self.Expand(lineClicked, True, True, 100)
                    else:
                        self.ToggleFold(lineClicked)

    def FoldAll(self):
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

class ProjectTree(wx.Panel):
    """Project panel"""
    def __init__(self, parent, mainPanel, size):
        wx.Panel.__init__(self, parent, -1, size=size, style=wx.WANTS_CHARS | wx.SUNKEN_BORDER | wx.EXPAND)
        self.SetMinSize((150, -1))
        self.mainPanel = mainPanel

        self.projectDict = {}
        self.edititem = None
        self.itempath = None
        self.scope = None

        tsize = (24, 24)
        file_add_bmp = catalog['file_add_icon.png'].GetBitmap()
        folder_add_bmp = catalog['folder_add_icon.png'].GetBitmap()
        close_panel_bmp = catalog['close_panel_icon.png'].GetBitmap()

        self.sizer = wx.BoxSizer(wx.VERTICAL)

        toolbarbox = wx.BoxSizer(wx.HORIZONTAL)
        tb = wx.ToolBar(self, -1, size=(-1,36))
        tb.SetToolBitmapSize(tsize)
        tb.AddLabelTool(10, "Add File", file_add_bmp, shortHelp="Add File")
        tb.AddLabelTool(11, "Add Folder", folder_add_bmp, shortHelp="Add Folder")
        tb.Realize()
        toolbarbox.Add(tb, 1, wx.LEFT | wx.RIGHT | wx.ALIGN_LEFT | wx.EXPAND, 0)

        tb2 = wx.ToolBar(self, -1, size=(-1,36))
        tb2.SetToolBitmapSize(tsize)
        tb2.AddLabelTool(15, "Close Panel", close_panel_bmp, shortHelp="Close Panel")
        tb2.Realize()
        toolbarbox.Add(tb2, 0, wx.LEFT | wx.RIGHT | wx.ALIGN_RIGHT, 0)

        wx.EVT_TOOL(self, 10, self.onAddFile)
        wx.EVT_TOOL(self, 15, self.onCloseProjectPanel)

        self.sizer.Add(toolbarbox, 0, wx.EXPAND)
        
        self.tree = wx.TreeCtrl(self, -1, (0, 26), size, wx.TR_DEFAULT_STYLE|wx.TR_HIDE_ROOT|wx.SUNKEN_BORDER|wx.EXPAND)

        if wx.Platform == '__WXMAC__':
            self.tree.SetFont(wx.Font(11, wx.ROMAN, wx.NORMAL, wx.NORMAL, face=faces['face']))
        else:
            self.tree.SetFont(wx.Font(8, wx.ROMAN, wx.NORMAL, wx.NORMAL, face=faces['face']))
        self.tree.SetBackgroundColour(faces['background'])

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

        self.root = self.tree.AddRoot("Project")
        self.tree.SetPyData(self.root, None)
        self.tree.SetItemImage(self.root, self.fldridx, wx.TreeItemIcon_Normal)
        self.tree.SetItemImage(self.root, self.fldropenidx, wx.TreeItemIcon_Expanded)
        self.tree.SetItemTextColour(self.root, faces['identifier'])

        self.tree.Bind(wx.EVT_TREE_END_LABEL_EDIT, self.OnEndEdit)
        self.tree.Bind(wx.EVT_RIGHT_DOWN, self.OnRightDown)
        self.tree.Bind(wx.EVT_LEFT_DOWN, self.OnLeftClick)
        self.tree.Bind(wx.EVT_LEFT_DCLICK, self.OnLeftDClick)

    def onAddFile(self, evt):
        treeItemId = self.tree.GetSelection()
        if self.selected != None:
            for dirPath in self.projectDict.keys():
                for root, dirs, files in os.walk(self.projectDict[dirPath]):
                    if self.selected == os.path.split(root)[1]:
                        self.scope = root
                        break
                    elif self.selected in dirs:
                        self.scope = os.path.join(root, self.selected)
                        break
                    elif self.selected in files:
                        self.scope = root
                        treeItemId = self.tree.GetItemParent(treeItemId)
                        break
            self.edititem = self.tree.AppendItem(treeItemId, "Untitled", self.fileidx, self.fileidx, None)
            self.tree.SetItemTextColour(self.edititem, faces['identifier'])
            self.tree.SelectItem(self.edititem)
            if PLATFORM == "darwin":
                self.tree.ScrollTo(self.edititem)
                self.tree.EditLabel(self.edititem)
                txtctrl = self.tree.GetEditControl()
                txtctrl.SetSize((self.GetSize()[0], 22))
                txtctrl.SelectAll()
            else:
                self.tree.EditLabel(self.edititem)                
        else:
            print "No scope where to create the file."

    def onCloseProjectPanel(self, evt):
        self.mainPanel.mainFrame.showProjectTree(False)

    def setStyle(self):
        if not self.tree.IsEmpty():
            self.tree.SetBackgroundColour(faces['background'])
            self.tree.SetItemTextColour(self.root, faces['identifier'])
            (child, cookie) = self.tree.GetFirstChild(self.root)
            while child.IsOk():
                self.tree.SetItemTextColour(child, faces['identifier'])
                if self.tree.ItemHasChildren(child):
                    (subchild, subcookie) = self.tree.GetFirstChild(child)
                    while subchild.IsOk():
                        self.tree.SetItemTextColour(subchild, faces['identifier'])
                        if self.tree.ItemHasChildren(subchild):
                            (ssubchild, ssubcookie) = self.tree.GetFirstChild(subchild)
                            while ssubchild.IsOk():
                                self.tree.SetItemTextColour(ssubchild, faces['identifier'])
                                if self.tree.ItemHasChildren(ssubchild):
                                    (sssubchild, sssubcookie) = self.tree.GetNextChild(ssubchild, ssubcookie)
                                    while sssubchild.IsOk():
                                        self.tree.SetItemTextColour(sssubchild, faces['identifier'])
                                        (sssubchild, sssubcookie) = self.tree.GetNextChild(ssubchild, sssubcookie)
                                (ssubchild, ssubcookie) = self.tree.GetNextChild(subchild, ssubcookie)
                        (subchild, subcookie) = self.tree.GetNextChild(child, subcookie)
                (child, cookie) = self.tree.GetNextChild(self.root, cookie)

    def loadFolder(self, dirPath):
        folderName = os.path.split(dirPath)[1]
        self.projectDict[folderName] = dirPath
        projectDir = {}
        self.mainPanel.mainFrame.showProjectTree(True)
        for root, dirs, files in os.walk(dirPath):
            if os.path.split(root)[1][0] != '.':
                if root == dirPath:
                    child = self.tree.AppendItem(self.root, folderName, self.fldridx, self.fldropenidx, None)
                    self.tree.SetItemTextColour(child, faces['identifier'])
                    if dirs:
                        ddirs = [dir for dir in dirs if dir[0] != '.']
                        for dir in sorted(ddirs):
                            subfol = self.tree.AppendItem(child, "%s" % dir, self.fldridx, self.fldropenidx, None)
                            projectDir[dir] = subfol
                            self.tree.SetItemTextColour(subfol, faces['identifier'])
                    if files:
                        ffiles = [file for file in files if file[0] != '.' and os.path.splitext(file)[1].strip(".") in ALLOWED_EXT]
                        for file in sorted(ffiles):
                            item = self.tree.AppendItem(child, "%s" % file, self.fileidx, self.fileidx, None)
                            self.tree.SetItemTextColour(item, faces['identifier'])
                else:
                    if os.path.split(root)[1] in projectDir.keys():
                        parent = projectDir[os.path.split(root)[1]]
                        if dirs:
                            ddirs = [dir for dir in dirs if dir[0] != '.']
                            for dir in sorted(ddirs):
                                subfol = self.tree.AppendItem(parent, "%s" % dir, self.fldridx, self.fldropenidx, None)
                                projectDir[dir] = subfol
                                self.tree.SetItemTextColour(subfol, faces['identifier'])
                        if files:
                            ffiles = [file for file in files if file[0] != '.' and os.path.splitext(file)[1].strip(".") in ALLOWED_EXT]
                            for file in sorted(ffiles):
                                item = self.tree.AppendItem(parent, "%s" % file, self.fileidx, self.fileidx, None)
                                self.tree.SetItemTextColour(item, faces['identifier'])
        self.tree.SortChildren(self.root)
        self.tree.SortChildren(child)

    def OnRightDown(self, event):
        pt = event.GetPosition();
        self.edititem, flags = self.tree.HitTest(pt)
        item = self.edititem
        if item:
            itemlist = []
            while self.tree.GetItemText(item) not in self.projectDict.keys():
                itemlist.insert(0, self.tree.GetItemText(item))
                item = self.tree.GetItemParent(item)
            itemlist.insert(0, self.projectDict[self.tree.GetItemText(item)])
            self.itempath = os.path.join(*itemlist)
            self.tree.EditLabel(self.edititem)

    def OnEndEdit(self, event):
        if self.edititem and self.itempath:
            newitem = event.GetLabel()
            head, tail = os.path.split(self.itempath)
            newpath = os.path.join(head, newitem)
            os.rename(self.itempath, newpath)
        elif self.edititem and self.scope:
            newitem = event.GetLabel()
            newpath = os.path.join(self.scope, newitem)
            print newpath
            f = open(newpath, "w")
            f.close()
        self.edititem = None
        self.itempath = None
        self.scope = None

    def OnLeftClick(self, event):
        pt = event.GetPosition()
        item, flags = self.tree.HitTest(pt)
        if not item:
            self.tree.UnselectAll()
            self.selected = None
        else:
            self.selected = self.tree.GetItemText(item)
        event.Skip()

    def OnLeftDClick(self, event):
        pt = event.GetPosition()
        item, flags = self.tree.HitTest(pt)
        if item:
            hasChild = self.tree.ItemHasChildren(item)
            if not hasChild:
                parent = None
                ritem = item
                while self.tree.GetItemParent(ritem) != self.tree.GetRootItem():
                    ritem = self.tree.GetItemParent(ritem)
                    parent = self.tree.GetItemText(ritem)
                dirPath = self.projectDict[parent]
                for root, dirs, files in os.walk(dirPath):
                    if files:
                        for file in files:
                            if file == self.tree.GetItemText(item):
                                path = os.path.join(root, file)
                self.mainPanel.addPage(path)
        event.Skip()

class MyNotebook(wx.aui.AuiNotebook):
    def __init__(self, parent, size=(0,-1), style=wx.aui.AUI_NB_TAB_FIXED_WIDTH | 
                                            wx.aui.AUI_NB_CLOSE_ON_ALL_TABS | 
                                            wx.aui.AUI_NB_SCROLL_BUTTONS | wx.SUNKEN_BORDER):
        wx.aui.AuiNotebook.__init__(self, parent, size=size, style=style)
        dt = MyFileDropTarget(self)
        self.SetDropTarget(dt)

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

    app = wx.PySimpleApp()
    X,Y = wx.SystemSettings.GetMetric(wx.SYS_SCREEN_X), wx.SystemSettings.GetMetric(wx.SYS_SCREEN_Y)
    if X < 800: X -= 50
    else: X = 800
    if Y < 700: Y -= 50
    else: Y = 700
    frame = MainFrame(None, -1, title='E-Pyo Editor', pos=(10,25), size=(X, Y))
    frame.Show()
    app.MainLoop()
