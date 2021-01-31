# encoding: utf-8
"""
This module is the interface between pyo's display demands and the different
available GUI toolkits (wxpython of tkinter).

"""

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
import sys
import os

use_wx = 1
if "PYO_GUI_WX" in os.environ:
    use_wx = int(os.environ["PYO_GUI_WX"])

if use_wx:
    try:
        import wx

        PYO_USE_WX = True
    except:
        PYO_USE_WX = False
        print(
            """
WxPython is not found for the current python version.
Pyo will use a minimal GUI toolkit written with Tkinter (if available).
This toolkit has limited functionnalities and is no more
maintained or updated. If you want to use all of pyo's
GUI features, you should install WxPython, available here:
http://www.wxpython.org/
"""
        )
else:
    PYO_USE_WX = False

if PYO_USE_WX:
    from ._wxwidgets import *

PYO_USE_TK = False
if not PYO_USE_WX:
    try:
        import tkinter as tk
        PYO_USE_TK = True
    except:
        PYO_USE_TK = False
        print(
            """
Neither WxPython nor Tkinter are found for the current python version.
Pyo's GUI features are disabled. For a complete GUI toolkit, you should
consider installing WxPython, available here: http://www.wxpython.org/
"""
        )

if PYO_USE_TK:
    from ._tkwidgets import *

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
EXPREDITORWINDOWS = []
MMLEDITORWINDOWS = []
NOTEINKEYBOARDWINDOWS = []


def createRootWindow():
    "Creates the main window (app object)."
    if not PYO_USE_WX:
        if len(WINDOWS) == 0:
            root = tk.Tk()

            screen_width = root.winfo_screenwidth()
            if sys.platform == "darwin" and screen_width > 1024:
                # Scale fonts on Hi-res displays.
                import tkinter.font

                for name in tkinter.font.names(root):
                    font = tkinter.font.Font(root=root, name=name, exists=True)
                    font["size"] *= 2

            root.withdraw()
            return None
        else:
            return None
    else:
        if wx.GetApp() is None:
            win = wx.App()
            return win
        else:
            return wx.GetApp()


def tkCloseWindow(win):
    "Closes a tkinter window."
    win.destroy()
    if win in WINDOWS:
        WINDOWS.remove(win)


def tkCloseWindowFromKeyboard(event):
    "Closes a tkinter window from a keyboard event."
    win = event.widget
    if not isinstance(win, ServerGUI):
        win.destroy()
        if win in WINDOWS:
            WINDOWS.remove(win)


def tkCreateToplevelWindow():
    "Creates a tkinter top level window."
    win = tk.Toplevel()
    WINDOWS.append(win)
    win.protocol("WM_DELETE_WINDOW", lambda win=win: tkCloseWindow(win))
    win.bind("<Escape>", tkCloseWindowFromKeyboard)
    dpi_value = int(win.winfo_fpixels("1i"))
    win.tk.eval(
        """
        foreach font [font names] {
            set cursize [font configure $font -size]
            font configure $font -size [expr {int($cursize * %d / 96.0)}]
        }
    """
        % dpi_value
    )
    return win


def wxDisplayWindow(f, title):
    "Displays a wxpython window on the screen."
    global CURRENT_X, MAX_X, NEXT_Y
    f.SetTitle(title)
    x, y = f.GetSize()
    if sys.platform.startswith("linux"):
        y += 25
    if y + NEXT_Y < Y:
        px, py, NEXT_Y = CURRENT_X, NEXT_Y, NEXT_Y + y
        if x + CURRENT_X > MAX_X:
            MAX_X = x + CURRENT_X
        f.SetPosition((px, py))
    elif x + MAX_X < X:
        px, py, NEXT_Y, CURRENT_X = MAX_X, 50, 50 + y, MAX_X
        if x + CURRENT_X > MAX_X:
            MAX_X = x + CURRENT_X
        f.SetPosition((px, py))
    else:
        CURRENT_X, MAX_X, NEXT_Y = 50, 50, 50
        wxDisplayWindow(f, title)
    f.Show()


def wxShowWindow(f, title, root):
    "Shows a wxpython window on the screen."
    f.SetTitle(title)
    f.Show()
    if root is not None:
        root.MainLoop()


def wxCreateDelayedCtrlWindows():
    "Postponed a wxpython window display."
    for win in CTRLWINDOWS:
        f = PyoObjectControl(None, win[0], win[1])
        if win[2] is None:
            title = win[0].__class__.__name__
        else:
            title = win[2]
        wxDisplayWindow(f, title)


def wxCreateDelayedGraphWindows():
    "Postponed a wxpython window display."
    for win in GRAPHWINDOWS:
        f = TableGrapher(None, win[0], win[1], win[2], win[3])
        if win[4] is None:
            title = win[0].__class__.__name__
        else:
            title = win[4]
        wxDisplayWindow(f, title)


def wxCreateDelayedDataGraphWindows():
    "Postponed a wxpython window display."
    for win in DATAGRAPHWINDOWS:
        f = DataTableGrapher(None, win[0], win[1])
        win[0]._setGraphFrame(f)
        if win[2] is None:
            title = win[0].__class__.__name__
        else:
            title = win[2]
        wxDisplayWindow(f, title)


def wxCreateDelayedTableWindows():
    "Postponed a wxpython window display."
    for win in TABLEWINDOWS:
        object = win[3]
        f = ViewTable(None, win[0], win[1], object)
        if object is not None:
            object._setViewFrame(f)
        wxDisplayWindow(f, win[2])


def wxCreateDelayedSndTableWindows():
    "Postponed a wxpython window display."
    for win in SNDTABLEWINDOWS:
        f = SndViewTable(None, win[0], win[1], win[3])
        win[0]._setViewFrame(f)
        wxDisplayWindow(f, win[2])


def wxCreateDelayedMatrixWindows():
    "Postponed a wxpython window display."
    for win in MATRIXWINDOWS:
        object = win[3]
        f = ViewMatrix(None, win[0], win[1], object)
        if object is not None:
            object._setViewFrame(f)
        wxDisplayWindow(f, win[2])


def wxCreateDelayedSpectrumWindows():
    "Postponed a wxpython window display."
    for win in SPECTRUMWINDOWS:
        f = SpectrumDisplay(None, win[0])
        if win[1] is None:
            title = win[0].__class__.__name__
        else:
            title = win[1]
        if win[0] is not None:
            win[0]._setViewFrame(f)
        wxDisplayWindow(f, title)


def wxCreateDelayedScopeWindows():
    "Postponed a wxpython window display."
    for win in SCOPEWINDOWS:
        f = ScopeDisplay(None, win[0])
        if win[1] is None:
            title = win[0].__class__.__name__
        else:
            title = win[1]
        if win[0] is not None:
            win[0]._setViewFrame(f)
        wxDisplayWindow(f, title)


def wxCreateDelayedExprEditorWindows():
    "Postponed a wxpython window display."
    for win in EXPREDITORWINDOWS:
        f = ExprEditorFrame(None, win[0])
        if win[1] is None:
            title = win[0].__class__.__name__
        else:
            title = win[1]
        wxDisplayWindow(f, title)


def wxCreateDelayedMMLEditorWindows():
    "Postponed a wxpython window display."
    for win in MMLEDITORWINDOWS:
        f = MMLEditorFrame(None, win[0])
        if win[1] is None:
            title = win[0].__class__.__name__
        else:
            title = win[1]
        wxDisplayWindow(f, title)


def wxCreateDelayedNoteinKeyboardWindows():
    "Postponed a wxpython window display."
    for win in NOTEINKEYBOARDWINDOWS:
        f = NoteinKeyboardFrame(None, win[0])
        if win[1] is None:
            title = win[0].__class__.__name__
        else:
            title = win[1]
        wxDisplayWindow(f, title)


def createCtrlWindow(obj, map_list, title, wxnoserver=False):
    "Creates a controller window (from a .ctrl() method."
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        f = PyoObjectControl(win, obj, map_list)
        win.resizable(True, False)
        if title is None:
            title = obj.__class__.__name__
        win.title(title)
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = PyoObjectControl(None, obj, map_list)
            if title is None:
                title = obj.__class__.__name__
            wxShowWindow(f, title, root)
        else:
            CTRLWINDOWS.append([obj, map_list, title])


def createGraphWindow(obj, mode, xlen, yrange, title, wxnoserver=False):
    "Creates a grapher window (from a .graph() method."
    if not PYO_USE_WX:
        print("WxPython must be installed to use the 'graph()' method.")
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = TableGrapher(None, obj, mode, xlen, yrange)
            if title is None:
                title = obj.__class__.__name__
            wxShowWindow(f, title, root)
        else:
            GRAPHWINDOWS.append([obj, mode, xlen, yrange, title])


def createDataGraphWindow(obj, yrange, title, wxnoserver=False):
    "Creates a data table grapher window (from a .graph() method."
    if not PYO_USE_WX:
        print("WxPython must be installed to use the 'graph()' method.")
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = DataTableGrapher(None, obj, yrange)
            if title is None:
                title = obj.__class__.__name__
            obj._setGraphFrame(f)
            wxShowWindow(f, title, root)
        else:
            DATAGRAPHWINDOWS.append([obj, yrange, title])


def createViewTableWindow(samples, title="Table waveform", wxnoserver=False, tableclass=None, object=None):
    "Creates a table view window (from a .view() method."
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        f = ViewTable(win, samples)
        win.resizable(False, False)
        win.title(title)
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = ViewTable(None, samples, tableclass, object)
            if object is not None:
                object._setViewFrame(f)
            wxShowWindow(f, title, root)
        else:
            TABLEWINDOWS.append([samples, tableclass, title, object])


def createSndViewTableWindow(obj, title="Table waveform", wxnoserver=False, tableclass=None, mouse_callback=None):
    "Creates a snd table view window (from a .view() method."
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        f = ViewTable(win, obj._base_objs[0].getViewTable())
        win.resizable(False, False)
        win.title(title)
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = SndViewTable(None, obj, tableclass, mouse_callback)
            if title is None:
                title = obj.__class__.__name__
            obj._setViewFrame(f)
            wxShowWindow(f, title, root)
        else:
            SNDTABLEWINDOWS.append([obj, tableclass, title, mouse_callback])


def createViewMatrixWindow(samples, size, title="Matrix viewer", wxnoserver=False, object=None):
    "Creates a matrix view window (from a .view() method."
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        f = ViewMatrix(win, samples, size)
        win.resizable(False, False)
        win.title(title)
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = ViewMatrix(None, samples, size, object)
            if object is not None:
                object._setViewFrame(f)
            wxShowWindow(f, title, root)
        else:
            MATRIXWINDOWS.append([samples, size, title, object])


def createSpectrumWindow(object, title, wxnoserver=False):
    "Creates a spectrum display."
    if not PYO_USE_WX:
        print("WxPython must be installed to use the Spectrum display.")
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = SpectrumDisplay(None, object)
            if title is None:
                title = object.__class__.__name__
            if object is not None:
                object._setViewFrame(f)
            wxShowWindow(f, title, root)
        else:
            SPECTRUMWINDOWS.append([object, title])


def createScopeWindow(object, title, wxnoserver=False):
    "Creates a scope display."
    if not PYO_USE_WX:
        print("WxPython must be installed to use the Scope display.")
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = ScopeDisplay(None, object)
            if title is None:
                title = object.__class__.__name__
            if object is not None:
                object._setViewFrame(f)
            wxShowWindow(f, title, root)
        else:
            SCOPEWINDOWS.append([object, title])


def createExprEditorWindow(object, title, wxnoserver=False):
    "Creates an expression editor window."
    if not PYO_USE_WX:
        print("WxPython must be installed to use the Expr editor display.")
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = ExprEditorFrame(None, object)
            if title is None:
                title = object.__class__.__name__
            wxShowWindow(f, title, root)
        else:
            EXPREDITORWINDOWS.append([object, title])


def createMMLEditorWindow(object, title, wxnoserver=False):
    "Creates an MML editor window."
    if not PYO_USE_WX:
        print("WxPython must be installed to use the MML editor display.")
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = MMLEditorFrame(None, object)
            if title is None:
                title = object.__class__.__name__
            wxShowWindow(f, title, root)
        else:
            MMLEDITORWINDOWS.append([object, title])


def createNoteinKeyboardWindow(object, title, wxnoserver=False):
    "Creates avirtual midi keyboard window."
    if not PYO_USE_WX:
        print("WxPython must be installed to use the Notein keyboard display.")
    else:
        if wxnoserver or wx.GetApp() is not None:
            root = createRootWindow()
            f = NoteinKeyboardFrame(None, object)
            if title is None:
                title = object.__class__.__name__
            wxShowWindow(f, title, root)
        else:
            NOTEINKEYBOARDWINDOWS.append([object, title])


def createServerGUI(
    nchnls,
    start,
    stop,
    recstart,
    recstop,
    setAmp,
    started,
    locals,
    shutdown,
    meter,
    timer,
    amp,
    exit,
    title,
    getIsBooted,
    getIsStarted,
):
    "Creates the server's GUI."
    global X, Y, MAX_X, NEXT_Y
    if title is None:
        title = "Pyo Server"
    if not PYO_USE_WX:
        createRootWindow()
        win = tkCreateToplevelWindow()
        f = ServerGUI(
            win,
            nchnls,
            start,
            stop,
            recstart,
            recstop,
            setAmp,
            started,
            locals,
            shutdown,
            meter,
            timer,
            amp,
            getIsBooted,
            getIsStarted,
        )
        f.master.title(title)
        f.focus_set()
    else:
        win = createRootWindow()
        f = ServerGUI(
            None,
            nchnls,
            start,
            stop,
            recstart,
            recstop,
            setAmp,
            started,
            locals,
            shutdown,
            meter,
            timer,
            amp,
            exit,
            getIsBooted,
            getIsStarted,
        )
        f.SetTitle(title)
        f.SetPosition((30, 30))
        f.Show()
        X, Y = (wx.SystemSettings.GetMetric(wx.SYS_SCREEN_X) - 50, wx.SystemSettings.GetMetric(wx.SYS_SCREEN_Y) - 50)
        if sys.platform.startswith("linux"):
            MAX_X, NEXT_Y = f.GetSize()[0] + 30, f.GetSize()[1] + 55
        else:
            MAX_X, NEXT_Y = f.GetSize()[0] + 30, f.GetSize()[1] + 30
        wx.CallAfter(wxCreateDelayedTableWindows)
        wx.CallAfter(wxCreateDelayedGraphWindows)
        wx.CallAfter(wxCreateDelayedDataGraphWindows)
        wx.CallAfter(wxCreateDelayedSndTableWindows)
        wx.CallAfter(wxCreateDelayedMatrixWindows)
        wx.CallAfter(wxCreateDelayedCtrlWindows)
        wx.CallAfter(wxCreateDelayedSpectrumWindows)
        wx.CallAfter(wxCreateDelayedScopeWindows)
        wx.CallAfter(wxCreateDelayedExprEditorWindows)
        wx.CallAfter(wxCreateDelayedMMLEditorWindows)
        wx.CallAfter(wxCreateDelayedNoteinKeyboardWindows)
        wx.CallAfter(f.Raise)
    return f, win, PYO_USE_WX
