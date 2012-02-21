#! /usr/bin/env python
# encoding: utf-8
"""
Display the sonogram of a sound using a PyoMatrixObject.
A better display can be achieved by using a custom drawing.

After the playback ending, call "m.view()" from the 
interpreter widget of the Server window to show the spectrum.

"""
from pyo import *
import wx, time

s = Server().boot()

a = SfPlayer('../snds/baseballmajeur_m.aif', loop=True, mul=.7).mix(1).out()

size = 512
m = DataTable(size=size)

trig = Metro(sampsToSec(size*2)).play()
fin = FFT(a*100, overlaps=1, wintype=0)
mag = Sqrt(fin["real"]*fin["real"] + fin["imag"]*fin["imag"])
rec = TrigTableRec(mag*2-1, trig, m, 0).play()

class MyFrame(wx.Frame):
    def __init__(self):
        wx.Frame.__init__(self, None, -1, size=(250,200))
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.panel = wx.Panel(self)
        self.samples = [(0,0,0,0)]*100
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_CLOSE, self.close)
        self.Show()

    def close(self, evt):
        s.stop()
        time.sleep(.5)
        self.Destroy()

    def draw(self, samples):
        self.samples = [(samples[i], 0, samples[i], samples[i+3]) for i in range(0, len(samples)/2, 4)]
        wx.CallAfter(self.Refresh)

    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)
        dc.SetBrush(wx.Brush("#000000"))
        dc.SetPen(wx.Pen('#FFFFFF', width=1, style=wx.SOLID))
        dc.Clear()
        dc.DrawRectangle(0,0,w,h)
        dc.DrawLineList(self.samples)

s.start()

app = wx.PySimpleApp()
f = MyFrame()

def getSpec():
    f.draw(m[0].getViewTable())

func = TrigFunc(trig, getSpec)

app.MainLoop()
