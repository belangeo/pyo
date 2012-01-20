#! /usr/bin/env python
"""
Copyright 2008 Olivier Belanger

This file is part of Ounk.

Ounk is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Ounk is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Ounk.  If not, see <http://www.gnu.org/licenses/>.
"""

import sys, os, time, types, string, inspect
import keyword
import wx
import wx.stc  as  stc
from wx.lib.splitter import MultiSplitterWindow
from wx.lib.wordwrap import wordwrap
from subprocess import Popen
import wx.aui
from pyo import OBJECTS_TREE

NAME = 'PyoEd'
VERSION = '0.1.0'

# PATHS #
CURRENTDIR = os.path.dirname(sys.argv[0])
if '/PyoEd.app' not in os.getcwd():
    if CURRENTDIR:
        os.chdir(CURRENTDIR)    
    RESOURCES_PATH = os.path.join(os.getcwd(), 'ounk')
    TEMP_PATH = os.path.join(os.path.expanduser('~'), '.ounk')           
else:
    RESOURCES_PATH = os.getcwd()
    TEMP_PATH = os.path.join(os.path.expanduser('~'), '.ounk')

# Bitstream Vera Sans Mono, Corbel, Monaco, Envy Code R, MonteCarlo 
conf = {"preferedStyle": "Default"}
STYLES = {'Default': {'face': 'Courier New', 'default': '#000000', 'comment': '#007F7F', 'commentblock': '#7F7F7F',
                    'number': '#367800', 'string': '#7F007F', 'triple': '#7F0000', 'keyword': '#00007F',
                    'class': '#0000FF', 'function': '#007F7F', 'identifier': '#000000', 'caret': '#00007E',
                    'background': '#FFFFFF', 'linenumber': '#000000', 'marginback': '#C0C0C0', 'markerfg': '#FFFFFF',
                      'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000'},
           'Custom': {'face': 'Monaco', 'default': '#FFFFFF', 'comment': '#BFBFBF', 'commentblock': '#7F7F7F',
                      'number': '#80BB33', 'string': '#FF47D7', 'triple': '#FF3300', 'keyword': '#2A74FF',
                      'class': '#4AF3FF', 'function': '#00E0B6', 'identifier': '#FFFFFF', 'caret': '#DDDDDD',
                      'background': '#000000', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                      'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000'},
            'Soft': {'face': 'Monaco', 'default': '#000000', 'comment': '#444444', 'commentblock': '#7F7F7F',
                     'number': '#222222', 'string': '#272727', 'triple': '#333333', 'keyword': '#000000',
                     'class': '#666666', 'function': '#555555', 'identifier': '#000000', 'caret': '#222222',
                     'background': '#EFEFEF', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                     'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000'},
            'Smooth': {'face': 'Monaco', 'default': '#FFFFFF', 'comment': '#DD0000', 'commentblock': '#7F0000',
                       'number': '#CCCCCC', 'string': '#00EE00', 'triple': '#00AA00', 'keyword': '#FFFFFF',
                       'class': '#00FFA2', 'function': '#00FFD5', 'identifier': '#CCCCCC', 'caret': '#EEEEEE',
                       'background': '#222222', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                       'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000'},
            'Espresso': {'face': 'Monaco', 'default': '#BDAE9C', 'comment': '#0066FF', 'commentblock': '#0044DD',
                         'number': '#44AA43', 'string': '#2FE420', 'triple': '#049B0A', 'keyword': '#43A8ED',
                         'class': '#6D79DE', 'function': '#7290D9', 'identifier': '#BDAE9C', 'caret': '#FFFFFF',
                         'background': '#2A211C', 'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD',
                         'markerbg': '#404040', 'bracelight': '#AABBDD', 'bracebad': '#DD0000'}
                         }               
if wx.Platform == '__WXMSW__':
    faces = { 'size' : 10,
              'size2': 8}
elif wx.Platform == '__WXMAC__':
    faces = { 'size' : 12,
              'size2': 10}
else:
    faces = { 'size' : 8,
              'size2': 7}

styles = STYLES.keys()

for key, value in STYLES[conf['preferedStyle']].items():
    faces[key] = value

class ControlPanel(wx.Panel):
    def __init__(self, parent, mainPanel, size=wx.DefaultSize):
        wx.Panel.__init__(self, parent, size=size, style = wx.EXPAND | wx.SUNKEN_BORDER)
        self.mainPanel = mainPanel
        sizer = wx.BoxSizer(wx.VERTICAL)
        self.project = ProjectTree(self, self.mainPanel, size=size)
        sizer.Add(self.project, 1, wx.EXPAND | wx.ALL, 5)
        self.SetSizer(sizer)
        
    def running(self):
        #script = '\n'.join([l for l in self.mainPanel.editor.GetText().splitlines(True)]) + '\n'
        pid = Popen(["python", self.mainPanel.editor.path]).pid
       
class MainFrame(wx.Frame):
    def __init__(self, parent, ID, title, pos=wx.DefaultPosition, size=wx.DefaultSize, 
                 style=wx.DEFAULT_FRAME_STYLE):
        wx.Frame.__init__(self, parent, ID, title, pos, size, style)

        self.panel = MainPanel(self, size=size)
        
        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        submenu1 = wx.Menu()
        submenu1.Append(110, "New\tCtrl+N", "Creates a new page")
        menu1.Append(100, "Open\tCtrl+O", "Opens an existing file")
        menu1.Append(112, "Open Project\tShift+Ctrl+O", "Opens a project folder. Path of the folder will be added to sys.path module")
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
        #menu1.InsertSeparator(5)
        menu1.Append(101, "Save\tCtrl+S")
        menu1.Append(102, "Save as...\tShift+Ctrl+S")
        menu1.Append(111, "Close\tCtrl+W", "Closes front window")
        #menu1.InsertSeparator(9)
        menu1.Append(128, "Preferences...\tCtrl+;", "Customize PyoEd behavior")
        #menu1.InsertSeparator(19)
        menu1.Append(129, "Quit\tCtrl+Q")  
        self.menuBar.Append(menu1, 'File')

        menu2 = wx.Menu()
        menu2.Append(103, "Collapse/Expand\tShift+Ctrl+F", "Collapse or Expand all folds")
        menu2.Append(108, "Un/Comment Selection\tCtrl+J", "Comments or Uncomments selected lines")
        menu2.Append(114, "Show AutoCompletion\tCtrl+K", "Checks for autoCompletion possibilities and opens popup menu")
        menu2.Append(121, "Insert file path...\tCtrl+L", "Opens standard dialog and insert chosen file path at the current position")
        menu2.Append(122, "Find...\tCtrl+F", "Opens Find and Replace standard dialog")
        self.menuBar.Append(menu2, 'Code')
        
        menu3 = wx.Menu()
        menu3.Append(104, "Run\tCtrl+R", "Executes the current script")
        self.menuBar.Append(menu3, 'Process')

        menu5 = wx.Menu()
        stId = 500
        for st in styles:
            menu5.Append(stId, st, "", wx.ITEM_RADIO)
            if st == conf['preferedStyle']: menu5.Check(stId, True)
            stId += 1
        menu5.InsertSeparator(len(styles))
        self.menuBar.Append(menu5, 'Styles')
        
        # Make a Help menu
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
        self.Bind(wx.EVT_MENU, self.openPrefs, id=128)
        
        if subId2 > 2000:
            for i in range(2000,subId2):
                self.Bind(wx.EVT_MENU, self.openRecent, id=i) 
        for i in range(500, stId):
            self.Bind(wx.EVT_MENU, self.changeStyle, id=i)
        self.Bind(wx.EVT_MENU, self.autoComp, id=114) 
        self.Bind(wx.EVT_MENU, self.insertPath, id=121)
        self.Bind(wx.EVT_MENU, self.showFind, id=122)
        
        self.Bind(wx.EVT_SIZE, self.changedSize)
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.Bind(wx.EVT_MENU, self.OnClose, id=129)

        if projectsToOpen:
            for p in projectsToOpen:
                self.panel.controlPanel.project.loadProject(p)
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

    ### Styles ###        
    def changeStyle(self, evt):
        menu = self.GetMenuBar()
        id = evt.GetId()
        st = menu.FindItemById(id).GetLabel() 
        for key, value in STYLES[st].items():
            faces[key] = value 
        for i in range(self.panel.notebook.GetPageCount()):
            ed = self.panel.notebook.GetPage(i)
            ed.setStyle() 
        self.panel.controlPanel.project.setStyle()        

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
            self.panel.controlPanel.project.loadProject(self.folder)
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
            self.panel.notebook.SetPageText(self.panel.notebook.GetSelection(), 
                                            os.path.split(path)[1])
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
        self.panel.controlPanel.running()
        
    def changedSize(self, event):
        self.panel.resize(self.GetSize())
        event.Skip()
    
    ### About ###        
    def onHelpAbout(self, evt):
        # First we create and fill the info object
        info = wx.AboutDialogInfo()
        info.Name = NAME
        info.Version = VERSION
        #info.Icon = wx.Icon(os.path.join(RESOURCES_PATH, 'cochon.png'), wx.BITMAP_TYPE_PNG)
        info.Copyright = "(C) 2008 Olivier Belanger"
        info.Description = wordwrap(
            "Ounk is a Python audio scripting environment that uses Csound as its engine.\n\n"

            "It can be used for a variety of tasks such as composing, sound design, "
            "live performances and much more. In addition to providing powerful "
            "synthesis and sampling capabilities, it supports MIDI and "
            "Open Sound Control protocols.\n\n"
            
            "Special thanks to Nathanael Lecaude and Jean-Michel Dumas\n",
            350, wx.ClientDC(self))
        info.WebSite = ("http://code.google.com/p/ounk", "Ounk home page")

        wx.AboutBox(info)   

    ### On Quit Application ###
    def OnClose(self, event): 
        self.panel.OnQuit()
        self.Destroy()
        
class MainPanel(wx.Panel):
    def __init__(self, parent, size=(1200,800), style=wx.SUNKEN_BORDER):
        wx.Panel.__init__(self, parent, size=(1200,800), style=wx.SUNKEN_BORDER)

        self.X, self.Y = size[0], size[1]
        self.mainFrame = parent
        
        mainBox = wx.BoxSizer(wx.HORIZONTAL) 

        self.mainSplitter = MultiSplitterWindow(self, size=size, 
                                        style=wx.SP_LIVE_UPDATE | wx.SP_3DSASH | wx.SUNKEN_BORDER)
        
        self.notebook = MyNotebook(self.mainSplitter)
        self.controlPanel = ControlPanel(self.mainSplitter, self, size=(0, -1))
        self.editor = Editor(self.notebook, -1, size=(0, -1), 
                         setTitle=self.SetTitle, getTitle=self.GetTitle, 
                         )
        self.notebook.AddPage(self.editor, "Untitled")

        self.mainSplitter.AppendWindow(self.controlPanel, self.X*.15)
        self.mainSplitter.AppendWindow(self.notebook, self.X*.85)
        mainBox.Add(self.mainSplitter, 1, wx.EXPAND)
        self.SetSizer(mainBox)

        self.editor.EmptyUndoBuffer()
        self.editor.SetFocus()

        self.Bind(wx.aui.EVT_AUINOTEBOOK_PAGE_CHANGED, self.onPageChange)
        self.Bind(wx.EVT_SPLITTER_SASH_POS_CHANGING, self.onSashChanging)
        self.Bind(wx.aui.EVT_AUINOTEBOOK_PAGE_CLOSE, self.onClosePage)

    def addNewPage(self):
        editor = Editor(self.notebook, -1, size=(0, -1), 
                        setTitle=self.SetTitle, getTitle=self.GetTitle, 
                        )
        self.notebook.AddPage(editor, "Untitled", True)
        self.editor = editor
        
    def addPage(self, file):
        editor = Editor(self.notebook, -1, size=(0, -1), 
                        setTitle=self.SetTitle, getTitle=self.GetTitle, 
                        )
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
        
    def onLoadExample(self, name):
        self.editor = self.notebook.GetPage(self.notebook.GetSelection())
        self.notebook.SetPageText(self.notebook.GetSelection(), name)

    def onPageChange(self, event):
        self.editor = self.notebook.GetPage(self.notebook.GetSelection())        
        if not self.editor.path:
            if self.editor.GetModify():
                self.SetTitle("*** PyoEd editor ***")
            else:    
                self.SetTitle("PyoEd editor")
        else:
            if self.editor.GetModify():
                self.SetTitle('*** ' + self.editor.path + ' ***')
            else:
                self.SetTitle(self.editor.path)
        
    def onSashChanging(self, evt):
        pos = self.mainSplitter.GetSashPosition(0)
        self.controlPanel.project.tree.SetSize((pos-13, -1))
        self.controlPanel.SetSize((pos, -1))
        self.editor.checkScrollbar()
        evt.Skip()
        
    def resize(self, size):
        self.X, self.Y = size[0], size[1]
        self.mainSplitter.SetSashPosition(0, size[0]*.15)
        self.controlPanel.project.tree.SetSize((size[0]*.15-13, size[1]))
        
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
        self.SetTabIndents(True)
        self.SetTabWidth(4)
        self.SetUseTabs(False)

        self.EmptyUndoBuffer()
        self.SetFocus()
        
        self.CmdKeyAssign(ord('='), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMIN)
        self.CmdKeyAssign(ord('-'), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMOUT)
        self.CmdKeyAssign(ord('Z'), stc.STC_SCMOD_SHIFT+stc.STC_SCMOD_CTRL, stc.STC_CMD_REDO)

        self.SetProperty("fold", "1")
        self.SetProperty("tab.timmy.whinge.level", "1")
        self.SetMargins(5,5)
        self.SetViewWhiteSpace(False)
        self.SetUseAntiAliasing(True)
        self.SetEdgeMode(stc.STC_EDGE_BACKGROUND)
        self.SetEdgeColumn(1000)
        
        self.SetMarginType(1, stc.STC_MARGIN_NUMBER)
        self.SetMarginWidth(1, 25)
        # Setup a margin to hold fold markers
        self.SetMarginType(2, stc.STC_MARGIN_SYMBOL)
        self.SetMarginMask(2, stc.STC_MASK_FOLDERS)
        self.SetMarginSensitive(2, True)
        self.SetMarginWidth(2, 12)

        self.Bind(stc.EVT_STC_UPDATEUI, self.OnUpdateUI)
        self.Bind(stc.EVT_STC_MARGINCLICK, self.OnMarginClick)
        self.Bind(wx.EVT_CLOSE, self.OnClose)

        self.Bind(wx.EVT_FIND, self.OnFind)
        self.Bind(wx.EVT_FIND_NEXT, self.OnFind)
        self.Bind(wx.EVT_FIND_REPLACE, self.OnFind)
        self.Bind(wx.EVT_FIND_REPLACE_ALL, self.OnFind)
        self.Bind(wx.EVT_FIND_CLOSE, self.OnFindClose)
        
        self.setStyle()
        self.Refresh()

    ### Style ###
    def setStyle(self):
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPEN,    stc.STC_MARK_BOXMINUS, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDER,        stc.STC_MARK_BOXPLUS, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERSUB,     stc.STC_MARK_VLINE, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERTAIL,    stc.STC_MARK_LCORNERCURVE, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEREND,     stc.STC_MARK_ARROW, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPENMID, stc.STC_MARK_ARROWDOWN, faces['markerfg'], faces['markerbg'])
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERMIDTAIL, stc.STC_MARK_TCORNERCURVE, faces['markerfg'], faces['markerbg'])   

        # Global default styles for all languages
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "fore:%(default)s,face:%(face)s,size:%(size)d,back:%(background)s" % faces)
        self.StyleClearAll()  # Reset all to be like the default

        self.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "fore:%(default)s,face:%(face)s,size:%(size)d" % faces)
        self.StyleSetSpec(stc.STC_STYLE_LINENUMBER,  "fore:%(linenumber)s,back:%(marginback)s,face:%(face)s,size:%(size2)d" % faces)
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "fore:%(default)s,face:%(face)s" % faces)
        self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT,  "fore:#000000,back:%(bracelight)s,bold" % faces)
        self.StyleSetSpec(stc.STC_STYLE_BRACEBAD,    "fore:#000000,back:%(bracebad)s,bold" % faces)

        tree = OBJECTS_TREE
        l = []
        for k1 in tree.keys():
            if type(tree[k1]) == type({}):
                for k2 in tree[k1].keys():
                    for val in tree[k1][k2]:
                        l.append(val)
            else:
                for val in tree[k1]:
                    l.append(val)
        l.append("PyoObject")
        l.append("PyoTableObject")
        l.append("PyoMatrixObject")
        l.append("Server")

        # Python styles
        self.wordlist = l
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
            
        # check if inside a ounklib function
        prevLine = self.GetCurrentLine()-1
        firstPos = self.GetLineEndPosition(prevLine) + 1    
        if caretPos > 0: 
            self.inside = False
            for i in range(firstPos, caretPos):
                if chr(self.GetCharAt(i)) == ")":
                    self.inside = False
                elif chr(self.GetCharAt(i)) == "(":
                    wordend = i
                    self.inside = True
                else:
                    continue
        else:
            self.inside = False   

        if self.inside:
            wordbegin = self.WordStartPosition(wordend, True)
            whichword = self.GetTextRange(wordbegin, wordend)

        startpos = self.WordStartPosition(caretPos, True)
        endpos = self.WordEndPosition(caretPos, True)
        currentword = self.GetTextRange(startpos, endpos)
                
        self.auto = False # sert a rien!
        if self.inside and whichword in self.wordlist and chr(charBefore) not in "=, '\"[]{}.":
            list = ''
            arglist = inspect.getargspec(eval(whichword))[0]
            for arg in arglist:
                if arg.startswith(currentword) and arg != currentword:
                    list = list + arg + ' '
            if list:        
                self.AutoCompShow(len(currentword), list)
                self.auto = True
        elif chr(charBefore) in self.alphaStr:
            list = ''
            for word in self.wordlist:
                if word.startswith(currentword) and word != currentword:
                    list = list + word + ' ' 
            if list:
                self.AutoCompShow(len(currentword), list)
                self.auto = True
                        
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

        startpos = self.WordStartPosition(caretPos, True)
        endpos = self.WordEndPosition(caretPos, True)
        currentword = self.GetTextRange(startpos, endpos)

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

        self.tree.Bind(wx.EVT_LEFT_DCLICK, self.OnLeftDClick)

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
                if str(os.path.split(path)[1].split('.')[1]) in ['aif', 'aiff', 'AIF', 'AIFF', 'wav', 'WAV']:
                    if wx.Platform == '__WXMSW__':
                        os.system('start %s %s' % (conf['audioEditor'], path))
                    elif wx.Platform == '__WXMAC__':    
                        os.system('open -a "%s" %s' % (conf['audioEditor'], path))
                    else:
                        os.system('%s %s' % (conf['audioEditor'], path))
                else:
                    self.mainPanel.addPage(path)
                    self.mainPanel.editor.saveMark = False
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
                self.window.GetTopLevelParent().panel.controlPanel.project.loadProject(file)
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
    print '%s %s by Olivier Belanger' % (NAME, VERSION)
    frame = MainFrame(None, -1, title='PyoEd editor', size=(X-20, Y-50))
    frame.Show()
    app.MainLoop()
