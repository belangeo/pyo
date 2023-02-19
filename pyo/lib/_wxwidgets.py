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
import wx, os, sys, math, time, unicodedata
import wx.stc as stc
from ._core import rescale

if "phoenix" in wx.version():
    wx.GraphicsContext_Create = wx.GraphicsContext.Create
    wx.EmptyBitmap = wx.Bitmap
    wx.EmptyImage = wx.Image
    wx.BitmapFromImage = wx.Bitmap
    wx.Image_HSVValue = wx.Image.HSVValue
    wx.Image_HSVtoRGB = wx.Image.HSVtoRGB

# TODO: Remove and fix all apps using it (Cecilia, SoundGrain, etc.)
BACKGROUND_COLOUR = "#EBEBEB"


def interpFloat(t, v1, v2):
    "interpolator for a single value; interprets t in [0-1] between v1 and v2"
    return (v2 - v1) * t + v1


def tFromValue(value, v1, v2):
    "returns a t (in range 0-1) given a value in the range v1 to v2"
    if (v2 - v1) == 0:
        return 1.0
    else:
        return float(value - v1) / (v2 - v1)


def clamp(v, minv, maxv):
    "clamps a value within a range"
    if v < minv:
        v = minv
    if v > maxv:
        v = maxv
    return v


def toLog(t, v1, v2):
    return math.log10(t / v1) / math.log10(v2 / v1)


def toExp(t, v1, v2):
    return math.pow(10, t * (math.log10(v2) - math.log10(v1)) + math.log10(v1))


POWOFTWO = {
    2: 1,
    4: 2,
    8: 3,
    16: 4,
    32: 5,
    64: 6,
    128: 7,
    256: 8,
    512: 9,
    1024: 10,
    2048: 11,
    4096: 12,
    8192: 13,
    16384: 14,
    32768: 15,
    65536: 16,
}


def powOfTwo(x):
    "Return 2 raised to the power of x."
    return 2 ** x


def powOfTwoToInt(x):
    "Return the exponent of 2 correponding to the value x."
    return POWOFTWO[x]


def GetRoundBitmap(w, h, r):
    maskColor = wx.Color(0, 0, 0)
    shownColor = wx.Color(5, 5, 5)
    b = wx.EmptyBitmap(w, h)
    dc = wx.MemoryDC(b)
    dc.SetBrush(wx.Brush(maskColor))
    dc.DrawRectangle(0, 0, w, h)
    dc.SetBrush(wx.Brush(shownColor))
    dc.SetPen(wx.Pen(shownColor))
    dc.DrawRoundedRectangle(0, 0, w, h, r)
    dc.SelectObject(wx.NullBitmap)
    b.SetMaskColour(maskColor)
    return b

class BasePanel(wx.Panel):
    def __init__(self, parent=None, id=wx.ID_ANY, pos=wx.DefaultPosition, size=wx.DefaultSize, style=wx.TAB_TRAVERSAL):
        wx.Panel.__init__(self, parent, id, pos, size, style)
        self.backgroundColour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.SetBackgroundColour(self.backgroundColour)
        self.SetForegroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_MENUTEXT))

class ControlSlider(BasePanel):
    def __init__(
        self,
        parent,
        minvalue,
        maxvalue,
        init=None,
        pos=(0, 0),
        size=(200, 16),
        log=False,
        outFunction=None,
        integer=False,
        powoftwo=False,
        backColour=None,
        orient=wx.HORIZONTAL,
        ctrllabel="",
    ):
        if size == (200, 16) and orient == wx.VERTICAL:
            size = (40, 200)
        BasePanel.__init__(
            self, parent=parent, id=wx.ID_ANY, pos=pos, size=size, style=wx.NO_BORDER | wx.WANTS_CHARS | wx.EXPAND
        )
        self.parent = parent
        self.backgroundColourOverride = False
        if backColour:
            self.backgroundColour = backColour
            self.backgroundColourOverride = True
        self.SetBackgroundColour(self.backgroundColour)
        self.orient = orient
        # self.SetMinSize(self.GetSize())
        if self.orient == wx.VERTICAL:
            self.knobSize = 17
            self.knobHalfSize = 8
            self.sliderWidth = size[0] - 29
        else:
            self.knobSize = 40
            self.knobHalfSize = 20
            self.sliderHeight = size[1] - 5
        self.outFunction = outFunction
        self.integer = integer
        self.log = log
        self.powoftwo = powoftwo
        if self.powoftwo:
            self.integer = True
            self.log = False
        self.ctrllabel = ctrllabel
        self.SetRange(minvalue, maxvalue)
        self.borderWidth = 1
        self.selected = False
        self._enable = True
        self.propagate = True
        self.midictl = None
        self.new = ""
        if init is not None:
            self.SetValue(init)
            self.init = init
        else:
            self.SetValue(minvalue)
            self.init = minvalue
        self.clampPos()
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_LEFT_DCLICK, self.DoubleClick)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnResize)
        self.Bind(wx.EVT_CHAR, self.onChar)
        self.Bind(wx.EVT_KILL_FOCUS, self.LooseFocus)

        if sys.platform == "win32" or sys.platform.startswith("linux"):
            self.dcref = wx.BufferedPaintDC
            self.font = wx.Font(7, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL)
        else:
            self.dcref = wx.PaintDC
            self.font = wx.Font(10, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL)

    def getCtrlLabel(self):
        return self.ctrllabel

    def setMidiCtl(self, x, propagate=True):
        self.propagate = propagate
        self.midictl = x
        self.Refresh()

    def getMidiCtl(self):
        return self.midictl

    def getMinValue(self):
        return self.minvalue

    def getMaxValue(self):
        return self.maxvalue

    def Enable(self):
        self._enable = True
        wx.CallAfter(self.Refresh)

    def Disable(self):
        self._enable = False
        wx.CallAfter(self.Refresh)

    def setSliderHeight(self, height):
        self.sliderHeight = height
        self.Refresh()

    def setSliderWidth(self, width):
        self.sliderWidth = width

    def getInit(self):
        return self.init

    def SetRange(self, minvalue, maxvalue):
        self.minvalue = minvalue
        self.maxvalue = maxvalue

    def getRange(self):
        return [self.minvalue, self.maxvalue]

    def scale(self):
        if self.orient == wx.VERTICAL:
            h = self.GetSize()[1]
            inter = tFromValue(h - self.pos, self.knobHalfSize, self.GetSize()[1] - self.knobHalfSize)
        else:
            inter = tFromValue(self.pos, self.knobHalfSize, self.GetSize()[0] - self.knobHalfSize)
        if not self.integer:
            return interpFloat(inter, self.minvalue, self.maxvalue)
        elif self.powoftwo:
            return powOfTwo(int(interpFloat(inter, self.minvalue, self.maxvalue)))
        else:
            return int(interpFloat(inter, self.minvalue, self.maxvalue))

    def SetValue(self, value, propagate=True):
        self.propagate = propagate
        if self.HasCapture():
            self.ReleaseMouse()
        if self.powoftwo:
            value = powOfTwoToInt(value)
        value = clamp(value, self.minvalue, self.maxvalue)
        if self.log:
            t = toLog(value, self.minvalue, self.maxvalue)
            self.value = interpFloat(t, self.minvalue, self.maxvalue)
        else:
            t = tFromValue(value, self.minvalue, self.maxvalue)
            self.value = interpFloat(t, self.minvalue, self.maxvalue)
        if self.integer:
            self.value = int(self.value)
        if self.powoftwo:
            self.value = powOfTwo(self.value)
        self.clampPos()
        self.selected = False
        wx.CallAfter(self.Refresh)

    def GetValue(self):
        if self.log:
            t = tFromValue(self.value, self.minvalue, self.maxvalue)
            val = toExp(t, self.minvalue, self.maxvalue)
        else:
            val = self.value
        if self.integer:
            val = int(val)
        return val

    def LooseFocus(self, event):
        self.selected = False
        self.Refresh()

    def onChar(self, event):
        if self.selected:
            char = ""
            if event.GetKeyCode() in range(wx.WXK_NUMPAD0, wx.WXK_NUMPAD9 + 1):
                char = str(event.GetKeyCode() - wx.WXK_NUMPAD0)
            elif event.GetKeyCode() in [wx.WXK_SUBTRACT, wx.WXK_NUMPAD_SUBTRACT]:
                char = "-"
            elif event.GetKeyCode() in [wx.WXK_DECIMAL, wx.WXK_NUMPAD_DECIMAL]:
                char = "."
            elif event.GetKeyCode() == wx.WXK_BACK:
                if self.new != "":
                    self.new = self.new[0:-1]
            elif event.GetKeyCode() < 256:
                char = chr(event.GetKeyCode())

            if char in ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "-"]:
                self.new += char
            elif event.GetKeyCode() in [wx.WXK_RETURN, wx.WXK_NUMPAD_ENTER]:
                self.SetValue(eval(self.new))
                self.new = ""
                self.selected = False
            self.Refresh()
        event.Skip()

    def MouseDown(self, evt):
        if evt.ShiftDown():
            self.DoubleClick(evt)
            return
        if self._enable:
            size = self.GetSize()
            if self.orient == wx.VERTICAL:
                self.pos = clamp(evt.GetPosition()[1], self.knobHalfSize, size[1] - self.knobHalfSize)
            else:
                self.pos = clamp(evt.GetPosition()[0], self.knobHalfSize, size[0] - self.knobHalfSize)
            self.value = self.scale()
            self.CaptureMouse()
            self.selected = False
            self.Refresh()
        evt.Skip()

    def MouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()

    def DoubleClick(self, event):
        if self._enable:
            w, h = self.GetSize()
            pos = event.GetPosition()
            if self.orient == wx.VERTICAL:
                if wx.Rect(0, int(self.pos) - self.knobHalfSize, w, self.knobSize).Contains(pos):
                    self.selected = True
            else:
                if wx.Rect(int(self.pos) - self.knobHalfSize, 0, self.knobSize, h).Contains(pos):
                    self.selected = True
            self.Refresh()
        event.Skip()

    def MouseMotion(self, evt):
        if self._enable:
            size = self.GetSize()
            if self.HasCapture():
                if self.orient == wx.VERTICAL:
                    self.pos = clamp(evt.GetPosition()[1], self.knobHalfSize, size[1] - self.knobHalfSize)
                else:
                    self.pos = clamp(evt.GetPosition()[0], self.knobHalfSize, size[0] - self.knobHalfSize)
                self.value = self.scale()
                self.selected = False
                self.Refresh()

    def OnResize(self, evt):
        self.clampPos()
        self.Refresh()

    def clampPos(self):
        size = self.GetSize()
        if self.powoftwo:
            val = powOfTwoToInt(self.value)
        else:
            val = self.value
        if self.orient == wx.VERTICAL:
            self.pos = tFromValue(val, self.minvalue, self.maxvalue) * (size[1] - self.knobSize) + self.knobHalfSize
            self.pos = clamp(size[1] - self.pos, self.knobHalfSize, size[1] - self.knobHalfSize)
        else:
            self.pos = tFromValue(val, self.minvalue, self.maxvalue) * (size[0] - self.knobSize) + self.knobHalfSize
            self.pos = clamp(self.pos, self.knobHalfSize, size[0] - self.knobHalfSize)

    def setBackgroundColour(self, colour):
        self.backgroundColour = colour
        self.backgroundColourOverride = True
        self.SetBackgroundColour(self.backgroundColour)
        self.Refresh()

    def OnPaint(self, evt):
        w, h = self.GetSize()

        if w <= 0 or h <= 0:
            evt.Skip()
            return

        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)

        if not self.backgroundColourOverride:
            self.backgroundColour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)
        dc.SetBrush(wx.Brush(self.backgroundColour, wx.SOLID))
        dc.Clear()

        # Draw background
        dc.SetPen(wx.Pen(self.backgroundColour, width=self.borderWidth, style=wx.SOLID))
        dc.DrawRectangle(0, 0, w, h)

        # Draw inner part
        if self._enable:
            sliderColour = "#99A7CC"
        else:
            sliderColour = "#BBBBBB"
        if self.orient == wx.VERTICAL:
            w2 = (w - self.sliderWidth) // 2
            rec = wx.Rect(w2, 0, self.sliderWidth, h)
            brush = gc.CreateLinearGradientBrush(w2, 0, w2 + self.sliderWidth, 0, "#646986", sliderColour)
        else:
            h2 = self.sliderHeight // 4
            rec = wx.Rect(0, h2, w, self.sliderHeight)
            brush = gc.CreateLinearGradientBrush(0, h2, 0, h2 + self.sliderHeight, "#646986", sliderColour)
        gc.SetBrush(brush)
        gc.DrawRoundedRectangle(rec[0], rec[1], rec[2], rec[3], 2)

        if self.midictl is not None:
            if sys.platform == "win32" or sys.platform.startswith("linux"):
                dc.SetFont(wx.Font(6, wx.FONTFAMILY_ROMAN, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL))
            else:
                dc.SetFont(wx.Font(9, wx.FONTFAMILY_ROMAN, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL))
            dc.SetTextForeground("#FFFFFF")
            if self.orient == wx.VERTICAL:
                dc.DrawLabel(str(self.midictl), wx.Rect(w2, 2, self.sliderWidth, 12), wx.ALIGN_CENTER)
                dc.DrawLabel(str(self.midictl), wx.Rect(w2, h - 12, self.sliderWidth, 12), wx.ALIGN_CENTER)
            else:
                dc.DrawLabel(str(self.midictl), wx.Rect(2, 0, h, h), wx.ALIGN_CENTER)
                dc.DrawLabel(str(self.midictl), wx.Rect(w - h, 0, h, h), wx.ALIGN_CENTER)

        # Draw knob
        if self._enable:
            knobColour = "#888888"
        else:
            knobColour = "#DDDDDD"
        if self.orient == wx.VERTICAL:
            rec = wx.Rect(0, int(self.pos) - self.knobHalfSize, w, self.knobSize - 1)
            if self.selected:
                brush = wx.Brush("#333333", wx.SOLID)
            else:
                brush = gc.CreateLinearGradientBrush(0, 0, w, 0, "#323854", knobColour)
            gc.SetBrush(brush)
            gc.DrawRoundedRectangle(rec[0], rec[1], rec[2], rec[3], 3)
        else:
            rec = wx.Rect(int(self.pos) - self.knobHalfSize, 0, self.knobSize - 1, h)
            if self.selected:
                brush = wx.Brush("#333333", wx.SOLID)
            else:
                brush = gc.CreateLinearGradientBrush(
                    self.pos - self.knobHalfSize, 0, self.pos + self.knobHalfSize, 0, "#323854", knobColour
                )
            gc.SetBrush(brush)
            gc.DrawRoundedRectangle(rec[0], rec[1], rec[2], rec[3], 3)

        dc.SetFont(self.font)

        # Draw text
        if self.selected and self.new:
            val = self.new
        else:
            if self.integer:
                val = "%d" % self.GetValue()
            elif abs(self.GetValue()) >= 1000:
                val = "%.0f" % self.GetValue()
            elif abs(self.GetValue()) >= 100:
                val = "%.1f" % self.GetValue()
            elif abs(self.GetValue()) >= 10:
                val = "%.2f" % self.GetValue()
            elif abs(self.GetValue()) < 10:
                val = "%.3f" % self.GetValue()
        if sys.platform.startswith("linux"):
            width = len(val) * (dc.GetCharWidth() - 3)
        else:
            width = len(val) * dc.GetCharWidth()
        dc.SetTextForeground("#FFFFFF")
        dc.DrawLabel(val, rec, wx.ALIGN_CENTER)

        # Send value
        if self.outFunction and self.propagate:
            self.outFunction(self.GetValue())
        self.propagate = True

        evt.Skip()


# TODO: key, command and slmap should be removed from the multislider widget.
# It should work in the same way as the ControlSlider widget.
class MultiSlider(BasePanel):
    def __init__(self, parent, init, key, command, slmap, ctrllabel=""):
        BasePanel.__init__(self, parent, size=(250, 250))
        self.Bind(wx.EVT_SIZE, self.OnResize)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self._slmap = slmap
        self.ctrllabel = ctrllabel
        self._values = [slmap.set(x) for x in init]
        self._nchnls = len(init)
        self._labels = init
        self._key = key
        self._command = command
        self._height = 16
        if sys.platform == "win32" or sys.platform.startswith("linux"):
            self._font = wx.Font(7, wx.FONTFAMILY_ROMAN, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL)
        else:
            self._font = wx.Font(10, wx.FONTFAMILY_ROMAN, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL)

        self.SetSize((250, self._nchnls * 16))
        self.SetMinSize((250, self._nchnls * 16))

    def getCtrlLabel(self):
        return self.ctrllabel

    def OnResize(self, event):
        self.Layout()
        self.Refresh()

    def OnPaint(self, event):
        w, h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)
        self.backgroundColour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)
        dc.SetBrush(wx.Brush(self.backgroundColour))
        dc.Clear()
        dc.DrawRectangle(0, 0, w, h)
        dc.SetBrush(wx.Brush("#000000"))
        dc.SetFont(self._font)
        dc.SetTextForeground("#999999")
        for i in range(self._nchnls):
            x = int(self._values[i] * w)
            y = self._height * i
            dc.DrawRectangle(0, y + 1, x, self._height - 2)
            rec = wx.Rect(w // 2 - 15, y, 30, self._height)
            dc.DrawLabel("%s" % self._labels[i], rec, wx.ALIGN_CENTER)

    def MouseDown(self, evt):
        w, h = self.GetSize()
        pos = evt.GetPosition()
        slide = pos[1] // self._height
        if slide >= 0 and slide < self._nchnls:
            self._values[slide] = pos[0] / float(w)
            if self._slmap._res == "int":
                self._labels = [int(self._slmap.get(x)) for x in self._values]
            else:
                self._labels = [self._slmap.get(x) for x in self._values]
            self._command(self._key, self._labels)
            self.CaptureMouse()
        self.Refresh()
        evt.Skip()

    def MouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()

    def MouseMotion(self, evt):
        w, h = self.GetSize()
        pos = evt.GetPosition()
        if evt.Dragging() and evt.LeftIsDown():
            slide = pos[1] // self._height
            if slide >= 0 and slide < self._nchnls:
                self._values[slide] = pos[0] / float(w)
                if self._slmap._res == "int":
                    self._labels = [int(self._slmap.get(x)) for x in self._values]
                else:
                    self._labels = [self._slmap.get(x) for x in self._values]
                self._command(self._key, self._labels)
            self.Refresh()

    def GetValue(self):
        return self._labels


class VuMeter(BasePanel):
    def __init__(self, parent, size=(200, 11), numSliders=2, orient=wx.HORIZONTAL, pos=wx.DefaultPosition, style=0):
        if orient == wx.HORIZONTAL:
            size = (size[0], numSliders * 5 + 1)
        else:
            size = (numSliders * 5 + 1, size[1])
        BasePanel.__init__(self, parent, -1, pos=pos, size=size, style=style)
        self.parent = parent
        self.orient = orient
        self.old_nchnls = numSliders
        self.numSliders = numSliders
        self.amplitude = [0] * self.numSliders
        self.createBitmaps()

        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_CLOSE, self.OnClose)

    def OnSize(self, evt):
        self.createBitmaps()
        wx.CallAfter(self.Refresh)

    def createBitmaps(self):
        w, h = self.GetSize()
        if w == 0 or h == 0:
            return

        b = wx.EmptyBitmap(w, h)
        f = wx.EmptyBitmap(w, h)
        dcb = wx.MemoryDC(b)
        dcf = wx.MemoryDC(f)
        dcb.SetPen(wx.Pen("#000000", width=1))
        dcf.SetPen(wx.Pen("#000000", width=1))
        if self.orient == wx.HORIZONTAL:
            height = 6
            steps = int(w / 10.0 + 0.5)
        else:
            width = 6
            steps = int(h / 10.0 + 0.5)
        bounds = int(steps / 6.0)
        for i in range(steps):
            if i == (steps - 1):
                dcb.SetBrush(wx.Brush("#770000"))
                dcf.SetBrush(wx.Brush("#FF0000"))
            elif i >= (steps - bounds):
                dcb.SetBrush(wx.Brush("#440000"))
                dcf.SetBrush(wx.Brush("#CC0000"))
            elif i >= (steps - (bounds * 2)):
                dcb.SetBrush(wx.Brush("#444400"))
                dcf.SetBrush(wx.Brush("#CCCC00"))
            else:
                dcb.SetBrush(wx.Brush("#004400"))
                dcf.SetBrush(wx.Brush("#00CC00"))
            if self.orient == wx.HORIZONTAL:
                dcb.DrawRectangle(i * 10, 0, 11, height)
                dcf.DrawRectangle(i * 10, 0, 11, height)
            else:
                ii = steps - 1 - i
                dcb.DrawRectangle(0, ii * 10, width, 11)
                dcf.DrawRectangle(0, ii * 10, width, 11)
        if self.orient == wx.HORIZONTAL:
            dcb.DrawLine(w - 1, 0, w - 1, height)
            dcf.DrawLine(w - 1, 0, w - 1, height)
        else:
            dcb.DrawLine(0, 0, width, 0)
            dcf.DrawLine(0, 0, width, 0)
        dcb.SelectObject(wx.NullBitmap)
        dcf.SelectObject(wx.NullBitmap)
        self.backBitmap = b
        self.bitmap = f

    def setNumSliders(self, numSliders):
        w, h = self.GetSize()
        oldChnls = self.old_nchnls
        self.numSliders = numSliders
        self.amplitude = [0] * self.numSliders
        gap = (self.numSliders - oldChnls) * 5
        parentSize = self.parent.GetSize()
        if self.orient == wx.HORIZONTAL:
            self.SetSize((w, self.numSliders * 5 + 1))
            self.SetMinSize((w, 5 * self.numSliders + 1))
            self.parent.SetSize((parentSize[0], parentSize[1] + gap))
            self.parent.SetMinSize((parentSize[0], parentSize[1] + gap))
        else:
            self.SetSize((self.numSliders * 5 + 1, h))
            self.SetMinSize((5 * self.numSliders + 1, h))
            self.parent.SetSize((parentSize[0] + gap, parentSize[1]))
            self.parent.SetMinSize((parentSize[0] + gap, parentSize[1]))
        wx.CallAfter(self.Refresh)
        wx.CallAfter(self.parent.Layout)
        wx.CallAfter(self.parent.Refresh)

    def setRms(self, *args):
        if args[0] < 0:
            return
        if not args:
            self.amplitude = [0 for i in range(self.numSliders)]
        else:
            self.amplitude = args
        wx.CallAfter(self.Refresh)

    def OnPaint(self, event):
        w, h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)
        dc.SetBrush(wx.Brush("#000000"))
        dc.Clear()
        dc.DrawRectangle(0, 0, w, h)
        if self.orient == wx.HORIZONTAL:
            height = 6
            for i in range(self.numSliders):
                y = i * (height - 1)
                if i < len(self.amplitude):
                    db = math.log10(self.amplitude[i] + 0.00001) * 0.2 + 1.0
                    width = int(db * w)
                else:
                    width = 0
                dc.DrawBitmap(self.backBitmap, 0, y)
                if width > 0:
                    dc.SetClippingRegion(0, y, width, height)
                    dc.DrawBitmap(self.bitmap, 0, y)
                    dc.DestroyClippingRegion()
        else:
            width = 6
            for i in range(self.numSliders):
                y = i * (width - 1)
                if i < len(self.amplitude):
                    db = math.log10(self.amplitude[i] + 0.00001) * 0.2 + 1.0
                    height = int(db * h)
                else:
                    height = 0
                dc.DrawBitmap(self.backBitmap, y, 0)
                if height > 0:
                    dc.SetClippingRegion(y, h - height, width, height)
                    dc.DrawBitmap(self.bitmap, y, 0)
                    dc.DestroyClippingRegion()
        event.Skip()

    def OnClose(self, evt):
        self.Destroy()


# TODO: BACKGROUND_COLOUR hard-coded all over the place in this class.
class RangeSlider(BasePanel):
    def __init__(
        self,
        parent,
        minvalue,
        maxvalue,
        init=None,
        pos=(0, 0),
        size=(200, 15),
        valtype="int",
        log=False,
        function=None,
        backColour=None,
    ):
        BasePanel.__init__(self, parent=parent, id=wx.ID_ANY, pos=pos, size=size, style=wx.NO_BORDER)
        if backColour:
            self.backgroundColour = backColour
        self.SetBackgroundColour(self.backgroundColour)
        self.SetMinSize(self.GetSize())
        self.sliderHeight = 15
        self.borderWidth = 1
        self.action = None
        self.fillcolor = "#AAAAAA"  # SLIDER_BACK_COLOUR
        self.knobcolor = "#333333"  # SLIDER_KNOB_COLOUR
        self.handlecolor = wx.Colour(
            int(self.knobcolor[1:3]) - 10, int(self.knobcolor[3:5]) - 10, int(self.knobcolor[5:7]) - 10
        )
        self.outFunction = function
        if valtype.startswith("i"):
            self.myType = int
        else:
            self.myType = float
        self.log = log
        self.SetRange(minvalue, maxvalue)
        self.handles = [minvalue, maxvalue]
        if init is not None:
            if type(init) in [list, tuple]:
                if len(init) == 1:
                    self.SetValue([init[0], init[0]])
                else:
                    self.SetValue([init[0], init[1]])
            else:
                self.SetValue([minvalue, maxvalue])
        else:
            self.SetValue([minvalue, maxvalue])
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_RIGHT_DOWN, self.MouseRightDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_RIGHT_UP, self.MouseUp)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnResize)

    def createSliderBitmap(self):
        w, h = self.GetSize()
        b = wx.EmptyBitmap(w, h)
        dc = wx.MemoryDC(b)
        dc.SetPen(wx.Pen(self.backgroundColour, width=1))
        dc.SetBrush(wx.Brush(self.backgroundColour))
        dc.DrawRectangle(0, 0, w, h)
        dc.SetBrush(wx.Brush("#777777"))
        dc.SetPen(wx.Pen("#FFFFFF", width=1))
        h2 = self.sliderHeight // 4
        dc.DrawRoundedRectangle(0, h2, w, self.sliderHeight, 4)
        dc.SelectObject(wx.NullBitmap)
        b.SetMaskColour("#777777")
        self.sliderMask = b

    def setFillColour(self, col1, col2):
        self.fillcolor = col1
        self.knobcolor = col2
        self.handlecolor = wx.Colour(self.knobcolor[0] * 0.35, self.knobcolor[1] * 0.35, self.knobcolor[2] * 0.35)
        self.createSliderBitmap()

    def SetRange(self, minvalue, maxvalue):
        self.minvalue = minvalue
        self.maxvalue = maxvalue

    def scale(self, pos):
        tmp = []
        for p in pos:
            inter = tFromValue(p, 1, self.GetSize()[0] - 1)
            inter2 = interpFloat(inter, self.minvalue, self.maxvalue)
            tmp.append(inter2)
        return tmp

    def MouseRightDown(self, evt):
        size = self.GetSize()
        xpos = evt.GetPosition()[0]
        if xpos > (self.handlePos[0] - 5) and xpos < (self.handlePos[1] + 5):
            self.lastpos = xpos
            self.length = self.handlePos[1] - self.handlePos[0]
            self.action = "drag"
            self.handles = self.scale(self.handlePos)
            self.CaptureMouse()
            self.Refresh()

    def MouseDown(self, evt):
        size = self.GetSize()
        xpos = evt.GetPosition()[0]
        self.middle = (self.handlePos[1] - self.handlePos[0]) // 2 + self.handlePos[0]
        midrec = wx.Rect(int(self.middle) - 7, 4, 15, size[1] - 9)
        if midrec.Contains(evt.GetPosition()):
            self.lastpos = xpos
            self.length = self.handlePos[1] - self.handlePos[0]
            self.action = "drag"
        elif xpos < self.middle:
            self.handlePos[0] = clamp(xpos, 1, self.handlePos[1])
            self.action = "left"
        elif xpos > self.middle:
            self.handlePos[1] = clamp(xpos, self.handlePos[0], size[0] - 1)
            self.action = "right"
        self.handles = self.scale(self.handlePos)
        self.CaptureMouse()
        self.Refresh()

    def MouseMotion(self, evt):
        size = self.GetSize()
        if evt.Dragging() and self.HasCapture() and evt.LeftIsDown() or evt.RightIsDown():
            xpos = evt.GetPosition()[0]
            if self.action == "drag":
                off = xpos - self.lastpos
                self.lastpos = xpos
                self.handlePos[0] = clamp(self.handlePos[0] + off, 1, size[0] - self.length)
                self.handlePos[1] = clamp(self.handlePos[1] + off, self.length, size[0] - 1)
            if self.action == "left":
                self.handlePos[0] = clamp(xpos, 1, self.handlePos[1] - 20)
            elif self.action == "right":
                self.handlePos[1] = clamp(xpos, self.handlePos[0] + 20, size[0] - 1)
            self.handles = self.scale(self.handlePos)
            self.Refresh()

    def MouseUp(self, evt):
        while self.HasCapture():
            self.ReleaseMouse()

    def OnResize(self, evt):
        self.createSliderBitmap()
        self.clampHandlePos()
        self.Refresh()

    def clampHandlePos(self):
        size = self.GetSize()
        tmp = []
        for handle in [min(self.handles), max(self.handles)]:
            pos = tFromValue(handle, self.minvalue, self.maxvalue) * size[0]
            pos = clamp(pos, 1, size[0] - 1)
            tmp.append(pos)
        self.handlePos = tmp


class HRangeSlider(RangeSlider):
    def __init__(
        self,
        parent,
        minvalue,
        maxvalue,
        init=None,
        pos=(0, 0),
        size=(200, 15),
        valtype="int",
        log=False,
        function=None,
        backColour=None,
    ):
        RangeSlider.__init__(self, parent, minvalue, maxvalue, init, pos, size, valtype, log, function, backColour)
        self.SetMinSize((50, 15))

        self.createSliderBitmap()
        self.clampHandlePos()

    def setSliderHeight(self, height):
        self.sliderHeight = height
        self.createSliderBitmap()
        self.Refresh()

    def SetOneValue(self, value, which):
        self.lasthandles = self.handles
        value = clamp(value, self.minvalue, self.maxvalue)
        if self.log:
            t = toLog(value, self.minvalue, self.maxvalue)
            value = interpFloat(t, self.minvalue, self.maxvalue)
        else:
            t = tFromValue(value, self.minvalue, self.maxvalue)
            value = interpFloat(t, self.minvalue, self.maxvalue)
        if self.myType == int:
            value = int(value)
        self.handles[which] = value
        self.OnResize(None)

    def SetValue(self, values):
        self.lasthandles = self.handles
        tmp = []
        for val in values:
            value = clamp(val, self.minvalue, self.maxvalue)
            if self.log:
                t = toLog(value, self.minvalue, self.maxvalue)
                value = interpFloat(t, self.minvalue, self.maxvalue)
            else:
                t = tFromValue(value, self.minvalue, self.maxvalue)
                value = interpFloat(t, self.minvalue, self.maxvalue)
            if self.myType == int:
                value = int(value)
            tmp.append(value)
        self.handles = tmp
        self.OnResize(None)

    def GetValue(self):
        tmp = []
        for value in self.handles:
            if self.log:
                t = tFromValue(value, self.minvalue, self.maxvalue)
                val = toExp(t, self.minvalue, self.maxvalue)
            else:
                val = value
            if self.myType == int:
                val = int(val)
            tmp.append(val)
        tmp = [min(tmp), max(tmp)]
        return tmp

    def OnPaint(self, evt):
        w, h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)

        # Draw background
        self.backgroundColour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)
        foregroundColour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_MENUTEXT)

        dc.SetBrush(wx.Brush(self.backgroundColour))
        dc.Clear()
        dc.SetPen(wx.Pen(self.backgroundColour))
        dc.DrawRectangle(0, 0, w, h)

        # Draw handles
        dc.SetPen(wx.Pen(foregroundColour, width=1, style=wx.SOLID))
        dc.SetBrush(wx.Brush(foregroundColour))

        rec = wx.Rect(int(self.handlePos[0]), 3, int(self.handlePos[1] - self.handlePos[0]), h - 7)
        dc.DrawRoundedRectangle(rec[0], rec[1], rec[2], rec[3], 4)

        dc.SetPen(wx.Pen(self.backgroundColour, width=1, style=wx.SOLID))
        dc.SetBrush(wx.Brush(self.backgroundColour))
        mid = (self.handlePos[1] - self.handlePos[0]) // 2 + self.handlePos[0]
        rec = wx.Rect(int(mid) - 4, 4, 8, h - 9)
        dc.DrawRoundedRectangle(rec[0], rec[1], rec[2], rec[3], 3)

        # Send value
        if self.outFunction:
            self.outFunction(self.GetValue())


######################################################################
### Control window for PyoObject
######################################################################
class Command:
    def __init__(self, func, key):
        self.func = func
        self.key = key

    def __call__(self, value):
        self.func(self.key, value)


class PyoObjectControl(wx.Frame):
    def __init__(self, parent=None, obj=None, map_list=None):
        wx.Frame.__init__(self, parent)
        from .controls import SigTo

        self.Bind(wx.EVT_SYS_COLOUR_CHANGED, self.OnColourChanged)

        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(9999, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.fileMenu.Bind(wx.EVT_MENU, self._destroy, id=9999)
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(
            10000, "Copy all parameters to the clipboard (4 digits of precision)\tCtrl+C", kind=wx.ITEM_NORMAL
        )
        self.Bind(wx.EVT_MENU, self.copy, id=10000)
        self.fileMenu.Append(
            10001, "Copy all parameters to the clipboard (full precision)\tShift+Ctrl+C", kind=wx.ITEM_NORMAL
        )
        self.Bind(wx.EVT_MENU, self.copy, id=10001)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self._obj = obj
        self._map_list = map_list
        self._labels = []
        self._sliders = []
        self._excluded = []
        self._values = {}
        self._displays = {}
        self._maps = {}
        self._sigs = {}

        if sys.platform == "darwin":
            panel = wx.Panel(self, style=wx.TAB_TRAVERSAL)
            panel.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
        else:
            panel = BasePanel(self)
        mainBox = wx.BoxSizer(wx.VERTICAL)
        self.box = wx.FlexGridSizer(len(self._map_list), 2, 5, 5)

        for i, m in enumerate(self._map_list):
            key, init, mini, maxi, scl, res, dataOnly = m.name, m.init, m.min, m.max, m.scale, m.res, m.dataOnly
            # filters PyoObjects
            if type(init) not in [list, float, int]:
                self._excluded.append(key)
            else:
                self._maps[key] = m
                # label (param name)
                if dataOnly:
                    label = wx.StaticText(panel, -1, key + " *")
                else:
                    label = wx.StaticText(panel, -1, key)
                self._labels.append(label)
                # create and pack slider
                if type(init) != list:
                    if scl == "log":
                        scl = True
                    else:
                        scl = False
                    if res == "int":
                        res = True
                    else:
                        res = False
                    self._sliders.append(
                        ControlSlider(
                            panel,
                            mini,
                            maxi,
                            init,
                            log=scl,
                            size=(300, 16),
                            outFunction=Command(self.setval, key),
                            integer=res,
                            ctrllabel=key,
                        )
                    )
                    self.box.AddMany([(label, 0, wx.LEFT, 5), (self._sliders[-1], 1, wx.EXPAND | wx.LEFT, 5)])
                else:
                    self._sliders.append(MultiSlider(panel, init, key, self.setval, m, ctrllabel=key))
                    self.box.AddMany([(label, 0, wx.LEFT, 5), (self._sliders[-1], 1, wx.EXPAND | wx.LEFT, 5)])
                # set obj attribute to PyoObject SigTo
                if not dataOnly:
                    self._values[key] = init
                    self._sigs[key] = SigTo(init, 0.025, init)
                    refStream = self._obj.getBaseObjects()[0]._getStream()
                    server = self._obj.getBaseObjects()[0].getServer()
                    for k in range(len(self._sigs[key].getBaseObjects())):
                        curStream = self._sigs[key].getBaseObjects()[k]._getStream()
                        server.changeStreamPosition(refStream, curStream)
                    setattr(self._obj, key, self._sigs[key])
        self.box.AddGrowableCol(1, 1)
        mainBox.Add(self.box, 1, wx.EXPAND | wx.TOP | wx.BOTTOM | wx.RIGHT, 10)

        panel.SetSizerAndFit(mainBox)
        self.SetClientSize(panel.GetSize())
        self.SetMinSize(self.GetSize())
        self.SetMaxSize((-1, self.GetSize()[1]))

    def OnColourChanged(self, evt):
        colour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_MENUTEXT)
        for label in self._labels:
            label.SetForegroundColour(colour)

    def _destroy(self, event):
        for m in self._map_list:
            key = m.name
            if key not in self._excluded and key in self._values:
                setattr(self._obj, key, self._values[key])
                del self._sigs[key]
        self.Destroy()

    def setval(self, key, x):
        if key in self._values:
            self._values[key] = x
            setattr(self._sigs[key], "value", x)
        else:
            setattr(self._obj, key, x)

    def copy(self, evt):
        labels = [slider.getCtrlLabel() for slider in self._sliders]
        values = [slider.GetValue() for slider in self._sliders]
        if evt.GetId() == 10000:
            pstr = ""
            for i in range(len(labels)):
                pstr += "%s=" % labels[i]
                if type(values[i]) == list:
                    pstr += "["
                    pstr += ", ".join(["%.4f" % val for val in values[i]])
                    pstr += "]"
                else:
                    pstr += "%.4f" % values[i]
                if i < (len(labels) - 1):
                    pstr += ", "
        else:
            pstr = ""
            for i in range(len(labels)):
                pstr += "%s=" % labels[i]
                if type(values[i]) == list:
                    pstr += "["
                    pstr += ", ".join([str(val) for val in values[i]])
                    pstr += "]"
                else:
                    pstr += str(values[i])
                if i < (len(labels) - 1):
                    pstr += ", "
        data = wx.TextDataObject(pstr)
        if wx.TheClipboard.Open():
            wx.TheClipboard.Clear()
            wx.TheClipboard.SetData(data)
            wx.TheClipboard.Close()


######################################################################
### View window for PyoTableObject
######################################################################
class ViewTable(wx.Frame):
    def __init__(self, parent, samples=None, tableclass=None, object=None):
        wx.Frame.__init__(self, parent, size=(500, 200))
        self.SetMinSize((300, 150))
        menubar = wx.MenuBar()
        fileMenu = wx.Menu()
        closeItem = fileMenu.Append(-1, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self._destroy, closeItem)
        menubar.Append(fileMenu, "&File")
        self.SetMenuBar(menubar)
        self.tableclass = tableclass
        self.object = object
        self.Bind(wx.EVT_CLOSE, self._destroy)
        if sys.platform == "darwin":
            self.panel = wx.Panel(self, style=wx.TAB_TRAVERSAL)
            self.panel.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
        else:
            self.panel = BasePanel(self)
        self.box = wx.BoxSizer(wx.VERTICAL)
        self.wavePanel = ViewTablePanel(self.panel, object)
        self.box.Add(self.wavePanel, 1, wx.EXPAND | wx.ALL, 5)
        self.panel.SetSizerAndFit(self.box)
        self.update(samples)

    def update(self, samples):
        self.wavePanel.draw(samples)

    def _destroy(self, evt):
        self.object._setViewFrame(None)
        self.Destroy()


class ViewTablePanel(wx.Panel):
    def __init__(self, parent, obj):
        wx.Panel.__init__(self, parent)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.obj = obj
        self.samples = []
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        if sys.platform == "win32" or sys.platform.startswith("linux"):
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC

    def draw(self, samples):
        self.samples = samples
        wx.CallAfter(self.Refresh)

    def OnPaint(self, evt):
        w, h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        dc.SetBrush(wx.Brush("#FFFFFF"))
        dc.SetPen(wx.Pen("#BBBBBB", width=1, style=wx.SOLID))
        dc.Clear()
        dc.DrawRectangle(0, 0, w, h)
        gc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
        gc.SetBrush(wx.Brush("#FFFFFF"))
        if len(self.samples) > 1:
            gc.DrawLines(self.samples)
        dc.DrawLine(0, h // 2 + 1, w, h // 2 + 1)

    def OnSize(self, evt):
        wx.CallAfter(self.obj.refreshView)


class SndViewTable(wx.Frame):
    def __init__(self, parent, obj=None, tableclass=None, mouse_callback=None):
        wx.Frame.__init__(self, parent, size=(500, 250))
        self.SetMinSize((300, 150))
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        closeItem = self.fileMenu.Append(-1, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self._destroy, closeItem)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self.obj = obj
        self.chnls = len(self.obj)
        self.dur = self.obj.getDur(False)
        if sys.platform == "darwin":
            self.panel = wx.Panel(self, style=wx.TAB_TRAVERSAL)
            self.panel.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
        else:
            self.panel = BasePanel(self)
        self.box = wx.BoxSizer(wx.VERTICAL)
        self.wavePanel = SndViewTablePanel(self.panel, obj, mouse_callback)
        self.box.Add(self.wavePanel, 1, wx.EXPAND | wx.ALL, 5)
        self.zoomH = HRangeSlider(
            self.panel,
            minvalue=0,
            maxvalue=1,
            init=None,
            pos=(0, 0),
            size=(200, 15),
            valtype="float",
            log=False,
            function=self.setZoomH,
        )
        self.box.Add(self.zoomH, 0, wx.EXPAND | wx.LEFT | wx.RIGHT, 5)
        self.panel.SetSizer(self.box)

    def setZoomH(self, values):
        self.wavePanel.setBegin(self.dur * values[0])
        self.wavePanel.setEnd(self.dur * values[1])
        self.update()

    def update(self):
        self.wavePanel.setImage()

    def _destroy(self, evt):
        self.obj._setViewFrame(None)
        self.Destroy()


class SndViewTablePanel(wx.Panel):
    def __init__(self, parent, obj=None, mouse_callback=None, select_callback=None):
        wx.Panel.__init__(self, parent)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
        self.Bind(wx.EVT_RIGHT_DOWN, self.OnRightDown)
        self.Bind(wx.EVT_RIGHT_UP, self.OnMouseUp)
        self.Bind(wx.EVT_MOTION, self.OnMotion)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.refresh_from_selection = False
        self.background_bitmap = None
        self.obj = obj
        self.selstart = self.selend = self.movepos = None
        self.moveSelection = False
        self.createSelection = False
        self.begin = 0
        if self.obj is not None:
            self.chnls = len(self.obj)
            self.end = self.obj.getDur(False)
        else:
            self.chnls = 1
            self.end = 1.0
        self.img = [[]]
        self.mouse_callback = mouse_callback
        self.select_callback = select_callback
        if sys.platform == "win32" or sys.platform.startswith("linux"):
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC
        self.setImage()

    def getDur(self):
        if self.obj is not None:
            return self.obj.getDur(False)
        else:
            return 1.0

    def resetSelection(self):
        self.selstart = self.selend = None
        if self.background_bitmap is not None:
            self.refresh_from_selection = True
        self.Refresh()
        if self.select_callback is not None:
            self.select_callback((0.0, 1.0))

    def setSelection(self, start, stop):
        self.selstart = start
        self.selend = stop
        if self.background_bitmap is not None:
            self.refresh_from_selection = True
        self.Refresh()
        if self.select_callback is not None:
            self.select_callback((self.selstart, self.selend))

    def setBegin(self, x):
        self.begin = x

    def setEnd(self, x):
        self.end = x

    def setImage(self):
        if self.obj is not None:
            self.img = self.obj.getViewTable(self.GetSize(), self.begin, self.end)
            wx.CallAfter(self.Refresh)

    def clipPos(self, pos):
        if pos[0] < 0.0:
            x = 0.0
        elif pos[0] > 1.0:
            x = 1.0
        else:
            x = pos[0]
        if pos[1] < 0.0:
            y = 0.0
        elif pos[1] > 1.0:
            y = 1.0
        else:
            y = pos[1]
        if self.obj is not None:
            x = x * ((self.end - self.begin) / self.obj.getDur(False)) + (self.begin / self.obj.getDur(False))
        return (x, y)

    def OnMouseDown(self, evt):
        size = self.GetSize()
        pos = evt.GetPosition()
        if pos[1] <= 0:
            pos = (float(pos[0]) / size[0], 1.0)
        else:
            pos = (float(pos[0]) / size[0], 1.0 - (float(pos[1]) / size[1]))
        pos = self.clipPos(pos)
        if self.mouse_callback is not None:
            self.mouse_callback(pos)
        self.CaptureMouse()

    def OnRightDown(self, evt):
        size = self.GetSize()
        pos = evt.GetPosition()
        if pos[1] <= 0:
            pos = (float(pos[0]) / size[0], 1.0)
        else:
            pos = (float(pos[0]) / size[0], 1.0 - (float(pos[1]) / size[1]))
        pos = self.clipPos(pos)
        if evt.ShiftDown():
            if self.selstart is not None and self.selend is not None:
                self.moveSelection = True
                self.movepos = pos[0]
        elif evt.CmdDown():
            self.selstart = self.selend = None
            self.refresh_from_selection = True
            self.Refresh()
            if self.select_callback is not None:
                self.select_callback((0.0, 1.0))
        else:
            self.createSelection = True
            self.selstart = pos[0]
        self.CaptureMouse()

    def OnMotion(self, evt):
        if self.HasCapture():
            size = self.GetSize()
            pos = evt.GetPosition()
            if pos[1] <= 0:
                pos = (float(pos[0]) / size[0], 1.0)
            else:
                pos = (float(pos[0]) / size[0], 1.0 - (float(pos[1]) / size[1]))
            pos = self.clipPos(pos)
            if evt.LeftIsDown():
                if self.mouse_callback is not None:
                    self.mouse_callback(pos)
            elif evt.RightIsDown():
                refresh = False
                if self.createSelection:
                    self.selend = pos[0]
                    refresh = True
                elif self.moveSelection:
                    diff = pos[0] - self.movepos
                    self.movepos = pos[0]
                    self.selstart += diff
                    self.selend += diff
                    refresh = True
                if refresh:
                    self.refresh_from_selection = True
                    self.Refresh()
                    if self.select_callback is not None:
                        self.select_callback((self.selstart, self.selend))

    def OnMouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()
        self.createSelection = self.moveSelection = False

    def create_background(self):
        w, h = self.GetSize()
        self.background_bitmap = wx.EmptyBitmap(w, h)
        dc = wx.MemoryDC(self.background_bitmap)
        gc = wx.GraphicsContext_Create(dc)
        dc.SetBrush(wx.Brush("#FFFFFF"))
        dc.Clear()
        dc.DrawRectangle(0, 0, w, h)

        off = h // self.chnls // 2
        gc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
        gc.SetBrush(wx.Brush("#FFFFFF", style=wx.TRANSPARENT))
        dc.SetTextForeground("#444444")
        if sys.platform in "darwin":
            font, ptsize = dc.GetFont(), dc.GetFont().GetPointSize()
            font.SetPointSize(ptsize - 3)
            dc.SetFont(font)
        else:
            font = dc.GetFont()
            font.SetPointSize(8)
            dc.SetFont(font)
        tickstep = w // 10
        if tickstep < 40:
            timelabel = "%.1f"
        elif tickstep < 80:
            timelabel = "%.2f"
        elif tickstep < 120:
            timelabel = "%.3f"
        else:
            timelabel = "%.4f"
        timestep = (self.end - self.begin) * 0.1
        for i, samples in enumerate(self.img):
            y = h // self.chnls * i
            if len(samples):
                gc.DrawLines(samples)
            dc.SetPen(wx.Pen("#888888", width=1, style=wx.DOT))
            dc.DrawLine(0, y + off, w, y + off)
            for j in range(10):
                dc.SetPen(wx.Pen("#888888", width=1, style=wx.DOT))
                dc.DrawLine(j * tickstep, 0, j * tickstep, h)
                dc.DrawText(timelabel % (self.begin + j * timestep), j * tickstep + 2, h - y - 12)
            dc.SetPen(wx.Pen("#000000", width=1))
            dc.DrawLine(0, h - y, w, h - y)

        dc.SelectObject(wx.NullBitmap)

    def OnPaint(self, evt):
        w, h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        dc.SetBrush(wx.Brush("#FFFFFF"))
        dc.Clear()
        dc.DrawRectangle(0, 0, w, h)

        if not self.refresh_from_selection:
            self.create_background()

        dc.DrawBitmap(self.background_bitmap, 0, 0)

        if self.selstart is not None and self.selend is not None:
            gc.SetPen(wx.Pen(wx.Colour(0, 0, 0, 64)))
            gc.SetBrush(wx.Brush(wx.Colour(0, 0, 0, 64)))
            if self.obj is not None:
                dur = self.obj.getDur(False)
            else:
                dur = 1.0
            selstartabs = min(self.selstart, self.selend) * dur
            selendabs = max(self.selstart, self.selend) * dur
            if selstartabs < self.begin:
                startpix = 0
            else:
                startpix = ((selstartabs - self.begin) / (self.end - self.begin)) * w
            if selendabs > self.end:
                endpix = w
            else:
                endpix = ((selendabs - self.begin) / (self.end - self.begin)) * w
            gc.DrawRectangle(int(startpix), 0, int(endpix - startpix), h)

        self.refresh_from_selection = False

    def OnSize(self, evt):
        wx.CallAfter(self.setImage)


######################################################################
## View window for PyoMatrixObject
#####################################################################
class ViewMatrixBase(wx.Frame):
    def __init__(self, parent, size=None, object=None):
        wx.Frame.__init__(self, parent)
        self.object = object
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        closeItem = self.fileMenu.Append(-1, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self._destroy, closeItem)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.SetClientSize(size)
        self.SetMinSize(self.GetSize())
        self.SetMaxSize(self.GetSize())

    def update(self, samples):
        self.setImage(samples)

    def _destroy(self, evt):
        self.object._setViewFrame(None)
        self.Destroy()


class ViewMatrix(ViewMatrixBase):
    def __init__(self, parent, samples=None, size=None, object=None):
        ViewMatrixBase.__init__(self, parent, size, object)
        self.size = size
        self.setImage(samples)

    def setImage(self, samples):
        image = wx.EmptyImage(self.size[0], self.size[1])
        image.SetData(samples)
        self.img = wx.BitmapFromImage(image)
        wx.CallAfter(self.Refresh)

    def OnPaint(self, evt):
        dc = wx.PaintDC(self)
        dc.DrawBitmap(self.img, 0, 0)


######################################################################
## Spectrum Display
######################################################################
class SpectrumDisplay(wx.Frame):
    def __init__(self, parent, obj=None):
        wx.Frame.__init__(self, parent, size=(600, 350))
        self.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
        self.SetMinSize((400, 240))
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        closeItem = self.fileMenu.Append(-1, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self._destroy, closeItem)
        self.menubar.Append(self.fileMenu, "&File")
        pollMenu = wx.Menu()
        pollID = 20000
        self.availableSpeeds = [0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1]
        for speed in self.availableSpeeds:
            pollMenu.Append(pollID, "%.3f" % speed, kind=wx.ITEM_RADIO)
            if speed == 0.05:
                pollMenu.Check(pollID, True)
            self.Bind(wx.EVT_MENU, self.setPollTime, id=pollID)
            pollID += 1
        self.menubar.Append(pollMenu, "&Polling Speed")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self.obj = obj
        if sys.platform == "darwin":
            self.panel = wx.Panel(self, style=wx.TAB_TRAVERSAL)
            self.panel.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
        else:
            self.panel = BasePanel(self)
        self.mainBox = wx.BoxSizer(wx.VERTICAL)
        self.toolBox = wx.BoxSizer(wx.HORIZONTAL)
        if sys.platform == "darwin":
            X_OFF = 24
        else:
            X_OFF = 16

        if self.obj is None:
            initgain = 0.0
            self.channelNamesVisible = True
            self.channelNames = []
        else:
            initgain = self.obj.gain
            self.channelNamesVisible = self.obj.channelNamesVisible
            self.channelNames = self.obj.channelNames

        tw, th = self.GetTextExtent("Start")
        self.activeTog = wx.ToggleButton(self.panel, -1, label="Start", size=(tw + X_OFF, th + 10))
        self.activeTog.SetValue(1)
        self.activeTog.Bind(wx.EVT_TOGGLEBUTTON, self.activate)
        self.toolBox.Add(self.activeTog, 0, wx.TOP | wx.LEFT, 5)
        tw, th = self.GetTextExtent("Freq Log")
        self.freqTog = wx.ToggleButton(self.panel, -1, label="Freq Log", size=(tw + X_OFF, th + 10))
        self.freqTog.SetValue(0)
        self.freqTog.Bind(wx.EVT_TOGGLEBUTTON, self.setFreqScale)
        self.toolBox.Add(self.freqTog, 0, wx.TOP | wx.LEFT, 5)
        tw, th = self.GetTextExtent("Mag Log")
        self.magTog = wx.ToggleButton(self.panel, -1, label="Mag Log", size=(tw + X_OFF, th + 10))
        self.magTog.SetValue(1)
        self.magTog.Bind(wx.EVT_TOGGLEBUTTON, self.setMagScale)
        self.toolBox.Add(self.magTog, 0, wx.TOP | wx.LEFT, 5)
        tw, th = self.GetTextExtent("Blackman 3-term")
        self.winPopup = wx.Choice(
            self.panel,
            -1,
            choices=[
                "Rectangular",
                "Hamming",
                "Hanning",
                "Bartlett",
                "Blackman 3",
                "Blackman-H 4",
                "Blackman-H 7",
                "Tuckey",
                "Half-sine",
            ],
            size=(tw + X_OFF, th + 10),
        )
        self.winPopup.SetSelection(2)
        self.winPopup.Bind(wx.EVT_CHOICE, self.setWinType)
        self.toolBox.Add(self.winPopup, 0, wx.TOP | wx.LEFT, 5)
        tw, th = self.GetTextExtent("16384")
        self.sizePopup = wx.Choice(
            self.panel,
            -1,
            choices=["64", "128", "256", "512", "1024", "2048", "4096", "8192", "16384"],
            size=(-1, th + 10),
        )
        self.sizePopup.SetSelection(4)
        self.sizePopup.Bind(wx.EVT_CHOICE, self.setSize)
        self.toolBox.Add(self.sizePopup, 0, wx.TOP | wx.LEFT, 5)
        self.mainBox.Add(self.toolBox, 0, wx.EXPAND)
        self.dispBox = wx.BoxSizer(wx.HORIZONTAL)
        self.box = wx.BoxSizer(wx.VERTICAL)
        self.spectrumPanel = SpectrumPanel(
            self.panel,
            len(self.obj),
            self.obj.getLowfreq(),
            self.obj.getHighfreq(),
            self.obj.getFscaling(),
            self.obj.getMscaling(),
        )
        self.box.Add(self.spectrumPanel, 1, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, 5)
        self.zoomH = HRangeSlider(
            self.panel,
            minvalue=0,
            maxvalue=0.5,
            init=None,
            pos=(0, 0),
            size=(200, 15),
            valtype="float",
            log=False,
            function=self.setZoomH,
        )
        self.box.Add(self.zoomH, 0, wx.EXPAND | wx.LEFT | wx.RIGHT, 5)
        self.dispBox.Add(self.box, 1, wx.EXPAND, 0)
        self.gainSlider = ControlSlider(self.panel, -24, 24, initgain, outFunction=self.setGain, orient=wx.VERTICAL)
        self.dispBox.Add(self.gainSlider, 0, wx.EXPAND | wx.TOP, 5)
        self.dispBox.AddSpacer(5)
        self.mainBox.Add(self.dispBox, 1, wx.EXPAND)
        self.panel.SetSizer(self.mainBox)

        self.Bind(wx.EVT_SYS_COLOUR_CHANGED, self.OnColourChanged)

    def OnColourChanged(self, evt):
        self.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))

    def activate(self, evt):
        if evt.GetInt() == 1:
            self.obj.poll(1)
        else:
            self.obj.poll(0)

    def setPollTime(self, evt):
        value = self.availableSpeeds[evt.GetId() - 20000]
        self.obj.polltime(value)

    def setFreqScale(self, evt):
        if evt.GetInt() == 1:
            self.obj.setFscaling(1)
        else:
            self.obj.setFscaling(0)

    def setMagScale(self, evt):
        if evt.GetInt() == 1:
            self.obj.setMscaling(1)
        else:
            self.obj.setMscaling(0)

    def setWinType(self, evt):
        self.obj.wintype = evt.GetInt()

    def setSize(self, evt):
        size = 1 << (evt.GetInt() + 6)
        self.obj.size = size

    def setGain(self, gain):
        self.obj.setGain(pow(10.0, gain * 0.05))

    def setZoomH(self, values):
        self.spectrumPanel.setLowFreq(self.obj.setLowbound(values[0]))
        self.spectrumPanel.setHighFreq(self.obj.setHighbound(values[1]))
        wx.CallAfter(self.spectrumPanel.Refresh)

    def setDisplaySize(self, size):
        self.obj.setWidth(size[0])
        self.obj.setHeight(size[1])

    def update(self, points):
        self.spectrumPanel.setImage(points)

    def setFscaling(self, x):
        self.spectrumPanel.setFscaling(x)
        wx.CallAfter(self.spectrumPanel.Refresh)

    def setMscaling(self, x):
        self.spectrumPanel.setMscaling(x)
        wx.CallAfter(self.spectrumPanel.Refresh)

    def showChannelNames(self, visible):
        self.spectrumPanel.showChannelNames(visible)
        self.channelNamesVisible = visible

    def setChannelNames(self, names):
        self.channelNames = names
        self.spectrumPanel.setChannelNames(names)

    def _destroy(self, evt):
        self.obj._setViewFrame(None)
        self.Destroy()


# TODO: Adjust the font size according to the size of the panel.
class SpectrumPanel(wx.Panel):
    def __init__(
        self, parent, chnls, lowfreq, highfreq, fscaling, mscaling, pos=wx.DefaultPosition, size=wx.DefaultSize, style=0
    ):
        wx.Panel.__init__(self, parent, pos=pos, size=size, style=style)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.SetMinSize((300, 100))
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        if chnls == 1:
            self.chnls = 64
        else:
            self.chnls = chnls
        try:
            self.channelNamesVisible = self.GetParent().GetParent().channelNamesVisible
        except:
            self.channelNamesVisible = True
        try:
            self.channelNames = self.GetParent().GetParent().channelNames
        except:
            self.channelNames = []
        self.img = None
        self.obj = None
        self.lowfreq = lowfreq
        self.highfreq = highfreq
        self.fscaling = fscaling
        self.mscaling = mscaling
        self.setPens()
        if sys.platform == "win32" or sys.platform.startswith("linux"):
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC

    def OnSize(self, evt):
        try:
            self.GetParent().GetParent().setDisplaySize(self.GetSize())
        except:
            pass
        try:
            size = self.GetSize()
            self.obj.setWidth(size[0])
            self.obj.setHeight(size[1])
        except:
            pass

        self.Refresh()

    def setImage(self, points):
        self.img = [points[i] for i in range(len(points))]
        wx.CallAfter(self.Refresh)

    def setPens(self):
        self.pens = []
        self.brushes = []
        for x in range(self.chnls):
            hue = rescale(x, xmin=0, xmax=self.chnls - 1, ymin=0, ymax=2.0 / 3)
            hsv = wx.Image_HSVValue(hue, 1.0, 0.6)
            rgb = wx.Image_HSVtoRGB(hsv)
            self.pens.append(wx.Pen(wx.Colour(rgb.red, rgb.green, rgb.blue), 1))
            self.brushes.append(wx.Brush(wx.Colour(rgb.red, rgb.green, rgb.blue, 128)))

    def setChnls(self, x):
        if x == 1:
            self.chnls = 64
        else:
            self.chnls = x
        self.setPens()

    def setFscaling(self, x):
        self.fscaling = x

    def setMscaling(self, x):
        self.mscaling = x

    def setLowFreq(self, x):
        self.lowfreq = x

    def setHighFreq(self, x):
        self.highfreq = x

    def showChannelNames(self, visible):
        self.channelNamesVisible = visible

    def setChannelNames(self, names):
        self.channelNames = names

    def OnPaint(self, evt):
        w, h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        tw, th = dc.GetTextExtent("0")

        # background
        background = gc.CreatePath()
        background.AddRectangle(0, 0, w - 1, h - 1)
        gc.SetPen(wx.BLACK_PEN)
        gc.SetBrush(wx.WHITE_BRUSH)
        gc.DrawPath(background)

        dc.SetTextForeground("#555555")
        dc.SetPen(wx.Pen("#555555", style=wx.DOT))

        # frequency linear grid
        if not self.fscaling:
            text = str(int(self.lowfreq))
            tw, th = dc.GetTextExtent(text)
            step = (self.highfreq - self.lowfreq) / 8
            dc.DrawText(text, 2, 2)
            w8 = w // 8
            for i in range(1, 8):
                pos = w8 * i
                dc.DrawLine(pos, th + 4, pos, h - 2)
                text = str(int(self.lowfreq + step * i))
                tw, th = dc.GetTextExtent(text)
                dc.DrawText(text, pos - tw // 2, 2)
        # frequency logarithmic grid
        else:
            if self.lowfreq < 20:
                lf = math.log10(20)
            else:
                lf = math.log10(self.lowfreq)

            hf = math.log10(self.highfreq)
            lrange = hf - lf
            mag = pow(10.0, math.floor(lf))
            if lrange > 6:
                t = pow(10.0, math.ceil(lf))
                base = pow(10.0, math.floor(lrange / 6))

                def inc(t, floor_t):
                    return t * base - t

            else:
                t = math.ceil(pow(10.0, lf) / mag) * mag

                def inc(t, floor_t):
                    return pow(10.0, floor_t)

            majortick = int(math.log10(mag))
            while t <= pow(10, hf):
                floor_t = int(math.floor(math.log10(t) + 1e-16))
                if majortick != floor_t:
                    majortick = floor_t
                    ticklabel = "1e%d" % majortick
                    ticklabel = str(int(float(ticklabel)))
                    tw, th = dc.GetTextExtent(ticklabel)
                else:
                    if hf - lf < 2:
                        minortick = int(t / pow(10.0, majortick) + 0.5)
                        ticklabel = "%de%d" % (minortick, majortick)
                        ticklabel = str(int(float(ticklabel)))
                        tw, th = dc.GetTextExtent(ticklabel)
                        if not minortick % 2 == 0:
                            ticklabel = ""
                    else:
                        ticklabel = ""
                pos = int((math.log10(t) - lf) / lrange * w)
                if pos < (w - 25):
                    dc.DrawLine(pos, th + 4, pos, h - 2)
                    dc.DrawText(ticklabel, pos - tw // 2, 2)
                t += inc(t, floor_t)

        # magnitude linear grid
        if not self.mscaling:
            h4 = h * 0.75
            step = h4 * 0.1
            for i in range(1, 11):
                pos = int(h - i * step)
                text = "%.1f" % (i * 0.1)
                tw, th = dc.GetTextExtent(text)
                dc.DrawText(text, w - tw - 2, pos - th // 2)
                dc.DrawLine(0, pos, w - tw - 4, pos)
            dc.SetPen(wx.Pen("#555555", style=wx.SOLID))
            dc.DrawLine(0, pos, w - tw - 6, pos)
            dc.SetPen(wx.Pen("#555555", style=wx.DOT))
            i += 1
            while i * step < (h - th - 5):
                pos = int(h - i * step)
                text = "%.1f" % (i * 0.1)
                tw, th = dc.GetTextExtent(text)
                dc.DrawText(text, w - tw - 2, pos - th // 2)
                dc.DrawLine(0, pos, w - tw - 6, pos)
                i += 1
        # magnitude logarithmic grid
        else:
            mw, mh = dc.GetTextExtent("-54")
            h4 = h * 0.75
            step = h4 * 0.1
            for i in range(1, 11):
                pos = int(h - i * step)
                mval = int((10 - i) * -6.0)
                if mval == -0:
                    mval = 0
                text = "%d" % mval
                tw, th = dc.GetTextExtent(text)
                dc.DrawText(text, w - tw - 2, pos - th // 2)
                dc.DrawLine(0, pos, w - mw - 6, pos)
            dc.SetPen(wx.Pen("#555555", style=wx.SOLID))
            dc.DrawLine(0, pos, w - mw - 4, pos)
            dc.SetPen(wx.Pen("#555555", style=wx.DOT))
            i += 1
            while i * step < (h - th - 5):
                pos = int(h - i * step)
                text = "%d" % int((10 - i) * -6.0)
                tw, th = dc.GetTextExtent(text)
                dc.DrawText(text, w - tw - 2, pos - th // 2)
                dc.DrawLine(0, pos, w - mw - 6, pos)
                i += 1

        # spectrum
        if self.img is not None:
            last_tw = tw
            # legend
            if len(self.img) > 1 and self.channelNamesVisible:
                if not self.channelNames:
                    tw, th = dc.GetTextExtent("chan 8")
                    for i in range(len(self.img)):
                        dc.SetTextForeground(self.pens[i % self.chnls].GetColour())
                        dc.DrawText("chan %d" % (i + 1), w - tw - 20 - last_tw, i * th + th + 7)
                else:
                    numChars = max([len(x) for x in self.channelNames])
                    tw, th = dc.GetTextExtent("0" * numChars)
                    for i in range(len(self.img)):
                        dc.SetTextForeground(self.pens[i % self.chnls].GetColour())
                        if i < len(self.channelNames):
                            dc.DrawText(self.channelNames[i], w - tw - 20 - last_tw, i * th + th + 7)
                        else:
                            dc.DrawText("chan %d" % (i + 1), w - tw - 20 - last_tw, i * th + th + 7)
            # channel spectrums
            for i, samples in enumerate(self.img):
                gc.SetPen(self.pens[i % self.chnls])
                gc.SetBrush(self.brushes[i % self.chnls])
                gc.DrawLines(samples)


######################################################################
## Scope Display
######################################################################
class ScopeDisplay(wx.Frame):
    def __init__(self, parent, obj=None):
        wx.Frame.__init__(self, parent, size=(600, 350))
        self.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
        self.SetMinSize((400, 240))
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        closeItem = self.fileMenu.Append(-1, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self._destroy, closeItem)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self.Bind(wx.EVT_SYS_COLOUR_CHANGED, self.OnColourChanged)

        self.obj = obj
        gain = self.obj.gain
        length = self.obj.length
        if sys.platform == "darwin":
            self.panel = wx.Panel(self, style=wx.TAB_TRAVERSAL)
            self.panel.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
        else:
            self.panel = BasePanel(self)
        self.mainBox = wx.BoxSizer(wx.VERTICAL)
        self.toolBox = wx.BoxSizer(wx.HORIZONTAL)
        if sys.platform == "darwin":
            X_OFF = 24
        else:
            X_OFF = 16
        tw, th = self.GetTextExtent("Start")
        self.activeTog = wx.ToggleButton(self.panel, -1, label="Start", size=(tw + X_OFF, th + 10))
        self.activeTog.SetValue(1)
        self.activeTog.Bind(wx.EVT_TOGGLEBUTTON, self.activate)
        self.toolBox.Add(self.activeTog, 0, wx.TOP | wx.LEFT | wx.RIGHT, 5)
        self.toolBox.AddSpacer(10)
        self.windowLengthText = wx.StaticText(self.panel, -1, label="Window length (ms):")
        self.toolBox.Add(self.windowLengthText, 0, wx.TOP, 11)
        self.lenSlider = ControlSlider(self.panel, 10, 1000, length * 1000, log=True, outFunction=self.setLength)
        self.toolBox.Add(self.lenSlider, 1, wx.TOP | wx.LEFT | wx.RIGHT, 11)
        self.toolBox.AddSpacer(40)
        self.mainBox.Add(self.toolBox, 0, wx.EXPAND)
        self.dispBox = wx.BoxSizer(wx.HORIZONTAL)
        self.box = wx.BoxSizer(wx.VERTICAL)
        self.scopePanel = ScopePanel(self.panel, self.obj)
        self.box.Add(self.scopePanel, 1, wx.EXPAND | wx.LEFT | wx.RIGHT, 5)
        self.dispBox.Add(self.box, 1, wx.EXPAND | wx.BOTTOM, 5)
        self.gainSlider = ControlSlider(
            self.panel, -24, 24, 20.0 * math.log10(gain), outFunction=self.setGain, orient=wx.VERTICAL
        )
        self.dispBox.Add(self.gainSlider, 0, wx.EXPAND | wx.BOTTOM, 5)
        self.dispBox.AddSpacer(5)
        self.mainBox.Add(self.dispBox, 1, wx.EXPAND)
        self.panel.SetSizer(self.mainBox)

    def OnColourChanged(self, evt):
        self.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
        colour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_MENUTEXT)
        self.windowLengthText.SetForegroundColour(colour)

    def activate(self, evt):
        self.obj.poll(evt.GetInt())

    def setLength(self, length):
        length *= 0.001
        self.obj.setLength(length)
        self.scopePanel.setLength(length)

    def setGain(self, gain):
        gain = pow(10.0, gain * 0.05)
        self.scopePanel.setGain(gain)
        self.obj.setGain(gain)

    def update(self, points):
        self.scopePanel.setImage(points)

    def showChannelNames(self, visible):
        self.scopePanel.showChannelNames(visible)

    def setChannelNames(self, names):
        self.scopePanel.setChannelNames(names)

    def _destroy(self, evt):
        self.obj._setViewFrame(None)
        self.Destroy()


class ScopePanel(wx.Panel):
    def __init__(self, parent, obj=None, pos=wx.DefaultPosition, size=wx.DefaultSize, style=0):
        wx.Panel.__init__(self, parent, pos=pos, size=size, style=style)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.SetMinSize((300, 100))
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.img = [[]]
        self.obj = obj
        if self.obj is not None:
            self.gain = self.obj.gain
            self.length = self.obj.length
            self.chnls = len(self.obj)
            self.channelNamesVisible = self.obj.channelNamesVisible
            self.channelNames = self.obj.channelNames
        else:
            self.gain = 1
            self.length = 0.05
            self.chnls = 64
            self.channelNamesVisible = True
            self.channelNamesVisible = []

        self.setPens()

        if sys.platform == "win32" or sys.platform.startswith("linux"):
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC

    def OnSize(self, evt):
        try:
            size = self.GetSize()
            self.obj.setWidth(size[0])
            self.obj.setHeight(size[1])
        except:
            pass
        wx.CallAfter(self.Refresh)

    def setChnls(self, x):
        if x == 1:
            self.chnls = 64
        else:
            self.chnls = x
        self.setPens()

    def setPens(self):
        self.pens = []
        if self.chnls < 2:
            hsv = wx.Image.HSVValue(0.0, 1.0, 0.6)
            rgb = wx.Image.HSVtoRGB(hsv)
            self.pens.append(wx.Pen(wx.Colour(rgb.red, rgb.green, rgb.blue), 1))
        else:
            for x in range(self.chnls):
                hue = rescale(x, xmin=0, xmax=self.chnls - 1, ymin=0, ymax=2.0 / 3)
                hsv = wx.Image.HSVValue(hue, 0.99, 0.6)
                rgb = wx.Image.HSVtoRGB(hsv)
                self.pens.append(wx.Pen(wx.Colour(rgb.red, rgb.green, rgb.blue), 1))

    def setGain(self, gain):
        self.gain = gain

    def setLength(self, length):
        self.length = length

    def setImage(self, points):
        self.img = points
        wx.CallAfter(self.Refresh)

    def showChannelNames(self, visible=True):
        self.channelNamesVisible = visible

    def setChannelNames(self, names):
        self.channelNames = names

    def OnPaint(self, evt):
        w, h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        tw, th = dc.GetTextExtent("0")
        dc.SetBrush(wx.Brush("#FFFFFF"))
        dc.Clear()
        dc.DrawRectangle(0, 0, w, h)
        gc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
        gc.SetBrush(wx.Brush("#FFFFFF", style=wx.TRANSPARENT))
        dc.SetTextForeground("#444444")
        if sys.platform == "darwin":
            font, ptsize = dc.GetFont(), dc.GetFont().GetPointSize()
            font.SetPointSize(ptsize - 3)
            dc.SetFont(font)
        elif sys.platform.startswith("linux"):
            font, ptsize = dc.GetFont(), dc.GetFont().GetPointSize()
            font.SetPointSize(ptsize - 1)
            dc.SetFont(font)
        elif sys.platform == "win32":
            font = dc.GetFont()
            font.SetPointSize(8)
            dc.SetFont(font)

        dc.SetPen(wx.Pen("#888888", width=1, style=wx.DOT))
        # horizontal grid
        step = h // 6
        ampstep = 1.0 / 3.0 / self.gain
        for i in range(1, 6):
            pos = int(h - i * step)
            npos = i - 3
            text = "%.2f" % (ampstep * npos)
            tw, th = dc.GetTextExtent(text)
            dc.DrawText(text, w - tw - 2, pos - th // 2)
            dc.DrawLine(0, pos, w - tw - 10, pos)

        # vertical grid
        tickstep = w // 4
        timestep = self.length * 0.25
        for j in range(4):
            dc.SetPen(wx.Pen("#888888", width=1, style=wx.DOT))
            dc.DrawLine(j * tickstep, 0, j * tickstep, h)
            dc.DrawText("%.3f" % (j * timestep), j * tickstep + 2, h - 15)
        # draw waveforms
        for i, samples in enumerate(self.img):
            samples.append((w+1, h+1))
            samples.append((-1, h+1))
            gc.SetPen(self.pens[i % 8])
            if len(samples) > 1:
                gc.DrawLines(samples)

        # legend
        last_tw = tw
        if len(self.img) > 1 and self.channelNamesVisible:
            if not self.channelNames:
                tw, th = dc.GetTextExtent("chan 8")
                for i in range(len(self.img)):
                    dc.SetTextForeground(self.pens[i % self.chnls].GetColour())
                    dc.DrawText("chan %d" % (i + 1), w - tw - 20 - last_tw, i * th + th + 7)  # 10
            else:
                numChars = max([len(x) for x in self.channelNames])
                tw, th = dc.GetTextExtent("0" * numChars)
                for i in range(len(self.img)):
                    dc.SetTextForeground(self.pens[i % self.chnls].GetColour())
                    if i < len(self.channelNames):
                        dc.DrawText(self.channelNames[i], w - tw - 20 - last_tw, i * th + th + 7)
                    else:
                        dc.DrawText("chan %d" % (i + 1), w - tw - 20 - last_tw, i * th + th + 7)


######################################################################
## Grapher window for PyoTableObject control
######################################################################
OFF = 10
OFF2 = OFF * 2
RAD = 3
RAD2 = RAD * 2
AREA = RAD + 2
AREA2 = AREA * 2


class Grapher(wx.Panel):
    def __init__(
        self,
        parent,
        xlen=8192,
        yrange=(0.0, 1.0),
        init=[(0.0, 0.0), (1.0, 1.0)],
        mode=0,
        exp=10.0,
        inverse=True,
        tension=0.0,
        bias=0.0,
        outFunction=None,
        pos=(0, 0),
        size=(300, 200),
        style=0,
    ):
        wx.Panel.__init__(self, parent, pos=pos, size=size, style=style)
        self.backgroundColour = "#FFFFFF"
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.SetBackgroundColour(self.backgroundColour)
        self.Bind(wx.EVT_LEAVE_WINDOW, self.OnLeave)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        self.Bind(wx.EVT_SIZE, self.OnResize)

        self.mode = mode
        self.exp = exp
        self.inverse = inverse
        self.tension = tension
        self.bias = bias
        self.pos = (OFF + RAD, OFF + RAD)
        self.selected = None
        self.xlen = xlen
        self.yrange = yrange
        self.init = [tup for tup in init]
        self.points = [tup for tup in init]
        self.outFunction = outFunction

        if sys.platform == "win32" or sys.platform.startswith("linux"):
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC

        self.SetFocus()
        wx.CallAfter(self.sendValues)

    def setInitPoints(self, pts):
        self.init = [(p[0], p[1]) for p in pts]
        self.points = [(p[0], p[1]) for p in pts]
        self.selected = None
        self.sendValues()
        self.Refresh()

    def pointToPixels(self, pt):
        w, h = self.GetSize()
        w, h = w - OFF2 - RAD2, h - OFF2 - RAD2
        x = int(round(pt[0] * w)) + OFF + RAD
        y = int(round(pt[1] * h)) + OFF + RAD
        return x, y

    def pixelsToPoint(self, pos):
        w, h = self.GetSize()
        w, h = w - OFF2 - RAD2, h - OFF2 - RAD2
        x = (pos[0] - OFF - RAD) / float(w)
        y = (pos[1] - OFF - RAD) / float(h)
        return x, y

    def pointToValues(self, pt):
        x = pt[0] * self.xlen
        if type(self.xlen) == int:
            x = int(x)
        y = pt[1] * (self.yrange[1] - self.yrange[0]) + self.yrange[0]
        return x, y

    def valuesToPoint(self, val):
        x = val[0] / float(self.xlen)
        y = (val[1] - self.yrange[0]) / float(self.yrange[1] - self.yrange[0])
        return x, y

    def borderClip(self, pos):
        w, h = self.GetSize()
        if pos[0] < (OFF + RAD):
            pos[0] = OFF + RAD
        elif pos[0] > (w - OFF - RAD):
            pos[0] = w - OFF - RAD
        if pos[1] < (OFF + RAD):
            pos[1] = OFF + RAD
        elif pos[1] > (h - OFF - RAD):
            pos[1] = h - OFF - RAD
        return pos

    def pointClip(self, pos):
        w, h = self.GetSize()
        if self.selected == 0:
            leftclip = OFF + RAD
        else:
            x, y = self.pointToPixels(self.points[self.selected - 1])
            leftclip = x
        if self.selected == (len(self.points) - 1):
            rightclip = w - OFF - RAD
        else:
            x, y = self.pointToPixels(self.points[self.selected + 1])
            rightclip = x

        if pos[0] < leftclip:
            pos[0] = leftclip
        elif pos[0] > rightclip:
            pos[0] = rightclip
        if pos[1] < (OFF + RAD):
            pos[1] = OFF + RAD
        elif pos[1] > (h - OFF - RAD):
            pos[1] = h - OFF - RAD
        return pos

    def reset(self):
        self.points = [tup for tup in self.init]
        self.Refresh()

    def getPoints(self):
        return [tup for tup in self.points]

    def getValues(self):
        values = []
        for pt in self.points:
            x, y = self.pointToValues(pt)
            values.append((x, y))
        return values

    def sendValues(self):
        if self.outFunction is not None:
            values = self.getValues()
            self.outFunction(values)

    def OnResize(self, evt):
        self.Refresh()
        evt.Skip()

    def OnLeave(self, evt):
        self.pos = (OFF + RAD, OFF + RAD)
        self.Refresh()

    def OnKeyDown(self, evt):
        if self.selected is not None and evt.GetKeyCode() in [wx.WXK_BACK, wx.WXK_DELETE, wx.WXK_NUMPAD_DELETE]:
            del self.points[self.selected]
            self.sendValues()
            self.selected = None
            self.Refresh()
        elif evt.GetKeyCode() in [wx.WXK_UP, wx.WXK_NUMPAD_UP]:
            self.points = [(pt[0], pt[1] + 0.002) for pt in self.points]
            self.sendValues()
            self.Refresh()
        elif evt.GetKeyCode() in [wx.WXK_DOWN, wx.WXK_NUMPAD_DOWN]:
            self.points = [(pt[0], pt[1] - 0.002) for pt in self.points]
            self.sendValues()
            self.Refresh()
        evt.Skip()

    def MouseDown(self, evt):
        self.CaptureMouse()
        w, h = self.GetSize()
        self.pos = self.borderClip(evt.GetPosition())
        self.pos[1] = h - self.pos[1]
        for i, p in enumerate(self.points):
            x, y = self.pointToPixels(p)
            if wx.Rect(x - AREA, y - AREA, AREA2, AREA2).Contains(self.pos):
                # Grab a point
                self.selected = i
                self.Refresh()
                return
        # Add a point
        pt = self.pixelsToPoint(self.pos)
        for i, p in enumerate(self.points):
            if p >= pt:
                self.points.insert(i, pt)
                break
        self.selected = self.points.index(pt)
        self.Refresh()

    def MouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()
            self.sendValues()

    def MouseMotion(self, evt):
        w, h = self.GetSize()
        self.pos = self.borderClip(evt.GetPosition())
        self.pos[1] = h - self.pos[1]
        if self.HasCapture():
            if self.selected is not None:
                self.pos = self.pointClip(self.pos)
                x, y = self.pixelsToPoint(self.pos)
                if self.mode == 4 and y <= 0:
                    y = 0.000001
                self.points[self.selected] = (x, y)
            self.Refresh()

    def getLogPoints(self, pt1, pt2):
        tmp = []
        if pt1[1] <= 0.0:
            pt1 = (pt1[0], 0.000001)
        if pt2[1] <= 0.0:
            pt2 = (pt2[0], 0.000001)
        if pt1[1] > pt2[1]:
            low = pt2[1]
            high = pt1[1]
        else:
            low = pt1[1]
            high = pt2[1]

        steps = pt2[0] - pt1[0]
        if steps > 0:
            lrange = high - low
            logrange = math.log10(high) - math.log10(low)
            logmin = math.log10(low)
            diff = (float(pt2[1]) - pt1[1]) / steps
            if lrange == 0:
                for i in range(steps):
                    tmp.append((pt1[0] + i, pt1[1]))
            else:
                for i in range(steps):
                    ratio = ((pt1[1] + diff * i) - low) / lrange
                    tmp.append((pt1[0] + i, pow(10, ratio * logrange + logmin)))
        return tmp

    def getCosLogPoints(self, pt1, pt2):
        tmp = []
        if pt1[1] <= 0.0:
            pt1 = (pt1[0], 0.000001)
        if pt2[1] <= 0.0:
            pt2 = (pt2[0], 0.000001)
        if pt1[1] > pt2[1]:
            low = pt2[1]
            high = pt1[1]
        else:
            low = pt1[1]
            high = pt2[1]

        steps = pt2[0] - pt1[0]
        if steps > 0:
            lrange = high - low
            logrange = math.log10(high) - math.log10(low)
            logmin = math.log10(low)
            diff = (float(pt2[1]) - pt1[1]) / steps
            if lrange == 0:
                for i in range(steps):
                    tmp.append((pt1[0] + i, pt1[1]))
            else:
                for i in range(steps):
                    mu = float(i) / steps
                    mu = (1.0 - math.cos(mu * math.pi)) * 0.5
                    mu = pt1[1] * (1.0 - mu) + pt2[1] * mu
                    ratio = (mu - low) / lrange
                    tmp.append((pt1[0] + i, pow(10, ratio * logrange + logmin)))
        return tmp

    def getCosPoints(self, pt1, pt2):
        tmp = []
        steps = pt2[0] - pt1[0]
        for i in range(steps):
            mu = float(i) / steps
            mu2 = (1.0 - math.cos(mu * math.pi)) * 0.5
            tmp.append((pt1[0] + i, pt1[1] * (1.0 - mu2) + pt2[1] * mu2))
        return tmp

    def getExpPoints(self, pt1, pt2):
        tmp = []
        ambitus = pt2[1] - pt1[1]
        steps = pt2[0] - pt1[0]
        if steps == 0:
            inc = 1.0 / 0.0001
        else:
            inc = 1.0 / steps
        pointer = 0.0
        if self.inverse:
            if ambitus >= 0:
                for i in range(steps):
                    scl = 1.0 - pow(1.0 - pointer, self.exp)
                    tmp.append((pt1[0] + i, scl * ambitus + pt1[1]))
                    pointer += inc
            else:
                for i in range(steps):
                    scl = pow(pointer, self.exp)
                    tmp.append((pt1[0] + i, scl * ambitus + pt1[1]))
                    pointer += inc
        else:
            for i in range(steps):
                scl = pow(pointer, self.exp)
                tmp.append((pt1[0] + i, scl * ambitus + pt1[1]))
                pointer += inc
        return tmp

    def addImaginaryPoints(self, tmp):
        lst = []
        x = tmp[1][0] - tmp[0][0]
        if tmp[0][1] < tmp[1][1]:
            y = tmp[0][1] - tmp[1][1]
        else:
            y = tmp[0][1] + tmp[1][1]
        lst.append((x, y))
        lst.extend(tmp)
        x = tmp[-2][0] - tmp[-1][0]
        if tmp[-2][1] < tmp[-1][1]:
            y = tmp[-1][1] + tmp[-2][1]
        else:
            y = tmp[-1][1] - tmp[-2][1]
        lst.append((x, y))
        return lst

    def getCurvePoints(self, pt0, pt1, pt2, pt3):
        tmp = []
        y0, y1, y2, y3 = pt0[1], pt1[1], pt2[1], pt3[1]
        steps = pt2[0] - pt1[0]
        for i in range(steps):
            mu = float(i) / steps
            mu2 = mu * mu
            mu3 = mu2 * mu
            m0 = (y1 - y0) * (1.0 + self.bias) * (1.0 - self.tension) * 0.5
            m0 += (y2 - y1) * (1.0 - self.bias) * (1.0 - self.tension) * 0.5
            m1 = (y2 - y1) * (1.0 + self.bias) * (1.0 - self.tension) * 0.5
            m1 += (y3 - y2) * (1.0 - self.bias) * (1.0 - self.tension) * 0.5
            a0 = 2.0 * mu3 - 3.0 * mu2 + 1.0
            a1 = mu3 - 2.0 * mu2 + mu
            a2 = mu3 - mu2
            a3 = -2.0 * mu3 + 3.0 * mu2
            tmp.append((pt1[0] + i, a0 * y1 + a1 * m0 + a2 * m1 + a3 * y2))
        return tmp

    def OnPaint(self, evt):
        w, h = self.GetSize()
        corners = [(OFF, OFF), (w - OFF, OFF), (w - OFF, h - OFF), (OFF, h - OFF)]
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        gc.SetBrush(wx.Brush("#000000"))
        gc.SetPen(wx.Pen("#000000"))
        if sys.platform == "darwin":
            font, ptsize = dc.GetFont(), dc.GetFont().GetPointSize()
        else:
            font, ptsize = dc.GetFont(), 10
        font.SetPointSize(ptsize - 4)
        dc.SetFont(font)
        dc.SetTextForeground("#888888")
        dc.Clear()

        # Draw grid
        dc.SetPen(wx.Pen("#CCCCCC", 1))
        xstep = int(round((w - OFF2) / 10.0))
        ystep = int(round((h - OFF2) / 10.0))
        for i in range(10):
            xpos = i * xstep + OFF
            dc.DrawLine(xpos, OFF, xpos, h - OFF)
            ypos = i * ystep + OFF
            dc.DrawLine(OFF, ypos, w - OFF, ypos)
            if i > 0:
                if type(self.xlen) == int:
                    t = "%d" % int(self.xlen * i * 0.1)
                else:
                    t = "%.2f" % (self.xlen * i * 0.1)
                dc.DrawText(t, xpos + 2, h - OFF - 10)
            if i < 9:
                t = "%.2f" % ((9 - i) * 0.1 * (self.yrange[1] - self.yrange[0]) + self.yrange[0])
                dc.DrawText(t, OFF + 2, ypos + ystep - 10)
            else:
                t = "%.2f" % ((9 - i) * 0.1 * (self.yrange[1] - self.yrange[0]) + self.yrange[0])
                dc.DrawText(t, OFF + 2, h - OFF - 10)

        dc.SetPen(wx.Pen("#000000", 1))
        dc.SetBrush(wx.Brush("#000000"))
        # Draw bounding box
        for i in range(4):
            dc.DrawLine(corners[i][0], corners[i][1], corners[(i + 1) % 4][0], corners[(i + 1) % 4][1])

        # Convert points in pixels
        w, h = w - OFF2 - RAD2, h - OFF2 - RAD2
        tmp = []
        back_y_for_log = []
        for p in self.points:
            x = int(round(p[0] * w)) + OFF + RAD
            y = int(round((1.0 - p[1]) * h)) + OFF + RAD
            tmp.append((x, y))
            back_y_for_log.append(p[1])

        # Draw lines
        dc.SetPen(wx.Pen("#000000", 1))
        last_p = None
        if len(tmp) > 1:
            if self.mode == 0:
                for i in range(len(tmp) - 1):
                    gc.DrawLines([tmp[i], tmp[i + 1]])
            elif self.mode == 1:
                for i in range(len(tmp) - 1):
                    tmp2 = self.getCosPoints(tmp[i], tmp[i + 1])
                    if i == 0 and len(tmp2) < 2:
                        gc.DrawLines([tmp[i], tmp[i + 1]])
                    if last_p is not None:
                        gc.DrawLines([last_p, tmp[i]])
                    for j in range(len(tmp2) - 1):
                        gc.DrawLines([tmp2[j], tmp2[j + 1]])
                        last_p = tmp2[j + 1]
                if last_p is not None:
                    gc.DrawLines([last_p, tmp[-1]])
            elif self.mode == 2:
                for i in range(len(tmp) - 1):
                    tmp2 = self.getExpPoints(tmp[i], tmp[i + 1])
                    if i == 0 and len(tmp2) < 2:
                        gc.DrawLines([tmp[i], tmp[i + 1]])
                    if last_p is not None:
                        gc.DrawLines([last_p, tmp[i]])
                    for j in range(len(tmp2) - 1):
                        gc.DrawLines([tmp2[j], tmp2[j + 1]])
                        last_p = tmp2[j + 1]
                if last_p is not None:
                    gc.DrawLines([last_p, tmp[-1]])
            elif self.mode == 3:
                curvetmp = self.addImaginaryPoints(tmp)
                for i in range(1, len(curvetmp) - 2):
                    tmp2 = self.getCurvePoints(curvetmp[i - 1], curvetmp[i], curvetmp[i + 1], curvetmp[i + 2])
                    if i == 1 and len(tmp2) < 2:
                        gc.DrawLines([curvetmp[i], curvetmp[i + 1]])
                    if last_p is not None:
                        gc.DrawLines([last_p, curvetmp[i]])
                    for j in range(len(tmp2) - 1):
                        gc.DrawLines([tmp2[j], tmp2[j + 1]])
                        last_p = tmp2[j + 1]
                if last_p is not None:
                    gc.DrawLines([last_p, tmp[-1]])
            elif self.mode == 4:
                back_tmp = [p for p in tmp]
                for i in range(len(tmp)):
                    tmp[i] = (tmp[i][0], back_y_for_log[i])
                for i in range(len(tmp) - 1):
                    tmp2 = self.getLogPoints(tmp[i], tmp[i + 1])
                    for j in range(len(tmp2)):
                        tmp2[j] = (tmp2[j][0], int(round((1.0 - tmp2[j][1]) * h)) + OFF + RAD)
                    if i == 0 and len(tmp2) < 2:
                        gc.DrawLines([back_tmp[i], back_tmp[i + 1]])
                    if last_p is not None:
                        gc.DrawLines([last_p, back_tmp[i]])
                    for j in range(len(tmp2) - 1):
                        gc.DrawLines([tmp2[j], tmp2[j + 1]])
                        last_p = tmp2[j + 1]
                if last_p is not None:
                    gc.DrawLines([last_p, back_tmp[-1]])
                tmp = [p for p in back_tmp]
            elif self.mode == 5:
                back_tmp = [p for p in tmp]
                for i in range(len(tmp)):
                    tmp[i] = (tmp[i][0], back_y_for_log[i])
                for i in range(len(tmp) - 1):
                    tmp2 = self.getCosLogPoints(tmp[i], tmp[i + 1])
                    for j in range(len(tmp2)):
                        tmp2[j] = (tmp2[j][0], int(round((1.0 - tmp2[j][1]) * h)) + OFF + RAD)
                    if i == 0 and len(tmp2) < 2:
                        gc.DrawLines([back_tmp[i], back_tmp[i + 1]])
                    if last_p is not None:
                        gc.DrawLines([last_p, back_tmp[i]])
                    for j in range(len(tmp2) - 1):
                        gc.DrawLines([tmp2[j], tmp2[j + 1]])
                        last_p = tmp2[j + 1]
                if last_p is not None:
                    gc.DrawLines([last_p, back_tmp[-1]])
                tmp = [p for p in back_tmp]

        # Draw points
        for i, p in enumerate(tmp):
            if i == self.selected:
                gc.SetBrush(wx.Brush("#FFFFFF"))
                dc.SetBrush(wx.Brush("#FFFFFF"))
            else:
                gc.SetBrush(wx.Brush("#000000"))
                dc.SetBrush(wx.Brush("#000000"))
            gc.DrawEllipse(p[0] - RAD, p[1] - RAD, RAD2, RAD2)

        # Draw position values
        font.SetPointSize(ptsize - 3)
        dc.SetFont(font)
        dc.SetTextForeground("#222222")
        posptx, pospty = self.pixelsToPoint(self.pos)
        xval, yval = self.pointToValues((posptx, pospty))
        if type(self.xlen) == int:
            dc.DrawText("%d, %.3f" % (xval, yval), w - 75, OFF)
        else:
            dc.DrawText("%.3f, %.3f" % (xval, yval), w - 75, OFF)


class TableGrapher(wx.Frame):
    def __init__(self, parent=None, obj=None, mode=0, xlen=8192, yrange=(0.0, 1.0)):
        wx.Frame.__init__(self, parent, size=(500, 250))
        pts = obj.getPoints()
        self.yrange = yrange
        for i in range(len(pts)):
            x = pts[i][0] / float(xlen)
            y = (pts[i][1] - float(yrange[0])) / (yrange[1] - yrange[0])
            pts[i] = (x, y)
        if mode == 2:
            self.graph = Grapher(
                self,
                xlen=xlen,
                yrange=yrange,
                init=pts,
                mode=mode,
                exp=obj.exp,
                inverse=obj.inverse,
                outFunction=obj.replace,
            )
        elif mode == 3:
            self.graph = Grapher(
                self,
                xlen=xlen,
                yrange=yrange,
                init=pts,
                mode=mode,
                tension=obj.tension,
                bias=obj.bias,
                outFunction=obj.replace,
            )
        else:
            self.graph = Grapher(self, xlen=xlen, yrange=yrange, init=pts, mode=mode, outFunction=obj.replace)

        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(9999, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.close, id=9999)
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(
            10000, "Copy all points to the clipboard (4 digits of precision)\tCtrl+C", kind=wx.ITEM_NORMAL
        )
        self.Bind(wx.EVT_MENU, self.copy, id=10000)
        self.fileMenu.Append(
            10001, "Copy all points to the clipboard (full precision)\tShift+Ctrl+C", kind=wx.ITEM_NORMAL
        )
        self.Bind(wx.EVT_MENU, self.copy, id=10001)
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(10002, "Reset\tCtrl+R", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.reset, id=10002)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)

    def close(self, evt):
        self.Destroy()

    def copy(self, evt):
        pts = self.graph.getValues()
        if evt.GetId() == 10000:
            pstr = "["
            for i, pt in enumerate(pts):
                pstr += "("
                if type(pt[0]) == int:
                    pstr += "%d," % pt[0]
                else:
                    pstr += "%.4f," % pt[0]
                pstr += "%.4f)" % pt[1]
                if i < (len(pts) - 1):
                    pstr += ","
            pstr += "]"
        else:
            pstr = str(pts)
        data = wx.TextDataObject(pstr)
        if wx.TheClipboard.Open():
            wx.TheClipboard.Clear()
            wx.TheClipboard.SetData(data)
            wx.TheClipboard.Close()

    def reset(self, evt):
        self.graph.reset()


class DataMultiSlider(BasePanel):
    def __init__(self, parent, init, yrange=(0, 1), outFunction=None, pos=(0, 0), size=(300, 200), style=0):
        BasePanel.__init__(self, parent, pos=pos, size=size, style=style)
        self.Bind(wx.EVT_SIZE, self.OnResize)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self.changed = True
        self.values = [v for v in init]
        self.len = len(self.values)
        self.yrange = (float(yrange[0]), float(yrange[1]))
        self.outFunction = outFunction

        if sys.platform == "win32" or sys.platform.startswith("linux"):
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC

    def OnResize(self, event):
        self.Layout()
        wx.CallAfter(self.Refresh)

    def update(self, points):
        self.values = points
        self.changed = True
        wx.CallAfter(self.Refresh)

    def getValues(self):
        return self.values

    def OnPaint(self, event):
        w, h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        backgroundColour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)
        foregroundColour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_MENUTEXT)
        dc.SetBrush(wx.Brush(backgroundColour))
        dc.SetPen(wx.Pen(foregroundColour))
        dc.Clear()
        dc.DrawRectangle(0, 0, w, h)
        gc.SetBrush(wx.Brush(foregroundColour))
        gc.SetPen(wx.Pen(foregroundColour))
        scl = self.yrange[1] - self.yrange[0]
        mini = self.yrange[0]
        bw = float(w) / self.len
        points = [(0, h)]
        x = 0
        if bw >= 1:
            for i in range(self.len):
                y = h - ((self.values[i] - mini) / scl * h)
                points.append((x, y))
                x = (i + 1) * bw
                points.append((x, y))
        else:
            slice = 1 / bw
            p1 = 0
            for i in range(w):
                p2 = int((i + 1) * slice)
                y = h - ((max(self.values[p1:p2]) - mini) / scl * h)
                points.append((i, y))
                p1 = p2
        points.append((w, y))
        points.append((w, h))
        gc.DrawLines(points)
        if self.outFunction is not None and self.changed:
            self.changed = False
            self.outFunction(self.values)

    def MouseDown(self, evt):
        w, h = self.GetSize()
        self.lastpos = pos = evt.GetPosition()
        self.CaptureMouse()
        scl = self.yrange[1] - self.yrange[0]
        mini = self.yrange[0]
        bw = float(w) / self.len
        x = int(pos[0] / bw)
        y = (h - pos[1]) / float(h) * scl + mini
        self.values[x] = y
        self.changed = True
        wx.CallAfter(self.Refresh)
        evt.Skip()

    def MouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()

    def MouseMotion(self, evt):
        w, h = self.GetSize()
        pos = evt.GetPosition()
        if pos[0] < 0:
            pos[0] = 0
        elif pos[0] > w:
            pos[0] = w
        if pos[1] < 0:
            pos[1] = 0
        elif pos[1] > h:
            pos[1] = h
        if self.HasCapture() and evt.Dragging() and evt.LeftIsDown():
            scl = self.yrange[1] - self.yrange[0]
            mini = self.yrange[0]
            bw = float(w) / self.len
            x1 = int(self.lastpos[0] / bw)
            y1 = (h - self.lastpos[1]) / float(h) * scl + mini
            x2 = int(pos[0] / bw)
            y2 = (h - pos[1]) / float(h) * scl + mini
            step = abs(x2 - x1)
            if step > 1:
                inc = (y2 - y1) / step
                if x2 > x1:
                    for i in range(0, step):
                        self.values[x1 + i] = y1 + inc * i
                else:
                    for i in range(1, step):
                        self.values[x1 - i] = y1 + inc * i
            if x2 >= 0 and x2 < self.len:
                self.values[x2] = y2
            self.lastpos = pos
            self.changed = True
            wx.CallAfter(self.Refresh)


class DataTableGrapher(wx.Frame):
    def __init__(self, parent=None, obj=None, yrange=(0.0, 1.0)):
        wx.Frame.__init__(self, parent, size=(500, 250))
        self.obj = obj
        self.length = len(self.obj._get_current_data())
        self.multi = DataMultiSlider(self, self.obj._get_current_data(), yrange, outFunction=self.obj.replace)
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(9999, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.close, id=9999)
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(
            10000, "Copy all points to the clipboard (4 digits of precision)\tCtrl+C", kind=wx.ITEM_NORMAL
        )
        self.Bind(wx.EVT_MENU, self.copy, id=10000)
        self.fileMenu.Append(
            10001, "Copy all points to the clipboard (full precision)\tShift+Ctrl+C", kind=wx.ITEM_NORMAL
        )
        self.Bind(wx.EVT_MENU, self.copy, id=10001)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)

    def getLength(self):
        return self.length

    def close(self, evt):
        self.Destroy()

    def update(self, samples):
        self.multi.update(samples)

    def copy(self, evt):
        values = self.multi.getValues()
        if evt.GetId() == 10000:
            pstr = "["
            for i, val in enumerate(values):
                pstr += "%.4f" % val
                if i < (len(values) - 1):
                    pstr += ", "
            pstr += "]"
        else:
            pstr = str(values)
        data = wx.TextDataObject(pstr)
        if wx.TheClipboard.Open():
            wx.TheClipboard.Clear()
            wx.TheClipboard.SetData(data)
            wx.TheClipboard.Close()


class ExprLexer(object):
    """Defines simple interface for custom lexer objects."""

    (
        STC_EXPR_DEFAULT,
        STC_EXPR_KEYWORD,
        STC_EXPR_KEYWORD2,
        STC_EXPR_COMMENT,
        STC_EXPR_VARIABLE,
        STC_EXPR_LETVARIABLE,
    ) = list(range(6))

    def __init__(self):
        super(ExprLexer, self).__init__()

        self.alpha = "abcdefghijklmnopqrstuvwxyz"
        self.digits = "0123456789"
        self.keywords = [
            "sin",
            "cos",
            "tan",
            "tanh",
            "atan",
            "atan2",
            "sqrt",
            "log",
            "sr",
            "log2",
            "log10",
            "pow",
            "abs",
            "floor",
            "ceil",
            "exp",
            "round",
            "min",
            "max",
            "randf",
            "randi",
            "sah",
            "const",
            "pi",
            "twopi",
            "e",
            "if",
            "rpole",
            "rzero",
            "neg",
            "and",
            "or",
            "wrap",
            "delay",
            "complex",
            "real",
            "imag",
            "cpole",
            "czero",
            "out",
        ]
        self.keywords2 = ["define", "load", "var", "let"]

    def StyleText(self, evt):
        """Handle the EVT_STC_STYLENEEDED event."""
        stc = evt.GetEventObject()
        last_styled_pos = stc.GetEndStyled()
        line = stc.LineFromPosition(last_styled_pos)
        start_pos = stc.PositionFromLine(line)
        end_pos = evt.GetPosition()
        var = letvar = False
        while start_pos < end_pos:
            stc.StartStyling(start_pos)
            curchar = chr(stc.GetCharAt(start_pos))
            if curchar == "$":
                var = True
            elif var and curchar in " \t\n()":
                var = False
            if curchar == "#":
                letvar = True
            elif letvar and curchar in " \t\n()":
                letvar = False

            if var:
                style = self.STC_EXPR_VARIABLE
                stc.SetStyling(1, style)
                start_pos += 1
            elif letvar:
                style = self.STC_EXPR_LETVARIABLE
                stc.SetStyling(1, style)
                start_pos += 1
            elif curchar in self.alpha:
                start = stc.WordStartPosition(start_pos, True)
                end = stc.WordEndPosition(start, True)
                word = stc.GetTextRange(start, end)
                if word in self.keywords:
                    style = self.STC_EXPR_KEYWORD
                    stc.SetStyling(len(word), style)
                elif word in self.keywords2:
                    style = self.STC_EXPR_KEYWORD2
                    stc.SetStyling(len(word), style)
                else:
                    style = self.STC_EXPR_DEFAULT
                    stc.SetStyling(len(word), style)
                start_pos += len(word)
            elif curchar == "/" and chr(stc.GetCharAt(start_pos + 1)) == "/":
                eol = stc.GetLineEndPosition(stc.LineFromPosition(start_pos))
                style = self.STC_EXPR_COMMENT
                stc.SetStyling(eol - start_pos, style)
                start_pos = eol
            else:
                style = self.STC_EXPR_DEFAULT
                stc.SetStyling(1, style)
                start_pos += 1


class ExprEditor(stc.StyledTextCtrl):
    def __init__(self, parent, id=-1, obj=None):
        stc.StyledTextCtrl.__init__(self, parent, id)

        self.obj = obj

        if sys.platform == "darwin":
            accel_ctrl = wx.ACCEL_CMD
            self.faces = {"mono": "Monaco", "size": 12}
        else:
            accel_ctrl = wx.ACCEL_CTRL
            self.faces = {"mono": "Monospace", "size": 10}

        atable = wx.AcceleratorTable(
            [
                (accel_ctrl, wx.WXK_RETURN, 10000),
                (accel_ctrl, ord("z"), wx.ID_UNDO),
                (accel_ctrl | wx.ACCEL_SHIFT, ord("z"), wx.ID_REDO),
            ]
        )
        self.SetAcceleratorTable(atable)

        self.Bind(wx.EVT_MENU, self.onExecute, id=10000)
        self.Bind(wx.EVT_MENU, self.undo, id=wx.ID_UNDO)
        self.Bind(wx.EVT_MENU, self.redo, id=wx.ID_REDO)
        self.Bind(stc.EVT_STC_UPDATEUI, self.OnUpdateUI)

        self.lexer = ExprLexer()

        self.currentfile = ""
        self.modified = False

        self.setup()
        self.setCmdKeys()
        self.setStyle()

        self.SetText(self.obj.expr)

    def undo(self, evt):
        self.Undo()

    def redo(self, evt):
        self.Redo()

    def setup(self):
        self.SetIndent(2)
        self.SetBackSpaceUnIndents(True)
        self.SetTabIndents(True)
        self.SetTabWidth(2)
        self.SetUseTabs(False)
        self.SetMargins(2, 2)
        self.SetMarginWidth(1, 1)

    def setCmdKeys(self):
        self.CmdKeyAssign(ord("="), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMIN)
        self.CmdKeyAssign(ord("-"), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMOUT)

    def setStyle(self):
        self.SetLexer(wx.stc.STC_LEX_CONTAINER)
        self.SetStyleBits(5)
        self.Bind(wx.stc.EVT_STC_STYLENEEDED, self.OnStyling)

        self.SetCaretForeground("#000000")
        self.SetCaretWidth(2)
        # Global default styles for all languages
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, "face:%(mono)s,size:%(size)d" % self.faces)
        self.StyleClearAll()

        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, "face:%(mono)s,size:%(size)d" % self.faces)
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "face:%(mono)s" % self.faces)
        self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT, "fore:#FFFFFF,back:#0000FF,bold")
        self.StyleSetSpec(stc.STC_STYLE_BRACEBAD, "fore:#000000,back:#FF0000,bold")

        # Expr specific styles
        self.StyleSetSpec(self.lexer.STC_EXPR_DEFAULT, "fore:#000000,face:%(mono)s,size:%(size)d" % self.faces)
        self.StyleSetSpec(self.lexer.STC_EXPR_KEYWORD, "fore:#3300DD,face:%(mono)s,size:%(size)d,bold" % self.faces)
        self.StyleSetSpec(self.lexer.STC_EXPR_KEYWORD2, "fore:#0033FF,face:%(mono)s,size:%(size)d,bold" % self.faces)
        self.StyleSetSpec(self.lexer.STC_EXPR_VARIABLE, "fore:#006600,face:%(mono)s,size:%(size)d,bold" % self.faces)
        self.StyleSetSpec(self.lexer.STC_EXPR_LETVARIABLE, "fore:#555500,face:%(mono)s,size:%(size)d,bold" % self.faces)
        self.StyleSetSpec(self.lexer.STC_EXPR_COMMENT, "fore:#444444,face:%(mono)s,size:%(size)d,italic" % self.faces)

        self.SetSelBackground(1, "#CCCCDD")

    def OnStyling(self, evt):
        self.lexer.StyleText(evt)

    def loadfile(self, filename):
        self.LoadFile(filename)
        self.currentfile = filename
        self.GetParent().SetTitle(self.currentfile)

    def savefile(self, filename):
        self.currentfile = filename
        self.GetParent().SetTitle(self.currentfile)
        self.SaveFile(filename)
        self.OnUpdateUI(None)

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
        if charBefore and chr(charBefore) in "[]{}()":
            braceAtCaret = caretPos - 1

        # check after
        if braceAtCaret < 0:
            charAfter = self.GetCharAt(caretPos)
            styleAfter = self.GetStyleAt(caretPos)

            if charAfter and chr(charAfter) in "[]{}()":
                braceAtCaret = caretPos
        if braceAtCaret >= 0:
            braceOpposite = self.BraceMatch(braceAtCaret)

        if braceAtCaret != -1 and braceOpposite == -1:
            self.BraceBadLight(braceAtCaret)
        else:
            self.BraceHighlight(braceAtCaret, braceOpposite)
        # Check if horizontal scrollbar is needed
        self.checkScrollbar()

    def checkScrollbar(self):
        lineslength = [self.LineLength(i) + 1 for i in range(self.GetLineCount())]
        maxlength = max(lineslength)
        width = self.GetCharWidth() + (self.GetZoom() * 0.5)
        if (self.GetSize()[0]) < (maxlength * width):
            self.SetUseHorizontalScrollBar(True)
        else:
            self.SetUseHorizontalScrollBar(False)

    def onExecute(self, evt):
        pos = self.GetCurrentPos()
        self.obj.expr = self.GetText()
        self.SetCurrentPos(pos)
        self.SetSelection(pos, pos)


class ExprEditorFrame(wx.Frame):
    def __init__(self, parent=None, obj=None):
        wx.Frame.__init__(self, parent, size=(650, 450))
        self.obj = obj
        self.obj._editor = self
        self.editor = ExprEditor(self, -1, self.obj)
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(wx.ID_OPEN, "Open\tCtrl+O")
        self.Bind(wx.EVT_MENU, self.open, id=wx.ID_OPEN)
        self.fileMenu.Append(wx.ID_CLOSE, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.close, id=wx.ID_CLOSE)
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(wx.ID_SAVE, "Save\tCtrl+S")
        self.Bind(wx.EVT_MENU, self.save, id=wx.ID_SAVE)
        self.fileMenu.Append(wx.ID_SAVEAS, "Save As...\tShift+Ctrl+S")
        self.Bind(wx.EVT_MENU, self.saveas, id=wx.ID_SAVEAS)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)

    def open(self, evt):
        dlg = wx.FileDialog(
            self, message="Choose a file", defaultDir=os.path.expanduser("~"), defaultFile="", style=wx.FD_OPEN
        )
        if dlg.ShowModal() == wx.ID_OK:
            path = ensureNFD(dlg.GetPath())
            self.editor.loadfile(path)
        dlg.Destroy()

    def close(self, evt):
        self.obj._editor = None
        self.Destroy()

    def save(self, evt):
        path = self.editor.currentfile
        if not path:
            self.saveas(None)
        else:
            self.editor.savefile(path)

    def saveas(self, evt):
        deffile = os.path.split(self.editor.currentfile)[1]
        dlg = wx.FileDialog(
            self, message="Save file as ...", defaultDir=os.path.expanduser("~"), defaultFile=deffile, style=wx.FD_SAVE
        )
        dlg.SetFilterIndex(0)
        if dlg.ShowModal() == wx.ID_OK:
            path = ensureNFD(dlg.GetPath())
            self.editor.savefile(path)
        dlg.Destroy()

    def update(self, text):
        self.editor.SetText(text)


class MMLLexer(object):
    """Defines simple interface for custom lexer objects."""

    STC_MML_DEFAULT, STC_MML_KEYWORD, STC_MML_KEYWORD2, STC_MML_COMMENT, STC_MML_VARIABLE, STC_MML_VOICE_TOKEN = list(
        range(6)
    )

    def __init__(self):
        super(MMLLexer, self).__init__()

        self.alpha = "abcdefghijklmnopqrstuvwxyz"
        self.digits = "0123456789"
        notes = ["a", "b", "c", "d", "e", "f", "g", "r"]
        self.keywords = notes + ["%s%d" % (n, i) for n in notes for i in range(10)]
        stmts = ["t", "o", "v"]
        self.keywords2 = (
            stmts + ["t%d" % i for i in range(256)] + ["o%d" % i for i in range(16)] + ["v%d" % i for i in range(101)]
        )

    def StyleText(self, evt):
        """Handle the EVT_STC_STYLENEEDED event."""
        stc = evt.GetEventObject()
        last_styled_pos = stc.GetEndStyled()
        line = stc.LineFromPosition(last_styled_pos)
        start_pos = stc.PositionFromLine(line)
        end_pos = evt.GetPosition()
        userXYZ = voiceToken = False
        while start_pos < end_pos:
            stc.StartStyling(start_pos)
            curchar = chr(stc.GetCharAt(start_pos))
            if curchar in "xyz":
                userXYZ = True
            elif userXYZ and curchar in " \t\n":
                userXYZ = False
            if curchar == "#":
                voiceToken = True
            elif voiceToken and curchar in " \t\n":
                voiceToken = False

            if userXYZ:
                style = self.STC_MML_VARIABLE
                stc.SetStyling(1, style)
                start_pos += 1
            elif voiceToken:
                style = self.STC_MML_VOICE_TOKEN
                stc.SetStyling(1, style)
                start_pos += 1
            elif curchar in self.alpha:
                start = stc.WordStartPosition(start_pos, True)
                end = stc.WordEndPosition(start, True)
                word = stc.GetTextRange(start, end)
                if word in self.keywords:
                    style = self.STC_MML_KEYWORD
                    stc.SetStyling(len(word), style)
                elif word in self.keywords2:
                    style = self.STC_MML_KEYWORD2
                    stc.SetStyling(len(word), style)
                else:
                    style = self.STC_MML_DEFAULT
                    stc.SetStyling(len(word), style)
                start_pos += len(word)
            elif curchar == ";":
                eol = stc.GetLineEndPosition(stc.LineFromPosition(start_pos))
                style = self.STC_MML_COMMENT
                stc.SetStyling(eol - start_pos, style)
                start_pos = eol
            else:
                style = self.STC_MML_DEFAULT
                stc.SetStyling(1, style)
                start_pos += 1


class MMLEditor(stc.StyledTextCtrl):
    def __init__(self, parent, id=-1, obj=None):
        stc.StyledTextCtrl.__init__(self, parent, id)

        self.obj = obj

        if sys.platform == "darwin":
            accel_ctrl = wx.ACCEL_CMD
            self.faces = {"mono": "Monaco", "size": 12}
        else:
            accel_ctrl = wx.ACCEL_CTRL
            self.faces = {"mono": "Monospace", "size": 10}

        atable = wx.AcceleratorTable(
            [
                (accel_ctrl, wx.WXK_RETURN, 10000),
                (accel_ctrl, ord("z"), wx.ID_UNDO),
                (accel_ctrl | wx.ACCEL_SHIFT, ord("z"), wx.ID_REDO),
            ]
        )
        self.SetAcceleratorTable(atable)

        self.Bind(wx.EVT_MENU, self.onExecute, id=10000)
        self.Bind(wx.EVT_MENU, self.undo, id=wx.ID_UNDO)
        self.Bind(wx.EVT_MENU, self.redo, id=wx.ID_REDO)
        self.Bind(stc.EVT_STC_UPDATEUI, self.OnUpdateUI)

        self.lexer = MMLLexer()

        self.currentfile = ""
        self.modified = False

        self.setup()
        self.setCmdKeys()
        self.setStyle()

        if os.path.isfile(self.obj.music):
            with open(self.obj.music, "r") as f:
                music = f.read()
        else:
            music = self.obj.music

        self.SetText(music)

    def undo(self, evt):
        self.Undo()

    def redo(self, evt):
        self.Redo()

    def setup(self):
        self.SetIndent(2)
        self.SetBackSpaceUnIndents(True)
        self.SetTabIndents(True)
        self.SetTabWidth(2)
        self.SetUseTabs(False)
        self.SetMargins(2, 2)
        self.SetMarginWidth(1, 1)

    def setCmdKeys(self):
        self.CmdKeyAssign(ord("="), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMIN)
        self.CmdKeyAssign(ord("-"), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMOUT)

    def setStyle(self):
        self.SetLexer(wx.stc.STC_LEX_CONTAINER)
        self.SetStyleBits(5)
        self.Bind(wx.stc.EVT_STC_STYLENEEDED, self.OnStyling)

        self.SetCaretForeground("#000000")
        self.SetCaretWidth(2)
        # Global default styles for all languages
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, "face:%(mono)s,size:%(size)d" % self.faces)
        self.StyleClearAll()

        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, "face:%(mono)s,size:%(size)d" % self.faces)
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "face:%(mono)s" % self.faces)
        self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT, "fore:#FFFFFF,back:#0000FF,bold")
        self.StyleSetSpec(stc.STC_STYLE_BRACEBAD, "fore:#000000,back:#FF0000,bold")

        # MML specific styles
        self.StyleSetSpec(self.lexer.STC_MML_DEFAULT, "fore:#000000,face:%(mono)s,size:%(size)d" % self.faces)
        self.StyleSetSpec(self.lexer.STC_MML_KEYWORD, "fore:#3300DD,face:%(mono)s,size:%(size)d,bold" % self.faces)
        self.StyleSetSpec(self.lexer.STC_MML_KEYWORD2, "fore:#0033FF,face:%(mono)s,size:%(size)d,bold" % self.faces)
        self.StyleSetSpec(self.lexer.STC_MML_VARIABLE, "fore:#006600,face:%(mono)s,size:%(size)d,bold" % self.faces)
        self.StyleSetSpec(self.lexer.STC_MML_VOICE_TOKEN, "fore:#555500,face:%(mono)s,size:%(size)d,bold" % self.faces)
        self.StyleSetSpec(self.lexer.STC_MML_COMMENT, "fore:#444444,face:%(mono)s,size:%(size)d,italic" % self.faces)

        self.SetSelBackground(1, "#CCCCDD")

    def OnStyling(self, evt):
        self.lexer.StyleText(evt)

    def loadfile(self, filename):
        self.LoadFile(filename)
        self.currentfile = filename
        self.GetParent().SetTitle(self.currentfile)

    def savefile(self, filename):
        self.currentfile = filename
        self.GetParent().SetTitle(self.currentfile)
        self.SaveFile(filename)
        self.OnUpdateUI(None)

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
        if charBefore and chr(charBefore) in "[]{}()":
            braceAtCaret = caretPos - 1

        # check after
        if braceAtCaret < 0:
            charAfter = self.GetCharAt(caretPos)
            styleAfter = self.GetStyleAt(caretPos)

            if charAfter and chr(charAfter) in "[]{}()":
                braceAtCaret = caretPos
        if braceAtCaret >= 0:
            braceOpposite = self.BraceMatch(braceAtCaret)

        if braceAtCaret != -1 and braceOpposite == -1:
            self.BraceBadLight(braceAtCaret)
        else:
            self.BraceHighlight(braceAtCaret, braceOpposite)
        # Check if horizontal scrollbar is needed
        self.checkScrollbar()

    def checkScrollbar(self):
        lineslength = [self.LineLength(i) + 1 for i in range(self.GetLineCount())]
        maxlength = max(lineslength)
        width = self.GetCharWidth() + (self.GetZoom() * 0.5)
        if (self.GetSize()[0]) < (maxlength * width):
            self.SetUseHorizontalScrollBar(True)
        else:
            self.SetUseHorizontalScrollBar(False)

    def onExecute(self, evt):
        pos = self.GetCurrentPos()
        self.obj.music = self.GetText()
        self.SetCurrentPos(pos)
        self.SetSelection(pos, pos)


class MMLEditorFrame(wx.Frame):
    def __init__(self, parent=None, obj=None):
        wx.Frame.__init__(self, parent, size=(650, 450))
        self.obj = obj
        self.obj._editor = self
        self.editor = MMLEditor(self, -1, self.obj)
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(wx.ID_OPEN, "Open\tCtrl+O")
        self.Bind(wx.EVT_MENU, self.open, id=wx.ID_OPEN)
        self.fileMenu.Append(wx.ID_CLOSE, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.close, id=wx.ID_CLOSE)
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(wx.ID_SAVE, "Save\tCtrl+S")
        self.Bind(wx.EVT_MENU, self.save, id=wx.ID_SAVE)
        self.fileMenu.Append(wx.ID_SAVEAS, "Save As...\tShift+Ctrl+S")
        self.Bind(wx.EVT_MENU, self.saveas, id=wx.ID_SAVEAS)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)

    def open(self, evt):
        dlg = wx.FileDialog(
            self, message="Choose a file", defaultDir=os.path.expanduser("~"), defaultFile="", style=wx.FD_OPEN
        )
        if dlg.ShowModal() == wx.ID_OK:
            path = ensureNFD(dlg.GetPath())
            self.editor.loadfile(path)
        dlg.Destroy()

    def close(self, evt):
        self.obj._editor = None
        self.Destroy()

    def save(self, evt):
        path = self.editor.currentfile
        if not path:
            self.saveas(None)
        else:
            self.editor.savefile(path)

    def saveas(self, evt):
        deffile = os.path.split(self.editor.currentfile)[1]
        dlg = wx.FileDialog(
            self, message="Save file as ...", defaultDir=os.path.expanduser("~"), defaultFile=deffile, style=wx.FD_SAVE
        )
        dlg.SetFilterIndex(0)
        if dlg.ShowModal() == wx.ID_OK:
            path = ensureNFD(dlg.GetPath())
            self.editor.savefile(path)
        dlg.Destroy()

    def update(self, text):
        self.editor.SetText(text)


class Keyboard(wx.Panel):
    def __init__(
        self,
        parent,
        id=wx.ID_ANY,
        pos=wx.DefaultPosition,
        size=wx.DefaultSize,
        poly=64,
        outFunction=None,
        style=wx.TAB_TRAVERSAL,
    ):
        wx.Panel.__init__(self, parent, id, pos, size, style)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.SetBackgroundColour("#FFFFFF")
        self.parent = parent
        self.outFunction = outFunction

        self.poly = poly
        self.gap = 0
        self.offset = 12
        self.w1 = 15
        self.w2 = int(self.w1 / 2) + 1
        self.hold = 1
        self.keyPressed = None

        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        self.Bind(wx.EVT_KEY_UP, self.OnKeyUp)

        self.white = (0, 2, 4, 5, 7, 9, 11)
        self.black = (1, 3, 6, 8, 10)
        self.whiteSelected = []
        self.blackSelected = []
        self.whiteVelocities = {}
        self.blackVelocities = {}
        self.whiteKeys = []
        self.blackKeys = []

        self.offRec = wx.Rect(900 - 55, 0, 28, 150)
        self.holdRec = wx.Rect(900 - 27, 0, 27, 150)

        self.keydown = []
        self.keymap = {
            90: 36,
            83: 37,
            88: 38,
            68: 39,
            67: 40,
            86: 41,
            71: 42,
            66: 43,
            72: 44,
            78: 45,
            74: 46,
            77: 47,
            44: 48,
            76: 49,
            46: 50,
            59: 51,
            47: 52,
            81: 60,
            50: 61,
            87: 62,
            51: 63,
            69: 64,
            82: 65,
            53: 66,
            84: 67,
            54: 68,
            89: 69,
            55: 70,
            85: 71,
            73: 72,
            57: 73,
            79: 74,
            48: 75,
            80: 76,
        }

        wx.CallAfter(self._setRects)

    def getCurrentNotes(self):
        "Returns a list of the current notes."
        notes = []
        for key in self.whiteSelected:
            notes.append((self.white[key % 7] + int(key / 7) * 12 + self.offset, 127 - self.whiteVelocities[key]))
        for key in self.blackSelected:
            notes.append((self.black[key % 5] + int(key / 5) * 12 + self.offset, 127 - self.blackVelocities[key]))
        notes.sort()
        return notes

    def reset(self):
        "Resets the keyboard state."
        for key in self.blackSelected:
            pit = self.black[key % 5] + int(key / 5) * 12 + self.offset
            note = (pit, 0)
            if self.outFunction:
                self.outFunction(note)
        for key in self.whiteSelected:
            pit = self.white[key % 7] + int(key / 7) * 12 + self.offset
            note = (pit, 0)
            if self.outFunction:
                self.outFunction(note)
        self.whiteSelected = []
        self.blackSelected = []
        self.whiteVelocities = {}
        self.blackVelocities = {}
        wx.CallAfter(self.Refresh)

    def setPoly(self, poly):
        "Sets the maximum number of notes that can be held at the same time."
        self.poly = poly

    def _setRects(self):
        w, h = self.GetSize()
        self.offRec = wx.Rect(w - 55, 0, 28, h)
        self.holdRec = wx.Rect(w - 27, 0, 27, h)
        num = int(w / self.w1)
        self.gap = w - num * self.w1
        self.whiteKeys = [wx.Rect(i * self.w1, 0, self.w1 - 1, h - 1) for i in range(num)]
        self.blackKeys = []
        height2 = int(h * 4 / 7)
        for i in range(int(num / 7) + 1):
            space2 = self.w1 * 7 * i
            off = int(self.w1 / 2) + space2 + 3
            self.blackKeys.append(wx.Rect(off, 0, self.w2, height2))
            off += self.w1
            self.blackKeys.append(wx.Rect(off, 0, self.w2, height2))
            off += self.w1 * 2
            self.blackKeys.append(wx.Rect(off, 0, self.w2, height2))
            off += self.w1
            self.blackKeys.append(wx.Rect(off, 0, self.w2, height2))
            off += self.w1
            self.blackKeys.append(wx.Rect(off, 0, self.w2, height2))
        wx.CallAfter(self.Refresh)

    def OnSize(self, evt):
        self._setRects()
        wx.CallAfter(self.Refresh)
        evt.Skip()

    def OnKeyDown(self, evt):
        if evt.HasAnyModifiers():
            evt.Skip()
            return

        if evt.GetKeyCode() in self.keymap and evt.GetKeyCode() not in self.keydown:
            self.keydown.append(evt.GetKeyCode())
            pit = self.keymap[evt.GetKeyCode()]
            deg = pit % 12

            total = len(self.blackSelected) + len(self.whiteSelected)
            note = None
            if self.hold:
                if deg in self.black:
                    which = self.black.index(deg) + int((pit - self.offset) / 12) * 5
                    if which in self.blackSelected:
                        self.blackSelected.remove(which)
                        del self.blackVelocities[which]
                        total -= 1
                        note = (pit, 0)
                    else:
                        if total < self.poly:
                            self.blackSelected.append(which)
                            self.blackVelocities[which] = 100
                            note = (pit, 100)

                elif deg in self.white:
                    which = self.white.index(deg) + int((pit - self.offset) / 12) * 7
                    if which in self.whiteSelected:
                        self.whiteSelected.remove(which)
                        del self.whiteVelocities[which]
                        total -= 1
                        note = (pit, 0)
                    else:
                        if total < self.poly:
                            self.whiteSelected.append(which)
                            self.whiteVelocities[which] = 100
                            note = (pit, 100)
            else:
                if deg in self.black:
                    which = self.black.index(deg) + int((pit - self.offset) / 12) * 5
                    if which not in self.blackSelected and total < self.poly:
                        self.blackSelected.append(which)
                        self.blackVelocities[which] = 100
                        note = (pit, 100)
                elif deg in self.white:
                    which = self.white.index(deg) + int((pit - self.offset) / 12) * 7
                    if which not in self.whiteSelected and total < self.poly:
                        self.whiteSelected.append(which)
                        self.whiteVelocities[which] = 100
                        note = (pit, 100)

            if note and self.outFunction and total < self.poly:
                self.outFunction(note)

            wx.CallAfter(self.Refresh)
        evt.Skip()

    def OnKeyUp(self, evt):
        if evt.HasAnyModifiers():
            evt.Skip()
            return

        if evt.GetKeyCode() in self.keydown:
            del self.keydown[self.keydown.index(evt.GetKeyCode())]

        if not self.hold and evt.GetKeyCode() in self.keymap:
            pit = self.keymap[evt.GetKeyCode()]
            deg = pit % 12

            note = None
            if deg in self.black:
                which = self.black.index(deg) + int((pit - self.offset) / 12) * 5
                if which in self.blackSelected:
                    self.blackSelected.remove(which)
                    del self.blackVelocities[which]
                    note = (pit, 0)
            elif deg in self.white:
                which = self.white.index(deg) + int((pit - self.offset) / 12) * 7
                if which in self.whiteSelected:
                    self.whiteSelected.remove(which)
                    del self.whiteVelocities[which]
                    note = (pit, 0)

            if note and self.outFunction:
                self.outFunction(note)

            wx.CallAfter(self.Refresh)

        evt.Skip()

    def MouseUp(self, evt):
        if not self.hold and self.keyPressed is not None:
            key = self.keyPressed[0]
            pit = self.keyPressed[1]
            if key in self.blackSelected:
                self.blackSelected.remove(key)
                del self.blackVelocities[key]
            if key in self.whiteSelected:
                self.whiteSelected.remove(key)
                del self.whiteVelocities[key]
            note = (pit, 0)
            if self.outFunction:
                self.outFunction(note)
            self.keyPressed = None
            wx.CallAfter(self.Refresh)
        evt.Skip()

    def MouseDown(self, evt):
        w, h = self.GetSize()
        pos = evt.GetPosition()
        if self.holdRec.Contains(pos):
            if self.hold:
                self.hold = 0
                self.reset()
            else:
                self.hold = 1
            wx.CallAfter(self.Refresh)
            return
        if self.offUpRec.Contains(pos):
            self.offset += 12
            if self.offset > 60:
                self.offset = 60
            wx.CallAfter(self.Refresh)
            return
        if self.offDownRec.Contains(pos):
            self.offset -= 12
            if self.offset < 0:
                self.offset = 0
            wx.CallAfter(self.Refresh)
            return

        total = len(self.blackSelected) + len(self.whiteSelected)
        scanWhite = True
        note = None
        if self.hold:
            for i, rec in enumerate(self.blackKeys):
                if rec.Contains(pos):
                    pit = self.black[i % 5] + int(i / 5) * 12 + self.offset
                    if i in self.blackSelected:
                        self.blackSelected.remove(i)
                        del self.blackVelocities[i]
                        total -= 1
                        vel = 0
                    else:
                        hb = int(h * 4 / 7)
                        vel = int((hb - pos[1]) * 127 / hb)
                        if total < self.poly:
                            self.blackSelected.append(i)
                            self.blackVelocities[i] = int(127 - vel)
                    note = (pit, vel)
                    scanWhite = False
                    break
            if scanWhite:
                for i, rec in enumerate(self.whiteKeys):
                    if rec.Contains(pos):
                        pit = self.white[i % 7] + int(i / 7) * 12 + self.offset
                        if i in self.whiteSelected:
                            self.whiteSelected.remove(i)
                            del self.whiteVelocities[i]
                            total -= 1
                            vel = 0
                        else:
                            vel = int((h - pos[1]) * 127 / h)
                            if total < self.poly:
                                self.whiteSelected.append(i)
                                self.whiteVelocities[i] = int(127 - vel)
                        note = (pit, vel)
                        break
            if note and self.outFunction and total < self.poly:
                self.outFunction(note)
        else:
            self.keyPressed = None
            for i, rec in enumerate(self.blackKeys):
                if rec.Contains(pos):
                    pit = self.black[i % 5] + int(i / 5) * 12 + self.offset
                    if i not in self.blackSelected:
                        hb = int(h * 4 / 7)
                        vel = int((hb - pos[1]) * 127 / hb)
                        if total < self.poly:
                            self.blackSelected.append(i)
                            self.blackVelocities[i] = int(127 - vel)
                    note = (pit, vel)
                    self.keyPressed = (i, pit)
                    scanWhite = False
                    break
            if scanWhite:
                for i, rec in enumerate(self.whiteKeys):
                    if rec.Contains(pos):
                        pit = self.white[i % 7] + int(i / 7) * 12 + self.offset
                        if i not in self.whiteSelected:
                            vel = int((h - pos[1]) * 127 / h)
                            if total < self.poly:
                                self.whiteSelected.append(i)
                                self.whiteVelocities[i] = int(127 - vel)
                        note = (pit, vel)
                        self.keyPressed = (i, pit)
                        break
            if note and self.outFunction and total < self.poly:
                self.outFunction(note)
        wx.CallAfter(self.Refresh)
        evt.Skip()

    def OnPaint(self, evt):
        w, h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)
        dc.SetBrush(wx.Brush("#000000", wx.SOLID))
        dc.Clear()
        dc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
        dc.DrawRectangle(0, 0, w, h)

        if sys.platform == "darwin":
            dc.SetFont(wx.Font(12, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD))
        else:
            dc.SetFont(wx.Font(8, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD))

        for i, rec in enumerate(self.whiteKeys):
            if i in self.whiteSelected:
                amp = int(self.whiteVelocities[i] * 1.5)
                dc.GradientFillLinear(rec, (250, 250, 250), (amp, amp, amp), wx.SOUTH)
                dc.SetBrush(wx.Brush("#CCCCCC", wx.SOLID))
                dc.SetPen(wx.Pen("#CCCCCC", width=1, style=wx.SOLID))
            else:
                dc.SetBrush(wx.Brush("#FFFFFF", wx.SOLID))
                dc.SetPen(wx.Pen("#CCCCCC", width=1, style=wx.SOLID))
                dc.DrawRectangle(rec)
            if i == (35 - (7 * int(self.offset / 12))):
                if i in self.whiteSelected:
                    dc.SetTextForeground("#FFFFFF")
                else:
                    dc.SetTextForeground("#000000")
                dc.DrawText("C", rec[0] + 3, rec[3] - 15)

        dc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
        for i, rec in enumerate(self.blackKeys):
            if i in self.blackSelected:
                amp = int(self.blackVelocities[i] * 1.5)
                dc.GradientFillLinear(rec, (250, 250, 250), (amp, amp, amp), wx.SOUTH)
                dc.DrawLine(rec[0], 0, rec[0], rec[3])
                dc.DrawLine(rec[0] + rec[2], 0, rec[0] + rec[2], rec[3])
                dc.DrawLine(rec[0], rec[3], rec[0] + rec[2], rec[3])
                dc.SetBrush(wx.Brush("#DDDDDD", wx.SOLID))
            else:
                dc.SetBrush(wx.Brush("#000000", wx.SOLID))
                dc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
                dc.DrawRectangle(rec)

        dc.SetBrush(wx.Brush("#FFFFFF", wx.SOLID))
        dc.SetPen(wx.Pen("#AAAAAA", width=1, style=wx.SOLID))
        dc.DrawRectangle(self.offRec)
        dc.DrawRectangle(self.holdRec)

        dc.SetTextForeground("#000000")
        dc.DrawText("oct", self.offRec[0] + 3, 15)
        x1, y1 = self.offRec[0], self.offRec[1]
        dc.SetBrush(wx.Brush("#000000", wx.SOLID))
        if sys.platform == "darwin":
            dc.DrawPolygon([wx.Point(x1 + 3, 36), wx.Point(x1 + 10, 29), wx.Point(x1 + 17, 36)])
            self.offUpRec = wx.Rect(x1, 28, x1 + 20, 10)
            dc.DrawPolygon([wx.Point(x1 + 3, 55), wx.Point(x1 + 10, 62), wx.Point(x1 + 17, 55)])
            self.offDownRec = wx.Rect(x1, 54, x1 + 20, 10)
        else:
            dc.DrawPolygon([wx.Point(x1 + 5, 38), wx.Point(x1 + 12, 31), wx.Point(x1 + 19, 38)])
            self.offUpRec = wx.Rect(x1, 30, x1 + 20, 10)
            dc.DrawPolygon([wx.Point(x1 + 5, 57), wx.Point(x1 + 12, 64), wx.Point(x1 + 19, 57)])
            self.offDownRec = wx.Rect(x1, 56, x1 + 20, 10)

        dc.DrawText("%d" % int(self.offset / 12), x1 + 9, 41)

        if self.hold:
            dc.SetTextForeground("#0000CC")
        else:
            dc.SetTextForeground("#000000")
        for i, c in enumerate("HOLD"):
            dc.DrawText(c, self.holdRec[0] + 8, int(self.holdRec[3] / 6) * i + 15)
        evt.Skip()


class NoteinKeyboardFrame(wx.Frame):
    def __init__(self, parent=None, obj=None):
        wx.Frame.__init__(self, parent, size=(900, 150))
        self.obj = obj
        self.keyboard = Keyboard(self, -1, outFunction=self.obj._newNote)
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(wx.ID_CLOSE, "Close\tCtrl+W", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.close, id=wx.ID_CLOSE)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)

    def close(self, evt):
        self.Destroy()


class ServerGUI(wx.Frame):
    def __init__(
        self,
        parent=None,
        nchnls=2,
        startf=None,
        stopf=None,
        recstartf=None,
        recstopf=None,
        ampf=None,
        started=0,
        locals=None,
        shutdown=None,
        meter=True,
        timer=True,
        amp=1.0,
        exit=True,
        getIsBooted=None,
        getIsStarted=None,
    ):
        wx.Frame.__init__(self, parent, style=wx.DEFAULT_FRAME_STYLE ^ wx.RESIZE_BORDER)

        self.menubar = wx.MenuBar()
        self.menu = wx.Menu()
        self.menu.Append(22999, "Start/Stop\tCtrl+R", kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.start, id=22999)
        quit_item = self.menu.Append(wx.ID_EXIT, "Quit\tCtrl+Q")
        self.Bind(wx.EVT_MENU, self.on_quit, id=wx.ID_EXIT)
        self.menubar.Append(self.menu, "&File")
        self.SetMenuBar(self.menubar)

        self.shutdown = shutdown
        self.locals = locals
        self.nchnls = nchnls
        self.startf = startf
        self.stopf = stopf
        self.recstartf = recstartf
        self.recstopf = recstopf
        self.ampf = ampf
        self.exit = exit
        self.getIsBooted = getIsBooted
        self.getIsStarted = getIsStarted
        self._started = False
        self._recstarted = False
        self._history = []
        self._histo_count = 0

        if sys.platform == "darwin":
            panel = wx.Panel(self, style=wx.TAB_TRAVERSAL)
            panel.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
        else:
            panel = BasePanel(self)

        box = wx.BoxSizer(wx.VERTICAL)

        buttonBox = wx.BoxSizer(wx.HORIZONTAL)
        self.startButton = wx.Button(panel, -1, "Start")
        self.startButton.Bind(wx.EVT_BUTTON, self.start)
        buttonBox.Add(self.startButton, 0, wx.LEFT | wx.RIGHT, 5)

        self.recButton = wx.Button(panel, -1, "Rec Start")
        self.recButton.Bind(wx.EVT_BUTTON, self.record)
        buttonBox.Add(self.recButton, 0, wx.RIGHT, 5)

        self.quitButton = wx.Button(panel, -1, "Quit")
        self.quitButton.Bind(wx.EVT_BUTTON, self.on_quit)
        buttonBox.Add(self.quitButton, 0, wx.RIGHT, 5)

        box.Add(buttonBox, 0, wx.TOP, 10)
        box.AddSpacer(10)

        panel.SetForegroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_MENUTEXT))

        self.amplitudeText = wx.StaticText(panel, -1, "Amplitude (dB)")
        self.amplitudeText.SetForegroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_MENUTEXT))
        box.Add(self.amplitudeText, 0, wx.LEFT, 5)
        self.ampScale = ControlSlider(panel, -60, 18, 20.0 * math.log10(amp), size=(202, 16), outFunction=self.setAmp)
        box.Add(self.ampScale, 0, wx.LEFT | wx.RIGHT | wx.EXPAND, 5)

        if meter:
            box.AddSpacer(10)
            self.meter = VuMeter(panel, size=(200, 5 * self.nchnls + 1), numSliders=self.nchnls)
            box.Add(self.meter, 0, wx.LEFT | wx.RIGHT | wx.EXPAND, 5)
            box.AddSpacer(5)

        if timer:
            box.AddSpacer(10)
            self.timerText = wx.StaticText(panel, -1, "Elapsed time (hh:mm:ss:ms)")
            box.Add(self.timerText, 0, wx.LEFT, 5)
            box.AddSpacer(3)
            self.timeText = wx.StaticText(panel, -1, "00 : 00 : 00 : 000")
            box.Add(self.timeText, 0, wx.LEFT, 5)

        if self.locals is not None:
            box.AddSpacer(10)
            self.interpreterText = wx.StaticText(panel, -1, "Interpreter")
            box.Add(self.interpreterText, 0, wx.LEFT, 5)
            tw, th = self.GetTextExtent("|")
            self.text = wx.TextCtrl(panel, -1, "", size=(202, th + 8), style=wx.TE_PROCESS_ENTER)
            self.text.Bind(wx.EVT_TEXT_ENTER, self.getText)
            self.text.Bind(wx.EVT_KEY_DOWN, self.onChar)
            box.Add(self.text, 0, wx.LEFT | wx.RIGHT | wx.EXPAND, 5)

        box.AddSpacer(10)
        panel.SetSizerAndFit(box)
        self.SetClientSize(panel.GetSize())

        self.Bind(wx.EVT_SYS_COLOUR_CHANGED, self.OnColourChanged)
        self.Bind(wx.EVT_CLOSE, self.on_quit)

        if started == 1:
            self.start(None, True)

    def OnColourChanged(self, evt):
        colour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_MENUTEXT)
        self.amplitudeText.SetForegroundColour(colour)
        if hasattr(self, "timerText"):
            self.timerText.SetForegroundColour(colour)
        if hasattr(self, "timeText"):
           self.timeText.SetForegroundColour(colour)
        if hasattr(self, "interpreterText"):
           self.interpreterText.SetForegroundColour(colour)

    def setTime(self, *args):
        wx.CallAfter(self.timeText.SetLabel, "%02d : %02d : %02d : %03d" % (args[0], args[1], args[2], args[3]))

    def start(self, evt=None, justSet=False):
        if self._started == False:
            self._started = True
            wx.CallAfter(self.startButton.SetLabel, "Stop")
            if self.exit:
                wx.CallAfter(self.quitButton.Disable)
            if not justSet:
                self.startf()
        else:
            self._started = False
            wx.CallAfter(self.startButton.SetLabel, "Start")
            if self.exit:
                wx.CallAfter(self.quitButton.Enable)
            # TODO: Need a common method for every OSes.
            # wx.CallLater(100, self.stopf)
            # wx.CallAfter(self.stopf)
            if self.getIsStarted():
                self.stopf()

    def record(self, evt):
        if self._recstarted == False:
            self.recstartf()
            self._recstarted = True
            wx.CallAfter(self.recButton.SetLabel, "Rec Stop")
        else:
            self.recstopf()
            self._recstarted = False
            wx.CallAfter(self.recButton.SetLabel, "Rec Start")

    def quit_from_code(self):
        wx.CallAfter(self.on_quit, None)

    def on_quit(self, evt):
        if self.exit and self.getIsBooted():
            self.shutdown()
            time.sleep(0.25)
        self.Destroy()
        if self.exit:
            sys.exit()

    def getPrev(self):
        self.text.Clear()
        self._histo_count -= 1
        if self._histo_count < 0:
            self._histo_count = 0
        self.text.SetValue(self._history[self._histo_count])
        wx.CallAfter(self.text.SetInsertionPointEnd)

    def getNext(self):
        self.text.Clear()
        self._histo_count += 1
        if self._histo_count >= len(self._history):
            self._histo_count = len(self._history)
        else:
            self.text.SetValue(self._history[self._histo_count])
            wx.CallAfter(self.text.SetInsertionPointEnd)

    def getText(self, evt):
        source = self.text.GetValue()
        self.text.Clear()
        self._history.append(source)
        self._histo_count = len(self._history)
        exec(source, self.locals)

    def onChar(self, evt):
        key = evt.GetKeyCode()
        if key == 315:
            self.getPrev()
            evt.StopPropagation()
        elif key == 317:
            self.getNext()
            evt.StopPropagation()
        else:
            evt.Skip()

    def setAmp(self, value):
        self.ampf(math.pow(10.0, float(value) * 0.05))

    def setRms(self, *args):
        self.meter.setRms(*args)

    def setStartButtonState(self, state):
        if state:
            self._started = True
            wx.CallAfter(self.startButton.SetLabel, "Stop")
            if self.exit:
                wx.CallAfter(self.quitButton.Disable)
        else:
            self._started = False
            wx.CallAfter(self.startButton.SetLabel, "Start")
            if self.exit:
                wx.CallAfter(self.quitButton.Enable)


def ensureNFD(unistr):
    if sys.platform == "win32" or sys.platform.startswith("linux"):
        encodings = [sys.getdefaultencoding(), sys.getfilesystemencoding(), "cp1252", "iso-8859-1", "utf-16"]
        format = "NFC"
    else:
        encodings = [sys.getdefaultencoding(), sys.getfilesystemencoding(), "macroman", "iso-8859-1", "utf-16"]
        format = "NFC"
    decstr = unistr
    if type(decstr) != str:
        for encoding in encodings:
            try:
                decstr = decstr.decode(encoding)
                break
            except UnicodeDecodeError:
                continue
            except:
                decstr = "UnableToDecodeString"
                print("Unicode encoding not in a recognized format...")
                break
    if decstr == "UnableToDecodeString":
        return unistr
    else:
        return unicodedata.normalize(format, decstr)
