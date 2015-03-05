# -*- coding: utf-8 -*-
"""
Copyright 2009-2015 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from types import ListType, FloatType, IntType
import math, sys, os, random

try:
    from PIL import Image, ImageDraw, ImageTk
    WITH_PIL = True
except:
    WITH_PIL = False

try:
    try:
        import wxversion
        if (wxversion.checkInstalled("2.8")):
            wxversion.ensureMinimal("2.8")
    except:
        pass
    import wx
    from _wxwidgets import *
    PYO_USE_WX = True
except:
    PYO_USE_WX = False

PYO_USE_TK = False
if not PYO_USE_WX:
    try:
        from Tkinter import *
        from _tkwidgets import *
        PYO_USE_TK = True
        print """
WxPython is not found for the current python version.
Pyo will use a minimal GUI toolkit written with Tkinter.
This toolkit has limited functionnalities and is no more
maintained or updated. If you want to use all of pyo's
GUI features, you should install WxPython, available here:
http://www.wxpython.org/
"""
    except:
        PYO_USE_TK = False
        print """
Neither WxPython nor Tkinter are found for the current python version.
Pyo's GUI features are disabled. For a complete GUI toolkit, you should
consider installing WxPython, available here: http://www.wxpython.org/
"""

X, Y, CURRENT_X, MAX_X, NEXT_Y = 800, 700, 30, 30, 30
WINDOWS = []
CTRLWINDOWS = []
GRAPHWINDOWS = []
DATAGRAPHWINDOWS = []
TABLEWINDOWS = []
SNDTABLEWINDOWS = []
MATRIXWINDOWS = []
SPECTRUMWINDOWS = []
SCOPEWINDOWS = []
WX_APP = False

def createRootWindow():
    global WX_APP
    if not PYO_USE_WX:
        if len(WINDOWS) == 0:
            root = Tk()
            root.withdraw()
            return None
        else:
            return None
    else:
        if not WX_APP:
            win = wx.App(False)
            WX_APP = True
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
        win[0]._setGraphFrame(f)
        if win[2] == None: title = win[0].__class__.__name__
        else: title = win[2]
        wxDisplayWindow(f, title)

def wxCreateDelayedTableWindows():
    global CURRENT_X, MAX_X, NEXT_Y
    for win in TABLEWINDOWS:
        object = win[3]
        f = ViewTable(None, win[0], win[1], object)
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

def wxCreateDelayedScopeWindows():
    for win in SCOPEWINDOWS:
        f = ScopeDisplay(None, win[0])
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
        if wxnoserver or WX_APP:
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
        if wxnoserver or WX_APP:
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
        if wxnoserver or WX_APP:
            root = createRootWindow()
            f = DataTableGrapher(None, obj, yrange)
            if title == None: title = obj.__class__.__name__
            wxShowWindow(f, title, root)
            obj._setGraphFrame(f)
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
        if wxnoserver or WX_APP:
            root = createRootWindow()
            f = ViewTable(None, samples, tableclass, object)
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
        if wxnoserver or WX_APP:
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
        if wxnoserver or WX_APP:
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
        if wxnoserver or WX_APP:
            root = createRootWindow()
            f = SpectrumDisplay(None, object)
            if title == None: title = object.__class__.__name__
            wxShowWindow(f, title, root)
            if object != None:
                object._setViewFrame(f)
        else:
            SPECTRUMWINDOWS.append([object, title])

def createScopeWindow(object, title, wxnoserver=False):
    if not PYO_USE_WX:
        print "WxPython must be installed to use the Scope display."
    else:
        if wxnoserver or WX_APP:
            root = createRootWindow()
            f = ScopeDisplay(None, object)
            if title == None: title = object.__class__.__name__
            wxShowWindow(f, title, root)
            if object != None:
                object._setViewFrame(f)
        else:
            SCOPEWINDOWS.append([object, title])

def createServerGUI(nchnls, start, stop, recstart, recstop, setAmp, started, locals, shutdown, meter, timer, amp, exit):
    global X, Y, MAX_X, NEXT_Y
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        f = ServerGUI(win, nchnls, start, stop, recstart, recstop, setAmp, started, locals, shutdown, meter, timer, amp)
        f.master.title("pyo server")
        f.focus_set()
    else:
        win = createRootWindow()
        f = ServerGUI(None, nchnls, start, stop, recstart, recstop, setAmp, started, locals, shutdown, meter, timer, amp, exit)
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
        wx.CallAfter(wxCreateDelayedScopeWindows)
        wx.CallAfter(f.Raise)
    return f, win
