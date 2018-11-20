import sys
import wx
#import wx.lib.newevent
from pyo import *
from pyolib._wxwidgets import BACKGROUND_COLOUR

KEYBOARD_BACKGROUND_COLOUR = BACKGROUND_COLOUR#"#CCCCCC"

#if "phoenix" not in wx.version():
#    wx.QueueEvent = wx.PostEvent

#PyoGuiKeyboardEvent, EVT_PYO_GUI_KEYBOARD = wx.lib.newevent.NewEvent()
#evt = PyoGuiKeyboardEvent(value=note, id=self.GetId(), object=self)
#wx.QueueEvent(self, evt)


class Keyboard(wx.Panel):
    def __init__(self, parent, id=wx.ID_ANY, pos=wx.DefaultPosition,
                 size=wx.DefaultSize, poly=64, outFunction=None, 
                 style=wx.TAB_TRAVERSAL):
        wx.Panel.__init__(self, parent, id, pos, size, style)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)  
        self.SetBackgroundColour(KEYBOARD_BACKGROUND_COLOUR)
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

        self.white = (0, 2, 4, 5, 7, 9, 11)
        self.black = (1, 3, 6, 8, 10)
        self.whiteSelected = []
        self.blackSelected = []
        self.whiteVelocities = {}
        self.blackVelocities = {}
        self.whiteKeys = []
        self.blackKeys = []

        wx.CallAfter(self._setRects)
   
    def getCurrentNotes(self):
        "Returns a list of the current notes."
        notes = []
        for key in self.whiteSelected:
            notes.append((self.white[key % 7] + int(key / 7) * 12  + self.offset,
                          127 - self.whiteVelocities[key]))
        for key in self.blackSelected:
            notes.append((self.black[key % 5] + int(key / 5) * 12  + self.offset,
                          127 - self.blackVelocities[key]))
        notes.sort()
        return notes

    def reset(self):
        "Resets the keyboard state."
        for key in self.blackSelected:
            pit = self.black[key % 5] + int(key / 5) * 12  + self.offset
            note = (pit, 0)
            if self.outFunction:
                self.outFunction(note)
        for key in self.whiteSelected:
            pit = self.white[key % 7] + int(key / 7) * 12  + self.offset
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
        print(w, h)
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

    def MouseDown(self, evt):
        w,h = self.GetSize()
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
                    pit = self.black[i % 5] + int(i / 5) * 12  + self.offset
                    if i in self.blackSelected:
                        self.blackSelected.remove(i)
                        del self.blackVelocities[i]
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
                        pit = self.white[i % 7] + int(i / 7) * 12  + self.offset
                        if i in self.whiteSelected:
                            self.whiteSelected.remove(i)
                            del self.whiteVelocities[i]
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
                    pit = self.black[i % 5] + int(i / 5) * 12  + self.offset
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
    
    def OnPaint(self, evt):
        w, h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)
        dc.SetBrush(wx.Brush("#000000", wx.SOLID))
        dc.Clear()
        dc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
        dc.DrawRectangle(0, 0, w, h)

        if sys.platform == "darwin":
            dc.SetFont(wx.Font(12, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL,
                               wx.FONTWEIGHT_BOLD))
        else:
            dc.SetFont(wx.Font(8, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL,
                               wx.FONTWEIGHT_BOLD))

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
    
        dc.SetBrush(wx.Brush(KEYBOARD_BACKGROUND_COLOUR, wx.SOLID))
        dc.SetPen(wx.Pen("#AAAAAA", width=1, style=wx.SOLID))
        dc.DrawRectangle(self.offRec)
        dc.DrawRectangle(self.holdRec)

        dc.SetTextForeground("#000000")
        dc.DrawText("oct", self.offRec[0] + 3, 15)
        x1, y1 = self.offRec[0], self.offRec[1]
        dc.SetBrush(wx.Brush("#000000", wx.SOLID))
        if sys.platform == "darwin":
            dc.DrawPolygon([wx.Point(x1 + 3, 36), wx.Point(x1 + 10, 29),
                            wx.Point(x1 + 17, 36)])
            self.offUpRec = wx.Rect(x1, 28, x1 + 20, 10)
            dc.DrawPolygon([wx.Point(x1 + 3, 55), wx.Point(x1 + 10, 62),
                            wx.Point(x1 + 17, 55)])
            self.offDownRec = wx.Rect(x1, 54, x1 + 20, 10)
        else:
            dc.DrawPolygon([wx.Point(x1 + 5, 38), wx.Point(x1 + 12, 31),
                            wx.Point(x1 + 19, 38)])
            self.offUpRec = wx.Rect(x1, 30, x1 + 20, 10)
            dc.DrawPolygon([wx.Point(x1 + 5, 57), wx.Point(x1 + 12, 64),
                            wx.Point(x1 + 19, 57)])
            self.offDownRec = wx.Rect(x1, 56, x1 + 20, 10)
            
        dc.DrawText("%d" % int(self.offset / 12), x1 + 9, 41)
    
        if self.hold:
            dc.SetTextForeground("#0000CC")
        else:
            dc.SetTextForeground("#000000")
        for i, c in enumerate("HOLD"):
            dc.DrawText(c, self.holdRec[0] + 8, int(self.holdRec[3] / 6) * i + 15)

if __name__ == "__main__":
    def ppp(note):
        print(note)

    app = wx.App()
    frame = wx.Frame(None, title="Piano keyboard test", size=(650, 110))
    keyboard = Keyboard(frame, outFunction=ppp)
    #keyboard.Bind(EVT_PYO_GUI_KEYBOARD, ppp)
    frame.Show()
    app.MainLoop()
