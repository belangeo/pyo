"""
Demo script for showing how GUI classes from pyo can be used to build
audio programs with graphical interface.

"""
import wx, time, random
from pyo import *

NCHNLS = 2

server = Server(nchnls=NCHNLS).boot()
server.start()

### A basic audio process ###
snd = SndTable([SNDS_PATH + "/transparent.aif"] * NCHNLS)
m = Metro(0.125, poly=1).play()
am = Iter(m, [1, 0, 0, 0] * 4)
t2 = ExpTable([(0, 1), (4096, 10), (8191, 10)], exp=4)
q = TrigEnv(m, t2, m.time)
pos = TrigLinseg(m, [(0.0, 0.0), (m.time, 1, 0)])
n = Pointer(snd, pos, mul=am)
fr = SigTo(1000, time=0.05, init=1000)
f = ButBP(n, freq=fr, q=q).out()
# pa = PeakAmp(f)
sp = Spectrum(f)
sc = Scope(f)


class MyFrame(wx.Frame):
    def __init__(self, parent, title, pos=(50, 50), size=(1000, 700)):
        wx.Frame.__init__(self, parent, -1, title, pos, size)

        self.Bind(wx.EVT_CLOSE, self.on_quit)

        self.panel = wx.Panel(self)
        vmainsizer = wx.BoxSizer(wx.VERTICAL)
        mainsizer = wx.BoxSizer(wx.HORIZONTAL)
        leftbox = wx.BoxSizer(wx.VERTICAL)
        midbox = wx.BoxSizer(wx.VERTICAL)
        rightbox = wx.BoxSizer(wx.VERTICAL)

        ### PyoGuiControlSlider - logarithmic scale ###
        sizer1 = self.createFreqSlider()

        ### PyoGuiControlSlider - dB scale & VuMeter ###
        sizer2 = self.createOutputBox()

        ### PyoGuiGrapher - Filter's Q automation ###
        sizer3 = self.createGrapher()

        ### PyoGuiMultiSlider - Step Sequencer ###
        sizer4 = self.createMultiSlider()

        ### PyoGuiSpectrum - Frequency display ###
        sizer5 = self.createSpectrum()

        ### PyoGuiScope - oscilloscope display ###
        sizer6 = self.createScope()

        ### PyoGuiSndView - Soundfile display ###
        sizer7 = self.createSndView()

        keyboard = PyoGuiKeyboard(self.panel)
        keyboard.Bind(EVT_PYO_GUI_KEYBOARD, self.onMidiNote)

        leftbox.Add(sizer1, 0, wx.ALL | wx.EXPAND, 5)
        leftbox.Add(sizer3, 1, wx.ALL | wx.EXPAND, 5)
        leftbox.Add(sizer4, 1, wx.ALL | wx.EXPAND, 5)

        midbox.Add(sizer5, 1, wx.ALL | wx.EXPAND, 5)
        midbox.Add(sizer6, 1, wx.ALL | wx.EXPAND, 5)

        rightbox.Add(sizer2, 1, wx.ALL | wx.EXPAND, 5)

        mainsizer.Add(leftbox, 1, wx.ALL | wx.EXPAND, 5)
        mainsizer.Add(midbox, 1, wx.ALL | wx.EXPAND, 5)
        mainsizer.Add(rightbox, 0, wx.ALL | wx.EXPAND, 5)
        vmainsizer.Add(mainsizer, 1, wx.ALL | wx.EXPAND, 5)
        vmainsizer.Add(sizer7, 1, wx.ALL | wx.EXPAND, 5)
        vmainsizer.Add(keyboard, 0, wx.ALL | wx.EXPAND, 5)
        self.panel.SetSizerAndFit(vmainsizer)

    def on_quit(self, evt):
        server.stop()
        time.sleep(0.25)
        self.Destroy()

    def onMidiNote(self, evt):
        print("Pitch:    %d" % evt.value[0])
        print("Velocity: %d" % evt.value[1])

    def createFreqSlider(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        label = wx.StaticText(self.panel, -1, "PyoGuiControlSlider: filter's center frequency (log scale)")
        sizer.Add(label, 0, wx.CENTER | wx.ALL, 5)
        self.freq = PyoGuiControlSlider(
            parent=self.panel,
            minvalue=20,
            maxvalue=20000,
            init=1000,
            pos=(0, 0),
            size=(200, 16),
            log=True,
            integer=False,
            powoftwo=False,
            orient=wx.HORIZONTAL,
        )
        # print(self.freq.getRange())
        # print(self.freq.isPowOfTwo())
        self.freq.Bind(EVT_PYO_GUI_CONTROL_SLIDER, self.changeFreq)
        sizer.Add(self.freq, 0, wx.ALL | wx.EXPAND, 5)
        return sizer

    def createOutputBox(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        label = wx.StaticText(self.panel, -1, "dB slider - PyoGuiVuMeter")
        sizer.Add(label, 0, wx.CENTER | wx.ALL, 5)
        sizer1 = wx.BoxSizer(wx.HORIZONTAL)
        self.amp = PyoGuiControlSlider(
            parent=self.panel,
            minvalue=-60,
            maxvalue=18,
            init=-12,
            pos=(0, 0),
            size=(200, 16),
            log=False,
            integer=False,
            powoftwo=False,
            orient=wx.VERTICAL,
        )
        self.amp.Bind(EVT_PYO_GUI_CONTROL_SLIDER, self.changeGain)
        self.meter = PyoGuiVuMeter(
            parent=self.panel, nchnls=NCHNLS, pos=(0, 0), size=(5 * NCHNLS, 200), orient=wx.VERTICAL, style=0,
        )
        self.meter.setNchnls(8)
        # Register the VuMeter in the Server object.
        server.setMeter(self.meter)
        # or register its `setRms` method in a PeakAmp object.
        # pa.setFunction(self.meter.setRms)

        sizer1.Add(self.amp, 0, wx.ALL | wx.EXPAND, 5)
        sizer1.Add(self.meter, 0, wx.ALL | wx.EXPAND, 5)
        sizer.Add(sizer1, 1, wx.CENTER | wx.ALL, 5)
        return sizer

    def createGrapher(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        label = wx.StaticText(self.panel, -1, "PyoGuiGrapher: Filter's Q automation")
        sizer.Add(label, 0, wx.CENTER | wx.ALL, 5)
        self.graph = PyoGuiGrapher(
            parent=self.panel,
            xlen=8192,
            yrange=(1.0, 10.0),
            init=[(0.0, 0.0), (1.0, 1.0)],
            mode=2,
            exp=t2.exp,
            inverse=True,
            tension=0.75,
            bias=8.0,
            size=(300, 100),
            style=0,
        )
        self.graph.setValues(t2.getPoints())
        self.graph.setYrange((0.1, 20))
        self.graph.setInverse(False)
        self.graph.Bind(EVT_PYO_GUI_GRAPHER, self.changeGraph)
        sizer.Add(self.graph, 1, wx.ALL | wx.EXPAND, 5)
        return sizer

    def createMultiSlider(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        label = wx.StaticText(self.panel, -1, "PyoGuiMultiSlider: Step Sequencer")
        sizer.Add(label, 0, wx.CENTER | wx.ALL, 5)
        self.steps = PyoGuiMultiSlider(
            parent=self.panel, xlen=16, yrange=(0, 1), init=[1, 0, 0, 0] * 4, size=(300, 100), style=0,
        )
        self.steps.setYrange((0, 2))
        self.steps.setValues([random.uniform(0, 2) for i in range(16)])
        self.steps.Bind(EVT_PYO_GUI_MULTI_SLIDER, self.changeSteps)
        sizer.Add(self.steps, 1, wx.ALL | wx.EXPAND, 5)
        return sizer

    def createSpectrum(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        label = wx.StaticText(self.panel, -1, "PyoGuiSpectrum: Frequency display")
        sizer.Add(label, 0, wx.CENTER | wx.ALL, 5)
        self.spectrum = PyoGuiSpectrum(
            parent=self.panel, lowfreq=0, highfreq=22050, fscaling=1, mscaling=1, size=(300, 150), style=0,
        )
        self.spectrum.setAnalyzer(sp)
        sizer.Add(self.spectrum, 1, wx.ALL | wx.EXPAND, 5)
        return sizer

    def createScope(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        label = wx.StaticText(self.panel, -1, "PyoGuiScope: Oscilloscope display")
        sizer.Add(label, 0, wx.CENTER | wx.ALL, 5)
        self.scope = PyoGuiScope(parent=self.panel, length=0.05, gain=1, size=(300, 150), style=0)
        self.scope.setAnalyzer(sc)
        self.scope.setLength(0.05)
        self.scope.setGain(0.67)
        sizer.Add(self.scope, 1, wx.ALL | wx.EXPAND, 5)
        return sizer

    def createSndView(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        label = wx.StaticText(self.panel, -1, "PyoGuiSndView: Soundfile display")
        sizer.Add(label, 0, wx.CENTER | wx.ALL, 5)
        self.sndview = PyoGuiSndView(parent=self.panel, size=(300, 200), style=0)
        self.sndview.setTable(snd)
        self.sndview.setSelection(0.51, 0.76)
        self.sndview.Bind(EVT_PYO_GUI_SNDVIEW_MOUSE_POSITION, self.mousePos)
        self.sndview.Bind(EVT_PYO_GUI_SNDVIEW_SELECTION, self.sndSelection)
        sizer.Add(self.sndview, 1, wx.ALL | wx.EXPAND, 5)
        return sizer

    def changeFreq(self, evt):
        fr.value = evt.value

    def changeGain(self, evt):
        am.mul = pow(10, evt.value * 0.05)

    def changeGraph(self, evt):
        t2.replace(evt.value)

    def changeSteps(self, evt):
        am.setChoice(evt.value)

    def mousePos(self, evt):
        print((evt.value))

    def sndSelection(self, evt):
        pos.replace([(0.0, evt.value[0]), (m.time, evt.value[1])])


app = wx.App(False)
mainFrame = MyFrame(None, title="Test Pyo GUI objects")
mainFrame.Show()
app.MainLoop()
