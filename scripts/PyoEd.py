#! /usr/bin/env python
# encoding: utf-8
"""
PyoEd is a simple text editor especially configured to edit pyo audio programs.

You can do absolutely everything you want with this piece of software.

Olivier Belanger - 2012

"""

import sys, os, string, inspect, keyword, wx, time
import wx.stc  as  stc
from subprocess import Popen, PIPE
import wx.aui
from pyo import OBJECTS_TREE

NAME = 'PyoEd'
VERSION = '0.1.0'

TEMP_PATH = os.path.join(os.path.expanduser('~'), '.pyoed')
if not os.path.isdir(TEMP_PATH):
    os.mkdir(TEMP_PATH)

# Bitstream Vera Sans Mono, Corbel, Monaco, Envy Code R, MonteCarlo, Courier New
conf = {"preferedStyle": "Espresso"}
STYLES = {'Default': {'default': '#000000', 'comment': '#007F7F', 'commentblock': '#7F7F7F', 'selback': '#CCCCCC',
                    'number': '#005000', 'string': '#7F007F', 'triple': '#7F0000', 'keyword': '#00007F',
                    'class': '#0000FF', 'function': '#007F7F', 'identifier': '#000000', 'caret': '#00007E',
                    'background': '#FFFFFF', 'linenumber': '#000000', 'marginback': '#B0B0B0', 'markerfg': '#CCCCCC',
                      'markerbg': '#000000', 'bracelight': '#AABBDD', 'bracebad': '#DD0000'},

           'Custom': {'default': '#FFFFFF', 'comment': '#9FFF9F', 'commentblock': '#7F7F7F', 'selback': '#333333',
                      'number': '#90CB43', 'string': '#FF47D7', 'triple': '#FF3300', 'keyword': '#4A94FF',
                      'class': '#4AF3FF', 'function': '#00E0B6', 'identifier': '#FFFFFF', 'caret': '#DDDDDD',
                      'background': '#000000', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                      'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000'},

            'Soft': {'default': '#000000', 'comment': '#444444', 'commentblock': '#7F7F7F', 'selback': '#CBCBCB',
                     'number': '#222222', 'string': '#272727', 'triple': '#333333', 'keyword': '#000000',
                     'class': '#666666', 'function': '#555555', 'identifier': '#000000', 'caret': '#222222',
                     'background': '#EFEFEF', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                     'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000'},

            'Smooth': {'default': '#FFFFFF', 'comment': '#DD0000', 'commentblock': '#AF0000', 'selback': '#555555',
                       'number': '#FFFFFF', 'string': '#00EE00', 'triple': '#00AA00', 'keyword': '#9999FF',
                       'class': '#00FFA2', 'function': '#00FFD5', 'identifier': '#CCCCCC', 'caret': '#EEEEEE',
                       'background': '#222222', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                       'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000'},

            'Espresso': {'default': '#BDAE9C', 'comment': '#0066FF', 'commentblock': '#0044DD', 'selback': '#5D544F',
                         'number': '#44AA43', 'string': '#2FE420', 'triple': '#049B0A', 'keyword': '#43A8ED',
                         'class': '#6D79DE', 'function': '#7290D9', 'identifier': '#BDAE9C', 'caret': '#DDDDDD',
                         'background': '#2A211C', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                         'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000'}
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

class MainFrame(wx.Frame):
    def __init__(self, parent, ID, title, pos=wx.DefaultPosition, size=wx.DefaultSize, style=wx.DEFAULT_FRAME_STYLE):
        wx.Frame.__init__(self, parent, ID, title, pos, size, style)

        self.panel = MainPanel(self, size=size)

        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(110, "New\tCtrl+N")
        menu1.Append(100, "Open\tCtrl+O")
        menu1.Append(112, "Open Project\tShift+Ctrl+O")
        self.submenu2 = wx.Menu()
        subId2 = 2000
        recentFiles = []
        filename = os.path.join(TEMP_PATH,'.recent.txt')
        if os.path.isfile(filename):
            f = open(filename, "r")
            for line in f.readlines():
                recentFiles.append(line)
            f.close()
        if recentFiles:
            for file in recentFiles:
                self.submenu2.Append(subId2, file)
                subId2 += 1
        menu1.AppendMenu(998, "Open Recent...", self.submenu2)
        menu1.InsertSeparator(4)
        menu1.Append(101, "Save\tCtrl+S")
        menu1.Append(102, "Save As...\tShift+Ctrl+S")
        menu1.Append(111, "Close\tCtrl+W")
        menu1.InsertSeparator(8)
        prefItem = menu1.Append(wx.ID_PREFERENCES, "Preferences...\tCtrl+;")
        menu1.InsertSeparator(10)
        menu1.Append(129, "Quit\tCtrl+Q")
        self.menuBar.Append(menu1, 'File')

        menu2 = wx.Menu()
        menu2.Append(103, "Collapse/Expand\tShift+Ctrl+F")
        menu2.Append(108, "Un/Comment Selection\tCtrl+J")
        menu2.Append(114, "Show AutoCompletion\tCtrl+K")
        menu2.Append(121, "Insert File Path...\tCtrl+L")
        menu2.Append(122, "Find...\tCtrl+F")
        self.menuBar.Append(menu2, 'Code')

        menu3 = wx.Menu()
        menu3.Append(104, "Run\tCtrl+R")
        self.menuBar.Append(menu3, 'Process')

        menu5 = wx.Menu()
        stId = 500
        for st in styles:
            menu5.Append(stId, st, "", wx.ITEM_RADIO)
            if st == conf['preferedStyle']: menu5.Check(stId, True)
            stId += 1
        self.menuBar.Append(menu5, 'Styles')

        menu = wx.Menu()
        helpItem = menu.Append(wx.ID_ABOUT, '&About %s %s' % (NAME, VERSION), 'wxPython RULES!!!')
        self.menuBar.Append(menu, '&Help')

        self.SetMenuBar(self.menuBar)

        self.Bind(wx.EVT_MENU, self.new, id=110)
        self.Bind(wx.EVT_MENU, self.open, id=100)
        self.Bind(wx.EVT_MENU, self.openProject, id=112)
        self.Bind(wx.EVT_MENU, self.save, id=101)
        self.Bind(wx.EVT_MENU, self.saveas, id=102)
        self.Bind(wx.EVT_MENU, self.delete, id=111)
        self.Bind(wx.EVT_MENU, self.fold, id=103)
        self.Bind(wx.EVT_MENU, self.runner, id=104)
        self.Bind(wx.EVT_MENU, self.onHelpAbout, helpItem)
        self.Bind(wx.EVT_MENU, self.OnComment, id=108)
        self.Bind(wx.EVT_MENU, self.openPrefs, prefItem)

        if subId2 > 2000:
            for i in range(2000,subId2):
                self.Bind(wx.EVT_MENU, self.openRecent, id=i)
        for i in range(500, stId):
            self.Bind(wx.EVT_MENU, self.changeStyle, id=i)
        self.Bind(wx.EVT_MENU, self.autoComp, id=114)
        self.Bind(wx.EVT_MENU, self.insertPath, id=121)
        self.Bind(wx.EVT_MENU, self.showFind, id=122)

        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.Bind(wx.EVT_MENU, self.OnClose, id=129)

        if projectsToOpen:
            for p in projectsToOpen:
                self.panel.project.loadProject(p)
                sys.path.append(p)

        if filesToOpen:
            for f in filesToOpen:
                self.panel.addPage(f)

    ### Editor functions ###
    def OnComment(self, evt):
        self.panel.editor.OnComment()

    def fold(self, event):
        self.panel.editor.FoldAll()

    def autoComp(self, evt):
        win = self.FindFocus()
        try:
            win.showAutoComp()
        except AttributeError:
            pass

    def showFind(self, evt):
        self.panel.editor.OnShowFindReplace()

    def insertPath(self, evt):
        dlg = wx.FileDialog(self, message="Choose a file", defaultDir=os.getcwd(),
                            defaultFile="", style=wx.OPEN | wx.MULTIPLE)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPaths()
            text = str(path[0])
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
        #self.panel.project.setStyle()

    ### Open Prefs ang Logs ###
    def openPrefs(self, evt):
        pass

    ### New / Open / Save / Delete ###
    def new(self, event):
        self.panel.addNewPage()

    def newRecent(self, file):
        filename = os.path.join(TEMP_PATH,'.recent.txt')
        try:
            f = open(filename, "r")
            lines = [line[:-1] for line in f.readlines()]
            f.close()
        except:
            lines = []
        if not file in lines:
            f = open(filename, "w")
            lines.insert(0, file)
            if len(lines) > 10:
                lines = lines[0:10]
            for line in lines:
                f.write(line + '\n')
            f.close()

        subId2 = 2000
        recentFiles = []
        f = open(filename, "r")
        for line in f.readlines():
            recentFiles.append(line)
        f.close()
        if recentFiles:
            for item in self.submenu2.GetMenuItems():
                self.submenu2.DeleteItem(item)
            for file in recentFiles:
                self.submenu2.Append(subId2, file)
                subId2 += 1

    def open(self, event):
        dlg = wx.FileDialog(self, message="Choose a file", defaultDir=os.getcwd(),
            defaultFile="", style=wx.OPEN | wx.MULTIPLE)

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPaths()
            for file in path:
                self.panel.addPage(file)
                self.newRecent(file)
        dlg.Destroy()

    def openProject(self, event):
        dlg = wx.DirDialog(self, message="Choose a project folder", defaultPath=os.getcwd(),
                           style=wx.DD_DEFAULT_STYLE)

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.folder = path
            self.panel.project.loadProject(self.folder)
            sys.path.append(path)
        dlg.Destroy()

    def openRecent(self, event):
        menu = self.GetMenuBar()
        id = event.GetId()
        file = menu.FindItemById(id).GetLabel()
        self.panel.addPage(file[:-1])

    def save(self, event):
        if not self.panel.editor.path:
            self.saveas(None)
        else:
            self.panel.editor.saveMyFile(self.panel.editor.path)
            self.SetTitle(self.panel.editor.path)

    def saveas(self, event):
        dlg = wx.FileDialog(self, message="Save file as ...", defaultDir=os.path.expanduser('~'),
            defaultFile="", style=wx.SAVE)
        dlg.SetFilterIndex(0)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.panel.editor.path = path
            self.panel.editor.setStyle()
            self.panel.editor.SetCurrentPos(0)
            self.panel.editor.AddText(" ")
            self.panel.editor.DeleteBackNotLine()
            self.panel.editor.saveMyFile(path)
            self.SetTitle(self.panel.editor.path)
            self.panel.notebook.SetPageText(self.panel.notebook.GetSelection(), os.path.split(path)[1])
            self.newRecent(path)
        dlg.Destroy()

    def delete(self, event):
        action = self.panel.editor.close()
        if action == 'delete':
            self.panel.deletePage()
        else:
            pass

    ### Run actions ###
    def runner(self, event):
        # Need to determine which python to use...
        path = self.panel.editor.path
        if os.path.isfile(path):
            cwd = os.path.split(path)[0]
            pid = Popen(["python", self.panel.editor.path], cwd=cwd).pid

    ### About ###
    def onHelpAbout(self, evt):
        info = wx.AboutDialogInfo()
        info.Name = NAME
        info.Version = VERSION
        info.Copyright = u"(C) 2012 Olivier Bélanger"
        info.Description = "PyoEd is a simple text editor especially configured to edit pyo audio programs.\n\n"
        wx.AboutBox(info)

    def OnClose(self, event):
        self.panel.OnQuit()
        self.Destroy()

class MainPanel(wx.Panel):
    def __init__(self, parent, size=(1200,800), style=wx.SUNKEN_BORDER):
        wx.Panel.__init__(self, parent, size=(1200,800), style=wx.SUNKEN_BORDER)

        self.mainFrame = parent
        mainBox = wx.BoxSizer(wx.HORIZONTAL)

        self.notebook = MyNotebook(self)
        self.editor = Editor(self.notebook, -1, size=(0, -1), setTitle=self.SetTitle, getTitle=self.GetTitle)
        mainBox.Add(self.notebook, 1, wx.EXPAND)
        self.SetSizer(mainBox)

        self.Bind(wx.aui.EVT_AUINOTEBOOK_PAGE_CHANGED, self.onPageChange)
        self.Bind(wx.aui.EVT_AUINOTEBOOK_PAGE_CLOSE, self.onClosePage)

    def addNewPage(self):
        editor = Editor(self.notebook, -1, size=(0, -1), setTitle=self.SetTitle, getTitle=self.GetTitle)
        self.notebook.AddPage(editor, "Untitled", True)
        self.editor = editor

    def addPage(self, file):
        editor = Editor(self.notebook, -1, size=(0, -1), setTitle=self.SetTitle, getTitle=self.GetTitle)
        label = os.path.split(file)[1].split('.')[0]
        self.notebook.AddPage(editor, label, True)
        editor.LoadFile(file)
        editor.path = file
        editor.setStyle()
        self.editor = editor
        self.SetTitle(file)

    def onClosePage(self, evt):
        ed = self.notebook.GetPage(self.notebook.GetSelection())
        ed.Close()

    def deletePage(self):
        ed = self.notebook.GetPage(self.notebook.GetSelection())
        self.notebook.DeletePage(self.notebook.GetSelection())

    def onPageChange(self, event):
        self.editor = self.notebook.GetPage(self.notebook.GetSelection())
        if not self.editor.path:
            if self.editor.GetModify():
                self.SetTitle("*** PyoEd Editor ***")
            else:
                self.SetTitle("PyoEd Editor")
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
    def __init__(self, parent, ID, pos=wx.DefaultPosition, size=wx.DefaultSize, style= wx.NO_BORDER,
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

        self.alphaStr = string.lowercase + string.uppercase + '0123456789'

        self.Colourise(0, -1)
        self.SetCurrentPos(0)

        self.SetIndent(4)
        self.SetBackSpaceUnIndents(True)
        self.SetTabIndents(True)
        self.SetTabWidth(4)
        self.SetUseTabs(False)
        self.SetViewWhiteSpace(False)

        self.SetEOLMode(wx.stc.STC_EOL_LF)
        self.SetViewEOL(False)

        self.SetProperty("fold", "1")
        self.SetProperty("tab.timmy.whinge.level", "1")
        self.SetMargins(5,5)
        self.SetUseAntiAliasing(True)
        self.SetEdgeMode(stc.STC_EDGE_BACKGROUND)
        self.SetEdgeColumn(1000)

        self.SetMarginType(1, stc.STC_MARGIN_NUMBER)
        self.SetMarginWidth(1, 28)
        self.SetMarginType(2, stc.STC_MARGIN_SYMBOL)
        self.SetMarginMask(2, stc.STC_MASK_FOLDERS)
        self.SetMarginSensitive(2, True)
        self.SetMarginWidth(2, 12)

        self.CmdKeyAssign(ord('B'), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMIN)
        self.CmdKeyAssign(ord('N'), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMOUT)
        self.CmdKeyAssign(ord('Z'), stc.STC_SCMOD_SHIFT+stc.STC_SCMOD_CTRL, stc.STC_CMD_REDO)

        self.Bind(stc.EVT_STC_UPDATEUI, self.OnUpdateUI)
        self.Bind(stc.EVT_STC_MARGINCLICK, self.OnMarginClick)
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.Bind(wx.EVT_FIND, self.OnFind)
        self.Bind(wx.EVT_FIND_NEXT, self.OnFind)
        self.Bind(wx.EVT_FIND_REPLACE, self.OnFind)
        self.Bind(wx.EVT_FIND_REPLACE_ALL, self.OnFind)
        self.Bind(wx.EVT_FIND_CLOSE, self.OnFindClose)

        tree = OBJECTS_TREE
        self.wordlist = []
        for k1 in tree.keys():
            if type(tree[k1]) == type({}):
                for k2 in tree[k1].keys():
                    for val in tree[k1][k2]:
                        self.wordlist.append(val)
            else:
                for val in tree[k1]:
                    self.wordlist.append(val)
        self.wordlist.append("PyoObject")
        self.wordlist.append("PyoTableObject")
        self.wordlist.append("PyoMatrixObject")
        self.wordlist.append("Server")

        self.EmptyUndoBuffer()
        self.SetFocus()
        self.setStyle()

        wx.CallAfter(self.SetAnchor, 0)
        self.Refresh()

    def setStyle(self):
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPEN,    stc.STC_MARK_BOXMINUS, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDER,        stc.STC_MARK_BOXPLUS, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERSUB,     stc.STC_MARK_VLINE, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERTAIL,    stc.STC_MARK_LCORNERCURVE, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEREND,     stc.STC_MARK_ARROW, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPENMID, stc.STC_MARK_ARROWDOWN, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERMIDTAIL, stc.STC_MARK_LCORNERCURVE, faces['markerfg'], faces['markerbg'])

        # Global default styles for all languages
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "fore:%(default)s,face:%(face)s,size:%(size)d,back:%(background)s" % faces)
        self.StyleClearAll()  # Reset all to be like the default

        self.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "fore:%(default)s,face:%(face)s,size:%(size)d" % faces)
        self.StyleSetSpec(stc.STC_STYLE_LINENUMBER,  "fore:%(linenumber)s,back:%(marginback)s,face:%(face)s,size:%(size2)d" % faces)
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "fore:%(default)s,face:%(face)s" % faces)
        self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT,  "fore:#000000,back:%(bracelight)s,bold" % faces)
        self.StyleSetSpec(stc.STC_STYLE_BRACEBAD,    "fore:#000000,back:%(bracebad)s,bold" % faces)

        self.SetLexer(stc.STC_LEX_PYTHON)
        self.SetKeyWords(0, " ".join(keyword.kwlist) + " None True False " + " ".join(self.wordlist))

        # Default
        self.StyleSetSpec(stc.STC_P_DEFAULT, "fore:%(default)s,face:%(face)s,size:%(size)d" % faces)
        # Comments
        self.StyleSetSpec(stc.STC_P_COMMENTLINE, "fore:%(comment)s,face:%(face)s,size:%(size)d" % faces)
        # Number
        self.StyleSetSpec(stc.STC_P_NUMBER, "fore:%(number)s,face:%(face)s,bold,size:%(size)d" % faces)
        # String
        self.StyleSetSpec(stc.STC_P_STRING, "fore:%(string)s,face:%(face)s,size:%(size)d" % faces)
        # Single quoted string
        self.StyleSetSpec(stc.STC_P_CHARACTER, "fore:%(string)s,face:%(face)s,size:%(size)d" % faces)
        # Keyword
        self.StyleSetSpec(stc.STC_P_WORD, "fore:%(keyword)s,face:%(face)s,bold,size:%(size)d" % faces)
        # Triple quotes
        self.StyleSetSpec(stc.STC_P_TRIPLE, "fore:%(triple)s,face:%(face)s,size:%(size)d" % faces)
        # Triple double quotes
        self.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, "fore:%(triple)s,face:%(face)s,size:%(size)d" % faces)
        # Class name definition
        self.StyleSetSpec(stc.STC_P_CLASSNAME, "fore:%(class)s,face:%(face)s,bold,size:%(size)d" % faces)
        # Function or method name definition
        self.StyleSetSpec(stc.STC_P_DEFNAME, "fore:%(function)s,face:%(face)s,bold,size:%(size)d" % faces)
        # Operators
        self.StyleSetSpec(stc.STC_P_OPERATOR, "bold,size:%(size)d,face:%(face)s" % faces)
        # Identifiers
        self.StyleSetSpec(stc.STC_P_IDENTIFIER, "fore:%(identifier)s,face:%(face)s,size:%(size)d" % faces)
        # Comment-blocks
        self.StyleSetSpec(stc.STC_P_COMMENTBLOCK, "fore:%(commentblock)s,face:%(face)s,size:%(size)d" % faces)

        self.SetCaretForeground(faces['caret'])
        self.SetSelBackground(1, faces['selback'])

    ### Find and Replace ###
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

    ### Save and Close file ###
    def saveMyFile(self, file):
        self.SaveFile(file)
        self.path = file
        self.saveMark = False

    def close(self):
        if self.GetModify():
            if not self.path: f = "Untitled"
            else: f = self.path
            dlg = wx.MessageDialog(None, 'file ' + f + ' has been modified. Do you want to save?', 'Warning!', wx.YES | wx.NO | wx.CANCEL)
            but = dlg.ShowModal()
            if but == wx.ID_YES:
                dlg.Destroy()
                if not self.path:
                    dlg2 = wx.FileDialog(None, message="Save file as ...", defaultDir=os.getcwd(), defaultFile="", style=wx.SAVE)
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
            else: f = self.path
            dlg = wx.MessageDialog(None, 'file ' + f + ' has been modified. Do you want to save?', 'Warning!', wx.YES | wx.NO)
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
        if self.GetModify() and not self.saveMark:
            title = self.getTitle()
            str = '*** ' + title + ' ***'
            self.setTitle(str)
            self.saveMark = True

    ### Editor functions ###
    def showAutoComp(self):
        charBefore = None
        caretPos = self.GetCurrentPos()
        if caretPos > 0:
            charBefore = self.GetCharAt(caretPos - 1)
        startpos = self.WordStartPosition(caretPos, True)
        endpos = self.WordEndPosition(caretPos, True)
        currentword = self.GetTextRange(startpos, endpos)
        if chr(charBefore) in self.alphaStr:
            list = ''
            for word in self.wordlist:
                if word.startswith(currentword) and word != currentword:
                    list = list + word + ' '
            if list:
                self.AutoCompShow(len(currentword), list)

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
        commentStr = '#'

        for i in range(self.firstLine, self.endLine+1):
            lineLen = len(self.GetLine(i))
            pos = self.PositionFromLine(i)
            if self.GetTextRange(pos,pos+1) != commentStr and lineLen > 2:
                self.InsertText(pos, commentStr)
            elif self.GetTextRange(pos,pos+1) == commentStr:
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
    """Projects panel"""
    def __init__(self, parent, mainPanel, size):
        wx.Panel.__init__(self, parent, -1, style=wx.WANTS_CHARS | wx.SUNKEN_BORDER | wx.EXPAND)

        self.mainPanel = mainPanel

        self.projectDict = {}

        size = size[0], size[1]
        self.tree = wx.TreeCtrl(self, -1, (0, 0), size,
                               wx.TR_DEFAULT_STYLE | wx.TR_HIDE_ROOT | wx.SUNKEN_BORDER | wx.EXPAND)

        if wx.Platform == '__WXMAC__':
            self.tree.SetFont(wx.Font(11, wx.ROMAN, wx.NORMAL, wx.NORMAL, face=faces['face']))
        else:
            self.tree.SetFont(wx.Font(8, wx.ROMAN, wx.NORMAL, wx.NORMAL, face=faces['face']))
        self.tree.SetBackgroundColour(faces['background'])

        isz = (12,12)
        self.il = wx.ImageList(isz[0], isz[1])
        self.fldridx     = self.il.Add(wx.ArtProvider_GetBitmap(wx.ART_FOLDER,      wx.ART_OTHER, isz))
        self.fldropenidx = self.il.Add(wx.ArtProvider_GetBitmap(wx.ART_FILE_OPEN,   wx.ART_OTHER, isz))
        self.fileidx     = self.il.Add(wx.ArtProvider_GetBitmap(wx.ART_NORMAL_FILE, wx.ART_OTHER, isz))

        self.tree.SetImageList(self.il)
        self.tree.SetSpacing(12)
        self.tree.SetIndent(6)

        self.root = self.tree.AddRoot("Projects")
        self.tree.SetPyData(self.root, None)
        self.tree.SetItemImage(self.root, self.fldridx, wx.TreeItemIcon_Normal)
        self.tree.SetItemImage(self.root, self.fldropenidx, wx.TreeItemIcon_Expanded)
        self.tree.SetItemTextColour(self.root, faces['identifier'])

        self.Bind(wx.EVT_TREE_BEGIN_LABEL_EDIT, self.OnBeginEdit, self.tree)
        self.Bind(wx.EVT_TREE_END_LABEL_EDIT, self.OnEndEdit, self.tree)

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
                                (ssubchild, ssubcookie) = self.tree.GetNextChild(subchild, ssubcookie)
                                if self.tree.ItemHasChildren(ssubchild):
                                    while sssubchild.IsOk():
                                        self.tree.SetItemTextColour(sssubchild, faces['identifier'])
                                        (sssubchild, sssubcookie) = self.tree.GetNextChild(ssubchild, sssubcookie)
                            (sssubchild, sssubcookie) = self.tree.GetFirstChild(ssubchild)
                        (subchild, subcookie) = self.tree.GetNextChild(child, subcookie)
                (child, cookie) = self.tree.GetNextChild(self.root, cookie)

    def loadProject(self, dirPath):
        projectName = os.path.split(dirPath)[1]

        self.projectDict[projectName] = dirPath

        projectDir = {}

        for root, dirs, files in os.walk(dirPath):
            if os.path.split(root)[1][0] != '.':
                if root == dirPath:
                    child = self.tree.AppendItem(self.root, projectName)
                    self.tree.SetPyData(child, None)
                    self.tree.SetItemImage(child, self.fldridx, wx.TreeItemIcon_Normal)
                    self.tree.SetItemImage(child, self.fldropenidx, wx.TreeItemIcon_Expanded)
                    self.tree.SetItemTextColour(child, faces['identifier'])
                    if dirs:
                        ddirs = [dir for dir in dirs if dir[0] != '.']
                        for dir in ddirs:
                            subfol = self.tree.AppendItem(child, "%s" % dir)
                            projectDir[dir] = subfol
                            self.tree.SetPyData(subfol, None)
                            self.tree.SetItemImage(subfol, self.fldridx, wx.TreeItemIcon_Normal)
                            self.tree.SetItemImage(subfol, self.fldropenidx, wx.TreeItemIcon_Expanded)
                            self.tree.SetItemTextColour(subfol, faces['identifier'])
                    if files:
                        ffiles = [file for file in files if file[0] != '.' and file[-3:] != 'pyc']
                        for file in ffiles:
                            item = self.tree.AppendItem(child, "%s" % file)
                            self.tree.SetPyData(item, None)
                            self.tree.SetItemImage(item, self.fileidx, wx.TreeItemIcon_Normal)
                            self.tree.SetItemImage(item, self.fileidx, wx.TreeItemIcon_Selected)
                            self.tree.SetItemTextColour(item, faces['identifier'])
                else:
                    if os.path.split(root)[1] in projectDir.keys():
                        parent = projectDir[os.path.split(root)[1]]
                        if dirs:
                            ddirs = [dir for dir in dirs if dir[0] != '.']
                            for dir in ddirs:
                                subfol = self.tree.AppendItem(parent, "%s" % dir)
                                projectDir[dir] = subfol
                                self.tree.SetPyData(subfol, None)
                                self.tree.SetItemImage(subfol, self.fldridx, wx.TreeItemIcon_Normal)
                                self.tree.SetItemImage(subfol, self.fldropenidx, wx.TreeItemIcon_Expanded)  
                                self.tree.SetItemTextColour(subfol, faces['identifier'])
                        if files:
                            ffiles = [file for file in files if file[0] != '.' and file[-3:] != 'pyc']
                            for file in ffiles:
                                item = self.tree.AppendItem(parent, "%s" % file)
                                self.tree.SetPyData(item, None)
                                self.tree.SetItemImage(item, self.fileidx, wx.TreeItemIcon_Normal)
                                self.tree.SetItemImage(item, self.fileidx, wx.TreeItemIcon_Selected)
                                self.tree.SetItemTextColour(item, faces['identifier'])

        self.tree.SortChildren(self.root)
        self.tree.SortChildren(child)

    def OnBeginEdit(self, event):
        # show how to prevent edit...
        item = event.GetItem()
        if item:
            wx.Bell()
            # Lets just see what's visible of its children
            cookie = 0
            root = event.GetItem()
            (child, cookie) = self.tree.GetFirstChild(root)
            while child.IsOk():
                (child, cookie) = self.tree.GetNextChild(root, cookie)
            event.Veto()

    def OnEndEdit(self, event):
        # show how to reject edit, we'll not allow any digits
        for x in event.GetLabel():
            if x in string.digits:
                event.Veto()
                return

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
                self.window.GetTopLevelParent().panel.project.loadProject(file)
                sys.path.append(file)
            elif os.path.isfile(file):
                self.window.GetTopLevelParent().panel.addPage(file)
            else:
                pass

if __name__ == '__main__':
    filesToOpen = []
    projectsToOpen = []
    if len(sys.argv) > 1:
        for f in sys.argv[1:]:
            if os.path.isdir(f):
                if f[-1] == '/': f = f[:-1]
                projectsToOpen.append(f)
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
    print u'%s %s by Olivier Bélanger' % (NAME, VERSION)
    frame = MainFrame(None, -1, title='PyoEd Editor', pos=(10,25), size=(X, Y))
    frame.Show()
    app.MainLoop()
