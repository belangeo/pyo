#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys, keyword, string, inspect, threading, time
import wx
import wx.py as py
import  wx.stc as stc
from pyo import *

######################### KEYWORDS ########################## 
_OBJECTS_TREE = {'functions': sorted(['pa_count_devices', 'pa_get_default_input', 'pa_get_default_output', 'pa_list_devices', 
                    'pm_count_devices', 'pm_list_devices', 'quit', 'sndinfo']),
        'PyoObject': sorted(['BandSplit', 'Biquad', 'Clip', 'Counter', 'Delay', 'Disto', 'Dummy', 'Fader', 'Follower', 'Freeverb',
                    'Hilbert', 'Input', 'InputFader', 'Metro', 'Midictl', 'Mix', 'Noise', 'Notein', 'Osc', 'OscReceive', 
                    'OscSend', 'Pan', 'Pattern', 'Phasor', 'Pointer', 'Port', 'PyPattern', 'SPan', 'TrigEnv', 'TrigRand', 
                    'Select', 'SfMarkerShuffler', 'SfPlayer', 'Sig', 'Sine', 'TableRec', 'Waveguide']),
        'PyoTableObject': sorted(['LinTable', 'NewTable', 'SndTable', 'HannTable', 'HarmTable']),
        'Server': [], 'Stream': [], 'TableStream': []}
        
_KEYWORDS_LIST = []
for key in _OBJECTS_TREE.keys():
    _KEYWORDS_LIST.append(key)
    _KEYWORDS_LIST.extend(_OBJECTS_TREE[key])
sorted(_KEYWORDS_LIST)    

_DOC_KEYWORDS = ['Attributes', 'Examples', 'Parameters', 'Methods', 'Notes', 'Methods details']

######################### STYLES ############################
## 'face' = default font                                   ##
## 'default' = default color for all langauges             ##
## 'comment' = color for comments (#)                      ##
## 'commentblock' = color for comments block (##)          ##
## 'number' = color for numbers                            ##
## 'string' = color for strings                            ##
## 'triple' = color for triple quotes strings              ##
## 'keyword' = color for python and ounklib keywords       ##
## 'class' = color for Class names                         ##
## 'function' = color for function names                   ##
## 'identifier' = color for normal text                    ##
## 'caret' = color of the caret                            ##
## 'background' = background color                         ##
## 'linenumber' = color for line numbers in the margin     ##
## 'marginback' = background color of line numbers margin  ##
## 'markerfg' = marker foreground color                    ##
## 'markerbg' = marker background color                    ##
#############################################################

_STYLES = {'Default': {'face': 'Monaco', 'default': '#000000', 'comment': '#007F7F', 'commentblock': '#7F7F7F',
                    'number': '#367800', 'string': '#7F007F', 'triple': '#7F0000', 'keyword': '#00007F', 'class': '#0000FF', 
                    'function': '#007F7F', 'identifier': '#000000', 'caret': '#00007E', 'background': '#FFFFFF', 
                    'linenumber': '#000000', 'marginback': '#C0C0C0', 'markerfg': '#FFFFFF', 'markerbg': '#404040'},
           'Custom': {'face': 'Monaco', 'default': '#FFFFFF', 'comment': '#BFBFBF', 'commentblock': '#7F7F7F',
                      'number': '#80BB33', 'string': '#FF47D7', 'triple': '#FF3300', 'keyword': '#2A74FF', 'class': '#4AF3FF', 
                      'function': '#00E0B6', 'identifier': '#FFFFFF', 'caret': '#DDDDDD', 'background': '#000000', 
                      'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD', 'markerbg': '#404040'},
            'Soft': {'face': 'Monaco', 'default': '#000000', 'comment': '#444444', 'commentblock': '#7F7F7F',
                     'number': '#222222', 'string': '#272727', 'triple': '#333333', 'keyword': '#000000', 'class': '#666666',
                     'function': '#555555', 'identifier': '#000000', 'caret': '#222222', 'background': '#EFEFEF',
                     'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD', 'markerbg': '#404040'},
            'Smooth': {'face': 'Monaco', 'default': '#FFFFFF', 'comment': '#DD0000', 'commentblock': '#7F0000',
                       'number': '#CCCCCC', 'string': '#00EE00', 'triple': '#00AA00', 'keyword': '#FFFFFF', 'class': '#00FFA2',
                       'function': '#00FFD5', 'identifier': '#CCCCCC', 'caret': '#EEEEEE', 'background': '#222222',
                       'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD', 'markerbg': '#404040'},
            'Espresso': {'face': 'Monaco', 'default': '#BDAE9C', 'comment': '#0066FF', 'commentblock': '#0044DD',
                         'number': '#44AA43', 'string': '#2FE420', 'triple': '#049B0A', 'keyword': '#43A8ED', 'class': '#6D79DE',
                         'function': '#FF9358', 'identifier': '#BDAE9C', 'caret': '#FFFFFF', 'background': '#2A211C',
                         'linenumber': '#111111', 'marginback': '#AFAFAF', 'markerfg': '#DDDDDD', 'markerbg': '#404040'}}

if wx.Platform == '__WXMSW__':
    _STYLES_FACES = { 'size' : 10, 'size2': 8, 'size3': 12 }
elif wx.Platform == '__WXMAC__':
    _STYLES_FACES = { 'size' : 11, 'size2': 9, 'size3': 13 }
else:
    _STYLES_FACES = { 'size' : 12, 'size2': 10, 'size3': 14 }

_STYLES_KEYS = _STYLES.keys()

for key, value in _STYLES['Default'].items():
    _STYLES_FACES[key] = value 

def _ed_change_style(evt):
    id = evt.GetId()
    st = _STYLES_KEYS[id-500]
    for key, value in _STYLES[st].items():
        _STYLES_FACES[key] = value 
    for editor in _EDITORS:
        _ed_set_style(editor)
    _ed_set_style(_INTERPRETER)
    try:
        numPages = _doc_panel.GetPageCount()
        for i in range(numPages):
            _ed_set_style(_doc_panel.GetPage(i).win, True)
        _doc_panel.setStyle()    
    except:
        pass        

def _ed_set_style(editor, doc=False):
    editor.SetLexer(stc.STC_LEX_PYTHON)
    if doc:
        editor.SetKeyWords(0, " None True False " + " ".join(_KEYWORDS_LIST))
        editor.SetKeyWords(1, " ".join(_DOC_KEYWORDS))
    else:
        editor.SetKeyWords(0, " ".join(keyword.kwlist) + " None True False " + " ".join(_KEYWORDS_LIST))

    editor.SetMargins(5,5)
    editor.SetSTCCursor(2)
    editor.SetIndent(4)
    editor.SetTabIndents(True)
    editor.SetTabWidth(4)
    editor.SetUseTabs(False)

    # Global default styles for all languages
    editor.StyleSetSpec(stc.STC_STYLE_DEFAULT,  "fore:%(default)s,face:%(face)s,size:%(size)d,back:%(background)s" %   _STYLES_FACES)
    editor.StyleClearAll()  # Reset all to be like the default

    editor.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "fore:%(default)s,face:%(face)s,size:%(size)d" % _STYLES_FACES)
    editor.StyleSetSpec(stc.STC_STYLE_LINENUMBER,  "fore:%(linenumber)s,back:%(marginback)s,face:%(face)s,size:%(size2)d" % _STYLES_FACES)
    editor.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "fore:%(default)s,face:%(face)s" % _STYLES_FACES)
    editor.StyleSetSpec(stc.STC_STYLE_BRACELIGHT,  "fore:#000000,back:#888BFF,bold")
    editor.StyleSetSpec(stc.STC_STYLE_BRACEBAD,    "fore:#000000,back:#AA0000,bold")

    # Default
    editor.StyleSetSpec(stc.STC_P_DEFAULT, "fore:%(default)s,face:%(face)s,size:%(size)d" % _STYLES_FACES)
    # Comments
    editor.StyleSetSpec(stc.STC_P_COMMENTLINE, "fore:%(comment)s,face:%(face)s,size:%(size)d" % _STYLES_FACES)
    # Number
    editor.StyleSetSpec(stc.STC_P_NUMBER, "fore:%(number)s,face:%(face)s,bold,size:%(size)d" % _STYLES_FACES)
    # String
    editor.StyleSetSpec(stc.STC_P_STRING, "fore:%(string)s,face:%(face)s,size:%(size)d" % _STYLES_FACES)
    # Single quoted string
    editor.StyleSetSpec(stc.STC_P_CHARACTER, "fore:%(string)s,face:%(face)s,size:%(size)d" % _STYLES_FACES)
    # Keyword
    if doc:
        editor.StyleSetSpec(stc.STC_P_WORD, "fore:%(keyword)s,face:%(face)s,bold,size:%(size)d" % _STYLES_FACES)
        editor.StyleSetSpec(stc.STC_P_WORD2, "fore:%(comment)s,face:%(face)s,bold,size:%(size3)d" % _STYLES_FACES)
    else:
        editor.StyleSetSpec(stc.STC_P_WORD, "fore:%(keyword)s,face:%(face)s,bold,size:%(size)d" % _STYLES_FACES)
    # Triple quotes
    editor.StyleSetSpec(stc.STC_P_TRIPLE, "fore:%(triple)s,face:%(face)s,size:%(size)d" % _STYLES_FACES)
    # Triple double quotes
    editor.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, "fore:%(triple)s,face:%(face)s,size:%(size)d" % _STYLES_FACES)
    # Class name definition
    editor.StyleSetSpec(stc.STC_P_CLASSNAME, "fore:%(class)s,face:%(face)s,bold,size:%(size)d" % _STYLES_FACES)
    # Function or method name definition
    editor.StyleSetSpec(stc.STC_P_DEFNAME, "fore:%(function)s,face:%(face)s,bold,size:%(size)d" % _STYLES_FACES)
    # Operators
    editor.StyleSetSpec(stc.STC_P_OPERATOR, "bold,size:%(size)d,face:%(face)s" % _STYLES_FACES)
    # Identifiers
    editor.StyleSetSpec(stc.STC_P_IDENTIFIER, "fore:%(identifier)s,face:%(face)s,size:%(size)d" % _STYLES_FACES)
    # Comment-blocks
    editor.StyleSetSpec(stc.STC_P_COMMENTBLOCK, "fore:%(commentblock)s,face:%(face)s,size:%(size)d" % _STYLES_FACES)

    if doc:
        editor.SetCaretForeground(_STYLES_FACES['background'])
    else:    
        editor.SetCaretForeground(_STYLES_FACES['caret'])

##################### GLOBAL FUNCTIONS ########################    
def _editor_new(evt):
    _ED_FRAMES.append(wx.Frame(None, -1, title='pyo editor', pos=(400+i*20, -1), size=(600, 700)))
    _EDITORS.append(ScriptEditor(_ED_FRAMES[-1], -1, _INTERPRETER, None))
    _ED_FRAMES[-1].Show()

def _editor_openfile(evt):
    dlg = wx.FileDialog(None, message="Choose a file", defaultDir=os.getcwd(),
        defaultFile="", style=wx.OPEN | wx.MULTIPLE)

    if dlg.ShowModal() == wx.ID_OK:
        path = dlg.GetPaths()
        for i, f in enumerate(path):          
            filepath, name = os.path.split(f)
            sys.path.append(filepath)
            os.chdir(filepath)
            _ED_FRAMES.append(wx.Frame(None, -1, title=name, pos=(400+i*20, -1), size=(600, 700)))
            _EDITORS.append(ScriptEditor(_ED_FRAMES[-1], -1, _INTERPRETER, f))
            _ED_FRAMES[-1].Show()
    dlg.Destroy()

def _open_manual(evt):
    global _DOC_FRAME, _doc_panel
    try:
        _DOC_FRAME.Show()
    except:    
        _DOC_FRAME = wx.Frame(None, -1, title='pyo documentation', size=(1000, 700))
        _doc_panel = HelpWin(_DOC_FRAME)
        _DOC_FRAME.Show() 
       
def _app_quit(evt):
    for editor in _EDITORS:
        action = editor.delete(None)
        if action == 'keep':
            return
    try:
        _DOC_FRAME.Destroy()
    except:
        pass        
    _INTERPRETER.run("quit()", False)
    _INTER_FRAME.Destroy()    

###################### MAIN CLASSES ######################### 
class ScriptEditor(stc.StyledTextCtrl):    
    def __init__(self, parent, ID, interpreter, openedFile=None,
                 pos=wx.DefaultPosition, size=wx.DefaultSize, style=0):
        stc.StyledTextCtrl.__init__(self, parent, ID, pos, size, style)

        self.parent = parent
        self.interpreter = interpreter
        self.timeline = None
        
        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(99, "New\tCtrl+N", "Creates a new editor page")
        menu1.Append(100, "Open\tCtrl+O", "Opens an existing file")
        menu1.Append(125, "Create timeline\tCtrl+T", "Creates a timeline for events sequencing")
        menu1.Append(126, "Open timeline\tShift+Ctrl+T", "Opens a timeline for events sequencing")
        menu1.Append(101, "Save\tCtrl+S", "Saves the front file")
        menu1.Append(102, "Save as...\tShift+Ctrl+S", "Saves the front file under a new name")
        menu1.Append(103, "Close\tCtrl+W", "Closes front editor window")
        quit_item = menu1.Append(120, "Quit\tCtrl+Q", "Quit app")  
        if wx.Platform=="__WXMAC__":
            wx.App.SetMacExitMenuItemId(quit_item.GetId())
        self.menuBar.Append(menu1, 'File')

        menu2 = wx.Menu()
        menu2.Append(122, "Un/Comment Selection\tCtrl+J", "Comments or Uncomments selected lines")
        menu2.Append(123, "Insert file path...\tCtrl+L", 
                    "Opens standard dialog and insert chosen file path at the current position")
        menu2.Append(124, "Find...\tCtrl+F", "Opens Find and Replace standard dialog")
        self.menuBar.Append(menu2, 'Code')

        menu4 = wx.Menu()
        menu4.Append(130, "pyo documentation\tCtrl+U", "Shows pyo objects manual pages.")
        self.menuBar.Append(menu4, 'Manual')

        menu5 = wx.Menu()
        stId = 500
        for st in _STYLES_KEYS:
            menu5.Append(stId, st, "", wx.ITEM_RADIO)
            if st == 'Default': menu5.Check(stId, True)
            stId += 1
        self.menuBar.Append(menu5, 'Styles')

        self.parent.SetMenuBar(self.menuBar)

        self.sb = wx.StatusBar(self.parent, -1)
        self.parent.SetStatusBar(self.sb)

        self.parent.Bind(wx.EVT_MENU, _editor_new, id=99)        
        self.parent.Bind(wx.EVT_MENU, _editor_openfile, id=100)
        self.parent.Bind(wx.EVT_MENU, self.save, id=101)
        self.parent.Bind(wx.EVT_MENU, self.saveas, id=102)
        self.parent.Bind(wx.EVT_MENU, self.delete, id=103)
        self.parent.Bind(wx.EVT_MENU, _app_quit, id=120)
        self.parent.Bind(wx.EVT_CLOSE, self.delete)
        self.Bind(wx.EVT_MENU, self.OnComment, id=122)    
        self.Bind(wx.EVT_MENU, self.insertPath, id=123)
        self.Bind(wx.EVT_MENU, self.OnShowFindReplace, id=124)
        self.Bind(wx.EVT_MENU, self.createTimeline, id=125)
        self.Bind(wx.EVT_MENU, self.openTimeline, id=126)
        self.parent.Bind(wx.EVT_MENU, _open_manual, id=130)

        for i in range(500, stId):
            self.Bind(wx.EVT_MENU, _ed_change_style, id=i)
                
        self.path = ""        
        self.line = 0
        
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

        _ed_set_style(self)

        if openedFile:
            self.LoadFile(openedFile)
            self.path = openedFile
   
    def OnKeyPressed(self, event):
        key = event.GetKeyCode()

        # Enter key -> sends commands to the interpreter
        if key == 370:
            # check if selection, if not, pick the current line
            selected = self.GetSelectedText()
            if selected:
                self.line = self.GetCurrentLine()
                script = ''.join([l for l in selected.splitlines(True)]) + '\n'
                self.interpreter.Execute(script)
            else:
                self.line = self.GetCurrentLine()
                selected = self.GetCurLine()[0]
                
                # Handles "def", "class", "for", "while" "if", "try" and "with" blocks of code
                if selected.startswith('def') or selected.startswith('class') or selected.startswith('for') or \
                    selected.startswith('while') or selected.startswith('with') or selected.startswith('if') or \
                    selected.startswith('try'):
                    keyword = ""
                    if selected.startswith('if'): keyword = 'if'
                    elif selected.startswith('try'): keyword = 'try'
                    line_is_ok = True
                    end_of_file = False
                    while True:
                        # if line is legal, push it and adjust interpreter caret position for next pass
                        if line_is_ok:
                            self.interpreter.run(selected, False)
                            inter_offset = self.interpreter.GetCurLine()[1] - 4
                            for i in range(inter_offset):
                                self.interpreter.DeleteBack()

                        # check for end of file        
                        if self.line+1 == self.GetLineCount():
                            end_of_file = True
                        else:
                            # if not, pick the next line and analyse it 
                            self.GotoLine(self.line+1)
                            self.line = self.GetCurrentLine()
                            selected = self.GetCurLine()[0]
                            if selected.strip() == '':
                                line_is_ok = False
                            else:
                                line_is_ok = True

                        # if line is legal and there is no indentation, break the loop
                        if line_is_ok and not selected.startswith(' ') or end_of_file:
                            # grab special case of "if" and "try" statements
                            if keyword == 'if':
                                if selected.startswith('elif') or selected.startswith('else'):
                                    continue
                            elif keyword == 'try':
                                if selected.startswith('except') or selected.startswith('else') or selected.startswith('finally'):
                                    continue

                            self.interpreter.run('\n', False)
                            self.line -= 1
                            break
                
                else: # no keywords, push the current line    
                    self.interpreter.run(selected, False)
                    self.GotoLine(self.line+1)
        else:
            event.Skip()

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

        # Check for word under caret
        startpos = self.WordStartPosition(caretPos, True)
        endpos = self.WordEndPosition(caretPos, True)
        currentword = self.GetTextRange(startpos, endpos)
        for key in _OBJECTS_TREE.keys():
            if currentword in _OBJECTS_TREE[key] or currentword == key:
                try:
                    self.sb.SetStatusText(eval(currentword).args())
                except:    
                    pass
                break
        self.OnModified()
                
    def OnMarginClick(self, evt):
        # fold and unfold as needed
        if evt.GetMargin() == 2:
            lineClicked = self.LineFromPosition(evt.GetPosition())
            if self.GetFoldLevel(lineClicked) & stc.STC_FOLDLEVELHEADERFLAG:
                self.ToggleFold(lineClicked)

    def OnModified(self):
        title = os.path.split(self.path)[1]
        if self.GetModify():
            title = '*** ' + title + ' ***'           
        self.parent.SetTitle(title)

    def save(self, event):
        if not self.path:
            self.saveas(None)
        else:
            self.SaveFile(self.path)
            self.OnModified()
        
    def saveas(self, event):
        dlg = wx.FileDialog(self, message="Save file as ...", defaultDir=os.path.expanduser('~'),
            defaultFile="", style=wx.SAVE)
        dlg.SetFilterIndex(0)

        if dlg.ShowModal() == wx.ID_OK:
            self.path = dlg.GetPath()
            self.SaveFile(self.path)
            self.OnModified()
        dlg.Destroy()

    def delete(self, event):
        action = self.close()
        if action == 'delete':
            del _ED_FRAMES[_ED_FRAMES.index(self.parent)]
            del _EDITORS[_EDITORS.index(self)]
            self.parent.Destroy()
            return 'delete'
        else:
            return 'keep'

    def close(self):
        if self.GetModify():
            if not self.path: f = "pyo editor"
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
            text = path[0]
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

    def createTimeline(self, evt):
        self.timeline = Timeline(self.interpreter)

    def openTimeline(self, evt):
        dlg = wx.FileDialog(None, message="Choose a timeline file", defaultDir=os.getcwd(),
            defaultFile="", style=wx.OPEN)

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPaths()
        dlg.Destroy()
        self.timeline = Timeline(self.interpreter, path[0])
        
class Interpreter(py.shell.Shell): 
    def __init__(self, parent):
        py.shell.Shell.__init__(self, parent, -1)
        self.parent = parent
        
        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(99, "New\tCtrl+N", "Creates a _editor_new page")
        menu1.Append(100, "Open\tCtrl+O", "Opens an existing file")
        quit_item = menu1.Append(120, "Quit\tCtrl+Q")
        if wx.Platform=="__WXMAC__":
            wx.App.SetMacExitMenuItemId(quit_item.GetId())
        self.menuBar.Append(menu1, 'File')

        menu4 = wx.Menu()
        menu4.Append(130, "pyo documentation\tCtrl+U", "Shows pyo objects manual pages.")
        self.menuBar.Append(menu4, 'Manual')

        menu5 = wx.Menu()
        stId = 500
        for st in _STYLES_KEYS:
            menu5.Append(stId, st, "", wx.ITEM_RADIO)
            if st == 'Default': menu5.Check(stId, True)
            stId += 1
        self.menuBar.Append(menu5, 'Styles')

        self.parent.SetMenuBar(self.menuBar)

        self.parent.Bind(wx.EVT_MENU, _editor_new, id=99)        
        self.parent.Bind(wx.EVT_MENU, _editor_openfile, id=100)
        self.parent.Bind(wx.EVT_MENU, _app_quit, id=120)
        self.parent.Bind(wx.EVT_CLOSE, self.delete)
        self.parent.Bind(wx.EVT_MENU, _open_manual, id=130)
        for i in range(500, stId):
            self.Bind(wx.EVT_MENU, _ed_change_style, id=i)
  
        _ed_set_style(self)
        
    def delete(self, evt):
        pass

    def ExecuteWithoutEVC(self, text):
        """
        Replace selection with text and run commands.
        
        Execute method modified to make sure EnsureCaretVisible
        method is not called from the Timeline. It causes memory 
        corruption. (Belanger, 2010).
        """
        ps1 = str(sys.ps1)
        ps2 = str(sys.ps2)
        endpos = self.GetTextLength()
        self.SetCurrentPos(endpos)
        startpos = self.promptPosEnd
        self.SetSelection(startpos, endpos)
        self.ReplaceSelection('')
        text = text.lstrip()
        text = self.fixLineEndings(text)
        text = self.lstripPrompt(text)
        text = text.replace(os.linesep + ps1, '\n')
        text = text.replace(os.linesep + ps2, '\n')
        text = text.replace(os.linesep, '\n')
        lines = text.split('\n')
        commands = []
        command = ''
        for line in lines:
            if line.strip() == ps2.strip():
                # If we are pasting from something like a
                # web page that drops the trailing space
                # from the ps2 prompt of a blank line.
                line = ''
            lstrip = line.lstrip()
            if line.strip() != '' and lstrip == line and \
                    lstrip[:4] not in ['else','elif'] and \
                    lstrip[:6] != 'except':
                # New command.
                if command:
                    # Add the previous command to the list.
                    commands.append(command)
                # Start a new command, which may be multiline.
                command = line
            else:
                # Multiline command. Add to the command.
                command += '\n'
                command += line
        commands.append(command)
        for command in commands:
            command = command.replace('\n', os.linesep + ps2)
            self.write(command)
            self.processLine2()

    def write(self, text):
        """
        Display text in the shell.
        Replace line endings with OS-specific endings.
        
        Overide write method to make sure EnsureCaretVisible
        method is not called from the Timeline. It causes memory 
        corruption. (Belanger, 2010).
        """
        text = self.fixLineEndings(text)
        self.AddText(text)

    def processLine2(self):
        """
        Process the line of text at which the user hit Enter.
                
        processLine method modified to make sure EnsureCaretVisible
        method is not called from the Timeline. It causes memory 
        corruption. (Belanger, 2010).
        """

        # The user hit ENTER and we need to decide what to do. They
        # could be sitting on any line in the shell.

        thepos = self.GetCurrentPos()
        startpos = self.promptPosEnd
        endpos = self.GetTextLength()
        ps2 = str(sys.ps2)
        # If they hit RETURN inside the current command, execute the
        # command.
        if self.CanEdit():
            self.SetCurrentPos(endpos)
            self.interp.more = False
            command = self.GetTextRange(startpos, endpos)
            lines = command.split(os.linesep + ps2)
            lines = [line.rstrip() for line in lines]
            command = '\n'.join(lines)
            if self.reader.isreading:
                if not command:
                    # Match the behavior of the standard Python shell
                    # when the user hits return without entering a
                    # value.
                    command = '\n'
                self.reader.input = command
                self.write(os.linesep)
            else:
                self.push2(command)
        # Or replace the current command with the other command.
        else:
            # If the line contains a command (even an invalid one).
            if self.getCommand(rstrip=False):
                command = self.getMultilineCommand()
                self.clearCommand()
                self.write(command)
            # Otherwise, put the cursor back where we started.
            else:
                self.SetCurrentPos(thepos)
                self.SetAnchor(thepos)

    def push2(self, command, silent = False):
        """
        Send command to the interpreter for execution.
        
        push method modified to make sure EnsureCaretVisible
        method is not called from the Timeline. It causes memory 
        corruption. (Belanger, 2010).        
        """
        if not silent:
            self.write(os.linesep)
        busy = wx.BusyCursor()
        self.waiting = True
        self.more = self.interp.push(command)
        self.waiting = False
        del busy
        if not self.more:
            self.addHistory(command.rstrip())
        if not silent:
            self.prompt2()

    def prompt2(self):
        """
        Display proper prompt for the context: ps1, ps2 or ps3.
        If this is a continuation line, autoindent as necessary.
        
        prompt method modified to make sure EnsureCaretVisible
        method is not called from the Timeline. It causes memory 
        corruption. (Belanger, 2010).
        """
        isreading = self.reader.isreading
        skip = False
        if isreading:
            prompt = str(sys.ps3)
        elif self.more:
            prompt = str(sys.ps2)
        else:
            prompt = str(sys.ps1)
        pos = self.GetCurLine()[1]
        if pos > 0:
            if isreading:
                skip = True
            else:
                self.write(os.linesep)
        if not self.more:
            self.promptPosStart = self.GetCurrentPos()
        if not skip:
            self.write(prompt)
        if not self.more:
            self.promptPosEnd = self.GetCurrentPos()
            # Keep the undo feature from undoing previous responses.
            self.EmptyUndoBuffer()
        # XXX Add some autoindent magic here if more.
        if self.more:
            self.write(' '*4)  # Temporary hack indentation.
        self.ScrollToColumn(0)
                            
class HelpWin(wx.Treebook):
    def __init__(self, parent):
        wx.Treebook.__init__(self, parent, -1, style=wx.BK_DEFAULT)

        self.parent = parent
        
        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(103, "Close\tCtrl+W", "Closes front window")
        quit_item = menu1.Append(120, "Quit\tCtrl+Q")  
        if wx.Platform=="__WXMAC__":
            wx.App.SetMacExitMenuItemId(quit_item.GetId())
        self.menuBar.Append(menu1, 'File')

        self.parent.SetMenuBar(self.menuBar)

        self.parent.Bind(wx.EVT_MENU, self.close, id=103)
        self.parent.Bind(wx.EVT_MENU, _app_quit, id=120)
        self.parent.Bind(wx.EVT_CLOSE, self.close)
        
        max = 0
        for key in _OBJECTS_TREE.keys():
            max += (len(_OBJECTS_TREE)+1)
        
        dlg = wx.ProgressDialog("Doc",
                               "Building manual...",
                               maximum = max,
                               parent=self,
                               style = wx.PD_APP_MODAL)
        keepGoing = True
        count = 0                       
        win = self.makePanel()
        self.AddPage(win, "--- pyo documentation ---")
        for key in sorted(_OBJECTS_TREE.keys()):
            count += 1
            win = self.makePanel(key)
            self.AddPage(win, key)
            for obj in _OBJECTS_TREE[key]:
                count += 1
                win = self.makePanel(obj)
                self.AddSubPage(win, obj)
                if count <= max:
                    (keepGoing, skip) = dlg.Update(count)
        dlg.Destroy()        

        self.setStyle()
        
        # This is a workaround for a sizing bug on Mac...
        wx.FutureCall(100, self.AdjustSize)

    def close(self, evt):
        self.parent.Hide()

    def AdjustSize(self):
        self.GetTreeCtrl().InvalidateBestSize()
        self.SendSizeEvent()

    def makePanel(self, obj=None):
        panel = wx.Panel(self, -1)
        if obj != None:
            try:
                args = obj + ':\n\n' + eval(obj).args() + '\n'
            except:
                args = obj + ':\n\n'    
            try:
                text = eval(obj).__doc__
                methods = self.getMethodsDoc(text, obj)
                panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600))
                panel.win.SetMarginWidth(1, 0)
                panel.win.SetText(args + text + methods)
            except:
                panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600))
                panel.win.SetText(args + "\nnot documented yet...")                
        else:
            panel.win = stc.StyledTextCtrl(panel, -1, size=(600, 600))
            panel.win.SetText("pyo documentation")
        panel.win.SetReadOnly(True)    
        _ed_set_style(panel.win, True)
            
        def OnPanelSize(evt, win=panel.win):
            win.SetPosition((0,0))
            win.SetSize(evt.GetSize())
            
        panel.Bind(wx.EVT_SIZE, OnPanelSize)
        return panel

    def getMethodsDoc(self, text, obj):
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
                        methods += obj + '.' + meth + args + ':\n'
                        docstr = getattr(eval(obj), meth).__doc__.rstrip()
                        methods += docstr + '\n\n    '
                    
            if 'Methods:' in line: 
                flag = True
                methods += 'Methods details:\n\n    '
                 
            for key in _DOC_KEYWORDS:
                if key != 'Methods':
                    if key in line: 
                        flag = False
        
        return methods
 
    def setStyle(self):
        tree = self.GetTreeCtrl()
        tree.SetBackgroundColour(_STYLES_FACES['background'])
        root = tree.GetRootItem()
        tree.SetItemTextColour(root, _STYLES_FACES['identifier'])
        (child, cookie) = tree.GetFirstChild(root)
        while child.IsOk():
            tree.SetItemTextColour(child, _STYLES_FACES['identifier'])
            if tree.ItemHasChildren(child):
                (child2, cookie2) = tree.GetFirstChild(child)
                while child2.IsOk():
                    tree.SetItemTextColour(child2, _STYLES_FACES['identifier'])
                    (child2, cookie2) = tree.GetNextChild(child, cookie2)
            (child, cookie) = tree.GetNextChild(root, cookie)

class Timeline(wx.Frame):
    def __init__(self, interpreter, path=None):
        wx.Frame.__init__(self, parent=None, id=wx.ID_ANY, title="Timeline", pos=(50, 300), size=(800, 202))
        self.SetMaxSize((-1, 202))
        self.interpreter = interpreter
        self.path = ""
        self.page = 0
        self.currentTime = 0

        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(133, "Save\tCtrl+S", "Saves timeline")
        menu1.Append(134, "Save as\tCtrl+S", "Saves timeline as...")
        menu1.Append(103, "Close\tCtrl+W", "Closes front window")
        quit_item = menu1.Append(120, "Quit\tCtrl+Q")
        if wx.Platform=="__WXMAC__":
            wx.App.SetMacExitMenuItemId(quit_item.GetId())
        self.menuBar.Append(menu1, 'File')
        menu4 = wx.Menu()
        menu4.Append(130, "pyo documentation\tCtrl+U", "Shows pyo objects manual pages.")
        self.menuBar.Append(menu4, 'Manual')
        self.SetMenuBar(self.menuBar)

        self.Bind(wx.EVT_MENU, self.save, id=133)
        self.Bind(wx.EVT_MENU, self.saveas, id=134)
        self.Bind(wx.EVT_MENU, self.delete, id=103)
        self.Bind(wx.EVT_MENU, _app_quit, id=120)
        self.Bind(wx.EVT_CLOSE, self.delete)
        self.Bind(wx.EVT_SIZE, self.OnSize)

        self.PIX = 40
        self.timer = SeqTimer(time=1./self.PIX, function=self.checkTime)
        self.timer.start()
        self.isRunning = False

        mainbox = wx.BoxSizer(wx.HORIZONTAL)
        ctlbox = wx.BoxSizer(wx.VERTICAL)
        box = wx.BoxSizer(wx.VERTICAL)

        sec_label = wx.StaticText(self, -1, "  Time: ", pos=(10,2))
        font = sec_label.GetFont()
        font.SetPointSize(font.GetPointSize()-4)
        font.SetFaceName('Monaco')
        sec_label.SetFont(font)
        ctlbox.Add(sec_label, 0)
        mainbox.Add(ctlbox, 0)
        
        self.timeline_time = TimelineTime(self)
        self.timeline_cursor = TimelineCursor(self, self.timer.setTime)
        self.timeline_seq = TimelineSeq(self, self.interpreter)
        box.Add(self.timeline_time, 0, wx.EXPAND)
        box.Add(self.timeline_cursor, 0, wx.EXPAND)
        box.Add(self.timeline_seq, 1, wx.EXPAND)
        mainbox.Add(box, 1)

        self.SetSizer(mainbox)
        
        if path != None:
            self.path = path
            self.LoadFile(path)
            
        self.Show()

    def save(self, evt):
        if not self.path:
            self.saveas(None)
        else:
            self.SaveFile(self.path)
        
    def saveas(self, evt):
        dlg = wx.FileDialog(self, message="Save timeline as ...", defaultDir=os.path.expanduser('~'),
            defaultFile="", style=wx.SAVE)
        dlg.SetFilterIndex(0)

        if dlg.ShowModal() == wx.ID_OK:
            self.path = dlg.GetPath()
            self.SaveFile(self.path)
        dlg.Destroy()

    def SaveFile(self, path):
        f = open(path, "w")
        f.write(str(self.timeline_seq.getEvents()))
        f.close()

    def LoadFile(self, path):
        f = open(path, "r")
        self.timeline_seq.setEvents(eval(f.read()))
        self.timeline_seq.Refresh()
        f.close()
        
    def delete(self, evt):
        self.timer.delete()
        self.Destroy()
        
    def checkTime(self, currentTime):
        self.currentTime = currentTime
        events = self.timeline_seq.getEvents()
        for event in events:
            if self.currentTime == event[0]:
                self.interpreter.ExecuteWithoutEVC(event[2] + '\n')    
        self.setPage()

        self.timeline_cursor.setPosition(currentTime)

    def OnSize(self, evt):
        try:
            self.setPage()
        except:
            pass
        evt.Skip()

    def setPage(self):
        s = self.timeline_cursor.GetSize()[0] / self.PIX * self.PIX
        page = self.currentTime / s
        if page != self.page:
            self.page = page
            self.timeline_time.setPage(page)
            self.timeline_cursor.setPage(page)
            self.timeline_seq.setPage(page)
        
    def KeyDown(self, evt):
        if evt.GetKeyCode() == 32:        
            if not self.isRunning:
                self.timer.play()
                self.isRunning = True
            else:    
                self.timer.stop()
                self.isRunning = False
        evt.Skip()

    def setToZero(self):
        self.currentTime = 0
        self.timer.setTime(0)
        self.timeline_cursor.setPosition(0)
        self.setPage()
        
    def rewind(self):
        self.currentTime = self.timer.getTime()-2
        if self.currentTime < 0:
            self.currentTime = 0
        self.timer.setTime(self.currentTime)
        self.timeline_cursor.setPosition(self.currentTime)
        self.setPage()
    
    def fastForward(self):
        self.currentTime = self.timer.getTime()+2
        self.timer.setTime(self.currentTime)
        self.timeline_cursor.setPosition(self.currentTime)
        self.setPage()

    def pageDown(self):
        page = self.page - 1
        if page <= 0:
            self.setToZero()
            return
            
        s = self.timeline_cursor.GetSize()[0] / self.PIX * self.PIX
        self.currentTime = s * page
        self.timer.setTime(self.currentTime)
        self.timeline_cursor.setPosition(self.currentTime)
        self.setPage()

    def pageUp(self):
        page = self.page + 1
        s = self.timeline_cursor.GetSize()[0] / self.PIX * self.PIX
        self.currentTime = s * page
        self.timer.setTime(self.currentTime)
        self.timeline_cursor.setPosition(self.currentTime)
        self.setPage()
        
class TimelineTime(wx.Panel):
    def __init__(self, parent):
        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY, pos=(0,0), size=(-1, 10))
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)  
        self.parent = parent
        self.PIX = self.parent.PIX
        self.page = 0
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_KEY_DOWN, self.KeyDown)

    def setPage(self, page):
        self.page = page
        self.Refresh()
        
    def KeyDown(self, evt):
        if evt.GetKeyCode() == 32:        
            self.parent.KeyDown(evt)
        elif evt.GetKeyCode() == 314 and evt.ShiftDown():
            self.parent.setToZero()
        elif evt.GetKeyCode() == 314:
            self.parent.rewind()
        elif evt.GetKeyCode() == 316:
            self.parent.fastForward()    
        elif evt.GetKeyCode() == 317:
            self.parent.pageDown()
        elif evt.GetKeyCode() == 315:
            self.parent.pageUp()    
        evt.Skip()
        
    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)

        dc.SetBrush(wx.Brush("#EEEEEE", wx.SOLID))
        dc.Clear()

        font = dc.GetFont()
        font.SetPointSize(font.GetPointSize()-6)
        font.SetFaceName('Monaco')
        dc.SetFont(font)

        num_marks = w / self.PIX + 1

        # Draw background
        dc.SetPen(wx.Pen("#777777", width=1, style=wx.SOLID))
        dc.DrawRectangle(0, 0, w, h)
        
        dc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
        for i in range(num_marks):
            first = self.page * (num_marks - 1)
            if first < 0: first = 0
            num = i + first
            minutes = num / 60
            seconds = num % 60
            dc.DrawText("%d:%.2d" % (minutes, seconds), i*self.PIX, 1)
            
        evt.Skip()
        
class TimelineCursor(wx.Panel):
    def __init__(self, parent, function):
        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY, pos=(0,0), size=(-1, 10))
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)  
        self.parent = parent
        self.PIX = self.parent.PIX
        self.function = function
        self.pos = 0
        self.page = 0
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_KEY_DOWN, self.KeyDown)

    def setPage(self, page):
        self.page = page
        self.Refresh()

    def KeyDown(self, evt):
        if evt.GetKeyCode() == 32:        
            self.parent.KeyDown(evt)
        elif evt.GetKeyCode() == 314 and evt.ShiftDown():
            self.parent.setToZero()
        elif evt.GetKeyCode() == 314:
            self.parent.rewind()
        elif evt.GetKeyCode() == 316:
            self.parent.fastForward()    
        elif evt.GetKeyCode() == 317:
            self.parent.pageDown()
        elif evt.GetKeyCode() == 315:
            self.parent.pageUp()    
        evt.Skip()
        
    def MouseDown(self, evt):
        if not self.parent.isRunning:
            pos = evt.GetPosition()
            self.setPosition(pos[0])
            self.function(pos[0])
        evt.Skip()
        
    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)

        dc.SetBrush(wx.Brush("#AAAAAA", wx.SOLID))
        dc.Clear()

        # Draw background
        dc.SetPen(wx.Pen("#777777", width=1, style=wx.SOLID))
        dc.DrawRectangle(0, 0, w, h)
                
        pos = self.pos % (w/self.PIX*self.PIX)
        dc.SetBrush(wx.Brush("#000000", wx.SOLID))
        dc.SetPen(wx.Pen("#666666", width=1, style=wx.SOLID))
        dc.DrawLine(pos+1, 0, pos+1, h)
        dc.DrawLine(pos-1, 0, pos-1, h)
        dc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
        dc.DrawLine(pos, 0, pos, h)

        evt.Skip()

    def setPosition(self, position):
        self.pos = position
        self.Refresh()
     
class TimelineSeq(wx.Panel):
    def __init__(self, parent, interpreter):
        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY, pos=(0,10), size=(-1, 160))
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)  
        self.SetMaxSize((-1, 160))
        self.parent = parent
        self.PIX = self.parent.PIX
        self.page = 0
        self.interpreter = interpreter
        self._events = []
        self.x_size = 60
        self.y_size = 20
        self.selected = None
        self.offset = (0,0)
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_LEFT_DCLICK, self.DoubleClick)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_KEY_DOWN, self.KeyDown)

    def getEvents(self):
        return self._events

    def setEvents(self, events):
        self._events = events

    def setPage(self, page):
        self.page = page
        self.Refresh()
        
    def KeyDown(self, evt):
        if evt.GetKeyCode() == 32:        
            self.parent.KeyDown(evt)
        elif evt.GetKeyCode() == 314 and evt.ShiftDown():
            self.parent.setToZero()
        elif evt.GetKeyCode() == 314:
            self.parent.rewind()
        elif evt.GetKeyCode() == 316:
            self.parent.fastForward()    
        elif evt.GetKeyCode() == 317:
            self.parent.pageDown()
        elif evt.GetKeyCode() == 315:
            self.parent.pageUp()    
        evt.Skip()
        
    def MouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()
            self.selected = None

    def DoubleClick(self, evt):
        w,h = self.GetSize()
        pos = evt.GetPosition()
        s = w / self.PIX * self.PIX
        pos[0] = s * self.page + pos[0]        
        clic_on_object = False
        if self._events:
            for event in self._events:
                if event[1].Contains(pos):
                    clic_on_object = True
                    dlg = wx.TextEntryDialog(self, 'Enter your code here!', 'Event', event[2], 
                                 style=wx.OK | wx.CANCEL | wx.TE_MULTILINE)
                    if dlg.ShowModal() == wx.ID_OK:
                        text = dlg.GetValue()
                        event[2] = text
                    dlg.Destroy()
                    break    
        if not clic_on_object:            
            rect = wx.Rect(pos[0], pos[1] / self.y_size * self.y_size, self.x_size, self.y_size)
            dlg = wx.TextEntryDialog(self, 'Enter your code here!', 'Event', '', 
                                     style=wx.OK | wx.CANCEL | wx.TE_MULTILINE)
            if dlg.ShowModal() == wx.ID_OK:
                text = dlg.GetValue()
                time = pos[0] 
                self._events.append([time, rect, text])
            dlg.Destroy()   
        self.Refresh()

    def MouseDown(self, evt):
        w,h = self.GetSize()
        pos = evt.GetPosition()
        s = w / self.PIX * self.PIX
        pos[0] = s * self.page + pos[0]        
        if evt.AltDown():
            if self._events:
                for event in self._events:
                    if event[1].Contains(pos):
                        self.CaptureMouse()
                        self.offset = (event[1][0] - pos[0], event[1][1] - pos[1])
                        rect = wx.Rect(event[1][0], event[1][1], self.x_size, self.y_size)
                        text = event[2]
                        time = event[1][0]
                        self._events.append([time, rect, text])
                        self.selected = len(self._events)-1
                        self.Refresh()
                        return
        elif evt.ShiftDown():
            if self._events:
                for event in self._events:
                    if event[1].Contains(pos):
                        del self._events[self._events.index(event)]
                        self.selected = None
                        self.Refresh()
                        return
        elif self._events:
            for event in self._events:
                if event[1].Contains(pos):
                    self.CaptureMouse()
                    self.offset = (event[1][0] - pos[0], event[1][1] - pos[1])
                    self.selected = self._events.index(event)
                    self.Refresh()
                    return
                    
        self.selected = None
        self.Refresh()            
        
    def MouseMotion(self, evt):
        w,h = self.GetSize()
        pos = evt.GetPosition()
        s = w / self.PIX * self.PIX
        poff = s * self.page        
        if evt.Dragging() and evt.LeftIsDown():
            if self.selected != None:
                if (pos[0] + self.offset[0]) < 0: X = 0 + poff
                elif (pos[0] + self.offset[0] + self.x_size) > w: X = w - self.x_size + poff
                else: X = pos[0] + self.offset[0] + poff
                if (pos[1] + self.offset[1]) < 0: Y = 0
                elif (pos[1] + self.offset[1] + self.y_size) > h: Y = h - self.y_size
                else: Y = (pos[1] + self.offset[1]) / self.y_size * self.y_size
                self._events[self.selected][0] = X
                self._events[self.selected][1].SetX(X)
                self._events[self.selected][1].SetY(Y)
                self.Refresh()
        
    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)

        dc.SetBrush(wx.Brush("#EEEEEE", wx.SOLID))
        dc.Clear()

        font = dc.GetFont()
        font.SetPointSize(font.GetPointSize()-4)
        font.SetFaceName('Monaco')
        dc.SetFont(font)
        
        # Draw background
        dc.SetPen(wx.Pen("#AAAAAA", width=1, style=wx.SOLID))
        dc.DrawRectangle(0, 0, w, h)

        num_marks = w / self.PIX + 1
        dc.SetPen(wx.Pen("#AAAAAA", width=1, style=wx.SOLID))
        for i in range(num_marks):
            mk_xpos = i * self.PIX
            dc.DrawLine(mk_xpos, 0, mk_xpos, 400)
            
        if self._events:
            dc.SetBrush(wx.Brush("#EEEEEE", wx.SOLID))
            dc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
            first = self.page * (w/self.PIX*self.PIX)
            for i, event in enumerate(self._events):
                if first <= event[0] <= (first + w):
                    if i == self.selected: dc.SetPen(wx.Pen("#000000", width=2, style=wx.SOLID))
                    else: dc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
                    x_pos = event[1][0] - first
                    dc.DrawRoundedRectangle(x_pos, event[1][1], event[1][2], event[1][3], 2)
                    dc.DrawText(event[2][0:10], x_pos+3, event[1][1]+4)

        evt.Skip()

class SeqTimer(threading.Thread):
    def __init__(self, time=.05, function=None):
        threading.Thread.__init__(self)
        self.currentTime = 0
        self.time = time
        self.function = function
        self._terminated = False
        self._started = False
        
    def run(self):
        while not self._terminated:
            if self._started:
                self.function(self.currentTime)
                self.currentTime += 1 #self.time
            time.sleep(self.time)

    def setTime(self, time):
        self.currentTime = time

    def getTime(self):
        return self.currentTime
        
    def play(self):
        self._started = True
        
    def stop(self):
        self._started = False
        
    def delete(self):
        self._terminated = True    
                             
# Check for files to open
filesToOpen = []
if len(sys.argv) > 1:
    for f in sys.argv[1:]:
        if os.path.isfile(f):    
            filesToOpen.append(f)
        else:
            pass

# Create app and frames
_MAIN_APP = wx.PySimpleApp()

_INTER_FRAME = wx.Frame(None, -1, title='pyo interpreter', size=(500, 400))
_INTERPRETER = Interpreter(_INTER_FRAME)

_ED_FRAMES = []
_EDITORS = []
if filesToOpen:
    for i, f in enumerate(filesToOpen):
        filepath, name = os.path.split(f)
        sys.path.append(filepath)
        _ED_FRAMES.append(wx.Frame(None, -1, title=name, pos=(400+i*20, -1), size=(600, 700)))
        _EDITORS.append(ScriptEditor(_ED_FRAMES[-1], -1, _INTERPRETER, f))
else:        
    _ED_FRAMES.append(wx.Frame(None, -1, title='pyo editor', pos=(400, -1), size=(600, 700)))
    _EDITORS.append(ScriptEditor(_ED_FRAMES[-1], -1, _INTERPRETER))

_INTER_FRAME.Show()
for f in _ED_FRAMES:
    f.Show()

_MAIN_APP.MainLoop()
