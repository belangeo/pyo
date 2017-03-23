import sys
import wx

KEYBOARD_BACKGROUND_COLOUR = "#CCCCCC"

class Keyboard(wx.Panel):
    def __init__(self, parent, id=wx.ID_ANY, pos=wx.DefaultPosition,
                 size=wx.DefaultSize, poly=64, style=wx.TAB_TRAVERSAL):
        wx.Panel.__init__(self, parent, id, pos, size, style)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)  
        self.SetBackgroundColour(KEYBOARD_BACKGROUND_COLOUR)
        self.parent = parent

        self.poly = poly
        self.gap = 0
        self.offset = 12
        self.w1 = 15
        self.w2 = self.w1 // 2 + 1
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

        wx.CallAfter(self.setRects)
   
    def getNotes(self):
        notes = []
        for key in self.whiteSelected:
            notes.append((self.white[key % 7] + key // 7 * 12  + self.offset,
                          127 - self.whiteVelocities[key]))
        for key in self.blackSelected:
            notes.append((self.black[key % 5] + key // 5 * 12  + self.offset,
                          127 - self.blackVelocities[key]))
        notes.sort()
        return notes

    def reset(self):
        self.whiteSelected = []
        self.blackSelected = []
        self.whiteVelocities = {}
        self.blackVelocities = {}
        wx.CallAfter(self.Refresh)
    
    def setPoly(self, poly):
        self.poly = poly
    
    def setRects(self):
        w, h = self.GetSize()
        self.offRec = wx.Rect(w - 55, 0, 21, h)
        self.holdRec = wx.Rect(w - 34, 0, 21, h)
        num = w // self.w1
        self.gap = w - num * self.w1
        self.whiteKeys = [wx.Rect(i * self.w1, 0, self.w1 - 1, h) for i in range(num)]
        self.blackKeys = []
        height2 = h * 4 // 7
        for i in range(num // 7 + 1):
            space2 = self.w1 * 7 * i
            off = self.w1 // 2 + space2 + 3
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
        self.setRects()
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
            self.keyPressed = None
            wx.CallAfter(self.Refresh)

    def MouseDown(self, evt):
        w,h = self.GetSize()
        pos = evt.GetPosition()
        if self.holdRec.Contains(pos):
            if self.hold:
                self.hold = 0
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
                    pit = self.black[i % 5] + i // 5 * 12  + self.offset
                    if i in self.blackSelected:
                        self.blackSelected.remove(i)
                        del self.blackVelocities[i]
                        vel = 0
                    else:
                        hb = h * 4 // 7
                        vel = (hb - pos[1]) * 127 // hb
                        if total < self.poly:
                            self.blackSelected.append(i)
                            self.blackVelocities[i] = int(127 - vel)
                    note = (pit, vel)
                    scanWhite = False
                    break
            if scanWhite:
                for i, rec in enumerate(self.whiteKeys):
                    if rec.Contains(pos):
                        pit = self.white[i % 7] + i // 7 * 12  + self.offset
                        if i in self.whiteSelected:
                            self.whiteSelected.remove(i)
                            del self.whiteVelocities[i]
                            vel = 0
                        else:
                            vel = (h - pos[1]) * 127 // h
                            if total < self.poly:
                                self.whiteSelected.append(i)
                                self.whiteVelocities[i] = int(127 - vel)
                        note = (pit, vel)
                        break
            if note:
                if note[1] == 0:
                    pass
                elif total < self.poly:
                    pass
        else:
            self.keyPressed = None
            for i, rec in enumerate(self.blackKeys):
                if rec.Contains(pos):
                    pit = self.black[i % 5] + i // 5 * 12  + self.offset
                    if i not in self.blackSelected:
                        hb = h * 4 // 7
                        vel = (hb - pos[1]) * 127 // hb
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
                        pit = self.white[i % 7] + i // 7 * 12 + self.offset
                        if i not in self.whiteSelected:
                            vel = (h - pos[1]) * 127 // h
                            if total < self.poly:
                                self.whiteSelected.append(i)
                                self.whiteVelocities[i] = int(127 - vel)
                        note = (pit, vel)
                        self.keyPressed = (i, pit)
                        break
            if note:
                if total < self.poly:
                    pass
        wx.CallAfter(self.Refresh)
    
    def OnPaint(self, evt):
        w,h = self.GetSize()
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
                dc.SetPen(wx.Pen("#FFFFFF", width=1, style=wx.SOLID))
                dc.DrawRectangle(rec)
            if i == (35 - (7 * (self.offset // 12))):
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
        dc.DrawRectangle(wx.Rect(w - 14, 0, 14, h))
        
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
            dc.DrawPolygon([wx.Point(x1 + 3, 38), wx.Point(x1 + 10, 31),
                            wx.Point(x1 + 17, 38)])
            self.offUpRec = wx.Rect(x1, 30, x1 + 20, 10)
            dc.DrawPolygon([wx.Point(x1 + 3, 57), wx.Point(x1 + 10, 64),
                            wx.Point(x1 + 17, 57)])
            self.offDownRec = wx.Rect(x1, 56, x1 + 20, 10)
            
        dc.DrawText("%d" % (self.offset // 12), x1 + 7, 41)
    
        if self.hold:
            dc.SetTextForeground("#0000CC")
        else:
            dc.SetTextForeground("#000000")
        for i, c in enumerate("HOLD"):
            dc.DrawText(c, self.holdRec[0] + 6, self.holdRec[3] // 6 * i + 15)
        
        dc.SetBrush(wx.Brush(KEYBOARD_BACKGROUND_COLOUR, wx.SOLID))
        dc.SetPen(wx.Pen(KEYBOARD_BACKGROUND_COLOUR, width=1, style=wx.SOLID))
        dc.DrawRectangle(w - self.gap, 0, self.gap, h)
        dc.SetPen(wx.Pen("#000000", width=1, style=wx.SOLID))
        dc.DrawLine(0, 3, w, 3)
        dc.SetPen(wx.Pen("#444444", width=1, style=wx.SOLID))
        dc.DrawLine(0, 2, w, 2)
        dc.SetPen(wx.Pen("#888888", width=1, style=wx.SOLID))
        dc.DrawLine(0, 1, w, 1)
        dc.SetPen(wx.Pen("#CCCCCC", width=1, style=wx.SOLID))
        dc.DrawLine(0, 0, w, 0)

if __name__ == "__main__":
    app = wx.App()
    frame = wx.Frame(None, title="Piano keyboard test", size=(650, 110))
    keyboard = Keyboard(frame)
    frame.Show()
    app.MainLoop()
