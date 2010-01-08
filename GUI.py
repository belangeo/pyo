#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys, keyword
import wx
import wx.py as py
import  wx.stc as stc
from pyo import *

if wx.Platform == '__WXMSW__':
    faces = { 'police': 'Monaco', 'size' : 10, 'size2': 8 }
elif wx.Platform == '__WXMAC__':
    faces = { 'police': 'Monaco', 'size' : 11, 'size2': 9 }
else:
    faces = { 'police': 'Monaco', 'size' : 12, 'size2': 10 }

tree = {'functions': sorted(['pa_count_devices', 'pa_get_default_input', 'pa_get_default_output', 'pa_list_devices', 
                    'pm_count_devices', 'pm_list_devices', 'quit', 'sndinfo']),
        'PyoObject': sorted(['BandSplit', 'Biquad', 'Counter', 'Delay', 'Disto', 'Dummy', 'Fader', 'Follower', 
                    'Hilbert', 'Input', 'InputFader', 'Metro', 'Midictl', 'Mix', 'Noise', 'Notein', 'Osc', 'OscReceive', 
                    'OscSend', 'Pan', 'Pattern', 'Phasor', 'Port', 'PyPattern', 'SPan', 'TrigEnv', 'TrigRand', 
                    'Select', 'SfMarkerShuffler', 'SfPlayer', 'Sig', 'Sine', 'TableRec']),
        'PyoTableObject': sorted(['LinTable', 'NewTable', 'SndTable', 'HannTable', 'HarmTable']),
        'Server': [],
        'Stream': [],
        'TableStream': []}
        
wordlist = []
for key in tree.keys():
    wordlist.append(key)
    wordlist.extend(tree[key])
sorted(wordlist)    

def new(evt):
    edFrames.append(wx.Frame(None, -1, title='pyo editor', pos=(400+i*20, -1), size=(600, 700)))
    editors.append(PythonSTC(edFrames[-1], -1, inter, None))
    edFrames[-1].Show()

def openfile(evt):
    dlg = wx.FileDialog(None, message="Choose a file", defaultDir=os.getcwd(),
        defaultFile="", style=wx.OPEN | wx.MULTIPLE)

    if dlg.ShowModal() == wx.ID_OK:
        path = dlg.GetPaths()
        for file in path:          
            name = os.path.split(file)[1]
            edFrames.append(wx.Frame(None, -1, title=name, pos=(400+i*20, -1), size=(600, 700)))
            editors.append(PythonSTC(edFrames[-1], -1, inter, file))
            edFrames[-1].Show()
    dlg.Destroy()

def openmanual(evt):
    global docFrame
    docFrame = wx.Frame(None, -1, title='pyo documentation', size=(1000, 700))
    doc = HelpWin(docFrame)
    docFrame.Show() 
       
def quit(evt):
    for editor in editors:
        editor.delete(None)
    try:
        docFrame.Destroy()
    except:
        pass        
    inter.run("quit()", False)
    interFrame.Destroy()    

def setStyle(editor):
    editor.SetLexer(stc.STC_LEX_PYTHON)
    editor.SetKeyWords(0, " ".join(keyword.kwlist) + " None True False " + " ".join(wordlist))

    editor.SetMargins(5,5)
    editor.SetSTCCursor(2)
    editor.SetIndent(4)
    editor.SetTabIndents(True)
    editor.SetTabWidth(4)
    editor.SetUseTabs(False)

    # Global default styles for all languages
    editor.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "face:%(police)s,size:%(size)d" % faces)
    editor.StyleClearAll()  # Reset all to be like the default

    # Global default styles for all languages
    editor.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "face:%(police)s,size:%(size)d" % faces)
    editor.StyleSetSpec(stc.STC_STYLE_LINENUMBER,  "back:#C0C0C0,face:%(police)s,size:%(size2)d" % faces)
    editor.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "face:%(police)s" % faces)
    editor.StyleSetSpec(stc.STC_STYLE_BRACELIGHT,  "fore:#FFFFFF,back:#0000FF,bold")
    editor.StyleSetSpec(stc.STC_STYLE_BRACEBAD,    "fore:#000000,back:#FF0000,bold")

    # Default 
    editor.StyleSetSpec(stc.STC_P_DEFAULT, "fore:#000000,face:%(police)s,size:%(size)d" % faces)
    # Comments
    editor.StyleSetSpec(stc.STC_P_COMMENTLINE, "fore:#007F00,face:%(police)s,size:%(size)d" % faces)
    # Number
    editor.StyleSetSpec(stc.STC_P_NUMBER, "fore:#007F7F,size:%(size)d" % faces)
    # String
    editor.StyleSetSpec(stc.STC_P_STRING, "fore:#7F007F,face:%(police)s,size:%(size)d" % faces)
    # Single quoted string
    editor.StyleSetSpec(stc.STC_P_CHARACTER, "fore:#7F007F,face:%(police)s,size:%(size)d" % faces)
    # Keyword
    editor.StyleSetSpec(stc.STC_P_WORD, "fore:#00007F,bold,size:%(size)d" % faces)
    # Triple quotes
    editor.StyleSetSpec(stc.STC_P_TRIPLE, "fore:#7F0000,size:%(size)d" % faces)
    # Triple double quotes
    editor.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, "fore:#7F0000,size:%(size)d" % faces)
    # Class name definition
    editor.StyleSetSpec(stc.STC_P_CLASSNAME, "fore:#0000FF,bold,underline,size:%(size)d" % faces)
    # Function or method name definition
    editor.StyleSetSpec(stc.STC_P_DEFNAME, "fore:#007F7F,bold,size:%(size)d" % faces)
    # Operators
    editor.StyleSetSpec(stc.STC_P_OPERATOR, "bold,size:%(size)d" % faces)
    # Identifiers
    editor.StyleSetSpec(stc.STC_P_IDENTIFIER, "fore:#000000,face:%(police)s,size:%(size)d" % faces)
    # Comment-blocks
    editor.StyleSetSpec(stc.STC_P_COMMENTBLOCK, "fore:#7F7F7F,size:%(size)d" % faces)
    # End of line where string is not closed
    editor.StyleSetSpec(stc.STC_P_STRINGEOL, "fore:#000000,face:%(police)s,back:#E0C0E0,eol,size:%(size)d" % faces)


class PythonSTC(stc.StyledTextCtrl):    
    def __init__(self, parent, ID, interpreter, openedFile=None,
                 pos=wx.DefaultPosition, size=wx.DefaultSize, style=0):
        stc.StyledTextCtrl.__init__(self, parent, ID, pos, size, style)

        self.parent = parent
        self.interpreter = interpreter

        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(99, "New\tCtrl+N", "Creates a new page")
        menu1.Append(100, "Open\tCtrl+O", "Opens an existing file")
        menu1.Append(101, "Save\tCtrl+S")
        menu1.Append(102, "Save as...\tShift+Ctrl+S")
        menu1.Append(103, "Close\tCtrl+W", "Closes front window")
        menu1.Append(120, "Quit\tCtrl+Q")  
        self.menuBar.Append(menu1, 'File')

        menu2 = wx.Menu()
        menu2.Append(121, "Un/Comment Selection\tCtrl+J", "Comments or Uncomments selected lines")
        menu2.Append(122, "Insert file path...\tCtrl+L", "Opens standard dialog and insert chosen file path at the current position")
        menu2.Append(123, "Find...\tCtrl+F", "Opens Find and Replace standard dialog")
        self.menuBar.Append(menu2, 'Code')

        menu4 = wx.Menu()
        menu4.Append(130, "pyo documentation\tCtrl+U", "Shows pyo objects manual pages.")
        self.menuBar.Append(menu4, 'Manual')

        self.parent.SetMenuBar(self.menuBar)

        self.sb = wx.StatusBar(self.parent, -1)
        self.parent.SetStatusBar(self.sb)

        self.parent.Bind(wx.EVT_MENU, new, id=99)        
        self.parent.Bind(wx.EVT_MENU, openfile, id=100)
        self.parent.Bind(wx.EVT_MENU, self.save, id=101)
        self.parent.Bind(wx.EVT_MENU, self.saveas, id=102)
        self.parent.Bind(wx.EVT_MENU, self.delete, id=103)
        self.parent.Bind(wx.EVT_MENU, quit, id=120)
        self.parent.Bind(wx.EVT_CLOSE, self.delete)
        self.Bind(wx.EVT_MENU, self.OnComment, id=121)    
        self.Bind(wx.EVT_MENU, self.insertPath, id=122)
        self.Bind(wx.EVT_MENU, self.OnShowFindReplace, id=123)
        self.parent.Bind(wx.EVT_MENU, openmanual, id=130)
                
        self.path = ""        
        self.line = 0
        self.textSent = False
        
        self.CmdKeyAssign(ord('='), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMIN)
        self.CmdKeyAssign(ord('-'), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMOUT)
        self.CmdKeyAssign(ord('Z'), stc.STC_SCMOD_SHIFT+stc.STC_SCMOD_CTRL, stc.STC_CMD_REDO)

        self.Bind(wx.EVT_FIND, self.OnFind)
        self.Bind(wx.EVT_FIND_NEXT, self.OnFind)
        self.Bind(wx.EVT_FIND_REPLACE, self.OnFind)
        self.Bind(wx.EVT_FIND_REPLACE_ALL, self.OnFind)
        self.Bind(wx.EVT_FIND_CLOSE, self.OnFindClose)

        self.SetProperty("fold", "1")
        self.SetProperty("tab.timmy.whinge.level", "1")
        self.SetViewWhiteSpace(False)
        self.SetUseAntiAliasing(True)
        self.SetEdgeMode(stc.STC_EDGE_BACKGROUND)
        self.SetEdgeColumn(200)

        self.SetMarginType(1, stc.STC_MARGIN_NUMBER)
        self.SetMarginWidth(1, 25)
        # Setup a margin to hold fold markers
        self.SetMarginType(2, stc.STC_MARGIN_SYMBOL)
        self.SetMarginMask(2, stc.STC_MASK_FOLDERS)
        self.SetMarginSensitive(2, True)
        self.SetMarginWidth(2, 12)

        # Plus for contracted folders, minus for expanded
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPEN,    stc.STC_MARK_MINUS, "white", "black")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDER,        stc.STC_MARK_PLUS,  "white", "black")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERSUB,     stc.STC_MARK_EMPTY, "white", "black")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERTAIL,    stc.STC_MARK_EMPTY, "white", "black")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEREND,     stc.STC_MARK_EMPTY, "white", "black")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPENMID, stc.STC_MARK_EMPTY, "white", "black")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERMIDTAIL, stc.STC_MARK_EMPTY, "white", "black")

        self.Bind(stc.EVT_STC_UPDATEUI, self.OnUpdateUI)
        self.Bind(stc.EVT_STC_MARGINCLICK, self.OnMarginClick)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyPressed)

        setStyle(self)

        if openedFile:
            self.LoadFile(openedFile)
            self.path = openedFile
            
    def OnKeyPressed(self, event):
        key = event.GetKeyCode()

        # Enter key, sends commands to the interpreter
        if key == 370:
            text = self.GetText()
            selected = self.GetSelectedText()
            if selected:
                self.line = self.GetCurrentLine()
                for sel in selected.splitlines(True):
                    self.interpreter.run(sel, False)      
            else:
                self.line = self.GetCurrentLine()
                selected = self.GetCurLine()[0]    
                self.interpreter.run(selected, False)
                
            self.ClearAll()
            self.SetText(text)
            self.textSent = True
        event.Skip()

    def OnUpdateUI(self, evt):
        # if commands were sent to the interpreter
        if self.textSent:
            self.DeleteBack()
            self.GotoLine(self.line+1)
            self.textSent = False
            self.Colourise(0, -1)

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

        # Check for word under caret
        startpos = self.WordStartPosition(caretPos, True)
        endpos = self.WordEndPosition(caretPos, True)
        currentword = self.GetTextRange(startpos, endpos)
        for key in tree.keys():
            if currentword in tree[key] or currentword == key:
                try:
                    self.sb.SetStatusText(eval(currentword).args())
                except:    
                    pass
                break
                
    def OnMarginClick(self, evt):
        # fold and unfold as needed
        if evt.GetMargin() == 2:
            lineClicked = self.LineFromPosition(evt.GetPosition())
            if self.GetFoldLevel(lineClicked) & stc.STC_FOLDLEVELHEADERFLAG:
                self.ToggleFold(lineClicked)

    def OnModified(self):
        if self.GetModify() and not self.saveMark:
            title = self.getTitle()
            str = '*** ' + title + ' ***'
            self.parent.SetTitle(str)
            self.saveMark = True

    def save(self, event):
        if not self.path:
            self.saveas(None)
        else:
            self.SaveFile(self.path)
        
    def saveas(self, event):
        dlg = wx.FileDialog(self, message="Save file as ...", defaultDir=os.path.expanduser('~'),
            defaultFile="", style=wx.SAVE)
        dlg.SetFilterIndex(0)

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            title = os.path.split(path)[1]
            self.path = path
            self.SaveFile(self.path)
            self.parent.SetTitle(title)
        dlg.Destroy()

    def delete(self, event):
        action = self.close()
        if action == 'delete':
            del edFrames[edFrames.index(self.parent)]
            del editors[editors.index(self)]
            self.parent.Destroy()
        else:
            pass

    def close(self):
        if self.GetModify():
            if not self.path: f = "pyo editor"
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

    def OnComment(self, evt):
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

    def insertPath(self, evt):
        dlg = wx.FileDialog(self, message="Choose a file", defaultDir=os.getcwd(),
            defaultFile="", style=wx.OPEN | wx.MULTIPLE)

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPaths()
            text = str(path[0])
            self.ReplaceSelection("'" + text + "'")
        dlg.Destroy()

    def OnShowFindReplace(self, evt):
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
        
class Interpreter(py.shell.Shell): 
    def __init__(self, parent):
        py.shell.Shell.__init__(self, parent, -1)
        self.parent = parent
        
        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(99, "New\tCtrl+N", "Creates a new page")
        menu1.Append(100, "Open\tCtrl+O", "Opens an existing file")
        menu1.Append(120, "Quit\tCtrl+Q")  
        self.menuBar.Append(menu1, 'File')

        menu4 = wx.Menu()
        menu4.Append(130, "pyo documentation\tCtrl+U", "Shows pyo objects manual pages.")
        self.menuBar.Append(menu4, 'Manual')

        self.parent.SetMenuBar(self.menuBar)

        self.parent.Bind(wx.EVT_MENU, new, id=99)        
        self.parent.Bind(wx.EVT_MENU, openfile, id=100)
        self.parent.Bind(wx.EVT_MENU, quit, id=120)
        self.parent.Bind(wx.EVT_CLOSE, self.delete)
        self.parent.Bind(wx.EVT_MENU, openmanual, id=130)
  
        setStyle(self)

    def delete(self, evt):
        pass
        
class HelpWin(wx.Treebook):
    def __init__(self, parent):
        wx.Treebook.__init__(self, parent, -1, style=wx.BK_DEFAULT)

        self.parent = parent
        
        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(103, "Close\tCtrl+W", "Closes front window")
        menu1.Append(120, "Quit\tCtrl+Q")  
        self.menuBar.Append(menu1, 'File')

        self.parent.SetMenuBar(self.menuBar)

        self.parent.Bind(wx.EVT_MENU, self.delete, id=103)
        self.parent.Bind(wx.EVT_MENU, quit, id=120)
        self.parent.Bind(wx.EVT_CLOSE, self.delete)
  
        win = self.makePanel()
        self.AddPage(win, "--- pyo documentation ---")
        for key in sorted(tree.keys()):
            win = self.makePanel(key)
            self.AddPage(win, key)
            for obj in tree[key]:
                win = self.makePanel(obj)
                self.AddSubPage(win, obj)

        # This is a workaround for a sizing bug on Mac...
        wx.FutureCall(100, self.AdjustSize)

    def delete(self, event):
        self.parent.Destroy()

    def AdjustSize(self):
        self.GetTreeCtrl().InvalidateBestSize()
        self.SendSizeEvent()

    def makePanel(self, obj=None):
        panel = wx.Panel(self, -1)
        if obj != None:
            try:
                args = obj + ' :\n\n' + eval(obj).args() + '\n'
            except:
                args = obj + ' :\n\n'    
            try:
                win = wx.TextCtrl(panel, -1, args + eval(obj).__doc__, size=(600, 600), style=wx.TE_MULTILINE|wx.TE_RICH2)
            except:
                win = wx.TextCtrl(panel, -1, args + "\nnot documented yet...", size=(600, 600), style=wx.TE_MULTILINE|wx.TE_RICH2)                
        else:
            win = wx.TextCtrl(panel, -1, "pyo documentation", size=(600, 600), style=wx.TE_MULTILINE|wx.TE_RICH2)
            
        def OnPanelSize(evt, win=win):
            win.SetPosition((0,0))
            win.SetSize(evt.GetSize())
            
        panel.Bind(wx.EVT_SIZE, OnPanelSize)
        return panel

filesToOpen = []
if len(sys.argv) > 1:
    for f in sys.argv[1:]:
        if os.path.isfile(f):    
            filesToOpen.append(f)
        else:
            pass

app = wx.PySimpleApp()

interFrame = wx.Frame(None, -1, title='pyo interpreter', size=(500, 400))
inter = Interpreter(interFrame)

edFrames = []
editors = []
if filesToOpen:
    for i, f in enumerate(filesToOpen):
        name = os.path.split(f)[1]
        edFrames.append(wx.Frame(None, -1, title=name, pos=(400+i*20, -1), size=(600, 700)))
        editors.append(PythonSTC(edFrames[-1], -1, inter, f))
else:        
    edFrames.append(wx.Frame(None, -1, title='pyo editor', pos=(400, -1), size=(600, 700)))
    editors.append(PythonSTC(edFrames[-1], -1, inter))

interFrame.Show()
for f in edFrames:
    f.Show()

app.MainLoop()
