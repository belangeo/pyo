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

WINDOWS = []
CTRLWINDOWS = []
GRAPHWINDOWS = []
TABLEWINDOWS = []
SNDTABLEWINDOWS = []
MATRIXWINDOWS = []

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
            win = wx.PySimpleApp() 
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

def wxCreateDelayedCtrlWindows():
    for win in CTRLWINDOWS:
        f = PyoObjectControl(None, win[0], win[1])
        if win[2] == None: title = win[0].__class__.__name__
        else: title = win[2]
        f.SetTitle(title)
        f.SetPosition((random.randint(250,500), random.randint(200,400)))
        f.Show()

def wxCreateDelayedGraphWindows():
    for win in GRAPHWINDOWS:
        f = TableGrapher(None, win[0], win[1], win[2], win[3])
        if win[4] == None: title = win[0].__class__.__name__
        else: title = win[4]
        f.SetTitle(title)
        f.SetPosition((random.randint(250,500), random.randint(200,400)))
        f.Show()

def wxCreateDelayedTableWindows():
    for win in TABLEWINDOWS:
        if WITH_PIL: f = ViewTable_withPIL(None, win[0], win[1])
        else: f = ViewTable_withoutPIL(None, win[0], win[1])
        f.SetPosition((random.randint(250,500), random.randint(200,400)))
        f.SetTitle(win[2])
        f.Show()

def wxCreateDelayedSndTableWindows():
    for win in SNDTABLEWINDOWS:
        if WITH_PIL: f = SndViewTable_withPIL(None, win[0], win[1], win[3])
        else: f = SndViewTable_withoutPIL(None, win[0], win[1], win[3])
        f.SetPosition((random.randint(250,500), random.randint(200,400)))
        f.SetTitle(win[2])
        f.Show()

def wxCreateDelayedMatrixWindows():
    for win in MATRIXWINDOWS:
        if WITH_PIL: f = ViewMatrix_withPIL(None, win[0], win[1])
        else: f = ViewMatrix_withoutPIL(None, win[0], win[1])
        f.SetPosition((random.randint(250,500), random.randint(200,400)))
        f.SetTitle(win[2])
        f.Show()
    
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
            if wx.GetApp() == None:
                root = createRootWindow()
            else:
                root = None    
            f = PyoObjectControl(None, obj, map_list)
            if title == None: title = obj.__class__.__name__
            f.SetTitle(title)
            f.Show()
            if root != None:
                root.MainLoop()
        else:
            CTRLWINDOWS.append([obj, map_list, title])   

def createGraphWindow(obj, mode, xlen, yrange, title, wxnoserver=False):
    if not PYO_USE_WX:
        print "WxPython must be installed to use the 'graph' method."
        if 0:
            createRootWindow()
            win = tkCreateToplevelWindow()
            f = PyoObjectControl(win, obj, map_list)
            win.resizable(True, False)
            if title == None: title = obj.__class__.__name__
            win.title(title)
    else:
        if wxnoserver or wx.GetApp() != None:
            if wx.GetApp() == None:
                root = createRootWindow()
            else:
                root = None    
            f = TableGrapher(None, obj, mode, xlen, yrange)
            if title == None: title = obj.__class__.__name__
            f.SetTitle(title)
            f.Show()
            if root != None:
                root.MainLoop()
        else:
            GRAPHWINDOWS.append([obj, mode, xlen, yrange, title])   
        
def createViewTableWindow(samples, title="Table waveform", wxnoserver=False, tableclass=None):
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        if WITH_PIL: f = ViewTable_withPIL(win, samples)
        else: f = ViewTable_withoutPIL(win, samples)
        win.resizable(False, False)
        win.title(title)
    else:
        if wxnoserver or wx.GetApp() != None:
            if wx.GetApp() == None:
                root = createRootWindow()
            else:
                root = None    
            if WITH_PIL: f = ViewTable_withPIL(None, samples, tableclass)
            else: f = ViewTable_withoutPIL(None, samples, tableclass)
            f.Show()
            f.SetTitle(title)
        else:
            TABLEWINDOWS.append([samples, tableclass, title])    

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
            if wx.GetApp() == None:
                root = createRootWindow()
            else:
                root = None    
            if WITH_PIL: f = SndViewTable_withPIL(None, obj, tableclass, mouse_callback)
            else: f = SndViewTable_withoutPIL(None, obj, tableclass, mouse_callback)
            f.Show()
            f.SetTitle(title)
        else:
            SNDTABLEWINDOWS.append([obj, tableclass, title, mouse_callback])
        
def createViewMatrixWindow(samples, size, title="Matrix viewer", wxnoserver=False):
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
            if wx.GetApp() == None:
                root = createRootWindow()
            else:
                root = None    
            if WITH_PIL: f = ViewMatrix_withPIL(None, samples, size)
            else: f = ViewMatrix_withoutPIL(None, samples, size)
            f.Show()
            f.SetTitle(title)
        else:
            MATRIXWINDOWS.append([samples,size,title])    
        
def createServerGUI(nchnls, start, stop, recstart, recstop, setAmp, started, locals, shutdown, meter, timer, amp):
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
        f.Show()
        wx.CallAfter(wxCreateDelayedCtrlWindows)
        wx.CallAfter(wxCreateDelayedGraphWindows)
        wx.CallAfter(wxCreateDelayedTableWindows)
        wx.CallAfter(wxCreateDelayedSndTableWindows)
        wx.CallAfter(wxCreateDelayedMatrixWindows)
        wx.CallAfter(f.Raise)
    return f, win
        