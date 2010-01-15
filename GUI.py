#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys, keyword, string, inspect
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
        
        self.menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(99, "New\tCtrl+N", "Creates a new editor page")
        menu1.Append(100, "Open\tCtrl+O", "Opens an existing file")
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
