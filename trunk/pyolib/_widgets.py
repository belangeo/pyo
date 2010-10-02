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
import math, sys, os

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
TABLEWINDOWS = []
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
        f.Show()

def wxCreateDelayedTableWindows():
    for win in TABLEWINDOWS:
        if WITH_PIL: f = ViewTable_withPIL(None, win[0], win[1])
        else: f = ViewTable_withoutPIL(None, win[0], win[1])
        f.Show()
        f.SetTitle("Table waveform")

def wxCreateDelayedMatrixWindows():
    for win in MATRIXWINDOWS:
        if WITH_PIL: f = ViewMatrix_withPIL(None, win[0], win[1])
        else: f = ViewMatrix_withoutPIL(None, win[0], win[1])
        f.Show()
        f.SetTitle("Matrix viewer")
    
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
        
def createViewTableWindow(samples, wxnoserver=False, tableclass=None):
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        if WITH_PIL: f = ViewTable_withPIL(win, samples)
        else: f = ViewTable_withoutPIL(win, samples)
        win.resizable(False, False)
        win.title("Table waveform")
    else:
        if wxnoserver or wx.GetApp() != None:
            if wx.GetApp() == None:
                root = createRootWindow()
            else:
                root = None    
            if WITH_PIL: f = ViewTable_withPIL(None, samples, tableclass)
            else: f = ViewTable_withoutPIL(None, samples, tableclass)
            f.Show()
            f.SetTitle("Table waveform")
        else:
            TABLEWINDOWS.append([samples, tableclass])    
        
def createViewMatrixWindow(samples, size, wxnoserver=False):
    if not WITH_PIL: print """The Python Imaging Library is not installed. 
It helps a lot to speed up matrix drawing!"""
    if not PYO_USE_WX:
        createRootWindow()    
        win = tkCreateToplevelWindow()
        if WITH_PIL: f = ViewMatrix_withPIL(win, samples, size)
        else: f = ViewMatrix_withoutPIL(win, samples, size)
        win.resizable(False, False)
        win.title("Matrix viewer")
    else:
        if wxnoserver or wx.GetApp() != None:
            if wx.GetApp() == None:
                root = createRootWindow()
            else:
                root = None    
            if WITH_PIL: f = ViewMatrix_withPIL(None, samples, size)
            else: f = ViewMatrix_withoutPIL(None, samples, size)
            f.Show()
            f.SetTitle("Matrix viewer")
        else:
            MATRIXWINDOWS.append([samples,size])    
        
def createServerGUI(nchnls, start, stop, recstart, recstop, setAmp, started, locals, shutdown):
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        f = ServerGUI(win, nchnls, start, stop, recstart, recstop, setAmp, started, locals, shutdown)
        f.master.title("pyo server")
        f.focus_set()
    else:
        win = createRootWindow()
        f = ServerGUI(None, nchnls, start, stop, recstart, recstop, setAmp, started, locals, shutdown) 
        f.SetTitle("pyo server") 
        f.Show()
        wx.CallAfter(wxCreateDelayedCtrlWindows)
        wx.CallAfter(wxCreateDelayedTableWindows)
        wx.CallAfter(wxCreateDelayedMatrixWindows)
    return f, win
        