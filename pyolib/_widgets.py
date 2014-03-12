# -*- coding: utf-8 -*-
"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public Licensehack for OSX display
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from types import ListType, FloatType, IntType
import math, sys, os, random

try:
    from PIL import Image, ImageDraw, ImageTk
    WITH_PIL = True
except:
    WITH_PIL = False

try:
    import wxversion
    if (wxversion.checkInstalled("2.8")):
        wxversion.ensureMinimal("2.8")
    import wx
    from _wxwidgets import *
    PYO_USE_WX = True
except:
    PYO_USE_WX = False

if not PYO_USE_WX:
    try:
        from Tkinter import *
        from _tkwidgets import *
    except:
        if sys.platform == "linux2":
            response = raw_input("""python-tk package is missing! It is needed to use pyo graphical interfaces.
Do you want to install it? (yes/no): """)
            if response == 'yes':
                os.system('sudo apt-get install python-tk')
        else:
            print "Tkinter is missing! It is needed to use pyo graphical interfaces. Please install it!"
        sys.exit()

X, Y, CURRENT_X, MAX_X, NEXT_Y = 800, 700, 30, 30, 30
WINDOWS = []
CTRLWINDOWS = []
GRAPHWINDOWS = []
DATAGRAPHWINDOWS = []
TABLEWINDOWS = []
SNDTABLEWINDOWS = []
MATRIXWINDOWS = []
SPECTRUMWINDOWS = []

def createRootWindow():
    if not PYO_USE_WX:
        if len(WINDOWS) == 0:
            root = Tk()
            root.withdraw()
            return None
        else:
            return None
    else:        
        if wx.GetApp() == None: 
            win = wx.App(False) 
            return win
        else:
            return None

def tkCloseWindow(win):
    win.destroy()
    if win in WINDOWS: WINDOWS.remove(win)

def tkCloseWindowFromKeyboard(event):
    win = event.widget
    if not isinstance(win, ServerGUI): 
        win.destroy()
        if win in WINDOWS: WINDOWS.remove(win)
                
def tkCreateToplevelWindow():
    win = Toplevel()
    WINDOWS.append(win)
    win.protocol('WM_DELETE_WINDOW', lambda win=win: tkCloseWindow(win))
    win.bind("<Escape>", tkCloseWindowFromKeyboard)
    return win

def wxDisplayWindow(f, title):
    global CURRENT_X, MAX_X, NEXT_Y
    f.SetTitle(title)
    x, y = f.GetSize()
    if sys.platform == "linux2":
        y += 25
    if y + NEXT_Y < Y:
        px, py, NEXT_Y = CURRENT_X, NEXT_Y, NEXT_Y + y
        if x + CURRENT_X > MAX_X: MAX_X = x + CURRENT_X
        f.SetPosition((px, py))
    elif x + MAX_X < X:
        px, py, NEXT_Y, CURRENT_X = MAX_X, 50, 50 + y, MAX_X
        if x + CURRENT_X > MAX_X: MAX_X = x + CURRENT_X
        f.SetPosition((px, py))
    else:
        f.SetPosition((random.randint(250,500), random.randint(200,400)))
    f.Show()

def wxShowWindow(f, title, root):
    f.SetTitle(title)
    f.Show()
    if root != None:
        root.MainLoop()
    
def wxCreateDelayedCtrlWindows():
    for win in CTRLWINDOWS:
        f = PyoObjectControl(None, win[0], win[1])
        if win[2] == None: title = win[0].__class__.__name__
        else: title = win[2]
        wxDisplayWindow(f, title)

def wxCreateDelayedGraphWindows():
    for win in GRAPHWINDOWS:
        f = TableGrapher(None, win[0], win[1], win[2], win[3])
        if win[4] == None: title = win[0].__class__.__name__
        else: title = win[4]
        wxDisplayWindow(f, title)

def wxCreateDelayedDataGraphWindows():
    for win in DATAGRAPHWINDOWS:
        f = DataTableGrapher(None, win[0], win[1])
        if win[2] == None: title = win[0].__class__.__name__
        else: title = win[2]
        wxDisplayWindow(f, title)

def wxCreateDelayedTableWindows():
    global CURRENT_X, MAX_X, NEXT_Y
    for win in TABLEWINDOWS:
        object = win[3]
        if WITH_PIL: f = ViewTable_withPIL(None, win[0], win[1], object)
        else: f = ViewTable_withoutPIL(None, win[0], win[1], object)
        if object != None:
            object._setViewFrame(f)
        wxDisplayWindow(f, win[2])

def wxCreateDelayedSndTableWindows():
    global CURRENT_X, MAX_X, NEXT_Y
    for win in SNDTABLEWINDOWS:
        f = SndViewTable(None, win[0], win[1], win[3])
        win[0]._setViewFrame(f)
        wxDisplayWindow(f, win[2])

def wxCreateDelayedMatrixWindows():
    global CURRENT_X, MAX_X, NEXT_Y
    for win in MATRIXWINDOWS:
        object = win[3]
        if WITH_PIL: f = ViewMatrix_withPIL(None, win[0], win[1], object)
        else: f = ViewMatrix_withoutPIL(None, win[0], win[1], object)
        if object != None:
            object._setViewFrame(f)
        wxDisplayWindow(f, win[2])

def wxCreateDelayedSpectrumWindows():
    for win in SPECTRUMWINDOWS:
        f = SpectrumDisplay(None, win[0])
        if win[1] == None: title = win[0].__class__.__name__
        else: title = win[1]
        if win[0] != None:
            win[0]._setViewFrame(f)
        wxDisplayWindow(f, title)
    
def createCtrlWindow(obj, map_list, title, wxnoserver=False):
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        f = PyoObjectControl(win, obj, map_list)
        win.resizable(True, False)
        if title == None: title = obj.__class__.__name__
        win.title(title)
    else:
        if wxnoserver or wx.GetApp() != None:
            root = createRootWindow()
            f = PyoObjectControl(None, obj, map_list)
            if title == None: title = obj.__class__.__name__
            wxShowWindow(f, title, root)
        else:
            CTRLWINDOWS.append([obj, map_list, title])

def createGraphWindow(obj, mode, xlen, yrange, title, wxnoserver=False):
    if not PYO_USE_WX:
        print "WxPython must be installed to use the 'graph()' method."
    else:
        if wxnoserver or wx.GetApp() != None:
            root = createRootWindow()
            f = TableGrapher(None, obj, mode, xlen, yrange)
            if title == None: title = obj.__class__.__name__
            wxShowWindow(f, title, root)
        else:
            GRAPHWINDOWS.append([obj, mode, xlen, yrange, title])   

def createDataGraphWindow(obj, yrange, title, wxnoserver=False):
    if not PYO_USE_WX:
        print "WxPython must be installed to use the 'graph()' method."
    else:
        if wxnoserver or wx.GetApp() != None:
            root = createRootWindow()
            f = DataTableGrapher(None, obj, yrange)
            if title == None: title = obj.__class__.__name__
            wxShowWindow(f, title, root)
        else:
            DATAGRAPHWINDOWS.append([obj, yrange, title])   
        
def createViewTableWindow(samples, title="Table waveform", wxnoserver=False, tableclass=None, object=None):
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        if WITH_PIL: f = ViewTable_withPIL(win, samples)
        else: f = ViewTable_withoutPIL(win, samples)
        win.resizable(False, False)
        win.title(title)
    else:
        if wxnoserver or wx.GetApp() != None:
            root = createRootWindow()
            if WITH_PIL: f = ViewTable_withPIL(None, samples, tableclass, object)
            else: f = ViewTable_withoutPIL(None, samples, tableclass, object)
            wxShowWindow(f, title, root)
            if object != None:
                object._setViewFrame(f)
        else:
            TABLEWINDOWS.append([samples, tableclass, title, object])    

def createSndViewTableWindow(obj, title="Table waveform", wxnoserver=False, tableclass=None, mouse_callback=None):
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        if WITH_PIL: f = ViewTable_withPIL(win, obj._base_objs[0].getViewTable())
        else: f = ViewTable_withoutPIL(win, obj._base_objs[0].getViewTable())
        win.resizable(False, False)
        win.title(title)
    else:
        if wxnoserver or wx.GetApp() != None:
            root = createRootWindow()
            f = SndViewTable(None, obj, tableclass, mouse_callback)
            if title == None: title = obj.__class__.__name__
            wxShowWindow(f, title, root)
            obj._setViewFrame(f)
        else:
            SNDTABLEWINDOWS.append([obj, tableclass, title, mouse_callback])
        
def createViewMatrixWindow(samples, size, title="Matrix viewer", wxnoserver=False, object=None):
    if not WITH_PIL: print """The Python Imaging Library is not installed. 
It helps a lot to speed up matrix drawing!"""
    if not PYO_USE_WX:
        createRootWindow()    
        win = tkCreateToplevelWindow()
        if WITH_PIL: f = ViewMatrix_withPIL(win, samples, size)
        else: f = ViewMatrix_withoutPIL(win, samples, size)
        win.resizable(False, False)
        win.title(title)
    else:
        if wxnoserver or wx.GetApp() != None:
            root = createRootWindow()
            if WITH_PIL: f = ViewMatrix_withPIL(None, samples, size, object)
            else: f = ViewMatrix_withoutPIL(None, samples, size, object)
            wxShowWindow(f, title, root)
            if object != None:
                object._setViewFrame(f)
        else:
            MATRIXWINDOWS.append([samples,size,title, object])    

def createSpectrumWindow(object, title, wxnoserver=False):
    if not PYO_USE_WX:
        print "WxPython must be installed to use the Spectrum display."
    else:
        if wxnoserver or wx.GetApp() != None:
            root = createRootWindow()
            f = SpectrumDisplay(None, object)
            if title == None: title = object.__class__.__name__
            wxShowWindow(f, title, root)
            if object != None:
                object._setViewFrame(f)
        else:
            SPECTRUMWINDOWS.append([object, title])   
        
def createServerGUI(nchnls, start, stop, recstart, recstop, setAmp, started, locals, shutdown, meter, timer, amp):
    global X, Y, MAX_X, NEXT_Y
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        f = ServerGUI(win, nchnls, start, stop, recstart, recstop, setAmp, started, locals, shutdown, meter, timer, amp)
        f.master.title("pyo server")
        f.focus_set()
    else:
        win = createRootWindow()
        f = ServerGUI(None, nchnls, start, stop, recstart, recstop, setAmp, started, locals, shutdown, meter, timer, amp) 
        f.SetTitle("pyo server")
        f.SetPosition((30, 30))
        f.Show()
        X,Y = wx.SystemSettings.GetMetric(wx.SYS_SCREEN_X)-50, wx.SystemSettings.GetMetric(wx.SYS_SCREEN_Y)-50
        if sys.platform == "linux2":
            MAX_X, NEXT_Y = f.GetSize()[0]+30, f.GetSize()[1]+55
        else:
            MAX_X, NEXT_Y = f.GetSize()[0]+30, f.GetSize()[1]+30
        wx.CallAfter(wxCreateDelayedTableWindows)
        wx.CallAfter(wxCreateDelayedGraphWindows)
        wx.CallAfter(wxCreateDelayedDataGraphWindows)
        wx.CallAfter(wxCreateDelayedSndTableWindows)
        wx.CallAfter(wxCreateDelayedMatrixWindows)
        wx.CallAfter(wxCreateDelayedCtrlWindows)
        wx.CallAfter(wxCreateDelayedSpectrumWindows)
        wx.CallAfter(f.Raise)
    return f, win
        