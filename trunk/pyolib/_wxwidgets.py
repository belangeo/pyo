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
import wx, os, sys, math, time, random
from types import ListType, FloatType, IntType

try:
    from PIL import Image, ImageDraw, ImageTk
except:
    pass

BACKGROUND_COLOUR = "#EBEBEB"

def interpFloat(t, v1, v2):
    "interpolator for a single value; interprets t in [0-1] between v1 and v2"
    return (v2-v1)*t + v1

def tFromValue(value, v1, v2):
    "returns a t (in range 0-1) given a value in the range v1 to v2"
    return float(value-v1)/(v2-v1)

def clamp(v, minv, maxv):
    "clamps a value within a range"
    if v<minv: v=minv
    if v> maxv: v=maxv
    return v

def toLog(t, v1, v2):
    return math.log10(t/v1) / math.log10(v2/v1)

def toExp(t, v1, v2):
    return math.pow(10, t * (math.log10(v2) - math.log10(v1)) + math.log10(v1))

POWOFTWO = {2:1, 4:2, 8:3, 16:4, 32:5, 64:6, 128:7, 256:8, 512:9, 1024:10, 2048:11, 4096:12, 8192:13, 16384:14, 32768:15, 65536:16}
def powOfTwo(x):
    return 2**x

def powOfTwoToInt(x):
    return POWOFTWO[x]

def GetRoundBitmap( w, h, r ):
    maskColor = wx.Color(0,0,0)
    shownColor = wx.Color(5,5,5)
    b = wx.EmptyBitmap(w,h)
    dc = wx.MemoryDC(b)
    dc.SetBrush(wx.Brush(maskColor))
    dc.DrawRectangle(0,0,w,h)
    dc.SetBrush(wx.Brush(shownColor))
    dc.SetPen(wx.Pen(shownColor))
    dc.DrawRoundedRectangle(0,0,w,h,r)
    dc.SelectObject(wx.NullBitmap)
    b.SetMaskColour(maskColor)
    return b

def GetRoundShape( w, h, r ):
    return wx.RegionFromBitmap( GetRoundBitmap(w,h,r) )

class ControlSlider(wx.Panel):
    def __init__(self, parent, minvalue, maxvalue, init=None, pos=(0,0), size=(200,16), log=False,
                 outFunction=None, integer=False, powoftwo=False, backColour=None, orient=wx.HORIZONTAL):
        if size == (200,16) and orient == wx.VERTICAL:
            size = (40, 200)
        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY, pos=pos, size=size,
                            style=wx.NO_BORDER | wx.WANTS_CHARS | wx.EXPAND)
        self.parent = parent
        if backColour:
            self.backgroundColour = backColour
        else:
            self.backgroundColour = BACKGROUND_COLOUR
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
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
        self.SetRange(minvalue, maxvalue)
        self.borderWidth = 1
        self.selected = False
        self._enable = True
        self.propagate = True
        self.midictl = None
        self.new = ''
        if init != None:
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
        self.Bind(wx.EVT_KEY_DOWN, self.keyDown)
        self.Bind(wx.EVT_KILL_FOCUS, self.LooseFocus)

        if sys.platform == "win32":
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC

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
        self.Refresh()

    def Disable(self):
        self._enable = False
        self.Refresh()

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
            inter = tFromValue(h-self.pos, self.knobHalfSize, self.GetSize()[1]-self.knobHalfSize)
        else:
            inter = tFromValue(self.pos, self.knobHalfSize, self.GetSize()[0]-self.knobHalfSize)
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
        self.Refresh()

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

    def keyDown(self, event):
        if self.selected:
            char = ''
            if event.GetKeyCode() in range(324, 334):
                char = str(event.GetKeyCode() - 324)
            elif event.GetKeyCode() == 390:
                char = '-'
            elif event.GetKeyCode() == 391:
                char = '.'
            elif event.GetKeyCode() == wx.WXK_BACK:
                if self.new != '':
                    self.new = self.new[0:-1]
            elif event.GetKeyCode() < 256:
                char = chr(event.GetKeyCode())
            if char in ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', '-']:
                self.new += char
            elif event.GetKeyCode() in [wx.WXK_RETURN, wx.WXK_NUMPAD_ENTER]:
                self.SetValue(eval(self.new))
                self.new = ''
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
                self.pos = clamp(evt.GetPosition()[1], self.knobHalfSize, size[1]-self.knobHalfSize)
            else:
                self.pos = clamp(evt.GetPosition()[0], self.knobHalfSize, size[0]-self.knobHalfSize)
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
                if wx.Rect(0, self.pos-self.knobHalfSize, w, self.knobSize).Contains(pos):
                    self.selected = True
            else:
                if wx.Rect(self.pos-self.knobHalfSize, 0, self.knobSize, h).Contains(pos):
                    self.selected = True
            self.Refresh()
        event.Skip()

    def MouseMotion(self, evt):
        if self._enable:
            size = self.GetSize()
            if self.HasCapture():
                if self.orient == wx.VERTICAL:
                    self.pos = clamp(evt.GetPosition()[1], self.knobHalfSize, size[1]-self.knobHalfSize)
                else:
                    self.pos = clamp(evt.GetPosition()[0], self.knobHalfSize, size[0]-self.knobHalfSize)
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
            self.pos = clamp(size[1]-self.pos, self.knobHalfSize, size[1]-self.knobHalfSize)
        else:
            self.pos = tFromValue(val, self.minvalue, self.maxvalue) * (size[0] - self.knobSize) + self.knobHalfSize
            self.pos = clamp(self.pos, self.knobHalfSize, size[0]-self.knobHalfSize)

    def setBackgroundColour(self, colour):
        self.backgroundColour = colour
        self.SetBackgroundColour(self.backgroundColour)
        self.Refresh()

    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)

        dc.SetBrush(wx.Brush(self.backgroundColour, wx.SOLID))
        dc.Clear()

        # Draw background
        dc.SetPen(wx.Pen(self.backgroundColour, width=self.borderWidth, style=wx.SOLID))
        dc.DrawRectangle(0, 0, w, h)

        # Draw inner part
        if self._enable: sliderColour =  "#99A7CC"
        else: sliderColour = "#BBBBBB"
        if self.orient == wx.VERTICAL:
            w2 = (w - self.sliderWidth) / 2
            rec = wx.Rect(w2, 0, self.sliderWidth, h)
            brush = gc.CreateLinearGradientBrush(w2, 0, w2+self.sliderWidth, 0, "#646986", sliderColour)
        else:
            h2 = self.sliderHeight / 4
            rec = wx.Rect(0, h2, w, self.sliderHeight)
            brush = gc.CreateLinearGradientBrush(0, h2, 0, h2+self.sliderHeight, "#646986", sliderColour)
        gc.SetBrush(brush)
        gc.DrawRoundedRectangle(rec[0], rec[1], rec[2], rec[3], 2)

        if self.midictl != None:
            if sys.platform in ['win32', 'linux2']:
                dc.SetFont(wx.Font(6, wx.ROMAN, wx.NORMAL, wx.NORMAL))
            else:
                dc.SetFont(wx.Font(9, wx.ROMAN, wx.NORMAL, wx.NORMAL))
            dc.SetTextForeground('#FFFFFF')
            if self.orient == wx.VERTICAL:
                dc.DrawLabel(str(self.midictl), wx.Rect(w2,2,self.sliderWidth,12), wx.ALIGN_CENTER)
                dc.DrawLabel(str(self.midictl), wx.Rect(w2,h-12,self.sliderWidth,12), wx.ALIGN_CENTER)
            else:
                dc.DrawLabel(str(self.midictl), wx.Rect(2,0,h,h), wx.ALIGN_CENTER)
                dc.DrawLabel(str(self.midictl), wx.Rect(w-h,0,h,h), wx.ALIGN_CENTER)

        # Draw knob
        if self._enable: knobColour = '#888888'
        else: knobColour = "#DDDDDD"
        if self.orient == wx.VERTICAL:
            rec = wx.Rect(0, self.pos-self.knobHalfSize, w, self.knobSize-1)
            if self.selected:
                brush = wx.Brush('#333333', wx.SOLID)
            else:
                brush = gc.CreateLinearGradientBrush(0, 0, w, 0, "#323854", knobColour)
            gc.SetBrush(brush)
            gc.DrawRoundedRectangle(rec[0], rec[1], rec[2], rec[3], 3)
        else:
            rec = wx.Rect(self.pos-self.knobHalfSize, 0, self.knobSize-1, h)
            if self.selected:
                brush = wx.Brush('#333333', wx.SOLID)
            else:
                brush = gc.CreateLinearGradientBrush(self.pos-self.knobHalfSize, 0, self.pos+self.knobHalfSize, 0, "#323854", knobColour)
            gc.SetBrush(brush)
            gc.DrawRoundedRectangle(rec[0], rec[1], rec[2], rec[3], 3)

        if sys.platform in ['win32', 'linux2']:
            dc.SetFont(wx.Font(7, wx.ROMAN, wx.NORMAL, wx.NORMAL))
        else:
            dc.SetFont(wx.Font(10, wx.ROMAN, wx.NORMAL, wx.NORMAL))

        # Draw text
        if self.selected and self.new:
            val = self.new
        else:
            if self.integer:
                val = '%d' % self.GetValue()
            elif abs(self.GetValue()) >= 1000:
                val = '%.1f' % self.GetValue()
            elif abs(self.GetValue()) >= 100:
                val = '%.2f' % self.GetValue()
            elif abs(self.GetValue()) >= 10:
                val = '%.3f' % self.GetValue()
            elif abs(self.GetValue()) < 10:
                val = '%.4f' % self.GetValue()
        if sys.platform == 'linux2':
            width = len(val) * (dc.GetCharWidth() - 3)
        else:
            width = len(val) * dc.GetCharWidth()
        dc.SetTextForeground('#FFFFFF')
        dc.DrawLabel(val, rec, wx.ALIGN_CENTER)

        # Send value
        if self.outFunction and self.propagate:
            self.outFunction(self.GetValue())
        self.propagate = True

        evt.Skip()

class MultiSlider(wx.Panel):
    def __init__(self, parent, init, key, command, slmap):
        wx.Panel.__init__(self, parent, size=(250,250))
        self.backgroundColour = BACKGROUND_COLOUR
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.SetBackgroundColour(self.backgroundColour)
        self.Bind(wx.EVT_SIZE, self.OnResize)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self._slmap = slmap
        self._values = [slmap.set(x) for x in init]
        self._nchnls = len(init)
        self._labels = init
        self._key = key
        self._command = command
        self._height = 16
        if sys.platform in ['win32', 'linux2']:
            self._font = wx.Font(7, wx.ROMAN, wx.NORMAL, wx.NORMAL)
        else:
            self._font = wx.Font(10, wx.ROMAN, wx.NORMAL, wx.NORMAL)

        self.SetSize((250, self._nchnls*16))
        self.SetMinSize((250,self._nchnls*16))

    def OnResize(self, event):
        self.Layout()
        self.Refresh()

    def OnPaint(self, event):
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)
        dc.SetBrush(wx.Brush(self.backgroundColour))
        dc.Clear()
        dc.DrawRectangle(0,0,w,h)
        dc.SetBrush(wx.Brush("#000000"))
        dc.SetFont(self._font)
        dc.SetTextForeground('#999999')
        for i in range(self._nchnls):
            x = int(self._values[i] * w)
            y = self._height * i
            dc.DrawRectangle(0, y+1, x, self._height-2)
            rec = wx.Rect(w/2-15, y, 30, self._height)
            dc.DrawLabel("%s" % self._labels[i], rec, wx.ALIGN_CENTER)

    def MouseDown(self, evt):
        w,h = self.GetSize()
        pos = evt.GetPosition()
        slide = pos[1] / self._height
        if 0 <= slide < self._nchnls:
            self._values[slide] = pos[0] / float(w)
            if self._slmap._res == 'int':
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
        w,h = self.GetSize()
        pos = evt.GetPosition()
        if evt.Dragging() and evt.LeftIsDown():
            slide = pos[1] / self._height
            if 0 <= slide < self._nchnls:
                self._values[slide] = pos[0] / float(w)
                if self._slmap._res == 'int':
                    self._labels = [int(self._slmap.get(x)) for x in self._values]
                else:
                    self._labels = [self._slmap.get(x) for x in self._values]
                self._command(self._key, self._labels)
            self.Refresh()

class VuMeter(wx.Panel):
    def __init__(self, parent, size=(200,11), numSliders=2, orient=wx.HORIZONTAL):
        if orient == wx.HORIZONTAL:
            size = (size[0], numSliders * 5 + 1)
        else:
            size = (numSliders * 5 + 1, size[1])
        wx.Panel.__init__(self, parent, -1, size=size)
        self.parent = parent
        self.orient = orient
        self.SetBackgroundColour("#000000")
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
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
        b = wx.EmptyBitmap(w,h)
        f = wx.EmptyBitmap(w,h)
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
            elif i >= (steps - (bounds*2)):
                dcb.SetBrush(wx.Brush("#444400"))
                dcf.SetBrush(wx.Brush("#CCCC00"))
            else:
                dcb.SetBrush(wx.Brush("#004400"))
                dcf.SetBrush(wx.Brush("#00CC00"))
            if self.orient == wx.HORIZONTAL:
                dcb.DrawRectangle(i*10, 0, 11, height)
                dcf.DrawRectangle(i*10, 0, 11, height)
            else:
                ii = steps - 1 - i
                dcb.DrawRectangle(0, ii*10, width, 11)
                dcf.DrawRectangle(0, ii*10, width, 11)
        if self.orient == wx.HORIZONTAL:
            dcb.DrawLine(w-1, 0, w-1, height)
            dcf.DrawLine(w-1, 0, w-1, height)
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
            self.SetMinSize((w, 5*self.numSliders+1))
            self.parent.SetSize((parentSize[0], parentSize[1]+gap))
            self.parent.SetMinSize((parentSize[0], parentSize[1]+gap))
        else:
            self.SetSize((self.numSliders * 5 + 1, h))
            self.SetMinSize((5*self.numSliders+1, h))
            self.parent.SetSize((parentSize[0]+gap, parentSize[1]))
            self.parent.SetMinSize((parentSize[0]+gap, parentSize[1]))
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
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)
        dc.SetBrush(wx.Brush("#000000"))
        dc.Clear()
        dc.DrawRectangle(0,0,w,h)
        if self.orient == wx.HORIZONTAL:
            height = 6
            for i in range(self.numSliders):
                y = i * (height - 1)
                db = math.log10(self.amplitude[i]+0.00001) * 0.2 + 1.
                width = int(db*w)
                dc.DrawBitmap(self.backBitmap, 0, y)
                if width > 0:
                    dc.SetClippingRegion(0, y, width, height)
                    dc.DrawBitmap(self.bitmap, 0, y)
                    dc.DestroyClippingRegion()
        else:
            width = 6
            for i in range(self.numSliders):
                y = i * (width - 1)
                db = math.log10(self.amplitude[i]+0.00001) * 0.2 + 1.
                height = int(db*h)
                dc.DrawBitmap(self.backBitmap, y, 0)
                if height > 0:
                    dc.SetClippingRegion(y, h-height, width, height)
                    dc.DrawBitmap(self.bitmap, y, 0)
                    dc.DestroyClippingRegion()
        event.Skip()

    def OnClose(self, evt):
        self.Destroy()

class RangeSlider(wx.Panel):
    def __init__(self, parent, minvalue, maxvalue, init=None, pos=(0,0), size=(200,15),
                 valtype='int', log=False, function=None):
        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY, pos=pos, size=size, style=wx.NO_BORDER)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.SetBackgroundColour(BACKGROUND_COLOUR)
        self.SetMinSize(self.GetSize())
        self.sliderHeight = 15
        self.borderWidth = 1
        self.action = None
        self.fillcolor = "#AAAAAA" #SLIDER_BACK_COLOUR
        self.knobcolor = "#333333" #SLIDER_KNOB_COLOUR
        self.handlecolor = wx.Colour(int(self.knobcolor[1:3])-10, int(self.knobcolor[3:5])-10, int(self.knobcolor[5:7])-10)
        self.outFunction = function
        if valtype.startswith('i'): self.myType = IntType
        else: self.myType = FloatType
        self.log = log
        self.SetRange(minvalue, maxvalue)
        self.handles = [minvalue, maxvalue]
        if init != None:
            if type(init) in [ListType, TupleType]:
                if len(init) == 1:
                    self.SetValue([init[0],init[0]])
                else:
                    self.SetValue([init[0],init[1]])
            else:
                self.SetValue([minvalue,maxvalue])
        else:
            self.SetValue([minvalue,maxvalue])
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_RIGHT_DOWN, self.MouseRightDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_RIGHT_UP, self.MouseUp)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnResize)

    def createSliderBitmap(self):
        w, h = self.GetSize()
        b = wx.EmptyBitmap(w,h)
        dc = wx.MemoryDC(b)
        dc.SetPen(wx.Pen(BACKGROUND_COLOUR, width=1))
        dc.SetBrush(wx.Brush(BACKGROUND_COLOUR))
        dc.DrawRectangle(0,0,w,h)
        dc.SetBrush(wx.Brush("#777777"))
        dc.SetPen(wx.Pen("#FFFFFF", width=1))
        h2 = self.sliderHeight / 4
        dc.DrawRoundedRectangle(0, h2, w, self.sliderHeight, 4)
        dc.SelectObject(wx.NullBitmap)
        b.SetMaskColour("#777777")
        self.sliderMask = b

    def setFillColour(self, col1, col2):
        self.fillcolor = col1
        self.knobcolor = col2
        self.handlecolor = wx.Colour(self.knobcolor[0]*0.35, self.knobcolor[1]*0.35, self.knobcolor[2]*0.35)
        self.createSliderBitmap()

    def SetRange(self, minvalue, maxvalue):
        self.minvalue = minvalue
        self.maxvalue = maxvalue

    def scale(self, pos):
        tmp = []
        for p in pos:
            inter = tFromValue(p, 1, self.GetSize()[0]-1)
            inter2 = interpFloat(inter, self.minvalue, self.maxvalue)
            tmp.append(inter2)
        return tmp

    def MouseRightDown(self, evt):
        size = self.GetSize()
        xpos = evt.GetPosition()[0]
        if xpos > (self.handlePos[0]-5) and xpos < (self.handlePos[1]+5):
            self.lastpos = xpos
            self.length = self.handlePos[1] - self.handlePos[0]
            self.action = 'drag'
            self.handles = self.scale(self.handlePos)
            self.CaptureMouse()
            self.Refresh()

    def MouseDown(self, evt):
        size = self.GetSize()
        xpos = evt.GetPosition()[0]
        self.middle = (self.handlePos[1] - self.handlePos[0]) / 2 + self.handlePos[0]
        midrec = wx.Rect(self.middle-7, 4, 15, size[1]-9)
        if midrec.Contains(evt.GetPosition()):
            self.lastpos = xpos
            self.length = self.handlePos[1] - self.handlePos[0]
            self.action = 'drag'
        elif xpos < self.middle:
            self.handlePos[0] = clamp(xpos, 1, self.handlePos[1])
            self.action = 'left'
        elif xpos > self.middle:
            self.handlePos[1] = clamp(xpos, self.handlePos[0], size[0]-1)
            self.action = 'right'
        self.handles = self.scale(self.handlePos)
        self.CaptureMouse()
        self.Refresh()

    def MouseMotion(self, evt):
        size = self.GetSize()
        if evt.Dragging() and self.HasCapture() and evt.LeftIsDown() or evt.RightIsDown():
            xpos = evt.GetPosition()[0]
            if self.action == 'drag':
                off = xpos - self.lastpos
                self.lastpos = xpos
                self.handlePos[0] = clamp(self.handlePos[0] + off, 1, size[0]-self.length)
                self.handlePos[1] = clamp(self.handlePos[1] + off, self.length, size[0]-1)
            if self.action == 'left':
                self.handlePos[0] = clamp(xpos, 1, self.handlePos[1]-20)
            elif self.action == 'right':
                self.handlePos[1] = clamp(xpos, self.handlePos[0]+20, size[0]-1)
            self.handles = self.scale(self.handlePos)
            self.Refresh()

    def MouseUp(self, evt):
        while self.HasCapture():
            self.ReleaseMouse()

    def OnResize(self, evt):
        self.createSliderBitmap()
        self.createBackgroundBitmap()
        self.clampHandlePos()
        self.Refresh()

    def clampHandlePos(self):
        size = self.GetSize()
        tmp = []
        for handle in [min(self.handles), max(self.handles)]:
            pos = tFromValue(handle, self.minvalue, self.maxvalue) * size[0]
            pos = clamp(pos, 1, size[0]-1)
            tmp.append(pos)
        self.handlePos = tmp

class HRangeSlider(RangeSlider):
    def __init__(self, parent, minvalue, maxvalue, init=None, pos=(0,0), size=(200,15),
                 valtype='int', log=False, function=None):
        RangeSlider.__init__(self, parent, minvalue, maxvalue, init, pos, size, valtype, log, function)
        self.SetMinSize((50, 15))
        self.createSliderBitmap()
        #self.createBackgroundBitmap()
        self.clampHandlePos()

    def setSliderHeight(self, height):
        self.sliderHeight = height
        self.createSliderBitmap()
        #self.createBackgroundBitmap()
        self.Refresh()

    def createBackgroundBitmap(self):
        w,h = self.GetSize()
        self.backgroundBitmap = wx.EmptyBitmap(w,h)
        dc = wx.MemoryDC(self.backgroundBitmap)

        dc.SetBrush(wx.Brush(BACKGROUND_COLOUR, wx.SOLID))
        dc.Clear()

        # Draw background
        dc.SetPen(wx.Pen(BACKGROUND_COLOUR, width=self.borderWidth, style=wx.SOLID))
        dc.DrawRectangle(0, 0, w, h)

        # Draw inner part
        h2 = self.sliderHeight / 4
        rec = wx.Rect(0, h2, w, self.sliderHeight)
        dc.GradientFillLinear(rec, "#666666", self.fillcolor, wx.BOTTOM)
        dc.DrawBitmap(self.sliderMask, 0, 0, True)
        dc.SelectObject(wx.NullBitmap)

    def SetOneValue(self, value, which):
        self.lasthandles = self.handles
        value = clamp(value, self.minvalue, self.maxvalue)
        if self.log:
            t = toLog(value, self.minvalue, self.maxvalue)
            value = interpFloat(t, self.minvalue, self.maxvalue)
        else:
            t = tFromValue(value, self.minvalue, self.maxvalue)
            value = interpFloat(t, self.minvalue, self.maxvalue)
        if self.myType == IntType:
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
            if self.myType == IntType:
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
            if self.myType == IntType:
                val = int(val)
            tmp.append(val)
        tmp = [min(tmp), max(tmp)]
        return tmp

    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)

        # Draw background
        dc.SetBrush(wx.Brush(BACKGROUND_COLOUR))
        dc.Clear()
        dc.SetPen(wx.Pen(BACKGROUND_COLOUR))
        dc.DrawRectangle(0, 0, w, h)

        #dc.DrawBitmap(self.backgroundBitmap, 0, 0)

        # Draw handles
        dc.SetPen(wx.Pen(self.handlecolor, width=1, style=wx.SOLID))
        dc.SetBrush(wx.Brush(self.handlecolor))

        rec = wx.Rect(self.handlePos[0], 3, self.handlePos[1]-self.handlePos[0], h-7)
        dc.DrawRoundedRectangleRect(rec, 4)
        dc.SetPen(wx.Pen(self.fillcolor, width=1, style=wx.SOLID))
        dc.SetBrush(wx.Brush(self.fillcolor))
        mid = (self.handlePos[1]-self.handlePos[0]) / 2 + self.handlePos[0]
        rec = wx.Rect(mid-4, 4, 8, h-9)
        dc.DrawRoundedRectangleRect(rec, 3)

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
        from controls import SigTo
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.fileMenu.Bind(wx.EVT_MENU, self._destroy)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self._obj = obj
        self._map_list = map_list
        self._sliders = []
        self._excluded = []
        self._values = {}
        self._displays = {}
        self._maps = {}
        self._sigs = {}

        panel = wx.Panel(self)
        panel.SetBackgroundColour(BACKGROUND_COLOUR)
        mainBox = wx.BoxSizer(wx.VERTICAL)
        self.box = wx.FlexGridSizer(10,2,5,5)

        for i, m in enumerate(self._map_list):
            key, init, mini, maxi, scl, res, dataOnly = m.name, m.init, m.min, m.max, m.scale, m.res, m.dataOnly
            # filters PyoObjects
            if type(init) not in [ListType, FloatType, IntType]:
                self._excluded.append(key)
            else:
                self._maps[key] = m
                # label (param name)
                if dataOnly:
                    label = wx.StaticText(panel, -1, key+" *")
                else:
                    label = wx.StaticText(panel, -1, key)
                # create and pack slider
                if type(init) != ListType:
                    if scl == 'log': scl = True
                    else: scl = False
                    if res == 'int': res = True
                    else: res = False
                    self._sliders.append(ControlSlider(panel, mini, maxi, init, log=scl, size=(300,16),
                                        outFunction=Command(self.setval, key), integer=res))
                    self.box.AddMany([(label, 0, wx.LEFT, 5), (self._sliders[-1], 1, wx.EXPAND | wx.LEFT, 5)])
                else:
                    self._sliders.append(MultiSlider(panel, init, key, self.setval, m))
                    self.box.AddMany([(label, 0, wx.LEFT, 5), (self._sliders[-1], 1, wx.EXPAND | wx.LEFT, 5)])
                # set obj attribute to PyoObject SigTo
                if not dataOnly:
                    self._values[key] = init
                    self._sigs[key] = SigTo(init, .025, init)
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

######################################################################
### View window for PyoTableObject
######################################################################
class ViewTable(wx.Frame):
    def __init__(self, parent, samples=None, tableclass=None, object=None):
        wx.Frame.__init__(self, parent, size=(500,200))
        self.SetMinSize((300, 150))
        menubar = wx.MenuBar()
        fileMenu = wx.Menu()
        closeItem = fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self._destroy, closeItem)
        menubar.Append(fileMenu, "&File")
        self.SetMenuBar(menubar)
        self.tableclass = tableclass
        self.object = object
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self.panel = wx.Panel(self)
        self.panel.SetBackgroundColour(BACKGROUND_COLOUR)
        self.box = wx.BoxSizer(wx.VERTICAL)
        self.wavePanel = ViewTablePanel(self.panel, object)
        self.box.Add(self.wavePanel, 1, wx.EXPAND|wx.ALL, 5)
        self.panel.SetSizerAndFit(self.box)
        self.update(samples)

    def update(self, samples):
        wx.CallAfter(self.wavePanel.draw, samples)

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
        if sys.platform == "win32":
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC


    def draw(self, samples):
        self.samples = samples
        self.Refresh()

    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        dc.SetBrush(wx.Brush("#FFFFFF"))
        dc.SetPen(wx.Pen('#BBBBBB', width=1, style=wx.SOLID))
        dc.Clear()
        dc.DrawRectangle(0,0,w,h)
        gc.SetPen(wx.Pen('#000000', width=1, style=wx.SOLID))
        gc.SetBrush(wx.Brush("#FFFFFF"))
        if len(self.samples) > 1:
            gc.DrawLines(self.samples)
        dc.DrawLine(0, h/2+1, w, h/2+1)

    def OnSize(self, evt):
        wx.CallAfter(self.obj.refreshView)

class SndViewTable(wx.Frame):
    def __init__(self, parent, obj=None, tableclass=None, mouse_callback=None):
        wx.Frame.__init__(self, parent, size=(500,250))
        self.SetMinSize((300, 150))
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        closeItem = self.fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self._destroy, closeItem)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self.obj = obj
        self.chnls = len(self.obj)
        self.dur = self.obj.getDur(False)
        self.panel = wx.Panel(self)
        self.panel.SetBackgroundColour(BACKGROUND_COLOUR)
        self.box = wx.BoxSizer(wx.VERTICAL)
        self.wavePanel = SndViewTablePanel(self.panel, obj, mouse_callback)
        self.box.Add(self.wavePanel, 1, wx.EXPAND|wx.ALL, 5)
        self.zoomH = HRangeSlider(self.panel, minvalue=0, maxvalue=1, init=None, pos=(0,0), size=(200,15),
                 valtype='float', log=False, function=self.setZoomH)
        self.box.Add(self.zoomH, 0, wx.EXPAND|wx.LEFT|wx.RIGHT, 5)
        self.panel.SetSizer(self.box)

    def setZoomH(self, values):
        self.wavePanel.setBegin(self.dur * values[0])
        self.wavePanel.setEnd(self.dur * values[1])
        self.update()

    def update(self):
        wx.CallAfter(self.wavePanel.setImage)

    def _destroy(self, evt):
        self.obj._setViewFrame(None)
        self.Destroy()

class SndViewTablePanel(wx.Panel):
    def __init__(self, parent, obj, mouse_callback=None):
        wx.Panel.__init__(self, parent)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
        self.Bind(wx.EVT_MOTION, self.OnMotion)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.obj = obj
        self.chnls = len(self.obj)
        self.begin = 0
        self.end = self.obj.getDur(False)
        self.mouse_callback = mouse_callback
        if sys.platform == "win32":
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC
        self.setImage()

    def setBegin(self, x):
        self.begin = x

    def setEnd(self, x):
        self.end = x

    def setImage(self):
        self.img = self.obj.getViewTable(self.GetSize(), self.begin, self.end)
        self.Refresh()

    def clipPos(self, pos):
        if pos[0] < 0.0: x = 0.0
        elif pos[0] > 1.0: x = 1.0
        else: x = pos[0]
        if pos[1] < 0.0: y = 0.0
        elif pos[1] > 1.0: y = 1.0
        else: y = pos[1]
        x = x * ((self.end - self.begin) / self.obj.getDur(False)) + (self.begin / self.obj.getDur(False))
        return (x, y)

    def OnMouseDown(self, evt):
        size = self.GetSize()
        pos = evt.GetPosition()
        if pos[1] <= 0:
            pos = (float(pos[0])/size[0], 1.0)
        else:
            pos = (float(pos[0])/size[0], 1.-(float(pos[1])/size[1]))
        pos = self.clipPos(pos)
        if self.mouse_callback != None:
            self.mouse_callback(pos)
        self.CaptureMouse()

    def OnMotion(self, evt):
        if self.HasCapture():
            size = self.GetSize()
            pos = evt.GetPosition()
            if pos[1] <= 0:
                pos = (float(pos[0])/size[0], 1.0)
            else:
                pos = (float(pos[0])/size[0], 1.-(float(pos[1])/size[1]))
            pos = self.clipPos(pos)
            if self.mouse_callback != None:
                self.mouse_callback(pos)

    def OnMouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()

    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        dc.SetBrush(wx.Brush("#FFFFFF"))
        dc.Clear()
        dc.DrawRectangle(0,0,w,h)
        off = h/self.chnls/2
        gc.SetPen(wx.Pen('#000000', width=1, style=wx.SOLID))
        gc.SetBrush(wx.Brush("#FFFFFF", style=wx.TRANSPARENT))
        dc.SetTextForeground("#444444")
        if sys.platform in "darwin":
            font, ptsize = dc.GetFont(), dc.GetFont().GetPointSize()
            font.SetPointSize(ptsize - 3)
            dc.SetFont(font)
        elif sys.platform == "win32":
            font = dc.GetFont()
            font.SetPointSize(8)
            dc.SetFont(font)
        tickstep = w / 10
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
            y = h/self.chnls*i
            if len(samples):
                gc.DrawLines(samples)
            dc.SetPen(wx.Pen('#888888', width=1, style=wx.DOT))
            dc.DrawLine(0, y+off, w, y+off)
            for j in range(10):
                dc.SetPen(wx.Pen('#888888', width=1, style=wx.DOT))
                dc.DrawLine(j*tickstep, 0, j*tickstep, h)
                dc.DrawText(timelabel % (self.begin+j*timestep), j*tickstep+2, h-y-12)
            dc.SetPen(wx.Pen('#000000', width=1))
            dc.DrawLine(0, h-y, w, h-y)

    def OnSize(self, evt):
        wx.CallAfter(self.setImage)

######################################################################
## View window for PyoMatrixObject
#####################################################################
class ViewMatrix(wx.Frame):
    def __init__(self, parent, size=None, object=None):
        wx.Frame.__init__(self, parent)
        self.object = object
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        closeItem = self.fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self._destroy, closeItem)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.SetClientSize(size)
        self.SetMinSize(self.GetSize())
        self.SetMaxSize(self.GetSize())

    def update(self, samples):
        wx.CallAfter(self.setImage, samples)

    def _destroy(self, evt):
        self.object._setViewFrame(None)
        self.Destroy()

class ViewMatrix_withPIL(ViewMatrix):
    _WITH_PIL = True
    def __init__(self, parent, samples=None, size=None, object=None):
        ViewMatrix.__init__(self, parent, size, object)
        self.size = size
        self.setImage(samples)

    def setImage(self, samples):
        im = Image.new("L", self.size, None)
        im.putdata(samples)
        image = wx.EmptyImage(self.size[0], self.size[1])
        image.SetData(im.convert("RGB").tostring())
        self.img = wx.BitmapFromImage(image)
        self.Refresh()

    def OnPaint(self, evt):
        dc = wx.PaintDC(self)
        dc.DrawBitmap(self.img, 0, 0)

class ViewMatrix_withoutPIL(ViewMatrix):
    _WITH_PIL = False
    def __init__(self, parent, samples=None, size=None, object=None):
        ViewMatrix.__init__(self, parent, size, object)
        self.width = size[0]
        self.height = size[1]
        self.setImage(samples)

    def setImage(self, samples):
        self.samples = samples
        self.Refresh()

    def OnPaint(self, evt):
        dc = wx.PaintDC(self)
        for i in range(self.width*self.height):
            x = i % self.width
            y = i / self.width
            amp = int(self.samples[i])
            amp = hex(amp).replace('0x', '')
            if len(amp) == 1:
                amp = "0%s" % amp
            amp = "#%s%s%s" % (amp, amp, amp)
            dc.SetPen(wx.Pen(amp, width=1, style=wx.SOLID))
            dc.DrawPoint(x, y)

######################################################################
## Spectrum Display
######################################################################
class SpectrumDisplay(wx.Frame):
    def __init__(self, parent, obj=None):
        wx.Frame.__init__(self, parent, size=(600,350))
        self.SetMinSize((400,240))
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        closeItem = self.fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self._destroy, closeItem)
        self.menubar.Append(self.fileMenu, "&File")
        pollMenu = wx.Menu()
        pollID = 20000
        self.availableSpeeds = [.01, .025, .05, .1, .25, .5, 1]
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
        self.panel = wx.Panel(self)
        self.panel.SetBackgroundColour(BACKGROUND_COLOUR)
        self.mainBox = wx.BoxSizer(wx.VERTICAL)
        self.toolBox = wx.BoxSizer(wx.HORIZONTAL)
        if sys.platform == "darwin":
            X_OFF = 24
        else:
            X_OFF = 16
        tw, th = self.GetTextExtent("Start")
        self.activeTog = wx.ToggleButton(self.panel, -1, label="Start", size=(tw+X_OFF, th+10))
        self.activeTog.SetValue(1)
        self.activeTog.Bind(wx.EVT_TOGGLEBUTTON, self.activate)
        self.toolBox.Add(self.activeTog, 0, wx.TOP|wx.LEFT, 5)
        tw, th = self.GetTextExtent("Freq Log")
        self.freqTog = wx.ToggleButton(self.panel, -1, label="Freq Log", size=(tw+X_OFF, th+10))
        self.freqTog.SetValue(0)
        self.freqTog.Bind(wx.EVT_TOGGLEBUTTON, self.setFreqScale)
        self.toolBox.Add(self.freqTog, 0, wx.TOP|wx.LEFT, 5)
        tw, th = self.GetTextExtent("Mag Log")
        self.magTog = wx.ToggleButton(self.panel, -1, label="Mag Log", size=(tw+X_OFF, th+10))
        self.magTog.SetValue(1)
        self.magTog.Bind(wx.EVT_TOGGLEBUTTON, self.setMagScale)
        self.toolBox.Add(self.magTog, 0, wx.TOP|wx.LEFT, 5)
        tw, th = self.GetTextExtent("Blackman 3-term")
        self.winPopup = wx.Choice(self.panel, -1, choices=["Rectangular", "Hamming", "Hanning", "Bartlett", "Blackman 3-term",
                                    "Blackman-Harris 4-term", "Blackman-Harris 7-term", "Tuckey", "Half-sine"], size=(tw+X_OFF, th+10))
        self.winPopup.SetSelection(2)
        self.winPopup.Bind(wx.EVT_CHOICE, self.setWinType)
        self.toolBox.Add(self.winPopup, 0, wx.TOP|wx.LEFT, 5)
        tw, th = self.GetTextExtent("16384")
        self.sizePopup = wx.Choice(self.panel, -1, choices=["64", "128", "256", "512", "1024",
                                    "2048", "4096", "8192", "16384"], size=(-1, th+10))
        self.sizePopup.SetSelection(4)
        self.sizePopup.Bind(wx.EVT_CHOICE, self.setSize)
        self.toolBox.Add(self.sizePopup, 0, wx.TOP|wx.LEFT, 5)
        self.mainBox.Add(self.toolBox, 0, wx.EXPAND)
        self.dispBox = wx.BoxSizer(wx.HORIZONTAL)
        self.box = wx.BoxSizer(wx.VERTICAL)
        self.spectrumPanel = SpectrumPanel(self.panel, len(self.obj), self.obj.getLowfreq(), self.obj.getHighfreq(),
                                            self.obj.getFscaling(), self.obj.getMscaling())
        self.box.Add(self.spectrumPanel, 1, wx.EXPAND|wx.LEFT|wx.RIGHT|wx.TOP, 5)
        self.zoomH = HRangeSlider(self.panel, minvalue=0, maxvalue=0.5, init=None, pos=(0,0), size=(200,15),
                 valtype='float', log=False, function=self.setZoomH)
        self.box.Add(self.zoomH, 0, wx.EXPAND|wx.LEFT|wx.RIGHT, 5)
        self.dispBox.Add(self.box, 1, wx.EXPAND, 0)
        self.gainSlider = ControlSlider(self.panel, -24, 24, 0, outFunction=self.setGain, orient=wx.VERTICAL)
        self.dispBox.Add(self.gainSlider, 0, wx.EXPAND|wx.TOP, 5)
        self.dispBox.AddSpacer(5)
        self.mainBox.Add(self.dispBox, 1, wx.EXPAND)
        self.panel.SetSizer(self.mainBox)

    def activate(self, evt):
        if evt.GetInt() == 1:
            self.obj.poll(1)
        else:
            self.obj.poll(0)

    def setPollTime(self, evt):
        value = self.availableSpeeds[evt.GetId()-20000]
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
        wx.CallAfter(self.spectrumPanel.setImage, points)

    def setFscaling(self, x):
        self.spectrumPanel.setFscaling(x)
        wx.CallAfter(self.spectrumPanel.Refresh)

    def setMscaling(self, x):
        self.spectrumPanel.setMscaling(x)
        wx.CallAfter(self.spectrumPanel.Refresh)

    def _destroy(self, evt):
        self.obj._setViewFrame(None)
        self.Destroy()

class SpectrumPanel(wx.Panel):
    def __init__(self, parent, chnls, lowfreq, highfreq, fscaling, mscaling):
        wx.Panel.__init__(self, parent)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.chnls = chnls
        self.img = None
        self.lowfreq = lowfreq
        self.highfreq = highfreq
        self.fscaling = fscaling
        self.mscaling = mscaling
        if self.chnls == 1:
            self.pens = [wx.Pen(wx.Colour(100,0,0))]
            self.brushes = [wx.Brush(wx.Colour(166,4,0))]
        else:
            self.pens = [wx.Pen(wx.Colour(166,4,0)), wx.Pen(wx.Colour(8,11,116)), wx.Pen(wx.Colour(0,204,0)),
                        wx.Pen(wx.Colour(255,167,0)), wx.Pen(wx.Colour(133,0,75)), wx.Pen(wx.Colour(255,236,0)),
                        wx.Pen(wx.Colour(1,147,154)), wx.Pen(wx.Colour(162,239,0))]
            self.brushes = [wx.Brush(wx.Colour(166,4,0,128)), wx.Brush(wx.Colour(8,11,116,128)), wx.Brush(wx.Colour(0,204,0,128)),
                            wx.Brush(wx.Colour(255,167,0,128)), wx.Brush(wx.Colour(133,0,75,128)), wx.Brush(wx.Colour(255,236,0,128)),
                            wx.Brush(wx.Colour(1,147,154,128)), wx.Brush(wx.Colour(162,239,0,128))]
        if sys.platform == "win32":
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC

    def OnSize(self, evt):
        self.GetParent().GetParent().setDisplaySize(self.GetSize())
        self.Refresh()

    def setImage(self, points):
        self.img = [points[i] for i in range(self.chnls)]
        self.Refresh()

    def setFscaling(self, x):
        self.fscaling = x

    def setMscaling(self, x):
        self.mscaling = x

    def setLowFreq(self, x):
        self.lowfreq = x

    def setHighFreq(self, x):
        self.highfreq = x

    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        tw, th = dc.GetTextExtent("0")

        # background
        background = gc.CreatePath()
        background.AddRectangle(0,0,w,h)
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
            w8 = w / 8
            for i in range(1,8):
                pos = w8*i
                dc.DrawLine(pos, th+4, pos, h-2)
                text = str(int(self.lowfreq+step*i))
                tw, th = dc.GetTextExtent(text)
                dc.DrawText(text, pos-tw/2, 2)
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
                base = pow(10.0, math.floor(lrange/6))
                def inc(t, floor_t):
                    return t*base-t
            else:
                t = math.ceil(pow(10.0,lf)/mag)*mag
                def inc(t, floor_t):
                    return pow(10.0, floor_t)

            majortick = int(math.log10(mag))
            while t <= pow(10,hf):
                floor_t = int(math.floor(math.log10(t)+1e-16))
                if majortick != floor_t:
                    majortick = floor_t
                    ticklabel = '1e%d'%majortick
                    ticklabel = str(int(float(ticklabel)))
                    tw, th = dc.GetTextExtent(ticklabel)
                else:
                    if hf-lf < 2:
                        minortick = int(t/pow(10.0,majortick)+.5)
                        ticklabel = '%de%d'%(minortick,majortick)
                        ticklabel = str(int(float(ticklabel)))
                        tw, th = dc.GetTextExtent(ticklabel)
                        if not minortick%2 == 0:
                            ticklabel = ''
                    else:
                        ticklabel = ''
                pos = (math.log10(t) - lf) / lrange * w
                if pos < (w-25):
                    dc.DrawLine(pos, th+4, pos, h-2)
                    dc.DrawText(ticklabel, pos-tw/2, 2)
                t += inc(t, floor_t)

        # magnitude linear grid
        if not self.mscaling:
            h4 = h * 0.75
            step = h4 * 0.1
            for i in range(1, 11):
                pos = int(h - i * step)
                text = "%.1f" % (i * 0.1)
                tw, th = dc.GetTextExtent(text)
                dc.DrawText(text, w-tw-2, pos-th/2)
                dc.DrawLine(0, pos, w-tw-4, pos)
            dc.SetPen(wx.Pen("#555555", style=wx.SOLID))
            dc.DrawLine(0, pos, w-tw-6, pos)
            dc.SetPen(wx.Pen("#555555", style=wx.DOT))
            i += 1
            while (i*step < (h-th-5)):
                pos = int(h - i * step)
                text = "%.1f" % (i * 0.1)
                tw, th = dc.GetTextExtent(text)
                dc.DrawText(text, w-tw-2, pos-th/2)
                dc.DrawLine(0, pos, w-tw-6, pos)
                i += 1
        # magnitude logarithmic grid
        else:
            mw, mh = dc.GetTextExtent("-54")
            h4 = h * 0.75
            step = h4 * 0.1
            for i in range(1, 11):
                pos = int(h - i * step)
                mval = int((10-i) * -6.0)
                if mval == -0:
                    mval = 0
                text = "%d" % mval
                tw, th = dc.GetTextExtent(text)
                dc.DrawText(text, w-tw-2, pos-th/2)
                dc.DrawLine(0, pos, w-mw-6, pos)
            dc.SetPen(wx.Pen("#555555", style=wx.SOLID))
            dc.DrawLine(0, pos, w-mw-4, pos)
            dc.SetPen(wx.Pen("#555555", style=wx.DOT))
            i += 1
            while (i*step < (h-th-5)):
                pos = int(h - i * step)
                text = "%d" % int((10-i) * -6.0)
                tw, th = dc.GetTextExtent(text)
                dc.DrawText(text, w-tw-2, pos-th/2)
                dc.DrawLine(0, pos, w-mw-6, pos)
                i += 1

        last_tw = tw
        # legend
        tw, th = dc.GetTextExtent("chan 8")
        for i in range(self.chnls):
            dc.SetTextForeground(self.pens[i].GetColour())
            dc.DrawText("chan %d" % (i+1), w-tw-20-last_tw, i*th+th+7)

        # spectrum
        if self.img != None:
            for i, samples in enumerate(self.img):
                gc.SetPen(self.pens[i])
                gc.SetBrush(self.brushes[i])
                gc.DrawLines(samples)

######################################################################
## Spectrum Display
######################################################################
class ScopeDisplay(wx.Frame):
    def __init__(self, parent, obj=None):
        wx.Frame.__init__(self, parent, size=(600,350))
        self.SetMinSize((400,240))
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        closeItem = self.fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self._destroy, closeItem)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self.obj = obj
        gain = self.obj.gain
        length = self.obj.length
        self.panel = wx.Panel(self)
        self.panel.SetBackgroundColour(BACKGROUND_COLOUR)
        self.mainBox = wx.BoxSizer(wx.VERTICAL)
        self.toolBox = wx.BoxSizer(wx.HORIZONTAL)
        if sys.platform == "darwin":
            X_OFF = 24
        else:
            X_OFF = 16
        tw, th = self.GetTextExtent("Start")
        self.activeTog = wx.ToggleButton(self.panel, -1, label="Start", size=(tw+X_OFF, th+10))
        self.activeTog.SetValue(1)
        self.activeTog.Bind(wx.EVT_TOGGLEBUTTON, self.activate)
        self.toolBox.Add(self.activeTog, 0, wx.TOP|wx.LEFT|wx.RIGHT, 5)
        self.toolBox.AddSpacer(10)
        self.toolBox.Add(wx.StaticText(self.panel, -1, label="Window length (ms):"), 0, wx.TOP, 11)
        self.lenSlider = ControlSlider(self.panel, 10, 60, length * 1000, outFunction=self.setLength)
        self.toolBox.Add(self.lenSlider, 1, wx.TOP|wx.LEFT|wx.RIGHT, 11)
        self.toolBox.AddSpacer(40)
        self.mainBox.Add(self.toolBox, 0, wx.EXPAND)
        self.dispBox = wx.BoxSizer(wx.HORIZONTAL)
        self.box = wx.BoxSizer(wx.VERTICAL)
        self.scopePanel = ScopePanel(self.panel, self.obj)
        self.box.Add(self.scopePanel, 1, wx.EXPAND|wx.LEFT|wx.RIGHT, 5)
        self.dispBox.Add(self.box, 1, wx.EXPAND, 0)
        self.gainSlider = ControlSlider(self.panel, -24, 24, 20.0 * math.log10(gain), outFunction=self.setGain, orient=wx.VERTICAL)
        self.dispBox.Add(self.gainSlider, 0, wx.EXPAND)
        self.dispBox.AddSpacer(5)
        self.mainBox.Add(self.dispBox, 1, wx.EXPAND)
        self.panel.SetSizer(self.mainBox)

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
        wx.CallAfter(self.scopePanel.setImage, points)

    def _destroy(self, evt):
        self.obj._setViewFrame(None)
        self.Destroy()

class ScopePanel(wx.Panel):
    def __init__(self, parent, obj):
        wx.Panel.__init__(self, parent)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.img = [[]]
        self.obj = obj
        self.gain = self.obj.gain
        self.length = self.obj.length
        self.chnls = len(self.obj)
        if self.chnls == 1:
            self.pens = [wx.Pen(wx.Colour(100,0,0), width=2)]
        else:
            self.pens = [wx.Pen(wx.Colour(166,4,0), width=2), wx.Pen(wx.Colour(8,11,116), width=2), wx.Pen(wx.Colour(0,204,0), width=2),
                        wx.Pen(wx.Colour(255,167,0), width=2), wx.Pen(wx.Colour(133,0,75), width=2), wx.Pen(wx.Colour(255,236,0), width=2),
                        wx.Pen(wx.Colour(1,147,154), width=2), wx.Pen(wx.Colour(162,239,0), width=2)]

        if sys.platform == "win32":
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC

    def OnSize(self, evt):
        size = self.GetSize()
        self.obj.setWidth(size[0])
        self.obj.setHeight(size[1])

    def setGain(self, gain):
        self.gain = gain
        print self.gain

    def setLength(self, length):
        self.length = length

    def setImage(self, points):
        self.img = points
        self.Refresh()

    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        tw, th = dc.GetTextExtent("0")
        dc.SetBrush(wx.Brush("#FFFFFF"))
        dc.Clear()
        dc.DrawRectangle(0,0,w,h)
        gc.SetPen(wx.Pen('#000000', width=1, style=wx.SOLID))
        gc.SetBrush(wx.Brush("#FFFFFF", style=wx.TRANSPARENT))
        dc.SetTextForeground("#444444")
        if sys.platform in "darwin":
            font, ptsize = dc.GetFont(), dc.GetFont().GetPointSize()
            font.SetPointSize(ptsize - 3)
            dc.SetFont(font)
        elif sys.platform == "win32":
            font = dc.GetFont()
            font.SetPointSize(8)
            dc.SetFont(font)

        dc.SetPen(wx.Pen('#888888', width=1, style=wx.DOT))
        # horizontal grid
        step = h / 6
        ampstep = 1.0 / 3.0 / self.gain
        for i in range(1, 6):
            pos = int(h - i * step)
            npos = i - 3
            text = "%.2f" % (ampstep * npos)
            tw, th = dc.GetTextExtent(text)
            dc.DrawText(text, w-tw-2, pos-th/2)
            dc.DrawLine(0, pos, w-tw-10, pos)

        # vertical grid
        tickstep = w / 4
        timestep = self.length * 0.25
        for j in range(4):
            dc.SetPen(wx.Pen('#888888', width=1, style=wx.DOT))
            dc.DrawLine(j*tickstep, 0, j*tickstep, h)
            dc.DrawText("%.3f" % (j*timestep), j*tickstep+2, h-12)
        # draw waveforms
        for i, samples in enumerate(self.img):
            gc.SetPen(self.pens[i])
            if len(samples):
                gc.DrawLines(samples)

        # legend
        last_tw = tw
        tw, th = dc.GetTextExtent("chan 8")
        for i in range(self.chnls):
            dc.SetTextForeground(self.pens[i].GetColour())
            dc.DrawText("chan %d" % (i+1), w-tw-20-last_tw, i*th+10)

######################################################################
## Grapher window for PyoTableObject control
######################################################################
OFF = 15
OFF2 = OFF*2
RAD = 3
RAD2 = RAD*2
AREA = RAD+2
AREA2 = AREA*2
class Grapher(wx.Panel):
    def __init__(self, parent, xlen=8192, yrange=(0.0, 1.0), init=[(0.0,0.0),(1.0,1.0)], mode=0,
                 exp=10.0, inverse=True, tension=0.0, bias=0.0, outFunction=None):
        wx.Panel.__init__(self, parent, size=(500,250), style=wx.SUNKEN_BORDER)
        self.backgroundColour = BACKGROUND_COLOUR
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
        self.pos = (OFF+RAD,OFF+RAD)
        self.selected = None
        self.xlen = xlen
        self.yrange = yrange
        self.init = [tup for tup in init]
        self.points = init
        self.outFunction = outFunction

        if sys.platform == "win32":
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC

        self.SetFocus()

    def setInitPoints(self, pts):
        self.init = [(p[0],p[1]) for p in pts]
        self.points = [(p[0],p[1]) for p in pts]
        self.selected = None
        self.Refresh()

    def pointToPixels(self, pt):
        w,h = self.GetSize()
        w,h = w-OFF2-RAD2, h-OFF2-RAD2
        x = int(round(pt[0] * w)) + OFF + RAD
        y = int(round(pt[1] * h)) + OFF + RAD
        return x, y

    def pixelsToPoint(self, pos):
        w,h = self.GetSize()
        w,h = w-OFF2-RAD2, h-OFF2-RAD2
        x = (pos[0] - OFF - RAD) / float(w)
        y = (pos[1] - OFF - RAD) / float(h)
        return x, y

    def pointToValues(self, pt):
        x = pt[0] * self.xlen
        if type(self.xlen) == IntType:
            x = int(x)
        y = pt[1] * (self.yrange[1]-self.yrange[0]) + self.yrange[0]
        return x, y

    def borderClip(self, pos):
        w,h = self.GetSize()
        if pos[0] < (OFF+RAD): pos[0] = (OFF+RAD)
        elif pos[0] > (w-OFF-RAD): pos[0] = w-OFF-RAD
        if pos[1] < (OFF+RAD): pos[1] = (OFF+RAD)
        elif pos[1] > (h-OFF-RAD): pos[1] = h-OFF-RAD
        return pos

    def pointClip(self, pos):
        w,h = self.GetSize()
        if self.selected == 0:
            leftclip = OFF+RAD
        else:
            x,y = self.pointToPixels(self.points[self.selected-1])
            leftclip = x
        if self.selected == (len(self.points) - 1):
            rightclip = w-OFF-RAD
        else:
            x,y = self.pointToPixels(self.points[self.selected+1])
            rightclip = x

        if pos[0] < leftclip: pos[0] = leftclip
        elif pos[0] > rightclip: pos[0] = rightclip
        if pos[1] < (OFF+RAD): pos[1] = (OFF+RAD)
        elif pos[1] > (h-OFF-RAD): pos[1] = h-OFF-RAD
        return pos

    def reset(self):
        self.points = self.init
        self.Refresh()

    def getPoints(self):
        return [tup for tup in self.points]

    def getValues(self):
        values = []
        for pt in self.points:
            x,y = self.pointToValues(pt)
            values.append((x,y))
        return values

    def sendValues(self):
        if self.outFunction != None:
            values = self.getValues()
            self.outFunction(values)

    def OnResize(self, evt):
        self.Refresh()
        evt.Skip()

    def OnLeave(self, evt):
        self.pos = (OFF+RAD,OFF+RAD)
        self.Refresh()

    def OnKeyDown(self, evt):
        if self.selected != None and evt.GetKeyCode() in [wx.WXK_BACK, wx.WXK_DELETE, wx.WXK_NUMPAD_DELETE]:
            del self.points[self.selected]
            self.sendValues()
            self.selected = None
            self.Refresh()
        elif evt.GetKeyCode() in [wx.WXK_UP, wx.WXK_NUMPAD_UP]:
            self.points = [(pt[0], pt[1]+0.002) for pt in self.points]
            self.sendValues()
            self.Refresh()
        elif evt.GetKeyCode() in [wx.WXK_DOWN, wx.WXK_NUMPAD_DOWN]:
            self.points = [(pt[0], pt[1]-0.002) for pt in self.points]
            self.sendValues()
            self.Refresh()
        evt.Skip()

    def MouseDown(self, evt):
        self.CaptureMouse()
        w,h = self.GetSize()
        self.pos = self.borderClip(evt.GetPosition())
        self.pos[1] = h - self.pos[1]
        for i, p in enumerate(self.points):
            x, y = self.pointToPixels(p)
            if wx.Rect(x-AREA, y-AREA, AREA2, AREA2).Contains(self.pos):
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
        w,h = self.GetSize()
        self.pos = self.borderClip(evt.GetPosition())
        self.pos[1] = h - self.pos[1]
        if self.HasCapture():
            if self.selected != None:
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
                    tmp.append((pt1[0]+i, pt1[1]))
            else:
                for i in range(steps):
                    ratio = ((pt1[1] + diff * i) - low) / lrange
                    tmp.append((pt1[0]+i, pow(10, ratio * logrange + logmin)))
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
                    tmp.append((pt1[0]+i, pt1[1]))
            else:
                for i in range(steps):
                    mu = float(i) / steps
                    mu = (1. - math.cos(mu*math.pi)) * 0.5
                    mu = pt1[1] * (1. - mu) + pt2[1] * mu
                    ratio = (mu - low) / lrange
                    tmp.append((pt1[0]+i, pow(10, ratio * logrange + logmin)))
        return tmp

    def getCosPoints(self, pt1, pt2):
        tmp = []
        steps = pt2[0] - pt1[0]
        for i in range(steps):
            mu = float(i) / steps
            mu2 = (1. - math.cos(mu*math.pi)) * 0.5
            tmp.append((pt1[0]+i, pt1[1] * (1. - mu2) + pt2[1] * mu2))
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
                    tmp.append((pt1[0]+i, scl * ambitus + pt1[1]))
                    pointer += inc
            else:
                for i in range(steps):
                    scl = pow(pointer, self.exp)
                    tmp.append((pt1[0]+i, scl * ambitus + pt1[1]))
                    pointer += inc
        else:
            for i in range(steps):
                scl = pow(pointer, self.exp)
                tmp.append((pt1[0]+i, scl * ambitus + pt1[1]))
                pointer += inc
        return tmp

    def addImaginaryPoints(self, tmp):
        lst = []
        x = tmp[1][0] - tmp[0][0]
        if tmp[0][1] < tmp[1][1]:
            y = tmp[0][1] - tmp[1][1]
        else:
            y = tmp[0][1] + tmp[1][1]
        lst.append((x,y))
        lst.extend(tmp)
        x = tmp[-2][0] - tmp[-1][0]
        if tmp[-2][1] < tmp[-1][1]:
            y = tmp[-1][1] + tmp[-2][1]
        else:
            y = tmp[-1][1] - tmp[-2][1]
        lst.append((x,y))
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
            tmp.append((pt1[0]+i, a0*y1 + a1*m0 + a2*m1 + a3*y2))
        return tmp

    def OnPaint(self, evt):
        w,h = self.GetSize()
        corners = [(OFF,OFF),(w-OFF,OFF),(w-OFF,h-OFF),(OFF,h-OFF)]
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        gc.SetBrush(wx.Brush("#000000"))
        gc.SetPen(wx.Pen("#000000"))
        if sys.platform == "darwin":
            font, ptsize = dc.GetFont(), dc.GetFont().GetPointSize()
        else:
            font, ptsize = dc.GetFont(), 10
        font.SetPointSize(ptsize-4)
        dc.SetFont(font)
        dc.SetTextForeground("#888888")
        dc.Clear()

        # Draw grid
        dc.SetPen(wx.Pen("#CCCCCC", 1))
        xstep = int(round((w-OFF2) / float(10)))
        ystep = int(round((h-OFF2) / float(10)))
        for i in range(10):
            xpos = i * xstep + OFF
            dc.DrawLine(xpos, OFF, xpos, h-OFF)
            ypos = i * ystep + OFF
            dc.DrawLine(OFF, ypos, w-OFF, ypos)
            if i > 0:
                if type(self.xlen) == IntType:
                    t = "%d" % int(self.xlen * i * 0.1)
                else:
                    t = "%.2f" % (self.xlen * i * 0.1)
                dc.DrawText(t, xpos+2, h-OFF-10)
            if i < 9:
                t = "%.2f" % ((9-i) * 0.1 * (self.yrange[1]-self.yrange[0]) + self.yrange[0])
                dc.DrawText(t, OFF+1, ypos+ystep-10)
            else:
                t = "%.2f" % ((9-i) * 0.1 * (self.yrange[1]-self.yrange[0]) + self.yrange[0])
                dc.DrawText(t, OFF+1, h-OFF-10)

        dc.SetPen(wx.Pen("#000000", 1))
        dc.SetBrush(wx.Brush("#000000"))
        # Draw bounding box
        for i in range(4):
            dc.DrawLinePoint(corners[i], corners[(i+1)%4])

        # Convert points in pixels
        w,h = w-OFF2-RAD2, h-OFF2-RAD2
        tmp = []
        back_y_for_log = []
        for p in self.points:
            x = int(round(p[0] * w)) + OFF + RAD
            y = int(round((1.0-p[1]) * h)) + OFF + RAD
            tmp.append((x,y))
            back_y_for_log.append(p[1])

        # Draw lines
        dc.SetPen(wx.Pen("#000000", 1))
        last_p = None
        if len(tmp) > 1:
            if self.mode == 0:
                for i in range(len(tmp)-1):
                    gc.DrawLines([tmp[i], tmp[i+1]])
            elif self.mode == 1:
                for i in range(len(tmp)-1):
                    tmp2 = self.getCosPoints(tmp[i], tmp[i+1])
                    if i == 0 and len(tmp2) < 2:
                        gc.DrawLines([tmp[i], tmp[i+1]])
                    if last_p != None:
                        gc.DrawLines([last_p, tmp[i]])
                    for j in range(len(tmp2)-1):
                        gc.DrawLines([tmp2[j], tmp2[j+1]])
                        last_p = tmp2[j+1]
                if last_p != None:
                    gc.DrawLines([last_p, tmp[-1]])
            elif self.mode == 2:
                for i in range(len(tmp)-1):
                    tmp2 = self.getExpPoints(tmp[i], tmp[i+1])
                    if i == 0 and len(tmp2) < 2:
                        gc.DrawLines([tmp[i], tmp[i+1]])
                    if last_p != None:
                        gc.DrawLines([last_p, tmp[i]])
                    for j in range(len(tmp2)-1):
                        gc.DrawLines([tmp2[j], tmp2[j+1]])
                        last_p = tmp2[j+1]
                if last_p != None:
                    gc.DrawLines([last_p, tmp[-1]])
            elif self.mode == 3:
                curvetmp = self.addImaginaryPoints(tmp)
                for i in range(1, len(curvetmp)-2):
                    tmp2 = self.getCurvePoints(curvetmp[i-1], curvetmp[i], curvetmp[i+1], curvetmp[i+2])
                    if i == 1 and len(tmp2) < 2:
                        gc.DrawLines([curvetmp[i], curvetmp[i+1]])
                    if last_p != None:
                        gc.DrawLines([last_p, curvetmp[i]])
                    for j in range(len(tmp2)-1):
                        gc.DrawLines([tmp2[j], tmp2[j+1]])
                        last_p = tmp2[j+1]
                if last_p != None:
                    gc.DrawLines([last_p, tmp[-1]])
            elif self.mode == 4:
                back_tmp = [p for p in tmp]
                for i in range(len(tmp)):
                    tmp[i] = (tmp[i][0], back_y_for_log[i])
                for i in range(len(tmp)-1):
                    tmp2 = self.getLogPoints(tmp[i], tmp[i+1])
                    for j in range(len(tmp2)):
                        tmp2[j] = (tmp2[j][0], int(round((1.0-tmp2[j][1]) * h)) + OFF + RAD)
                    if i == 0 and len(tmp2) < 2:
                        gc.DrawLines([back_tmp[i], back_tmp[i+1]])
                    if last_p != None:
                        gc.DrawLines([last_p, back_tmp[i]])
                    for j in range(len(tmp2)-1):
                        gc.DrawLines([tmp2[j], tmp2[j+1]])
                        last_p = tmp2[j+1]
                if last_p != None:
                    gc.DrawLines([last_p, back_tmp[-1]])
                tmp = [p for p in back_tmp]
            elif self.mode == 5:
                back_tmp = [p for p in tmp]
                for i in range(len(tmp)):
                    tmp[i] = (tmp[i][0], back_y_for_log[i])
                for i in range(len(tmp)-1):
                    tmp2 = self.getCosLogPoints(tmp[i], tmp[i+1])
                    for j in range(len(tmp2)):
                        tmp2[j] = (tmp2[j][0], int(round((1.0-tmp2[j][1]) * h)) + OFF + RAD)
                    if i == 0 and len(tmp2) < 2:
                        gc.DrawLines([back_tmp[i], back_tmp[i+1]])
                    if last_p != None:
                        gc.DrawLines([last_p, back_tmp[i]])
                    for j in range(len(tmp2)-1):
                        gc.DrawLines([tmp2[j], tmp2[j+1]])
                        last_p = tmp2[j+1]
                if last_p != None:
                    gc.DrawLines([last_p, back_tmp[-1]])
                tmp = [p for p in back_tmp]

        # Draw points
        for i,p in enumerate(tmp):
            if i == self.selected:
                gc.SetBrush(wx.Brush("#FFFFFF"))
                dc.SetBrush(wx.Brush("#FFFFFF"))
            else:
                gc.SetBrush(wx.Brush("#000000"))
                dc.SetBrush(wx.Brush("#000000"))
            gc.DrawEllipse(p[0]-RAD,p[1]-RAD,RAD2,RAD2)

        # Draw position values
        font.SetPointSize(ptsize-3)
        dc.SetFont(font)
        dc.SetTextForeground("#222222")
        posptx, pospty = self.pixelsToPoint(self.pos)
        xval, yval = self.pointToValues((posptx, pospty))
        if type(self.xlen) == IntType:
            dc.DrawText("%d, %.3f" % (xval, yval), w-75, OFF)
        else:
            dc.DrawText("%.3f, %.3f" % (xval, yval), w-75, OFF)

class TableGrapher(wx.Frame):
    def __init__(self, parent=None, obj=None, mode=0, xlen=8192, yrange=(0.0, 1.0)):
        wx.Frame.__init__(self, parent, size=(500,250))
        pts = obj.getPoints()
        self.yrange = yrange
        for i in range(len(pts)):
            x = pts[i][0] / float(xlen)
            y = (pts[i][1] - float(yrange[0])) / (yrange[1]-yrange[0])
            pts[i] = (x,y)
        if mode == 2:
            self.graph = Grapher(self, xlen=xlen, yrange=yrange, init=pts, mode=mode, exp=obj.exp, inverse=obj.inverse, outFunction=obj.replace)
        elif mode == 3:
            self.graph = Grapher(self, xlen=xlen, yrange=yrange, init=pts, mode=mode, tension=obj.tension, bias=obj.bias, outFunction=obj.replace)
        else:
            self.graph = Grapher(self, xlen=xlen, yrange=yrange, init=pts, mode=mode, outFunction=obj.replace)

        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(9999, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.close, id=9999)
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(10000, 'Copy all points to the clipboard (4 digits of precision)\tCtrl+C', kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.copy, id=10000)
        self.fileMenu.Append(10001, 'Copy all points to the clipboard (full precision)\tShift+Ctrl+C', kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.copy, id=10001)
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(10002, 'Reset\tCtrl+R', kind=wx.ITEM_NORMAL)
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
                if type(pt[0]) == IntType:
                    pstr += "%d," % pt[0]
                else:
                    pstr += "%.4f," % pt[0]
                pstr += "%.4f)" % pt[1]
                if i < (len(pts)-1):
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

class DataMultiSlider(wx.Panel):
    def __init__(self, parent, init, yrange=(0,1), outFunction=None):
        wx.Panel.__init__(self, parent, size=(250,250), style=wx.SUNKEN_BORDER)
        self.backgroundColour = BACKGROUND_COLOUR
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.SetBackgroundColour(self.backgroundColour)
        self.Bind(wx.EVT_SIZE, self.OnResize)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self.values = init
        self.len = len(self.values)
        self.yrange = yrange
        self.outFunction = outFunction
        if sys.platform == "win32":
            self.dcref = wx.BufferedPaintDC
        else:
            self.dcref = wx.PaintDC

    def OnResize(self, event):
        self.Layout()
        self.Refresh()

    def update(self, points):
        self.values = points
        self.Refresh()

    def OnPaint(self, event):
        w,h = self.GetSize()
        dc = self.dcref(self)
        gc = wx.GraphicsContext_Create(dc)
        dc.SetBrush(wx.Brush("#FFFFFF"))
        dc.SetPen(wx.Pen("#FFFFFF"))
        dc.Clear()
        dc.DrawRectangle(0,0,w,h)
        gc.SetBrush(wx.Brush("#000000"))
        gc.SetPen(wx.Pen("#000000"))
        scl = self.yrange[1] - self.yrange[0]
        mini = self.yrange[0]
        bw = float(w) / self.len
        points = [(0,h)]
        x = 0
        if bw >= 1:
            for i in range(self.len):
                y = h - ((self.values[i] - mini) / scl * h)
                points.append((x,y))
                x = (i+1) * bw
                points.append((x,y))
        else:
            slice = 1 / bw
            p1 = 0
            for i in range(w):
                p2 = int((i+1) * slice)
                y = h - ((max(self.values[p1:p2]) - mini) / scl * h)
                points.append((i,y))
                p1 = p2
        points.append((w,y))
        points.append((w,h))
        gc.DrawLines(points)
        if self.outFunction != None:
            self.outFunction(self.values)

    def MouseDown(self, evt):
        w,h = self.GetSize()
        self.lastpos = pos = evt.GetPosition()
        self.CaptureMouse()
        scl = self.yrange[1] - self.yrange[0]
        mini = self.yrange[0]
        bw = float(w) / self.len
        x = int(pos[0] / bw)
        y = (h - pos[1]) / float(h) * scl + mini
        self.values[x] = y
        self.Refresh()
        evt.Skip()

    def MouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()

    def MouseMotion(self, evt):
        w,h = self.GetSize()
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
                        self.values[x1+i] = y1 + inc * i
                else:
                    for i in range(1, step):
                        self.values[x1-i] = y1 + inc * i
            if x2 >= 0 and x2 < self.len:
                self.values[x2] = y2
            self.lastpos = pos
            self.Refresh()

class DataTableGrapher(wx.Frame):
    def __init__(self, parent=None, obj=None, yrange=(0.0, 1.0)):
        wx.Frame.__init__(self, parent, size=(500,250))
        self.obj = obj
        self.multi = DataMultiSlider(self, self.obj.getTable(), yrange, outFunction=self.obj.replace)
        self.menubar = wx.MenuBar()
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(9999, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.close, id=9999)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)

    def close(self, evt):
        self.Destroy()

    def update(self, samples):
        wx.CallAfter(self.multi.update, samples)

class ServerGUI(wx.Frame):
    def __init__(self, parent=None, nchnls=2, startf=None, stopf=None, recstartf=None,
                recstopf=None, ampf=None, started=0, locals=None, shutdown=None, meter=True, timer=True, amp=1., exit=True):
        wx.Frame.__init__(self, parent, style=wx.DEFAULT_FRAME_STYLE ^ wx.RESIZE_BORDER)

        self.SetTitle("pyo server")

        self.menubar = wx.MenuBar()
        self.menu = wx.Menu()
        self.menu.Append(22999, 'Start/Stop\tCtrl+R', kind=wx.ITEM_NORMAL)
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
        self._started = False
        self._recstarted = False
        self._history = []
        self._histo_count = 0

        panel = wx.Panel(self)
        panel.SetBackgroundColour(BACKGROUND_COLOUR)
        box = wx.BoxSizer(wx.VERTICAL)

        if sys.platform == "win32":
            leftMargin = 24
        else:
            leftMargin = 25

        buttonBox = wx.BoxSizer(wx.HORIZONTAL)
        self.startButton = wx.Button(panel, -1, 'Start', (20,20), (72,-1))
        self.startButton.Bind(wx.EVT_BUTTON, self.start)
        buttonBox.Add(self.startButton, 0, wx.RIGHT, 5)

        self.recButton = wx.Button(panel, -1, 'Rec Start', (20,20), (72,-1))
        self.recButton.Bind(wx.EVT_BUTTON, self.record)
        buttonBox.Add(self.recButton, 0, wx.RIGHT, 5)

        self.quitButton = wx.Button(panel, -1, 'Quit', (20,20), (72,-1))
        self.quitButton.Bind(wx.EVT_BUTTON, self.on_quit)
        buttonBox.Add(self.quitButton, 0, wx.RIGHT, 0)

        box.Add(buttonBox, 0, wx.TOP | wx.LEFT | wx.RIGHT, 10)
        box.AddSpacer(10)

        box.Add(wx.StaticText(panel, -1, "Amplitude (dB)"), 0, wx.LEFT, leftMargin)
        ampBox = wx.BoxSizer(wx.HORIZONTAL)
        self.ampScale = ControlSlider(panel, -60, 18, 20.0 * math.log10(amp), size=(202, 16), outFunction=self.setAmp)
        ampBox.Add(self.ampScale, 0, wx.LEFT, leftMargin-10)
        box.Add(ampBox, 0, wx.LEFT | wx.RIGHT, 8)

        if meter:
            box.AddSpacer(10)
            self.meter = VuMeter(panel, size=(200,5*self.nchnls+1), numSliders=self.nchnls)
            box.Add(self.meter, 0, wx.LEFT, leftMargin-1)
            box.AddSpacer(5)

        if timer:
            box.AddSpacer(10)
            tt = wx.StaticText(panel, -1, "Elapsed time (hh : mm : ss : ms)")
            box.Add(tt, 0, wx.LEFT, leftMargin)
            box.AddSpacer(3)
            self.timetext = wx.StaticText(panel, -1, "00 : 00 : 00 : 000")
            box.Add(self.timetext, 0, wx.LEFT, leftMargin)

        if self.locals != None:
            box.AddSpacer(10)
            t = wx.StaticText(panel, -1, "Interpreter")
            box.Add(t, 0, wx.LEFT, leftMargin)
            tw, th = self.GetTextExtent("|")
            self.text = wx.TextCtrl(panel, -1, "", size=(202, th+8), style=wx.TE_PROCESS_ENTER)
            self.text.Bind(wx.EVT_TEXT_ENTER, self.getText)
            self.text.Bind(wx.EVT_CHAR, self.onChar)
            box.Add(self.text, 0, wx.LEFT, leftMargin-1)

        box.AddSpacer(10)
        panel.SetSizerAndFit(box)
        self.SetClientSize(panel.GetSize())

        if started == 1:
            self.start(None, True)

    def setTime(self, *args):
        wx.CallAfter(self.timetext.SetLabel, "%02d : %02d : %02d : %03d" % (args[0], args[1], args[2], args[3]))

    def start(self, evt=None, justSet=False):
        if self._started == False:
            if not justSet:
                self.startf()
            self._started = True
            wx.CallAfter(self.startButton.SetLabel, 'Stop')
            if self.exit:
                wx.CallAfter(self.quitButton.Disable)
        else:
            wx.CallLater(100, self.stopf)
            self._started = False
            wx.CallAfter(self.startButton.SetLabel, 'Start')
            if self.exit:
                wx.CallAfter(self.quitButton.Enable)

    def record(self, evt):
        if self._recstarted == False:
            self.recstartf()
            self._recstarted = True
            self.recButton.SetLabel('Rec Stop')
        else:
            self.recstopf()
            self._recstarted = False
            self.recButton.SetLabel('Rec Start')

    def on_quit(self, evt):
        if self.exit:
            self.shutdown()
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
            self.text.SetInsertionPointEnd()

    def getText(self, evt):
        source = self.text.GetValue()
        self.text.Clear()
        self._history.append(source)
        self._histo_count = len(self._history)
        exec source in self.locals

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
