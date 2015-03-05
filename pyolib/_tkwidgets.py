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
import math, sys, os
from Tkinter import *

try:
    from PIL import Image, ImageDraw, ImageTk
except:
    pass

# constants for platform displays with Tk
if sys.platform == 'linux2':
    Y_OFFSET = 0
    VM_OFFSET = 2
elif sys.platform == 'win32':
    Y_OFFSET = 3
    VM_OFFSET = 1
else:
    Y_OFFSET = 4
    VM_OFFSET = 0

######################################################################
### Multisliders
######################################################################
class MultiSlider(Frame):
    def __init__(self, master, init, key, command):
        Frame.__init__(self, master, bd=0, relief=FLAT)
        self._values = init
        self._nchnls = len(init)
        self._key = key
        self._command = command
        self._lines = []
        self._height = 16
        self.canvas = Canvas(self, height=self._height*self._nchnls+1,
                            width=225, relief=FLAT, bd=0, bg="#BCBCAA")
        w = self.canvas.winfo_width()
        for i in range(self._nchnls):
            x = int(self._values[i] * w)
            y = self._height * i + Y_OFFSET
            self._lines.append(self.canvas.create_rectangle(0, y, x,
                                y+self._height-1, width=0, fill="#121212"))
        self.canvas.bind("<Button-1>", self.clicked)
        self.canvas.bind("<Motion>", self.move)
        self.canvas.bind("<Configure>", self.size)
        self.canvas.grid(sticky=E+W)
        self.columnconfigure(0, weight=1)
        self.grid()

    def size(self, event):
        w = self.canvas.winfo_width()
        for i in range(len(self._lines)):
            y = self._height * i + Y_OFFSET
            x = self._values[i] * w
            self.canvas.coords(self._lines[i], 0, y, x, y+self._height-1)

    def clicked(self, event):
        self.update(event)

    def move(self, event):
        if event.state == 0x0100:
            slide = (event.y - Y_OFFSET) / self._height
            if 0 <= slide < len(self._lines):
                self.update(event)

    def update(self, event):
        w = self.canvas.winfo_width()
        slide = (event.y - Y_OFFSET) / self._height
        val = event.x / float(w)
        self._values[slide] = val
        y = self._height * slide + Y_OFFSET
        self.canvas.coords(self._lines[slide], 0, y, event.x, y+self._height-1)
        self._command(self._key, self._values)

######################################################################
### Control window for PyoObject
######################################################################
class Command:
    def __init__(self, func, key):
        self.func = func
        self.key = key

    def __call__(self, value):
        self.func(self.key, value)

class PyoObjectControl(Frame):
    def __init__(self, master=None, obj=None, map_list=None):
        Frame.__init__(self, master, bd=1, relief=GROOVE)
        from controls import SigTo
        self.bind('<Destroy>', self._destroy)
        self._obj = obj
        self._map_list = map_list
        self._sliders = []
        self._excluded = []
        self._values = {}
        self._displays = {}
        self._maps = {}
        self._sigs = {}
        for i, m in enumerate(self._map_list):
            key, init = m.name, m.init
            # filters PyoObjects
            if type(init) not in [ListType, FloatType, IntType]:
                self._excluded.append(key)
            else:
                self._maps[key] = m
                # label (param name)
                label = Label(self, height=1, width=10, highlightthickness=0, text=key)
                label.grid(row=i, column=0)
                # create and pack slider
                if type(init) != ListType:
                    self._sliders.append(Scale(self, command=Command(self.setval, key),
                                  orient=HORIZONTAL, relief=GROOVE, from_=0., to=1., showvalue=False,
                                  resolution=.0001, bd=1, length=225, troughcolor="#BCBCAA", width=12))
                    self._sliders[-1].set(m.set(init))
                    disp_height = 1
                else:
                    self._sliders.append(MultiSlider(self, [m.set(x) for x in init], key, self.setval))
                    disp_height = len(init)
                self._sliders[-1].grid(row=i, column=1, sticky=E+W)
                # display of numeric values
                textvar = StringVar(self)
                display = Label(self, height=disp_height, width=10, highlightthickness=0, textvariable=textvar)
                display.grid(row=i, column=2)
                self._displays[key] = textvar
                if type(init) != ListType:
                    self._displays[key].set("%.4f" % init)
                else:
                    self._displays[key].set("\n".join(["%.4f" % i for i in init]))
                # set obj attribute to PyoObject SigTo
                self._sigs[key] = SigTo(init, .025, init)
                refStream = self._obj.getBaseObjects()[0]._getStream()
                server = self._obj.getBaseObjects()[0].getServer()
                for k in range(len(self._sigs[key].getBaseObjects())):
                    curStream = self._sigs[key].getBaseObjects()[k]._getStream()
                    server.changeStreamPosition(refStream, curStream)
                setattr(self._obj, key, self._sigs[key])
        # padding
        top = self.winfo_toplevel()
        top.rowconfigure(0, weight=1)
        top.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)
        self.grid(ipadx=5, ipady=5, sticky=E+W)

    def _destroy(self, event):
        for m in self._map_list:
            key = m.name
            if key not in self._excluded:
                setattr(self._obj, key, self._values[key])
                del self._sigs[key]

    def setval(self, key, x):
        if type(x) != ListType:
            value = self._maps[key].get(float(x))
            self._displays[key].set("%.4f" % value)
        else:
            value = [self._maps[key].get(float(y)) for y in x]
            self._displays[key].set("\n".join(["%.4f" % i for i in value]))

        self._values[key] = value
        setattr(self._sigs[key], "value", value)

######################################################################
### View window for PyoTableObject
######################################################################
class ViewTable_withPIL(Frame):
    def __init__(self, master=None, samples=None):
        Frame.__init__(self, master, bd=1, relief=GROOVE)
        self.width = 500
        self.height = 200
        self.half_height = self.height / 2
        self.canvas = Canvas(self, height=self.height, width=self.width, relief=SUNKEN, bd=1, bg="#EFEFEF")
        print Image
        im = Image.new("L", (self.width, self.height), 255)
        draw = ImageDraw.Draw(im)
        draw.line(samples, fill=0, width=1)
        self.img = ImageTk.PhotoImage(im)
        self.canvas.create_image(self.width/2,self.height/2,image=self.img)
        self.canvas.create_line(0, self.half_height+2, self.width, self.half_height+2, fill='grey', dash=(4,2))
        self.canvas.grid()
        self.grid(ipadx=10, ipady=10)

class ViewTable_withoutPIL(Frame):
    def __init__(self, master=None, samples=None):
        Frame.__init__(self, master, bd=1, relief=GROOVE)
        self.width = 500
        self.height = 200
        self.half_height = self.height / 2
        self.canvas = Canvas(self, height=self.height, width=self.width, relief=SUNKEN, bd=1, bg="#EFEFEF")
        self.canvas.create_line(0, self.half_height+Y_OFFSET, self.width, self.half_height+Y_OFFSET, fill='grey', dash=(4,2))
        self.canvas.create_line(*samples)
        self.canvas.grid()
        self.grid(ipadx=10, ipady=10)

######################################################################
## View window for PyoMatrixObject
#####################################################################
class ViewMatrix_withPIL(Frame):
    def __init__(self, master=None, samples=None, size=None):
        Frame.__init__(self, master, bd=1, relief=GROOVE)
        self.canvas = Canvas(self, width=size[0], height=size[1], relief=SUNKEN, bd=1, bg="#EFEFEF")
        im = Image.new("L", size, None)
        im.putdata(samples)
        self.img = ImageTk.PhotoImage(im)
        self.canvas.create_image(size[0]/2+Y_OFFSET,size[1]/2+Y_OFFSET,image=self.img)
        self.canvas.grid()
        self.grid(ipadx=0, ipady=0)

class ViewMatrix_withoutPIL(Frame):
    def __init__(self, master=None, samples=None, size=None):
        Frame.__init__(self, master, bd=1, relief=GROOVE)
        self.width = size[0]
        self.height = size[1]
        self.canvas = Canvas(self, width=self.width, height=self.height, relief=SUNKEN, bd=1, bg="#EFEFEF")
        for i in range(self.width*self.height):
            x = i % self.width
            y = i / self.width
            x1 = x+Y_OFFSET
            y1 = y+Y_OFFSET
            x2 = x+Y_OFFSET+1
            y2 = y+Y_OFFSET+1
            amp = int(samples[i])
            amp = hex(amp).replace('0x', '')
            if len(amp) == 1:
                amp = "0%s" % amp
            amp = "#%s%s%s" % (amp, amp, amp)
            self.canvas.create_line(x1, y1, x2, y2, fill=amp)
        self.canvas.grid()
        self.grid(ipadx=0, ipady=0)

######################################################################
### Server Object User Interface (Tk)
######################################################################
class ServerGUI(Frame):
    def __init__(self, master=None, nchnls=2, startf=None, stopf=None, recstartf=None,
                recstopf=None, ampf=None, started=0, locals=None, shutdown=None, meter=True, timer=True, amp=1.):
        Frame.__init__(self, master, padx=10, pady=10, bd=2, relief=GROOVE)
        self.shutdown = shutdown
        self.locals = locals
        self.meter = meter
        self.timer = timer
        self.nchnls = nchnls
        self.startf = startf
        self.stopf = stopf
        self.recstartf = recstartf
        self.recstopf = recstopf
        self.ampf = ampf
        self.amp = amp
        self._started = False
        self._recstarted = False
        self.B1, self.B2 = 193 - VM_OFFSET, 244 - VM_OFFSET
        self._history = []
        self._histo_count = 0
        self.grid(ipadx=5)
        self.rowconfigure(0, pad=20)
        self.rowconfigure(1, pad=10)
        self.rowconfigure(2, pad=10)
        self.createWidgets()
        if started == 1:
            self.start(True)


    def createWidgets(self):
        row = 0
        self.startStringVar = StringVar(self)
        self.startStringVar.set('Start')
        self.startButton = Button(self, textvariable=self.startStringVar, command=self.start)
        self.startButton.grid(ipadx=5)

        self.recStringVar = StringVar(self)
        self.recStringVar.set('Rec Start')
        self.recButton = Button(self, textvariable=self.recStringVar, command=self.record)
        self.recButton.grid(ipadx=5, row=row, column=1)

        self.quitButton = Button(self, text='Quit', command=self.on_quit)
        self.quitButton.grid(ipadx=5, row=row, column=2)
        row += 1

        self.ampScale = Scale(self, command=self.setAmp, digits=4, label='Amplitude (dB)',
                              orient=HORIZONTAL, relief=GROOVE, from_=-60.0, to=18.0,
                              resolution=.01, bd=1, length=250, troughcolor="#BCBCAA", width=10)
        self.ampScale.set(20.0 * math.log10(self.amp))
        self.ampScale.grid(ipadx=5, ipady=5, row=row, column=0, columnspan=3)
        row += 1

        if self.meter:
            self.vumeter = Canvas(self, height=5*self.nchnls+1, width=250, relief=FLAT, bd=0, bg="#323232")
            self.green = []
            self.yellow = []
            self.red = []
            for i in range(self.nchnls):
                y = 5 * (i + 1) + 1 - VM_OFFSET
                self.green.append(self.vumeter.create_line(0, y, 1, y, width=4, fill='green', dash=(9,1), dashoff=6+VM_OFFSET))
                self.yellow.append(self.vumeter.create_line(self.B1, y, self.B1, y, width=4, fill='yellow', dash=(9,1), dashoff=9))
                self.red.append(self.vumeter.create_line(self.B2, y, self.B2, y, width=4, fill='red', dash=(9,1), dashoff=0))
            self.vumeter.grid(ipadx=5, row=row, column=0, columnspan=3)
            row += 1

        if self.timer:
            self.timer_label = Label(self, text='Elapsed time (h:m:s:ms)')
            self.timer_label.grid(ipadx=0, row=row, column=0, columnspan=3)
            row += 1
            self.timer_strvar = StringVar(self, " 00 : 00 : 00 : 000")
            self.timetext = Label(self, textvariable=self.timer_strvar)
            self.timetext.grid(ipadx=5, row=row, column=0, columnspan=3)
            row += 1

        if self.locals != None:
            self.interp_label = Label(self, text='Interpreter')
            self.interp_label.grid(ipadx=0, row=row, column=0, columnspan=3)
            row += 1
            self.text = Text(self, height=1, width=33, bd=1, relief=RIDGE, highlightthickness=0,
                            spacing1=2, spacing3=2)
            self.text.grid(ipadx=5, row=row, column=0, columnspan=3)
            self.text.bind("<Return>", self.getText)
            self.text.bind("<Up>", self.getPrev)
            self.text.bind("<Down>", self.getNext)


    def on_quit(self):
        self.shutdown()
        self.quit()

    def getPrev(self, event):
        self.text.delete("1.0", END)
        self._histo_count -= 1
        if self._histo_count < 0:
            self._histo_count = 0
        self.text.insert("1.0", self._history[self._histo_count])
        return "break"

    def setTime(self, *args):
        self.timer_strvar.set(" %02d : %02d : %02d : %03d" % (args[0], args[1], args[2], args[3]))

    def getNext(self, event):
        self.text.delete("1.0", END)
        self._histo_count += 1
        if self._histo_count >= len(self._history):
            self._histo_count = len(self._history)
        else:
            self.text.insert("1.0", self._history[self._histo_count])
        return "break"

    def getText(self, event):
        source = self.text.get("1.0", END)
        self.text.delete("1.0", END)
        exec source in self.locals
        self._history.append(source)
        self._histo_count = len(self._history)
        return "break"

    def start(self, justSet=False):
        if self._started == False:
            if not justSet:
                self.startf()
            self._started = True
            self.startStringVar.set('Stop')
            self.quitButton.configure(state = DISABLED)
        else:
            self.stopf()
            self._started = False
            self.startStringVar.set('Start')
            self.quitButton.configure(state = NORMAL)

    def record(self):
        if self._recstarted == False:
            self.recstartf()
            self._recstarted = True
            self.recStringVar.set('Rec Stop')
        else:
            self.recstopf()
            self._recstarted = False
            self.recStringVar.set('Rec Start')

    def setAmp(self, value):
        self.ampf(math.pow(10.0, float(value) * 0.05))

    def setRms(self, *args):
        for i in range(self.nchnls):
            y = 5 * (i + 1) + 1 - VM_OFFSET
            db = math.log10(args[i]+0.00001) * 0.2 + 1.
            amp = int(db*250)
            if amp <= self.B1:
                self.vumeter.coords(self.green[i], 0, y, amp, y)
                self.vumeter.coords(self.yellow[i], self.B1, y, self.B1, y)
                self.vumeter.coords(self.red[i], self.B2, y, self.B2, y)
            elif amp <= self.B2:
                self.vumeter.coords(self.green[i], 0, y, self.B1, y)
                self.vumeter.coords(self.yellow[i], self.B1, y, amp, y)
                self.vumeter.coords(self.red[i], self.B2, y, self.B2, y)
            else:
                self.vumeter.coords(self.green[i], 0, y, self.B1, y)
                self.vumeter.coords(self.yellow[i], self.B1, y, self.B2, y)
                self.vumeter.coords(self.red[i], self.B2, y, amp, y)