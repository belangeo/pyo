# encoding: utf-8
"""
This module defines GUI widgets built with tkinter.

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
import math
import sys
import tkinter as tk

# constants for platform displays with Tk
if sys.platform.startswith("linux"):
    Y_OFFSET = 0
    VM_OFFSET = 2
elif sys.platform == "win32":
    Y_OFFSET = 3
    VM_OFFSET = 1
else:
    Y_OFFSET = 4
    VM_OFFSET = 0


def ptoi(pix):
    # return pix
    return "%fi" % (pix / 96)


######################################################################
### Multisliders
######################################################################
class MultiSlider(tk.Frame):
    """
    Draw multiple sliders on a canvas.

    :Args:
        master: tk.Frame
            Parent frame.
        init: list of floats
            Initial values.
        key: str
            Name of the attribute to control.
        command: callable
            Function called with values as argument.

    """

    def __init__(self, master, init, key, command):
        tk.Frame.__init__(self, master, bd=0, relief=tk.FLAT)
        self._values = init
        self._nchnls = len(init)
        self._key = key
        self._command = command
        self._lines = []
        self._height = 16
        self.canvas = tk.Canvas(
            self, height=self._height * self._nchnls + 1, width=225, relief=tk.FLAT, bd=0, bg="#BCBCAA"
        )
        w = self.canvas.winfo_width()
        for i in range(self._nchnls):
            x = int(self._values[i] * w)
            y = self._height * i + Y_OFFSET
            rect = self.canvas.create_rectangle(0, y, x, y + self._height - 1, width=0, fill="#121212")
            self._lines.append(rect)
        self.canvas.bind("<Button-1>", self.clicked)
        self.canvas.bind("<Motion>", self.move)
        self.canvas.bind("<Configure>", self.size)
        self.canvas.grid(sticky=tk.E + tk.W)
        self.columnconfigure(0, weight=1)
        self.grid()

    def size(self, event):
        "Method called when resizing the window."
        w = self.canvas.winfo_width()
        for i in range(len(self._lines)):
            y = self._height * i + Y_OFFSET
            x = self._values[i] * w
            self.canvas.coords(self._lines[i], 0, y, x, y + self._height - 1)

    def clicked(self, event):
        "Method called when clicking on the window."
        self.update(event)

    def move(self, event):
        "Method called on left mouse moion."
        if event.state == 0x0100:
            slide = (event.y - Y_OFFSET) // self._height
            if 0 <= slide < len(self._lines):
                self.update(event)

    def update(self, event):
        "Update and send current values."
        w = self.canvas.winfo_width()
        slide = (event.y - Y_OFFSET) // self._height
        val = event.x / float(w)
        self._values[slide] = val
        y = self._height * slide + Y_OFFSET
        self.canvas.coords(self._lines[slide], 0, y, event.x, y + self._height - 1)
        self._command(self._key, self._values)


######################################################################
### Control window for PyoObject
######################################################################
class Command(object):
    "Command event used to control a slider value."

    def __init__(self, func, key):
        self.func = func
        self.key = key

    def __call__(self, value):
        self.func(self.key, value)


class PyoObjectControl(tk.Frame):
    """
    Create a slider/multislider control window.

    :Args:
        master: tk.Frame
            Parent frame.
        obj: PyoObjectBase
            The object to control attributes.
        map_list: list of SLMap
            The list of attribute definitions.

    """

    def __init__(self, master=None, obj=None, map_list=None):
        tk.Frame.__init__(self, master, bd=1, relief=tk.GROOVE)
        from .controls import SigTo

        self.bind("<Destroy>", self._destroy)
        self._obj = obj
        self._map_list = map_list
        self._sliders = []
        self._excluded = []
        self._values = {}
        self._displays = {}
        self._maps = {}
        self._sigs = {}
        self._res = {}
        for i, m in enumerate(self._map_list):
            key, init, res, dataOnly = m.name, m.init, m.res, m.dataOnly
            # filters PyoObjects
            if type(init) not in [list, float, int]:
                self._excluded.append(key)
            else:
                self._maps[key] = m
                self._res[key] = res
                # label (param name)
                if dataOnly:
                    label = tk.Label(self, height=1, width=10, highlightthickness=0, text=key + " *")
                else:
                    label = tk.Label(self, height=1, width=10, highlightthickness=0, text=key)
                label.grid(row=i, column=0)
                # create and pack slider
                if not isinstance(init, list):
                    slider = tk.Scale(
                        self,
                        command=Command(self.setval, key),
                        orient=tk.HORIZONTAL,
                        relief=tk.GROOVE,
                        from_=0.0,
                        to=1.0,
                        showvalue=False,
                        resolution=0.0001,
                        bd=1,
                        length=225,
                        troughcolor="#BCBCAA",
                        width=12,
                    )
                    self._sliders.append(slider)
                    self._sliders[-1].set(m.set(init))
                    disp_height = 1
                else:
                    self._sliders.append(MultiSlider(self, [m.set(x) for x in init], key, self.setval))
                    disp_height = len(init)
                self._sliders[-1].grid(row=i, column=1, sticky=tk.E + tk.W)
                # display of numeric values
                textvar = tk.StringVar(self)
                display = tk.Label(self, height=disp_height, width=10, highlightthickness=0, textvariable=textvar)
                display.grid(row=i, column=2)
                self._displays[key] = textvar
                if not isinstance(init, list):
                    self._displays[key].set("%.4f" % init)
                else:
                    self._displays[key].set("\n".join(["%.4f" % i for i in init]))
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
        # padding
        top = self.winfo_toplevel()
        top.rowconfigure(0, weight=1)
        top.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)
        self.grid(ipadx=5, ipady=5, sticky=tk.E + tk.W)

    def _destroy(self, event):
        for m in self._map_list:
            key = m.name
            if key not in self._excluded:
                setattr(self._obj, key, self._values[key])
                del self._sigs[key]

    def setval(self, key, x):
        "Set a new value 'x' to attribute 'key'."
        if not isinstance(x, list):
            value = self._maps[key].get(float(x))
            self._displays[key].set("%.4f" % value)
        else:
            value = [self._maps[key].get(float(y)) for y in x]
            self._displays[key].set("\n".join(["%.4f" % i for i in value]))

        if self._res[key].startswith("i"):
            value = int(value)

        if key in self._values:
            self._values[key] = value
            setattr(self._sigs[key], "value", value)
        else:
            setattr(self._obj, key, value)


######################################################################
### View window for PyoTableObject
######################################################################
class ViewTable(tk.Frame):
    "PyoTableObject display."

    def __init__(self, master=None, samples=None):
        tk.Frame.__init__(self, master, bd=1, relief=tk.GROOVE)
        self.width = 500
        self.height = 200
        self.half_height = self.height // 2
        self.canvas = tk.Canvas(self, height=self.height, width=self.width, relief=tk.SUNKEN, bd=1, bg="#EFEFEF")
        self.canvas.create_line(
            0, self.half_height + Y_OFFSET, self.width, self.half_height + Y_OFFSET, fill="grey", dash=(4, 2)
        )
        self.canvas.create_line(*samples)
        self.canvas.grid()
        self.grid(ipadx=10, ipady=10)


######################################################################
## View window for PyoMatrixObject
#####################################################################
class ViewMatrix(tk.Frame):
    "PyoMatrixObject display."

    def __init__(self, master=None, samples=None, size=None):
        tk.Frame.__init__(self, master, bd=1, relief=tk.GROOVE)
        self.width = size[0]
        self.height = size[1]
        self.canvas = tk.Canvas(self, width=self.width, height=self.height, relief=tk.SUNKEN, bd=1, bg="#EFEFEF")
        for i in range(self.width * self.height):
            x = i % self.width
            y = i // self.width
            x1 = x + Y_OFFSET
            y1 = y + Y_OFFSET
            x2 = x + Y_OFFSET + 1
            y2 = y + Y_OFFSET + 1
            amp = int(samples[i])
            amp = hex(amp).replace("0x", "")
            if len(amp) == 1:
                amp = "0%s" % amp
            amp = "#%s%s%s" % (amp, amp, amp)
            self.canvas.create_line(x1, y1, x2, y2, fill=amp)
        self.canvas.grid()
        self.grid(ipadx=0, ipady=0)


######################################################################
### Server Object User Interface (Tk)
######################################################################
class ServerGUI(tk.Frame):
    "Server's graphical interface."

    def __init__(
        self,
        master=None,
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
        getIsBooted=None,
        getIsStarted=None,
    ):
        tk.Frame.__init__(self, master, padx=10, pady=10, bd=2, relief=tk.GROOVE)
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
        self.getIsBooted = getIsBooted
        self.getIsStarted = getIsStarted
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
        "Interface builder."
        row = 0
        self.startStringVar = tk.StringVar(self)
        self.startStringVar.set("Start")
        self.startButton = tk.Button(self, textvariable=self.startStringVar, command=self.start)
        self.startButton.grid(ipadx=5)

        self.recStringVar = tk.StringVar(self)
        self.recStringVar.set("Rec Start")
        self.recButton = tk.Button(self, textvariable=self.recStringVar, command=self.record)
        self.recButton.grid(ipadx=5, row=row, column=1)

        self.quitButton = tk.Button(self, text="Quit", command=self.on_quit)
        self.quitButton.grid(ipadx=5, row=row, column=2)
        row += 1

        self.ampScale = tk.Scale(
            self,
            command=self.setAmp,
            digits=4,
            label="Amplitude (dB)",
            orient=tk.HORIZONTAL,
            relief=tk.GROOVE,
            from_=-60.0,
            to=18.0,
            resolution=0.01,
            bd=1,
            length=ptoi(250),
            troughcolor="#BCBCAA",
            width=ptoi(10),
        )
        self.ampScale.set(20.0 * math.log10(self.amp))
        self.ampScale.grid(ipadx=ptoi(5), ipady=ptoi(5), row=row, column=0, columnspan=3)
        row += 1

        if self.meter:
            self.vumeter = tk.Canvas(
                self, height=ptoi(5 * self.nchnls + 1), width=ptoi(250), relief=tk.FLAT, bd=0, bg="#323232"
            )
            self.green = []
            self.yellow = []
            self.red = []
            for i in range(self.nchnls):
                y = 5 * (i + 1) + 1 - VM_OFFSET
                vum = self.vumeter
                self.green.append(
                    vum.create_line(
                        0,
                        ptoi(y),
                        ptoi(1),
                        ptoi(y),
                        width=ptoi(4),
                        fill="green",
                        dash=(9, 1),
                        dashoff=ptoi(6 + VM_OFFSET),
                    )
                )
                self.yellow.append(
                    vum.create_line(
                        ptoi(self.B1),
                        ptoi(y),
                        ptoi(self.B1),
                        ptoi(y),
                        width=ptoi(4),
                        fill="yellow",
                        dash=(9, 1),
                        dashoff=ptoi(9),
                    )
                )
                self.red.append(
                    vum.create_line(
                        ptoi(self.B2),
                        ptoi(y),
                        ptoi(self.B2),
                        ptoi(y),
                        width=ptoi(4),
                        fill="red",
                        dash=(9, 1),
                        dashoff=0,
                    )
                )
            self.vumeter.grid(ipadx=ptoi(5), row=row, column=0, columnspan=3)
            row += 1

        if self.timer:
            self.timer_label = tk.Label(self, text="Elapsed time (h:m:s:ms)")
            self.timer_label.grid(ipadx=0, row=row, column=0, columnspan=3)
            row += 1
            self.timer_strvar = tk.StringVar(self, " 00 : 00 : 00 : 000")
            self.timetext = tk.Label(self, textvariable=self.timer_strvar)
            self.timetext.grid(ipadx=5, row=row, column=0, columnspan=3)
            row += 1

        if self.locals is not None:
            self.interp_label = tk.Label(self, text="Interpreter")
            self.interp_label.grid(ipadx=0, row=row, column=0, columnspan=3)
            row += 1
            self.text = tk.Text(
                self, height=1, width=33, bd=1, relief=tk.RIDGE, highlightthickness=0, spacing1=2, spacing3=2
            )
            self.text.grid(ipadx=5, row=row, column=0, columnspan=3)
            self.text.bind("<Return>", self.getText)
            self.text.bind("<Up>", self.getPrev)
            self.text.bind("<Down>", self.getNext)

    def on_quit(self):
        "Clean up on quit."
        if self.getIsBooted():
            self.shutdown()
        self.quit()

    def getPrev(self, event):
        "Get previous command in history."
        self.text.delete("1.0", tk.END)
        self._histo_count -= 1
        if self._histo_count < 0:
            self._histo_count = 0
        self.text.insert("1.0", self._history[self._histo_count])
        return "break"

    def setTime(self, *args):
        "Display the current server time."
        txt = " %02d : %02d : %02d : %03d" % (args[0], args[1], args[2], args[3])
        self.timer_strvar.set(txt)

    def getNext(self, event):
        "Get next command in history."
        self.text.delete("1.0", tk.END)
        self._histo_count += 1
        if self._histo_count >= len(self._history):
            self._histo_count = len(self._history)
        else:
            self.text.insert("1.0", self._history[self._histo_count])
        return "break"

    def getText(self, event):
        "Retrieve text from the interpreter widget."
        source = self.text.get("1.0", tk.END)
        self.text.delete("1.0", tk.END)
        exec(source, self.locals)
        self._history.append(source)
        self._histo_count = len(self._history)
        return "break"

    def start(self, justSet=False):
        "Start/stop the server."
        if self._started is False:
            if not justSet:
                self.startf()
            self._started = True
            self.startStringVar.set("Stop")
            self.quitButton.configure(state=tk.DISABLED)
        else:
            if self.getIsStarted():
                self.stopf()
            self._started = False
            self.startStringVar.set("Start")
            self.quitButton.configure(state=tk.NORMAL)

    def record(self):
        "Record sound on disk."
        if self._recstarted is False:
            self.recstartf()
            self._recstarted = True
            self.recStringVar.set("Rec Stop")
        else:
            self.recstopf()
            self._recstarted = False
            self.recStringVar.set("Rec Start")

    def setAmp(self, value):
        "Update server's global amplitude value."
        self.ampf(math.pow(10.0, float(value) * 0.05))

    def setRms(self, *args):
        "Update the vumeter."
        for i in range(self.nchnls):
            y = 5 * (i + 1) + 1 - VM_OFFSET
            db = math.log10(args[i] + 0.00001) * 0.2 + 1.0
            amp = int(db * 250)
            if amp <= self.B1:
                self.vumeter.coords(self.green[i], 0, ptoi(y), ptoi(amp), ptoi(y))
                self.vumeter.coords(self.yellow[i], ptoi(self.B1), ptoi(y), ptoi(self.B1), ptoi(y))
                self.vumeter.coords(self.red[i], ptoi(self.B2), ptoi(y), ptoi(self.B2), ptoi(y))
            elif amp <= self.B2:
                self.vumeter.coords(self.green[i], 0, ptoi(y), ptoi(self.B1), ptoi(y))
                self.vumeter.coords(self.yellow[i], ptoi(self.B1), ptoi(y), ptoi(amp), ptoi(y))
                self.vumeter.coords(self.red[i], ptoi(self.B2), ptoi(y), ptoi(self.B2), ptoi(y))
            else:
                self.vumeter.coords(self.green[i], 0, ptoi(y), ptoi(self.B1), ptoi(y))
                self.vumeter.coords(self.yellow[i], ptoi(self.B1), ptoi(y), ptoi(self.B2), ptoi(y))
                self.vumeter.coords(self.red[i], ptoi(self.B2), ptoi(y), ptoi(amp), ptoi(y))

    def setStartButtonState(self, state):
        if state:
            self._started = True
            self.startStringVar.set("Stop")
            self.quitButton.configure(state=tk.DISABLED)
        else:
            self._started = False
            self.startStringVar.set("Start")
            self.quitButton.configure(state=tk.NORMAL)
