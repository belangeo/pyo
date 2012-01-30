#!/usr/bin/env python
# encoding: utf-8
import subprocess
import wx
import wx.stc  as  stc
from pyo import *

TEMP_PATH = os.path.join(os.path.expanduser('~'), '.pyoed')
if not os.path.isdir(TEMP_PATH):
    os.mkdir(TEMP_PATH)
DOC_PATH = os.path.join(TEMP_PATH, 'doc')
EXAMPLE_PATH = os.path.join(TEMP_PATH, 'manual_example.py')

STYLES = {'Default': {'default': '#000000', 'comment': '#007F7F', 'commentblock': '#7F7F7F', 'selback': '#CCCCCC',
                    'number': '#005000', 'string': '#7F007F', 'triple': '#7F0000', 'keyword': '#00007F', 'keyword2': '#007F9F',
                    'class': '#0000FF', 'function': '#007F7F', 'identifier': '#000000', 'caret': '#00007E',
                    'background': '#EEEEEE', 'linenumber': '#000000', 'marginback': '#B0B0B0', 'markerfg': '#CCCCCC',
                      'markerbg': '#000000', 'bracelight': '#AABBDD', 'bracebad': '#DD0000', 'lineedge': '#CCCCCC'}}

if wx.Platform == '__WXMSW__':
  faces2 = {'face': 'Courier', 'size' : 10, 'size2': 8}
elif wx.Platform == '__WXMAC__':
  faces2 = {'face': 'Monaco', 'size' : 12, 'size2': 9}
else:
  faces2 = {'face': 'Courier New', 'size' : 8, 'size2': 7}
faces2['size3'] = faces2['size2'] + 4
for key, value in STYLES['Default'].items():
  faces2[key] = value

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
PyoObject : Base class for all pyo objects that manipulate vectors of samples.
PyoTableObject : Base class for all pyo table objects.
PyoMatrixObject : Base class for all pyo matrix objects.
functions : Miscellaneous functions.

""" % PYO_VERSION

_DOC_KEYWORDS = ['Attributes', 'Examples', 'Parameters', 'Methods', 'Notes', 'Methods details', 
                 'See also', 'Parentclass', 'Overview', 'Initline', 'Description']
_HEADERS = ["Server", "PyoObject", "PyoTableObject", "PyoMatrixObject", "Map", "Stream", "TableStream", "functions"]
_KEYWORDS_LIST = []
_KEYWORDS_LIST.extend(_HEADERS)
_KEYWORDS_LIST.append("SLMap")
_NUM_PAGES = 1
_NUM_PAGES += len(_HEADERS)
for k1 in _HEADERS:
    if type(OBJECTS_TREE[k1]) == type({}):
        _NUM_PAGES += len(OBJECTS_TREE[k1].keys())
        for k2 in OBJECTS_TREE[k1].keys():
            _KEYWORDS_LIST.extend(OBJECTS_TREE[k1][k2])
            _NUM_PAGES += len(OBJECTS_TREE[k1][k2])
    else:
        _KEYWORDS_LIST.extend(OBJECTS_TREE[k1])
        _NUM_PAGES += len(OBJECTS_TREE[k1])

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

    editor.StyleSetSpec(stc.STC_STYLE_DEFAULT,  "fore:%(default)s,face:%(face)s,size:%(size)d,back:%(background)s" % faces2)
    editor.StyleClearAll()

    editor.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "fore:%(default)s,face:%(face)s,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_STYLE_LINENUMBER,  "fore:%(linenumber)s,back:%(marginback)s,face:%(face)s,size:%(size2)d" % faces2)
    editor.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "fore:%(default)s,face:%(face)s" % faces2)
    editor.StyleSetSpec(stc.STC_P_DEFAULT, "fore:%(default)s,face:%(face)s,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_COMMENTLINE, "fore:%(comment)s,face:%(face)s,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_NUMBER, "fore:%(number)s,face:%(face)s,bold,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_STRING, "fore:%(string)s,face:%(face)s,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_CHARACTER, "fore:%(string)s,face:%(face)s,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_WORD, "fore:%(keyword)s,face:%(face)s,bold,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_WORD2, "fore:%(keyword2)s,face:%(face)s,bold,size:%(size3)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_TRIPLE, "fore:%(triple)s,face:%(face)s,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, "fore:%(triple)s,face:%(face)s,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_CLASSNAME, "fore:%(class)s,face:%(face)s,bold,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_DEFNAME, "fore:%(function)s,face:%(face)s,bold,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_OPERATOR, "bold,size:%(size)d,face:%(face)s" % faces2)
    editor.StyleSetSpec(stc.STC_P_IDENTIFIER, "fore:%(identifier)s,face:%(face)s,size:%(size)d" % faces2)
    editor.StyleSetSpec(stc.STC_P_COMMENTBLOCK, "fore:%(commentblock)s,face:%(face)s,size:%(size)d" % faces2)
    editor.SetCaretForeground(faces2["background"])

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
        wx.Treebook.__init__(self, parent, -1, style=wx.BK_DEFAULT)
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
                if key == "PyoObject":
                    count += 1
                    head = "PyoObj - "
                    win = self.makePanel("PyoObject")
                    self.AddPage(win, "PyoObject")
                    for key2 in sorted(OBJECTS_TREE[key]):
                        count += 1
                        win = self.makePanel("%s" % key2)
                        self.AddPage(win, "PyoObj - %s" % key2)
                        for obj in OBJECTS_TREE[key][key2]:
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
                if key == "PyoObject":
                    head = "PyoObj - "
                    if keyword in "pyoobject":
                        win = self.makePanel("PyoObject")
                        self.AddPage(win, "PyoObject")
                    for key2 in sorted(OBJECTS_TREE[key]):
                        objs = []
                        for obj in OBJECTS_TREE[key][key2]:
                            if keyword in obj.lower():
                                objs.append(obj)
                        if objs != []:
                            win = self.makePanel("%s" % key2)
                            self.AddPage(win, "PyoObj - %s" % key2)
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
                if key == "PyoObject":
                    head = "PyoObj - "
                    with open(os.path.join(DOC_PATH, "PyoObject"), "r") as f:
                        text = f.read().lower()
                    if keyword in text:
                        win = self.makePanel("PyoObject")
                        self.AddPage(win, "PyoObject")
                    for key2 in sorted(OBJECTS_TREE[key]):
                        objs = []
                        for obj in OBJECTS_TREE[key][key2]:
                            with open(os.path.join(DOC_PATH, obj), "r") as f:
                                text = f.read().lower()
                            if keyword in text:
                                objs.append(obj)
                        if objs != []:
                            win = self.makePanel("%s" % key2)
                            self.AddPage(win, "PyoObj - %s" % key2)
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
                    args = '\nInitline:\n\n' + class_args(eval(obj)) + '\n\nDescription:\n'
                    isAnObject = True
                except:
                    args = '\nDescription:\n'
                    if obj in OBJECTS_TREE["functions"]:
                        isAnObject = True
                    else:
                        isAnObject = False
                if isAnObject:
                    try:
                        text = eval(obj).__doc__
                        text_form = last_line = ""
                        inside_examples = False
                        for line in text.splitlines():
                            if inside_examples and line.strip() == "":
                                if obj not in OBJECTS_TREE["functions"]:
                                    text_form += "s.gui(locals())"
                                inside_examples = False
                            if '>>>' in line or '...' in line:
                                l = line[8:]
                                if l.strip() != "":
                                    text_form += l + '\n'
                            else:
                                if line.startswith("    "):
                                    text_form += line[4:].rstrip() + '\n'
                                else:
                                    text_form += line.rstrip() + '\n'
                            if 'Examples' in last_line:
                                text_form += "from pyo import *\n"
                                inside_examples = True
                            last_line = line
                        methods = self.getMethodsDoc(text, obj)
                        panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600))
                        panel.win.SetText(args + text_form + methods)
                    except:
                        panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600))
                        panel.win.SetText(args + "\nnot documented yet...\n\n")
                else:
                    try:
                        text = eval(obj).__doc__
                    except:
                        if obj == "functions":
                            text = "Miscellaneous functions...\n\n"
                            text += "\nOverview:\n"
                            for o in OBJECTS_TREE["functions"]:
                                text += o + ": " + self.getDocFirstLine(o)
                        else:
                            text = "\nnot documented yet...\n\n"
                    if obj in OBJECTS_TREE["PyoObject"].keys():
                        text += "\nOverview:\n"
                        for o in OBJECTS_TREE["PyoObject"][obj]:
                            text += o + ": " + self.getDocFirstLine(o)
                        obj = "PyoObj - " + obj
                    panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600))
                    panel.win.SetText(text)
            else:
                panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600))
                panel.win.SetText(_INTRO_TEXT)

            panel.win.SaveFile(os.path.join(DOC_PATH, obj))
        return panel

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
                    panel.win = stc.StyledTextCtrl(panel, -1, size=panel.GetSize())
                    panel.win.LoadFile(os.path.join(DOC_PATH, word))
                    panel.win.SetMarginWidth(1, 0)
                    panel.win.SetReadOnly(True)
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

    def getMethodsDoc(self, text, obj):
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
                        args = inspect.getargspec(getattr(eval(obj), meth))
                        args = inspect.formatargspec(*args)
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
        text = stc.GetTextRange(stc.PositionFromLine(start), stc.PositionFromLine(end))
        return text

    def setStyle(self):
        tree = self.GetTreeCtrl()
        tree.SetBackgroundColour(STYLES['Default']['background'])
        root = tree.GetRootItem()
        tree.SetItemTextColour(root, STYLES['Default']['identifier'])
        (child, cookie) = tree.GetFirstChild(root)
        while child.IsOk():
            tree.SetItemTextColour(child, STYLES['Default']['identifier'])
            if tree.ItemHasChildren(child):
                (child2, cookie2) = tree.GetFirstChild(child)
                while child2.IsOk():
                    tree.SetItemTextColour(child2, STYLES['Default']['identifier'])
                    (child2, cookie2) = tree.GetNextChild(child, cookie2)
            (child, cookie) = tree.GetNextChild(root, cookie)

gosearchID = 1000
class ManualFrame(wx.Frame):
    def __init__(self, parent=None, id=-1, title='Pyo Documentation', size=(940, 700)):
        wx.Frame.__init__(self, parent=parent, id=id, title=title, size=size)
        self.SetMinSize((600, -1))

        aTable = wx.AcceleratorTable([(wx.ACCEL_NORMAL, 47, gosearchID)])
        self.SetAcceleratorTable(aTable)
        self.Bind(wx.EVT_MENU, self.setSearchFocus, id=gosearchID)

        self.toolbar = self.CreateToolBar()
        self.toolbar.SetToolBitmapSize((16,16))  # sets icon size

        # Use wx.ArtProvider for default icons
        back_ico = wx.ArtProvider.GetBitmap(wx.ART_GO_BACK, wx.ART_FRAME_ICON, (16,16))
        backTool = self.toolbar.AddSimpleTool(wx.ID_BACKWARD, back_ico, "Back")
        self.toolbar.EnableTool(wx.ID_BACKWARD, False)
        self.Bind(wx.EVT_MENU, self.onBack, backTool)

        self.toolbar.AddSeparator()

        forward_ico = wx.ArtProvider.GetBitmap(wx.ART_GO_FORWARD, wx.ART_FRAME_ICON, (16,16))
        forwardTool = self.toolbar.AddSimpleTool(wx.ID_FORWARD, forward_ico, "Forward")
        self.toolbar.EnableTool(wx.ID_FORWARD, False)
        self.Bind(wx.EVT_MENU, self.onForward, forwardTool)

        self.toolbar.AddSeparator()

        home_ico = wx.ArtProvider.GetBitmap(wx.ART_GO_HOME, wx.ART_FRAME_ICON, (16,16))
        homeTool = self.toolbar.AddSimpleTool(wx.ID_HOME, home_ico, "Go Home")
        self.toolbar.EnableTool(wx.ID_HOME, True)
        self.Bind(wx.EVT_MENU, self.onHome, homeTool)

        self.toolbar.AddSeparator()

        exec_ico = wx.ArtProvider.GetBitmap(wx.ART_EXECUTABLE_FILE, wx.ART_FRAME_ICON, (16,16))
        execTool = self.toolbar.AddSimpleTool(wx.ID_PREVIEW, exec_ico, "Run Example")
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

        self.search = wx.SearchCtrl(self.toolbar, 200, size=(200,-1))
        self.search.ShowCancelButton(True)
        self.search.SetMenu(self.searchMenu)
        self.toolbar.AddControl(self.search)
        self.Bind(wx.EVT_TEXT, self.onSearch, id=200)
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
        menu1.Append(wx.ID_EXIT, "Quit\tCtrl+Q")
        self.menuBar.Append(menu1, 'Action')

        menu2 = wx.Menu()
        menu2.Append(101, "Copy\tCtrl+C")
        self.menuBar.Append(menu2, 'Text')

        self.SetMenuBar(self.menuBar)

        self.Bind(wx.EVT_MENU, self.onRun, id=100)
        self.Bind(wx.EVT_MENU, self.copy, id=101)
        self.Bind(wx.EVT_MENU, self.quit, id=wx.ID_EXIT)
        self.Bind(wx.EVT_CLOSE, self.quit)

    def setSearchFocus(self, evt):
        self.search.SetFocus()

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
        with open(EXAMPLE_PATH, "w") as f:
            f.write(text)
        pid = subprocess.Popen(["python", EXAMPLE_PATH], cwd=TEMP_PATH, shell=False).pid
        wx.FutureCall(8000, self.status.SetStatusText, "", 0)

if __name__ == "__main__":
    app = wx.PySimpleApp()
    doc_frame = ManualFrame()
    doc_frame.Show()
    app.MainLoop()
