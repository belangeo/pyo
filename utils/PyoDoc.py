#!/usr/bin/env python
# encoding: utf-8
from __future__ import with_statement
import subprocess, threading, os, sys, unicodedata
import wx
import wx.stc  as  stc
from wx.lib.embeddedimage import PyEmbeddedImage
from pyo import *

DOC_AS_SINGLE_APP = False

PLATFORM = sys.platform
DEFAULT_ENCODING = sys.getdefaultencoding()
ENCODING = sys.getfilesystemencoding()

TEMP_PATH = os.path.join(os.path.expanduser('~'), '.epyo')
if not os.path.isdir(TEMP_PATH):
    os.mkdir(TEMP_PATH)
DOC_PATH = os.path.join(TEMP_PATH, 'doc')
DOC_EXAMPLE_PATH = os.path.join(TEMP_PATH, 'manual_example.py')

DOC_STYLES = {'Default': {'default': '#000000', 'comment': '#007F7F', 'commentblock': '#7F7F7F', 'selback': '#CCCCCC',
                    'number': '#005000', 'string': '#7F007F', 'triple': '#7F0000', 'keyword': '#00007F', 'keyword2': '#007F9F',
                    'class': '#0000FF', 'function': '#007F7F', 'identifier': '#000000', 'caret': '#00007E',
                    'background': '#EEEEEE', 'linenumber': '#000000', 'marginback': '#B0B0B0', 'markerfg': '#CCCCCC',
                    'markerbg': '#000000', 'bracelight': '#AABBDD', 'bracebad': '#DD0000', 'lineedge': '#CCCCCC'}}

if wx.Platform == '__WXMSW__':
  DOC_FACES = {'face': 'Verdana', 'size' : 8, 'size2': 7}
elif wx.Platform == '__WXMAC__':
  DOC_FACES = {'face': 'Monaco', 'size' : 12, 'size2': 9}
else:
  DOC_FACES = {'face': 'Monospace', 'size' : 8, 'size2': 7}
DOC_FACES['size3'] = DOC_FACES['size2'] + 4
for key, value in DOC_STYLES['Default'].items():
  DOC_FACES[key] = value

# ***************** Catalog starts here *******************

catalog = {}

#----------------------------------------------------------------------
next_24_png = PyEmbeddedImage(
    "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"
    "QmCC")
catalog['next_24.png'] = next_24_png

#----------------------------------------------------------------------
play_24_png = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwY"
    "AAAABGdBTUEAALGOfPtRkwAAACBjSFJNAAB6JQAAgIMAAPn/AACA6QAAdTAAAOpgAAA6mAAA"
    "F2+SX8VGAAAEUUlEQVR42mL4//8/AwybxGw55ZB5+LW8VUUpAxMHNwMVAEAAMSFz+Pj5lbn4"
    "JUS0rGO7TCKWHhVS9fViZGKjyAKAAEKx4N+//wwfP/9iePv5L4OAlJ6+oV/nZjW3zuVcYgbq"
    "5FoAEEAoFvxnYGRgZARZ9I/hw+dvDN9//WeS0fOPMAiZc1LOtqaOhVOIn1QLAAIIzQImhv9A"
    "C0C2MDIxMvz+85fh85dvDExsvPwKZkmNOmGrTonpx4UxsfIQbQFAAKFZgDCcEUYDVfz++5vh"
    "15+fDDwiimpqro0rNfxnbOeTtzMlxgKAAMIIIgYGqOFAzASygBlEMwH5/xn+/vsFlP/NIK5u"
    "72EQNvuwqvfEfjY+WTF8FgAEEKYF0CBiALueCWIZyBJmZgZmFqByZqAVf74zMLGysSuahRSY"
    "JK49JW1VkMTCJcqKzQKAAMJiASRomJghljABDWViZgKz/wIj6Ofv/6DIZ/j47RfD6w9fGRi4"
    "xOQ13Evn6kct3Cuo6mnDwMiCYgFAAKEmUwZouDMjLPnzj4HhK9DAzz/+gelf/4AWgSIGaCkj"
    "EP/49Yfh05cfDPxy+ramsTP36UQsmMspoikPMxMggHD64PdfBoZP3/8xfPnxH8wG+YKFlZmB"
    "lQ0JQ/mMLIxgS77++scqZ+iaZJG65rS4QZwvyEyAAMKwAJjXGL4CDf4MxH+BbGZWRrAhLGAM"
    "soQJzAaLsUPEWYE0Kwczwy+ghnefvjMIysiLiqo7RAONZAMIIJQAAyZ7hq9f/wJd/B9sAChS"
    "QZgF6HomoCuZofEBTl1MsBQHKQGYgI7h52Nh4GRnZ3h55+LzR8dm7QaZCRBAqBYAFf4BamAB"
    "GcwMMZAZbDjE5UwwPjMshUHyDjfQMTxcHAxf3rz6cWHH5FWPjs6a8ff764tAqV8AAYRiAchl"
    "zMyQzAUyBG4JDAPDHGYBAxCzAfn8vJwMf779YLh5aP2hmzu6Jn9/dWUv0Kj3MDMBAgjFAnDK"
    "YYJZwAS3EGIBM9hnEN8ADeZjZ2BlZmV4cOHUvcubu6a9u755FdCIp+DEiAQAAgjVB4ywIoIB"
    "mlyZ4MkVHCxAw3mBBvNyczK8ffL0y4VNkxc+Pj537v9fH6+CggNbRgMIILQggmAGRuSyCJJ0"
    "2TlYGISFeBh+ff3278yGRXtu7uiY/Ov9nUNA1Z/wFRUAAYQaRFCDGYDlDogCAVCYCwlyMbCz"
    "sjLcOXng6tUtXVM+3d+/ESj1Ej04sAGAAEKLA3DKgxd6PNxswLDmYvjw5N6bazunzntxYcWi"
    "/3++3gQlOGKLa4AAYkEXAFbNDOzAjCMiwsPw78enX5e2Lth0/8CkKX++vTgNlP5GaoUDEEAo"
    "FrCwsDII8/MAU8c/hqcX95y7s7dvwtfnZ7YBpd7BqgtSAUAAoVjw/dPLZ7++PPt7/+is6W+u"
    "b1rM8O/3A1KCAxsACCAG5GYLEyuXOAMTuyZQnIuBSgAgwABgBVyq5zW6mgAAAABJRU5ErkJg"
    "gg==")
catalog['play_24.png'] = play_24_png

#----------------------------------------------------------------------
previous_24_png = PyEmbeddedImage(
    "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"
    "BgD9qrwV+ofghAAAAABJRU5ErkJggg==")
catalog['previous_24.png'] = previous_24_png

#----------------------------------------------------------------------
up_24_png = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwY"
    "AAAABGdBTUEAALGOfPtRkwAAACBjSFJNAAB6JQAAgIMAAPn/AACA6QAAdTAAAOpgAAA6mAAA"
    "F2+SX8VGAAAEpElEQVR42mJkmMZAGPxnYGBmZo61kLZsPb3uVMuv+b9mMRAJAAKImcGbgIp/"
    "DAyMTIzREQoxc825bIW41Li97v25/fLf1X9nibEAIIDwWwA0nI2FLTpKMXaeKKM027WPNxlE"
    "OcSZJEwkPe//JM4SgADCbQHQcHYW9uhY5cR5PP9F2O59fcDAwMrI8PnPZwYhFlEmSWMpzwe/"
    "7hC0BCCAsFsANTxaOWEex18+tnvfgIazMDD8BUr8+/+P4fOvL0BLRJikjKWBltzFawlAAGFa"
    "gGQ4+z9etvvfHjIwMjMy/P33l+Ev0PD/DCDMwPAVaIkgqzCTtAnQkp+4LQEIIFQLgIZzsHBE"
    "x6skz2P9x8N2/+tDBgYmoOH//4DcDnQ92A8M/xlBGGjJb6BPWEWYZIxlPR98A1pyHdMSgABC"
    "WAAzXDVlHutfbmCwAA1nZgC7GhQsYMgISlIQ5YxAmglo+be/3xlE2MWY5ICW3P8MtOQGqiUA"
    "AQSxAGp4AtBwtr884GBhZmYCBwUoSP4AffD732+GH/9+MPz4+4Ph57+fYPwLiP8BY+bjz48M"
    "ghzCTLKGMp4PP95/+e8WwhKAAGJkmMrAwMXMFR0HDBb2v8Aw//6Q4Q/jX4Yvf76CNX76/Ynh"
    "259vYEsYmICeYmJiYGZkBjuABchmZGQC5RNgSDIyiPOIMTD/+fv3xIwj2b+2/ZoJsgAggJhZ"
    "wlmy47QT58jzKjO/ZHjN8J/zH8OHXx8Y7r17wHD30wOGr0DDQcHDBDaQmYGFmYWBlQWIgTQT"
    "MzODGKcIgxSfBIMQLz8DKxsLg5SQNJOQvpDPww/3v/2//f8EQACx/F3zV3wl6/J5f/79+Q90"
    "xf8///7+4Nfms9KwNDV69ugFAwcrG9BgFiTDmeFsoOMZhHkEGE5vOHLp681vx4DhyfL/Pzie"
    "mP9//68A9LEoQACx/N/8v+4Tw0eUmOdgZK9ncmA0AgcHEyg4mME0yHCQy1mYITQ4aNiAme/y"
    "l6PfD3zLwpJK2QACiAVDSBvoAE+gxh+MDCygsAZawgJ0KiszzOUQw0EYFA9gmoWJFUc++wUQ"
    "QCzohjOkM4ByLSPjPya4y0E0C9jlLHDDWVlYgTQTAxszKzgV4gIAAYSwQAuIUyBFM8MPBnCq"
    "YIIFERPEJ6xQ17NBI5mVFcIH68EBAAKICUxqAnESwnBg0gaHLyRJQpIjzCJYBIMsYQP7ggWv"
    "DwACiAVseBwkszH8gor+BSV5UNpmgvuCBZRMwRjoaiakoALy8fkAIIBYGKKgLv+JqL0Y/oCK"
    "AkYwZoYFFSjCGaG+YEbkBxZG/D4ACCAWkGtBBiJXjxA+KIgghkPiAxpkUAy2hAmSZBnxWAAQ"
    "QCwMvzHrX3AcoKuE+giEmaAYZhk+ABCAjjpIAgCCYSgahrTF9P5ntTDK1sgNsni/PPcOxJVy"
    "tw6fjsEWISQkRCsJo1zdWgWmAjfHDdJnWwCxMHzG9MGXn1++P7h66xv7X2ABzfid4SfjT2Co"
    "MTN8B5rzCSlVgeLhFssthh8/fvxigHgaI7oBAoiRgR1rM4UXGHTyQBY72E//8YYC0DagM/8y"
    "3Aey0QOcASDAAN0xfmdOgZiqAAAAAElFTkSuQmCC")
catalog['up_24.png'] = up_24_png

_INTRO_TEXT =   """
pyo manual version %s

pyo is a Python module written in C to help digital signal processing script creation.

pyo is a Python module containing classes for a wide variety of audio signal processing types. 
With pyo, user will be able to include signal processing chains directly in Python scripts or 
projects, and to manipulate them in real-time through the interpreter. Tools in pyo module 
offer primitives, like mathematical operations on audio signal, basic signal processing 
(filters, delays, synthesis generators, etc.) together with complex algorithms to create 
granulation and others creative sound manipulations. pyo supports OSC protocol (Open Sound 
Control), to ease communications between softwares, and MIDI protocol, for generating sound 
events and controlling process parameters. pyo allows creation of sophisticated signal 
processing chains with all the benefits of a mature, and wild used, general programming 
language.

Overview:

Server : Main processing audio loop callback handler. 
PyoObjectBase : Base class for all pyo objects.
PyoObject : Base class for all pyo objects that manipulate vectors of samples.
PyoTableObject : Base class for all pyo table objects.
PyoMatrixObject : Base class for all pyo matrix objects.
PyoPVObject : Base class for all pyo phase vocoder objects.
functions : Miscellaneous functions.

""" % PYO_VERSION

_DOC_KEYWORDS = ['Attributes', 'Examples', 'Methods', 'Notes', 'Methods details', 
                 'Parentclass', 'Overview', 'Initline', 'Description', 'Parameters']
_HEADERS = ["Server", "PyoObjectBase", "Map", "Stream", "TableStream", "functions"]
_KEYWORDS_LIST = ['SLMap']
_KEYWORDS_LIST.extend(_HEADERS)
_NUM_PAGES = 1
_NUM_PAGES += len(_HEADERS)
for k1 in _HEADERS:
    if type(OBJECTS_TREE[k1]) == type({}):
        _NUM_PAGES += len(OBJECTS_TREE[k1].keys())
        for k2 in OBJECTS_TREE[k1].keys():
            _NUM_PAGES += 1
            _KEYWORDS_LIST.append(k2)
            if type(OBJECTS_TREE[k1][k2]) == type({}):
                for k3 in OBJECTS_TREE[k1][k2].keys():
                    _KEYWORDS_LIST.extend(OBJECTS_TREE[k1][k2][k3])
                    _NUM_PAGES += len(OBJECTS_TREE[k1][k2][k3])
            else:
                _KEYWORDS_LIST.extend(OBJECTS_TREE[k1][k2])
                _NUM_PAGES += len(OBJECTS_TREE[k1][k2])
    else:
        _KEYWORDS_LIST.extend(OBJECTS_TREE[k1])
        _NUM_PAGES += len(OBJECTS_TREE[k1])
        
PYOOBJECTBASE_METHODS_FILTER = [x[0] for x in inspect.getmembers(PyoObjectBase, inspect.ismethod)]
PYOOBJECT_METHODS_FILTER = [x[0] for x in inspect.getmembers(PyoObject, inspect.ismethod)]
PYOMATRIXOBJECT_METHODS_FILTER = [x[0] for x in inspect.getmembers(PyoMatrixObject, inspect.ismethod)]
PYOTABLEOBJECT_METHODS_FILTER = [x[0] for x in inspect.getmembers(PyoTableObject, inspect.ismethod)]
PYOPVOBJECT_METHODS_FILTER = [x[0] for x in inspect.getmembers(PyoPVObject, inspect.ismethod)]
MAP_METHODS_FILTER = [x[0] for x in inspect.getmembers(Map, inspect.ismethod)]
SLMAP_METHODS_FILTER = [x[0] for x in inspect.getmembers(SLMap, inspect.ismethod)]

def _ed_set_style(editor, searchKey=None):
    editor.SetLexer(stc.STC_LEX_PYTHON)
    editor.SetKeyWords(0, " ".join(_KEYWORDS_LIST))
    if searchKey == None:
        editor.SetKeyWords(1, " ".join(_DOC_KEYWORDS))
    else:
        editor.SetKeyWords(1, " ".join(_DOC_KEYWORDS) + " " + searchKey)

    editor.SetMargins(5,5)
    editor.SetSTCCursor(2)
    editor.SetIndent(4)
    editor.SetTabIndents(True)
    editor.SetTabWidth(4)
    editor.SetUseTabs(False)

    editor.StyleSetSpec(stc.STC_STYLE_DEFAULT,  "fore:%(default)s,face:%(face)s,size:%(size)d,back:%(background)s" % DOC_FACES)
    editor.StyleClearAll()
    editor.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "fore:%(default)s,face:%(face)s,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_STYLE_LINENUMBER,  "fore:%(linenumber)s,back:%(marginback)s,face:%(face)s,size:%(size2)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "fore:%(default)s,face:%(face)s" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_DEFAULT, "fore:%(default)s,face:%(face)s,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_COMMENTLINE, "fore:%(comment)s,face:%(face)s,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_NUMBER, "fore:%(number)s,face:%(face)s,bold,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_STRING, "fore:%(string)s,face:%(face)s,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_CHARACTER, "fore:%(string)s,face:%(face)s,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_WORD, "fore:%(keyword)s,face:%(face)s,bold,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_WORD2, "fore:%(keyword2)s,face:%(face)s,bold,size:%(size3)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_TRIPLE, "fore:%(triple)s,face:%(face)s,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, "fore:%(triple)s,face:%(face)s,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_CLASSNAME, "fore:%(class)s,face:%(face)s,bold,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_DEFNAME, "fore:%(function)s,face:%(face)s,bold,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_OPERATOR, "bold,size:%(size)d,face:%(face)s" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_IDENTIFIER, "fore:%(identifier)s,face:%(face)s,size:%(size)d" % DOC_FACES)
    editor.StyleSetSpec(stc.STC_P_COMMENTBLOCK, "fore:%(commentblock)s,face:%(face)s,size:%(size)d" % DOC_FACES)

def complete_words_from_str(text, keyword):
    words = [keyword]
    keyword = keyword.lower()
    text_ori = text
    text = text.replace("`", " ").replace("'", " ").replace(".", " ").replace(",", " ").replace('"', " ").replace("=", " ").replace("\n", " ").lower()
    found = text.find(keyword)
    while found > -1:
        start = text.rfind(" ", 0, found)
        end = text.find(" ", found)
        words.append(text_ori[start:end])
        found = text.find(keyword, found+1)
    words = " ".join(words)
    return words

class ManualPanel(wx.Treebook):
    def __init__(self, parent):
        wx.Treebook.__init__(self, parent, -1, style=wx.BK_DEFAULT | wx.SUNKEN_BORDER)
        self.parent = parent
        self.searchKey = None
        self.Bind(wx.EVT_TREEBOOK_PAGE_CHANGED, self.OnPageChanged)
        self.parse()

    def reset_history(self):
        self.fromToolbar = False
        self.oldPage = ""
        self.sequence = []
        self.seq_index = 0

    def parse(self):
        self.searchKey = None
        self.DeleteAllPages()
        self.reset_history()

        self.needToParse = False
        if not os.path.isdir(DOC_PATH):
            os.mkdir(DOC_PATH)
            self.needToParse = True

        if self.needToParse:
            dlg = wx.ProgressDialog("Pyo Documentation", "    Building manual...    ",
                                   maximum = _NUM_PAGES, parent=self, style = wx.PD_APP_MODAL | wx.PD_AUTO_HIDE | wx.PD_SMOOTH)
            if wx.Platform == '__WXMSW__':
                dlg.SetSize((300, 150))
            else:
                dlg.SetSize((300, 100))
            keepGoing = True
        count = 1
        win = self.makePanel("Intro")
        self.AddPage(win, "Intro")
        for key in _HEADERS:
            if type(OBJECTS_TREE[key]) == type([]):
                count += 1
                win = self.makePanel(key)
                self.AddPage(win, key)
                for obj in OBJECTS_TREE[key]:
                    count += 1
                    win = self.makePanel(obj)
                    self.AddSubPage(win, obj)
                    if self.needToParse and count <= _NUM_PAGES:
                        (keepGoing, skip) = dlg.Update(count)
            else:
                if key == "PyoObjectBase":
                    count += 1
                    win = self.makePanel("PyoObjectBase")
                    self.AddPage(win, "PyoObjectBase")
                    for key2 in OBJECTS_TREE[key]:
                        if type(OBJECTS_TREE[key][key2]) == type([]):
                            count += 1
                            win = self.makePanel(key2)
                            self.AddPage(win, key2)
                            for obj in OBJECTS_TREE[key][key2]:
                                count += 1
                                win = self.makePanel(obj)
                                self.AddSubPage(win, obj)
                                if self.needToParse and count <= _NUM_PAGES:
                                    (keepGoing, skip) = dlg.Update(count)
                        else:
                            count += 1
                            head = "PyoObj - "
                            win = self.makePanel("PyoObject")
                            self.AddPage(win, "PyoObject")
                            for key3 in sorted(OBJECTS_TREE[key][key2]):
                                count += 1
                                win = self.makePanel("%s" % key3)
                                self.AddPage(win, "PyoObj - %s" % key3)
                                for obj in OBJECTS_TREE[key][key2][key3]:
                                    count += 1
                                    win = self.makePanel(obj)
                                    self.AddSubPage(win, obj)
                                    if self.needToParse and count <= _NUM_PAGES:
                                        (keepGoing, skip) = dlg.Update(count)
                else:
                    count += 2
                    win = self.makePanel("Map")
                    self.AddPage(win, "Map")
                    win = self.makePanel("SLMap")
                    self.AddPage(win, "SLMap")
                    for obj in OBJECTS_TREE[key]["SLMap"]:
                        count += 1
                        win = self.makePanel(obj)
                        self.AddSubPage(win, obj)
                        if self.needToParse and count <= _NUM_PAGES:
                            (keepGoing, skip) = dlg.Update(count)

        if self.needToParse:
            dlg.Destroy()
        self.setStyle()
        self.getPage("Intro")
        wx.FutureCall(100, self.AdjustSize)

    def parseOnSearchName(self, keyword):
        self.searchKey = None
        self.DeleteAllPages()
        self.reset_history()

        keyword = keyword.lower()
        for key in _HEADERS:
            if type(OBJECTS_TREE[key]) == type([]):
                objs = []
                for obj in OBJECTS_TREE[key]:
                    if keyword in obj.lower():
                        objs.append(obj)
                if objs != []:
                    win = self.makePanel(key)
                    self.AddPage(win, key)
                    node = self.GetPageCount()-1
                    for obj in objs:
                        win = self.makePanel(obj)
                        self.AddSubPage(win, obj)
                    self.ExpandNode(node, True)
            else:
                if key == "PyoObjectBase":
                    if keyword in "pyoobjectbase":
                        win = self.makePanel("PyoObjectBase")
                        self.AddPage(win, "PyoObjectBase")
                    for key2 in OBJECTS_TREE[key].keys():
                        if type(OBJECTS_TREE[key][key2]) == type([]):
                            objs = []
                            for obj in OBJECTS_TREE[key][key2]:
                                if keyword in obj.lower():
                                    objs.append(obj)
                            if objs != []:
                                win = self.makePanel(key2)
                                self.AddPage(win, key2)
                                node = self.GetPageCount()-1
                                for obj in objs:
                                    win = self.makePanel(obj)
                                    self.AddSubPage(win, obj)
                                self.ExpandNode(node, True)
                        else:
                            head = "PyoObj - "
                            if keyword in "pyoobject":
                                win = self.makePanel("PyoObject")
                                self.AddPage(win, "PyoObject")
                            for key3 in sorted(OBJECTS_TREE[key][key2]):
                                objs = []
                                for obj in OBJECTS_TREE[key][key2][key3]:
                                    if keyword in obj.lower():
                                        objs.append(obj)
                                if objs != []:
                                    win = self.makePanel("%s" % key3)
                                    self.AddPage(win, "PyoObj - %s" % key3)
                                    node = self.GetPageCount()-1
                                    for obj in objs:
                                        win = self.makePanel(obj)
                                        self.AddSubPage(win, obj)
                                    self.ExpandNode(node, True)
                else:
                    if keyword in "map":
                        win = self.makePanel("Map")
                        self.AddPage(win, "Map")
                    objs = []
                    for obj in OBJECTS_TREE[key]["SLMap"]:
                        if keyword in obj.lower():
                            objs.append(obj)
                        if objs != []:
                            win = self.makePanel("SLMap")
                            self.AddPage(win, "SLMap")
                            node = self.GetPageCount()-1
                            for obj in objs:
                                win = self.makePanel(obj)
                                self.AddSubPage(win, obj)
                            self.ExpandNode(node, True)
        self.setStyle()
        self.getPage("Intro")
        wx.CallAfter(self.AdjustSize)

    def parseOnSearchPage(self, keyword):
        self.searchKey = keyword
        self.DeleteAllPages()
        self.reset_history()

        keyword = keyword.lower()
        for key in _HEADERS:
            if type(OBJECTS_TREE[key]) == type([]):
                objs = []
                for obj in OBJECTS_TREE[key]:
                    with open(os.path.join(DOC_PATH, obj), "r") as f:
                        text = f.read().lower()
                    if keyword in text:
                        objs.append(obj)
                if objs != []:
                    win = self.makePanel(key)
                    self.AddPage(win, key)
                    node = self.GetPageCount()-1
                    for obj in objs:
                        win = self.makePanel(obj)
                        self.AddSubPage(win, obj)
                    self.ExpandNode(node, True)
            else:
                if key == "PyoObjectBase":
                    for key2 in OBJECTS_TREE[key].keys():
                        if type(OBJECTS_TREE[key][key2]) == type([]):
                            objs = []
                            for obj in OBJECTS_TREE[key][key2]:
                                with open(os.path.join(DOC_PATH, obj), "r") as f:
                                    text = f.read().lower()
                                if keyword in text:
                                    objs.append(obj)
                            if objs != []:
                                win = self.makePanel(key2)
                                self.AddPage(win, key2)
                                node = self.GetPageCount()-1
                                for obj in objs:
                                    win = self.makePanel(obj)
                                    self.AddSubPage(win, obj)
                                self.ExpandNode(node, True)
                        else:
                            head = "PyoObj - "
                            with open(os.path.join(DOC_PATH, "PyoObject"), "r") as f:
                                text = f.read().lower()
                            if keyword in text:
                                win = self.makePanel("PyoObject")
                                self.AddPage(win, "PyoObject")
                            for key3 in sorted(OBJECTS_TREE[key][key2]):
                                objs = []
                                for obj in OBJECTS_TREE[key][key2][key3]:
                                    with open(os.path.join(DOC_PATH, obj), "r") as f:
                                        text = f.read().lower()
                                    if keyword in text:
                                        objs.append(obj)
                                if objs != []:
                                    win = self.makePanel("%s" % key3)
                                    self.AddPage(win, "PyoObj - %s" % key3)
                                    node = self.GetPageCount()-1
                                    for obj in objs:
                                        win = self.makePanel(obj)
                                        self.AddSubPage(win, obj)
                                    self.ExpandNode(node, True)
                else:
                    with open(os.path.join(DOC_PATH, "Map"), "r") as f:
                        text = f.read().lower()
                    if keyword in text:
                        win = self.makePanel("Map")
                        self.AddPage(win, "Map")
                    objs = []
                    for obj in OBJECTS_TREE[key]["SLMap"]:
                        with open(os.path.join(DOC_PATH, obj), "r") as f:
                            text = f.read().lower()
                        if keyword in text:
                            objs.append(obj)
                        if objs != []:
                            win = self.makePanel("SLMap")
                            self.AddPage(win, "SLMap")
                            node = self.GetPageCount()-1
                            for obj in objs:
                                win = self.makePanel(obj)
                                self.AddSubPage(win, obj)
                            self.ExpandNode(node, True)
        self.setStyle()
        self.getPage("Intro")
        wx.CallAfter(self.AdjustSize)

    def AdjustSize(self):
        self.GetTreeCtrl().InvalidateBestSize()
        self.SendSizeEvent()

    def copy(self):
        self.GetPage(self.GetSelection()).win.Copy()

    def collapseAll(self):
        count = self.GetPageCount()
        for i in range(count):
            if self.IsNodeExpanded(i):
                self.CollapseNode(i)

    def OnPageChanged(self, event):
        old = event.GetOldSelection()
        new = event.GetSelection()
        if new != old:
            text = self.GetPageText(new)
            self.getPage(text)
        event.Skip()

    def makePanel(self, obj=None):
        panel = wx.Panel(self, -1)
        panel.isLoad = False
        if self.needToParse:
            if obj != "Intro":
                try:
                    args = '\nInitline:\n\n' + class_args(eval(obj)) + '\n\nDescription:\n\n'
                    isAnObject = True
                except:
                    args = '\nDescription:\n\n'
                    if obj in OBJECTS_TREE["functions"]:
                        isAnObject = True
                    else:
                        isAnObject = False
                if isAnObject:
                    try:
                        parentclass = ""
                        text = eval(obj).__doc__
                        text = inspect.cleandoc(text)
                        text = text.replace(".. note::", "Notes:").replace(".. seealso::", "See also:").replace(":Args:", "Parameters:")
                        lines = text.splitlines()
                        num = len(lines)
                        text_form = ""
                        inside_examples = False
                        text_ex, linenum = self.getExample(text)
                        if text_ex == "":
                            for i in range(num):
                                line = lines[i]
                                if ":Parent:" in line:
                                    line = line.replace(":Parent:", "Parent:").replace(":py:class:", "").replace("`", "")
                                    parentclass = line.replace("Parent:", "").strip()
                                text_form += line.rstrip() + '\n'
                        else:
                            for i in range(linenum):
                                line = lines[i]
                                if ":Parent:" in line:
                                    line = line.replace(":Parent:", "Parent:").replace(":py:class:", "").replace("`", "")
                                    parentclass = line.replace("Parent:", "").strip()
                                text_form += line.rstrip() + '\n'
                            text_form += "Examples:\n\n"
                            text_form += "from pyo import *\n"
                            text_form += text_ex
                            if obj not in OBJECTS_TREE["functions"]:
                                text_form += "s.gui(locals())\n"
                        methods = self.getMethodsDoc(text, obj, parentclass)
                        panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600), style=wx.SUNKEN_BORDER)
                        panel.win.SetText(args + text_form + methods)
                    except:
                        panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600), style=wx.SUNKEN_BORDER)
                        panel.win.SetText(args + "\nNot documented yet...\n\n")
                else:
                    try:
                        text = eval(obj).__doc__
                        text = text.replace(".. note::", "Notes:").replace(".. seealso::", "See also:").replace(":Args:", "Parameters:")
                    except:
                        if obj == "functions":
                            text = "Miscellaneous functions...\n\n"
                            text += "\nOverview:\n"
                            for o in OBJECTS_TREE["functions"]:
                                text += o + ": " + self.getDocFirstLine(o)
                        else:
                            text = "\nNot documented yet...\n\n"
                    if obj in OBJECTS_TREE["PyoObjectBase"]["PyoObject"].keys():
                        text += "\nOverview:\n"
                        for o in OBJECTS_TREE["PyoObjectBase"]["PyoObject"][obj]:
                            text += o + ": " + self.getDocFirstLine(o)
                        obj = "PyoObj - " + obj
                    panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600), style=wx.SUNKEN_BORDER)
                    panel.win.SetText(text)
            else:
                panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600), style=wx.SUNKEN_BORDER)
                panel.win.SetText(_INTRO_TEXT)

            panel.win.SaveFile(os.path.join(DOC_PATH, obj))
        return panel

    def getExample(self, text):
        text = inspect.cleandoc(text)
        lines = text.splitlines()
        num = len(lines)
        first_i = -1
        last_i = -1
        found = False
        wait = 0
        for i in range(num-1, -1, -1):
            if not found:
                if lines[i].strip() == "":
                    continue
                else:
                    if lines[i].startswith(">>>") or lines[i].startswith("..."):
                        found = True
                        last_i = i+1+wait
                    else:
                        wait += 1
                        if wait == 2:
                            break
            else:
                if lines[i].strip() == "":
                    first_i = i+1
                    break
        ex_text = ""
        for i in range(first_i, last_i):
            if lines[i].startswith(">>>") or lines[i].startswith("..."):
                ex_text += lines[i][4:] + "\n"
            else:
                ex_text += lines[i] + "\n"
        return ex_text, first_i

    def getDocFirstLine(self, obj):
        try:
            text = eval(obj).__doc__
            if text == None:
                text = ''
        except:
            text = ''

        if text != '':
            spl = text.split('\n')
            if len(spl) == 1:
                f = spl[0]
            else:
                f = spl[1]
        else:
            f = text
        return f.strip() + "\n"

    def getMethodsDoc(self, text, obj, parentclass):
        if parentclass == "PyoObjectBase":
            filter = PYOOBJECTBASE_METHODS_FILTER
        elif parentclass == "PyoObject":
            filter = PYOOBJECT_METHODS_FILTER
        elif parentclass == "PyoTableObject":
            filter = PYOTABLEOBJECT_METHODS_FILTER
        elif parentclass == "PyoMatrixObject":
            filter = PYOMATRIXOBJECT_METHODS_FILTER
        elif parentclass == "PyoPVObject":
            filter = PYOPVOBJECT_METHODS_FILTER
        elif parentclass == "Map":
            filter = MAP_METHODS_FILTER
        elif parentclass == "SLMap":
            filter = SLMAP_METHODS_FILTER
        else:
            filter = []
        obj_meths = [x[0] for x in inspect.getmembers(eval(obj), inspect.ismethod) if x[0] not in filter]
        methods = ''
        for meth in obj_meths:
            docstr = getattr(eval(obj), meth).__doc__
            if docstr != None:
                docstr = docstr.replace(":Args:", "Parameters:")
                args, varargs, varkw, defaults = inspect.getargspec(getattr(eval(obj), meth))
                args = inspect.formatargspec(args, varargs, varkw, defaults, formatvalue=removeExtraDecimals)
                args = args.replace('self, ', '')
                methods += obj + '.' + meth + args + ':\n'
                methods += docstr + '\n    '
        methods_form = ''
        if methods != '':
            methods_form += "\nMethods details:\n\n"
            for i, line in enumerate(methods.splitlines()):
                if i != 0:
                    methods_form += line[4:] + '\n'
                else:
                    methods_form += line + '\n'
        return methods_form

    def MouseDown(self, evt):
        stc = self.GetPage(self.GetSelection()).win
        pos = stc.PositionFromPoint(evt.GetPosition())
        start = stc.WordStartPosition(pos, False)
        end = stc.WordEndPosition(pos, False)
        word = stc.GetTextRange(start, end)
        self.getPage(word)
        evt.Skip()

    def history_check(self):
        back = True
        forward = True
        if self.seq_index <= 0:
            back = False
        if self.seq_index == (len(self.sequence) - 1):
            forward = False
        self.parent.history_check(back, forward)

    def history_back(self):
        self.seq_index -= 1
        if self.seq_index < 0:
            self.seq_index = 0
        self.fromToolbar = True
        self.SetSelection(self.sequence[self.seq_index])
        self.history_check()

    def history_forward(self):
        seq_len = len(self.sequence)
        self.seq_index += 1
        if self.seq_index == seq_len:
            self.seq_index = seq_len - 1
        self.fromToolbar = True
        self.SetSelection(self.sequence[self.seq_index])
        self.history_check()

    def getPage(self, word):
        if word == self.oldPage:
            self.fromToolbar = False
            return
        page_count = self.GetPageCount()
        for i in range(page_count):
            text = self.GetPageText(i)
            if text == word:
                self.oldPage = word
                if not self.fromToolbar:
                    self.sequence = self.sequence[0:self.seq_index+1]
                    self.sequence.append(i)
                    self.seq_index = len(self.sequence) - 1
                    self.history_check()
                self.parent.setTitle(text)
                self.SetSelection(i)
                panel = self.GetPage(self.GetSelection())
                if not panel.isLoad:
                    panel.isLoad = True
                    panel.win = stc.StyledTextCtrl(panel, -1, size=panel.GetSize(), style=wx.SUNKEN_BORDER)
                    panel.win.LoadFile(os.path.join(ensureNFD(DOC_PATH), word))
                    panel.win.SetMarginWidth(1, 0)
                    panel.win.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
                    if self.searchKey != None:
                        words = complete_words_from_str(panel.win.GetText(), self.searchKey)
                        _ed_set_style(panel.win, words)
                    else:
                        _ed_set_style(panel.win)
                    panel.win.SetSelectionEnd(0)

                    def OnPanelSize(evt, win=panel.win):
                        win.SetPosition((0,0))
                        win.SetSize(evt.GetSize())

                    panel.Bind(wx.EVT_SIZE, OnPanelSize)
                self.fromToolbar = False
                return

    def getMethodsDoc2(self, text, obj):
        if obj == "Clean_objects":
            return "Methods details:\n\nClean_objects.start():\n\n    Starts the thread. The timer begins on this call."
        lines = text.splitlines(True)
        flag = False
        methods = ''
        for line in lines:
            if flag:
                if line.strip() == '': continue
                else:
                    l = line.lstrip()
                    ppos = l.find('(')
                    if ppos != -1:
                        meth = l[0:ppos]
                        args, varargs, varkw, defaults = inspect.getargspec(getattr(eval(obj), meth))
                        args = inspect.formatargspec(args, varargs, varkw, defaults, formatvalue=removeExtraDecimals)
                        args = args.replace('self, ', '')
                        methods += obj + '.' + meth + args + ':\n'
                        docstr = getattr(eval(obj), meth).__doc__.rstrip()
                        methods += docstr + '\n\n    '

            if 'Methods:' in line: 
                flag = True
                methods += '    Methods details:\n\n    '

            for key in _DOC_KEYWORDS:
                if key != 'Methods':
                    if key in line: 
                        flag = False

        methods_form = ''
        if methods != '':
            for line in methods.splitlines():
                methods_form += line[4:] + '\n'
        return methods_form

    def getExampleScript(self):
        stc = self.GetPage(self.GetSelection()).win
        start = stc.LineFromPosition(stc.FindText(0, stc.GetLength(), "Examples:")) + 1
        end = stc.LineFromPosition(stc.FindText(0, stc.GetLength(), "Methods details:")) - 1
        if end <= 0:
            end = stc.GetLineCount() - 1
        text = stc.GetTextRange(stc.PositionFromLine(start), stc.PositionFromLine(end))
        return text

    def setStyle(self):
        tree = self.GetTreeCtrl()
        tree.SetBackgroundColour(DOC_STYLES['Default']['background'])
        root = tree.GetRootItem()
        tree.SetItemTextColour(root, DOC_STYLES['Default']['identifier'])
        (child, cookie) = tree.GetFirstChild(root)
        while child.IsOk():
            tree.SetItemTextColour(child, DOC_STYLES['Default']['identifier'])
            if tree.ItemHasChildren(child):
                (child2, cookie2) = tree.GetFirstChild(child)
                while child2.IsOk():
                    tree.SetItemTextColour(child2, DOC_STYLES['Default']['identifier'])
                    (child2, cookie2) = tree.GetNextChild(child, cookie2)
            (child, cookie) = tree.GetNextChild(root, cookie)

class ManualFrame(wx.Frame):
    def __init__(self, parent=None, id=-1, title='Pyo Documentation', size=(940, 700), 
                    osx_app_bundled=False, which_python="python",
                    caller_need_to_invoke_32_bit=False,
                    set_32_bit_arch="export VERSIONER_PYTHON_PREFER_32_BIT=yes;"):
        wx.Frame.__init__(self, parent=parent, id=id, title=title, size=size)
        self.SetMinSize((600, -1))

        self.osx_app_bundled = osx_app_bundled
        self.which_python = which_python
        self.caller_need_to_invoke_32_bit = caller_need_to_invoke_32_bit
        self.set_32_bit_arch = set_32_bit_arch
        gosearchID = 1000
        aTable = wx.AcceleratorTable([(wx.ACCEL_NORMAL, 47, gosearchID)])
        self.SetAcceleratorTable(aTable)
        self.Bind(wx.EVT_MENU, self.setSearchFocus, id=gosearchID)

        tb_size = 24

        self.toolbar = self.CreateToolBar()
        self.toolbar.SetToolBitmapSize((tb_size, tb_size))  # sets icon size

        back_ico = catalog["previous_%d.png" % tb_size]
        forward_ico = catalog["next_%d.png" % tb_size]
        home_ico = catalog["up_%d.png" % tb_size]
        exec_ico = catalog["play_%d.png" % tb_size]

        backTool = self.toolbar.AddSimpleTool(wx.ID_BACKWARD, back_ico.GetBitmap(), "Back")
        self.toolbar.EnableTool(wx.ID_BACKWARD, False)
        self.Bind(wx.EVT_MENU, self.onBack, backTool)

        self.toolbar.AddSeparator()

        forwardTool = self.toolbar.AddSimpleTool(wx.ID_FORWARD, forward_ico.GetBitmap(), "Forward")
        self.toolbar.EnableTool(wx.ID_FORWARD, False)
        self.Bind(wx.EVT_MENU, self.onForward, forwardTool)

        self.toolbar.AddSeparator()

        homeTool = self.toolbar.AddSimpleTool(wx.ID_HOME, home_ico.GetBitmap(), "Go Home")
        self.toolbar.EnableTool(wx.ID_HOME, True)
        self.Bind(wx.EVT_MENU, self.onHome, homeTool)

        self.toolbar.AddSeparator()

        execTool = self.toolbar.AddSimpleTool(wx.ID_PREVIEW, exec_ico.GetBitmap(), "Run Example")
        self.toolbar.EnableTool(wx.ID_PREVIEW, True)
        self.Bind(wx.EVT_MENU, self.onRun, execTool)

        self.toolbar.AddSeparator()

        self.searchTimer = None
        self.searchScope = "Object's Name"
        self.searchMenu = wx.Menu()
        item = self.searchMenu.Append(-1, "Search Scope")
        item.Enable(False)
        for i, txt in enumerate(["Object's Name", "Manual Pages"]):
            id = i+10
            self.searchMenu.Append(id, txt)
            self.Bind(wx.EVT_MENU, self.onSearchScope, id=id)

        tw, th = self.GetTextExtent("Q")
        self.search = wx.SearchCtrl(self.toolbar, 200, size=(200,th+6), style=wx.WANTS_CHARS | wx.TE_PROCESS_ENTER)
        self.search.ShowCancelButton(True)
        self.search.SetMenu(self.searchMenu)
        self.toolbar.AddControl(self.search)
        self.Bind(wx.EVT_TEXT, self.onSearch, id=200)
        self.Bind(wx.EVT_TEXT_ENTER, self.onSearchEnter, id=200)
        self.Bind(wx.EVT_SEARCHCTRL_CANCEL_BTN, self.onSearchCancel, id=200)

        self.toolbar.Realize()

        self.status = wx.StatusBar(self, -1)
        self.SetStatusBar(self.status)

        self.doc_panel = ManualPanel(self)
        self.doc_panel.getPage("Intro")

        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(100, "Run Example\tCtrl+R")
        menu1.AppendSeparator()
        if DOC_AS_SINGLE_APP:
            menu1.Append(wx.ID_EXIT, "Quit\tCtrl+Q")
        else:
            menu1.Append(99, "Close\tCtrl+W")
        self.menuBar.Append(menu1, 'Action')

        menu2 = wx.Menu()
        menu2.Append(101, "Copy\tCtrl+C")
        self.menuBar.Append(menu2, 'Text')

        self.SetMenuBar(self.menuBar)

        self.Bind(wx.EVT_MENU, self.onRun, id=100)
        self.Bind(wx.EVT_MENU, self.copy, id=101)
        if DOC_AS_SINGLE_APP:
            self.Bind(wx.EVT_MENU, self.quit, id=wx.ID_EXIT)
            self.Bind(wx.EVT_CLOSE, self.quit)
        else:
            self.Bind(wx.EVT_MENU, self.close, id=99)
            self.Bind(wx.EVT_CLOSE, self.close)

    def setSearchFocus(self, evt):
        self.search.SetFocus()

    def onSearchEnter(self, evt):
        self.doc_panel.GetTreeCtrl().SetFocus()

    def onSearch(self, evt):
        if self.searchTimer != None:
            self.searchTimer.Stop()
        self.searchTimer = wx.CallLater(200, self.doSearch)

    def doSearch(self):
        keyword = self.search.GetValue()
        if keyword == "":
            self.doc_panel.parse()
        else:
            if self.searchScope == "Object's Name":
                self.doc_panel.parseOnSearchName(keyword)
            else:
                self.doc_panel.parseOnSearchPage(keyword)
        self.searchTimer = None

    def onSearchCancel(self, evt):
        self.search.SetValue("")

    def onSearchScope(self, evt):
        id = evt.GetId()
        if id == 10:
            self.searchScope = "Object's Name"
        else:
            self.searchScope = "Manual Pages"

    def copy(self, evt):
        self.doc_panel.copy()

    def quit(self, evt):
        self.Destroy()

    def close(self, evt):
        self.Hide()

    def setTitle(self, page):
        self.SetTitle('Pyo Documentation - %s' % page)

    def history_check(self, back, forward):
        self.toolbar.EnableTool(wx.ID_BACKWARD, back)
        self.toolbar.EnableTool(wx.ID_FORWARD, forward)

    def onBack(self, evt):
        self.doc_panel.history_back()

    def onForward(self, evt):
        self.doc_panel.history_forward()

    def onHome(self, evt):
        search = self.search.GetValue()
        if search != "":
            self.search.SetValue("")
        self.doc_panel.getPage("Intro")
        self.doc_panel.collapseAll()

    def onRun(self, evt):
        obj = self.doc_panel.GetPageText(self.doc_panel.GetSelection())
        self.status.SetStatusText('Running "%s" example...' % obj, 0)
        text = self.doc_panel.getExampleScript()
        with open(DOC_EXAMPLE_PATH, "w") as f:
            f.write(text)
        th = RunningThread(DOC_EXAMPLE_PATH, TEMP_PATH, self.which_python, self.osx_app_bundled, self.caller_need_to_invoke_32_bit, self.set_32_bit_arch)
        th.start()
        wx.FutureCall(8000, self.status.SetStatusText, "", 0)

class RunningThread(threading.Thread):
    def __init__(self, path, cwd, which_python, osx_app_bundled, caller_need_to_invoke_32_bit, set_32_bit_arch):
        threading.Thread.__init__(self)
        self.path = path
        self.cwd = cwd
        self.which_python = which_python
        self.osx_app_bundled = osx_app_bundled
        self.caller_need_to_invoke_32_bit = caller_need_to_invoke_32_bit
        self.set_32_bit_arch = set_32_bit_arch
        self.terminated = False

    def kill(self):
        self.terminated = True
        self.proc.terminate()

    def run(self):
        if self.osx_app_bundled:
            vars_to_remove = "PYTHONHOME PYTHONPATH EXECUTABLEPATH RESOURCEPATH ARGVZERO PYTHONOPTIMIZE"
            prelude = "export -n %s;export PATH=/usr/local/bin:/usr/local/lib:$PATH;env;" % vars_to_remove
            if self.caller_need_to_invoke_32_bit:
                self.proc = subprocess.Popen(["%s%s%s %s" % (prelude, self.set_32_bit_arch, self.which_python, self.path)], 
                                shell=True, cwd=self.cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            else:
                self.proc = subprocess.Popen(["%s%s %s" % (prelude, self.which_python, self.path)], cwd=self.cwd, 
                                    shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        elif wx.Platform == '__WXMAC__':
            if self.caller_need_to_invoke_32_bit:
                self.proc = subprocess.Popen(["%s%s %s" % (self.set_32_bit_arch, self.which_python, self.path)], 
                                shell=True, cwd=self.cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            else:
                self.proc = subprocess.Popen(["%s %s" % (self.which_python, self.path)], cwd=self.cwd, 
                                shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        elif wx.Platform == "__WXMSW__":
                self.proc = subprocess.Popen([self.which_python, self.path], cwd=self.cwd, 
                                shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        else:
                self.proc = subprocess.Popen([self.which_python, self.path], cwd=self.cwd, 
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)


        while self.proc.poll() == None and not self.terminated:
            time.sleep(.25)

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

if __name__ == "__main__":
    DOC_AS_SINGLE_APP = True
    app = wx.PySimpleApp()
    doc_frame = ManualFrame()
    doc_frame.Show()
    app.MainLoop()
