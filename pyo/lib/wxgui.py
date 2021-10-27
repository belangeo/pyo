"""
The classes in this module are based on internal classes that where
originally designed to help the creation of graphical tools for the
control and the visualization of audio signals. WxPython must be installed
under the current Python distribution to access these classes.

"""
from ._widgets import PYO_USE_WX

if not PYO_USE_WX:
    NO_WX_MESSAGE = "WxPython must be installed on the system to use pyo's wx widgets."

    class PyoGuiControlSlider:
        def __init__(self, *args, **kwargs):
            raise Exception(NO_WX_MESSAGE)

    class PyoGuiVuMeter:
        def __init__(self, *args, **kwargs):
            raise Exception(NO_WX_MESSAGE)

    class PyoGuiGrapher:
        def __init__(self, *args, **kwargs):
            raise Exception(NO_WX_MESSAGE)

    class PyoGuiMultiSlider:
        def __init__(self, *args, **kwargs):
            raise Exception(NO_WX_MESSAGE)

    class PyoGuiSpectrum:
        def __init__(self, *args, **kwargs):
            raise Exception(NO_WX_MESSAGE)

    class PyoGuiScope:
        def __init__(self, *args, **kwargs):
            raise Exception(NO_WX_MESSAGE)

    class PyoGuiSndView:
        def __init__(self, *args, **kwargs):
            raise Exception(NO_WX_MESSAGE)

    class PyoGuiKeyboard:
        def __init__(self, *args, **kwargs):
            raise Exception(NO_WX_MESSAGE)


else:
    import wx
    import wx.lib.newevent
    from ._wxwidgets import ControlSlider, VuMeter, Grapher, DataMultiSlider, HRangeSlider
    from ._wxwidgets import SpectrumPanel, ScopePanel, SndViewTablePanel, Keyboard

    if "phoenix" not in wx.version():
        wx.QueueEvent = wx.PostEvent

    # Custom events
    PyoGuiControlSliderEvent, EVT_PYO_GUI_CONTROL_SLIDER = wx.lib.newevent.NewEvent()
    PyoGuiGrapherEvent, EVT_PYO_GUI_GRAPHER = wx.lib.newevent.NewEvent()
    PyoGuiMultiSliderEvent, EVT_PYO_GUI_MULTI_SLIDER = wx.lib.newevent.NewEvent()
    PyoGuiSndViewMousePositionEvent, EVT_PYO_GUI_SNDVIEW_MOUSE_POSITION = wx.lib.newevent.NewEvent()
    PyoGuiSndViewSelectionEvent, EVT_PYO_GUI_SNDVIEW_SELECTION = wx.lib.newevent.NewEvent()
    PyoGuiKeyboardEvent, EVT_PYO_GUI_KEYBOARD = wx.lib.newevent.NewEvent()

    class PyoGuiControlSlider(ControlSlider):
        """
        Floating-point control slider.

        :Parent: wx.Panel

        :Events:

            EVT_PYO_GUI_CONTROL_SLIDER
                Sent after any change of the slider position. The current
                value of the slider can be retrieve with the `value`
                attribute of the generated event. The object itself can be
                retrieve with the `object` attribute of the event and the
                object's id with the `id` attrbute.

        :Args:

            parent: wx.Window
                The parent window.
            minvalue: float
                The minimum value of the slider.
            maxvalue: float
                The maximum value of the slider.
            init: float, optional
                The initial value of the slider. If None, the slider
                inits to the minimum value. Defaults to None.
            pos: tuple, optional
                The slider's position in pixel (x, y). Defaults to (0, 0).
            size: tuple, optional
                The slider's size in pixel (x, y). Defaults to (200, 16).
            log: boolean, optional
                If True, creates a logarithmic slider (minvalue must be
                greater than 0). Defaults to False.
            integer: boolean, optional
                If True, creates an integer slider. Defaults to False.
            powoftwo: boolean, optional
                If True, creates a power-of-two slider (log is automatically
                False and integer is True). If True, minvalue and maxvalue
                must be exponents to base 2 but init is a real power-of-two
                value. Defaults to False.
            orient: {wx.HORIZONTAL or wx.VERTICAL}, optional
                The slider's orientation. Defaults to wx.HORIZONTAL.

        """

        def __init__(
            self,
            parent,
            minvalue,
            maxvalue,
            init=None,
            pos=(0, 0),
            size=(200, 16),
            log=False,
            integer=False,
            powoftwo=False,
            orient=wx.HORIZONTAL,
        ):
            super(PyoGuiControlSlider, self).__init__(
                parent,
                minvalue,
                maxvalue,
                init,
                pos,
                size,
                log,
                self._outFunction,
                integer,
                powoftwo,
                parent.GetBackgroundColour(),
                orient,
            )

        def _outFunction(self, value):
            evt = PyoGuiControlSliderEvent(value=value, id=self.GetId(), object=self)
            wx.QueueEvent(self, evt)

        def enable(self):
            """
            Enable the slider for user input.

            """
            super(PyoGuiControlSlider, self).Enable()

        def disable(self):
            """
            Disable the slider for user input.

            """
            super(PyoGuiControlSlider, self).Disable()

        def setValue(self, x, propagate=True):
            """
            Sets a new value to the slider.

            :Args:

                x: int or float
                    The controller number.
                propagate: boolean, optional
                    If True, an event will be sent after the call.

            """
            super(PyoGuiControlSlider, self).SetValue(x, propagate)

        def setMidiCtl(self, x, propagate=True):
            """
            Sets the midi controller number to show on the slider.

            :Args:

                x: int
                    The controller number.
                propagate: boolean, optional
                    If True, an event will be sent after the call.

            """
            super(PyoGuiControlSlider, self).setMidiCtl(x, propagate)

        def setRange(self, minvalue, maxvalue):
            """
            Sets new minimum and maximum values.

            :Args:

                minvalue: int or float
                    The new minimum value.
                maxvalue: int or float
                    The new maximum value.

            """
            super(PyoGuiControlSlider, self).setRange(minvalue, maxvalue)

        def getValue(self):
            """
            Returns the current value of the slider.

            """
            return super(PyoGuiControlSlider, self).GetValue()

        def getMidiCtl(self):
            """
            Returns the midi controller number, if any, assigned to the slider.

            """
            return super(PyoGuiControlSlider, self).getMidiCtl()

        def getMinValue(self):
            """
            Returns the current minimum value.

            """
            return super(PyoGuiControlSlider, self).getMinValue()

        def getMaxValue(self):
            """
            Returns the current maximum value.

            """
            return super(PyoGuiControlSlider, self).getMaxValue()

        def getInit(self):
            """
            Returns the initial value.

            """
            return super(PyoGuiControlSlider, self).getInit()

        def getRange(self):
            """
            Returns minimum and maximum values as a list.

            """
            return super(PyoGuiControlSlider, self).getRange()

        def isInteger(self):
            """
            Returns True if the slider manage only integer, False otherwise.

            """
            return self.integer

        def isLog(self):
            """
            Returns True if the slider is logarithmic, False otherwise.

            """
            return self.log

        def isPowOfTwo(self):
            """
            Returns True if the slider manage only power-of-two values, False otherwise.

            """
            return self.powoftwo

    class PyoGuiVuMeter(VuMeter):
        """
        Multi-channels Vu Meter.

        When registered as the Server's meter, its internal method `setRms`
        will be called each buffer size with a list of normalized amplitudes
        as argument. The `setRms` method can also be registered as the
        function callback of a PeakAmp object.

        :Parent: wx.Panel

        :Args:

            parent: wx.Window
                The parent window.
            nchnls: int, optional
                The initial number of channels of the meter. Defaults to 2.
            pos: wx.Point, optional
                Window position in pixels. Defaults to (0, 0).
            size: tuple, optional
                The meter's size in pixels (x, y). Defaults to (200, 11).
            orient: {wx.HORIZONTAL or wx.VERTICAL}, optional
                The meter's orientation. Defaults to wx.HORIZONTAL.
            style: int, optional
                Window style (see wx.Window documentation). Defaults to 0.

        """

        def __init__(self, parent, nchnls=2, pos=(0, 0), size=(200, 11), orient=wx.HORIZONTAL, style=0):
            super(PyoGuiVuMeter, self).__init__(parent, size, nchnls, orient, pos, style)

        def setNchnls(self, nchnls):
            """
            Sets the number of channels of the meter.

            :Args:

                nchnls: int
                    The number of channels.

            """
            super(PyoGuiVuMeter, self).setNumSliders(nchnls)

    class PyoGuiGrapher(Grapher):
        """
        Multi-modes break-points function editor.

        :Parent: wx.Panel

        :Events:

            EVT_PYO_GUI_GRAPHER
                Sent after any change of the grapher function. The current
                list of points of the grapher can be retrieve with the `value`
                attribute of the generated event. The object itself can be
                retrieve with the `object` attribute of the event and the
                object's id with the `id` attrbute.

        :Args:

            parent: wx.Window
                The parent window.
            xlen: int, optional
                The length, in samples, of the grapher. Defaults to 8192.
            yrange: two-values tuple, optional
                A tuple indicating the minimum and maximum values of the Y-axis.
                Defaults to (0, 1).
            init: list of two-values tuples, optional
                The initial break-points function set as normalized values.
                A point is defined with its X and Y positions as a tuple.
                Defaults to [(0.0, 0.0), (1.0, 1.0)].
            mode: int, optional
                The grapher mode definning how line segments will be draw.
                Possible modes are:

                0. linear (default)
                1. cosine
                2. exponential (uses `exp` and `inverse` arguments)
                3. curve       (uses `tension` and `bias` arguments)
                4. logarithmic
                5. logarithmic cosine
            exp: int or float, optional
                The exponent factor for an exponential graph. Defaults to 10.0.
            inverse: boolean, optional
                If True, downward slope will be inversed. Useful to create
                biexponential curves. Defaults to True.
            tension: int or float, optional
                Curvature at the known points. 1 is high, 0 normal, -1 is low.
                Defaults to 0.
            bias: int or float, optional
                Curve attraction (for each segments) toward bundary points.
                0 is even, positive is towards first point, negative is towards
                the second point. Defaults to 0.
            pos: wx.Point, optional
                Window position in pixels. Defaults to (0, 0).
            size: wx.Size, optional
                Window size in pixels. Defaults to (300, 200).
            style: int, optional
                Window style (see wx.Window documentation). Defaults to 0.

        """

        def __init__(
            self,
            parent,
            xlen=8192,
            yrange=(0, 1),
            init=[(0.0, 0.0), (1.0, 1.0)],
            mode=0,
            exp=10,
            inverse=True,
            tension=0,
            bias=0,
            pos=(0, 0),
            size=(300, 200),
            style=0,
        ):
            super(PyoGuiGrapher, self).__init__(
                parent, xlen, yrange, init, mode, exp, inverse, tension, bias, self._outFunction, pos, size, style
            )

        def _outFunction(self, value):
            evt = PyoGuiGrapherEvent(value=value, id=self.GetId(), object=self)
            wx.QueueEvent(self, evt)

        def _refresh(self):
            self.Refresh()
            self.sendValues()

        def reset(self):
            """
            Resets the points to the initial state.

            """
            super(PyoGuiGrapher, self).reset()

        def getPoints(self):
            """
            Returns the current normalized points of the grapher.

            """
            return super(PyoGuiGrapher, self).getPoints()

        def getValues(self):
            """
            Returns the current points, according to Y-axis range, of the grapher.

            """
            return super(PyoGuiGrapher, self).getValues()

        def setPoints(self, pts):
            """
            Sets a new group of normalized points in the grapher.

            :Args:

                pts: list of two-values tuples
                    New normalized (between 0 and 1) points.

            """
            self.points = [pt for pt in pts]
            self._refresh()

        def setValues(self, vals):
            """
            Sets a new group of points, according to Y-axis range, in the grapher.

            :Args:

                vals: list of two-values tuples
                    New real points.

            """
            self.points = [self.valuesToPoint(val) for val in vals]
            self._refresh()

        def setYrange(self, yrange):
            """
            Sets a new Y-axis range to the grapher.

            :Args:

                yrange: two-values tuple
                    New Y-axis range.

            """
            vals = self.getValues()
            self.yrange = yrange
            self.setValues(vals)

        def setInitPoints(self, pts):
            """
            Sets a new initial normalized points list to the grapher.

            :Args:

                pts: list of two-values tuples
                    New normalized (between 0 and 1) initial points.

            """
            super(PyoGuiGrapher, self).setInitPoints(pts)

        def setMode(self, x):
            """
            Changes the grapher's mode.

            :Args:

                x: int
                    New mode. Possible modes are:
                        0. linear (default)
                        1. cosine
                        2. exponential (uses `exp` and `inverse` arguments)
                        3. curve       (uses `tension` and `bias` arguments)
                        4. logarithmic
                        5. logarithmic cosine

            """
            self.mode = x
            self._refresh()

        def setExp(self, x):
            """
            Changes the grapher's exponent factor for exponential graph.

            :Args:

                x: float
                    New exponent factor.

            """
            self.exp = x
            self._refresh()

        def setInverse(self, x):
            """
            Changes the grapher's inverse boolean for exponential graph.

            :Args:

                x: boolean
                    New inverse factor.

            """
            self.inverse = x
            self._refresh()

        def setTension(self, x):
            """
            Changes the grapher's tension factor for curved graph.

            :Args:

                x: float
                    New tension factor.

            """
            self.tension = x
            self._refresh()

        def setBias(self, x):
            """
            Changes the grapher's bias factor for curved graph.

            :Args:

                x: float
                    New bias factor.

            """
            self.bias = x
            self._refresh()

    class PyoGuiMultiSlider(DataMultiSlider):
        """
        Data multi-sliders editor.

        :Parent: wx.Panel

        :Events:

            EVT_PYO_GUI_MULTI_SLIDER
                Sent after any change of the multi-sliders values. The current
                list of values of the multi-sliders can be retrieve with the
                `value` attribute of the generated event. The object itself can
                be retrieve with the `object` attribute of the event and the
                object's id with the `id` attrbute.

        :Args:

            parent: wx.Window
                The parent window.
            xlen: int, optional
                The number of sliders in the multi-sliders. Defaults to 16.
            yrange: two-values tuple
                A tuple indicating the minimum and maximum values of the Y-axis.
                Defaults to (0, 1).
            init: list values, optional
                The initial list of values of the multi-sliders.
                Defaults to None, meaning all sliders initialized to the
                minimum value.
            pos: wx.Point, optional
                Window position in pixels. Defaults to (0, 0).
            size: wx.Size, optional
                Window size in pixels. Defaults to (300, 200).
            style: int, optional
                Window style (see wx.Window documentation). Defaults to 0.

        """

        def __init__(self, parent, xlen=16, yrange=(0, 1), init=None, pos=(0, 0), size=(300, 200), style=0):
            if init is None:
                init = [yrange[0]] * xlen
            else:
                if len(init) < xlen:
                    init += [yrange[0]] * (xlen - len(init))
                elif len(init) > xlen:
                    init = init[:xlen]
            super(PyoGuiMultiSlider, self).__init__(parent, init, yrange, self._outFunction, pos, size, style)

        def _outFunction(self, value):
            evt = PyoGuiMultiSliderEvent(value=value, id=self.GetId(), object=self)
            wx.QueueEvent(self, evt)

        def reset(self):
            """
            Resets the sliders to their initial state.

            """
            super(PyoGuiMultiSlider, self).reset()

        def getValues(self):
            """
            Returns the current values of the sliders.

            """
            return [v for v in self.values]

        def setValues(self, vals):
            """
            Sets new values to the sliders.

            :Args:

                vals: list of values
                    New values.

            """
            wx.CallAfter(super(PyoGuiMultiSlider, self).update, vals)

        def setYrange(self, yrange):
            """
            Sets a new Y-axis range to the multi-sliders.

            :Args:

                yrange: two-values tuple
                    New Y-axis range.

            """
            self.yrange = (float(yrange[0]), float(yrange[1]))
            self.Refresh()

    class PyoGuiSpectrum(SpectrumPanel):
        """
        Frequency spectrum display.

        This widget should be used with the Spectrum object, which measures
        the magnitude of an input signal versus frequency within a user
        defined range. It can show both magnitude and frequency on linear
        or logarithmic scale.

        To create the bridge between the analyzer and the display, the
        Spectrum object must be registered in the PyoGuiSpectrum object
        with the setAnalyzer(obj) method. The Spectrum object will
        automatically call the update(points) method to refresh the display.

        :Parent: wx.Panel

        :Args:

            parent: wx.Window
                The parent window.
            lowfreq: int or float, optional
                The lowest frequency, in Hz, to display on the X-axis.
                Defaults to 0.
            highfreq: int or float, optional
                The highest frequency, in Hz, to display on the X-axis.
                Defaults to 22050.
            fscaling: int, optional
                The frequency scaling on the X-axis. 0 means linear, 1 means
                logarithmic. Defaults to 0.
            mscaling: int, optional
                The magnitude scaling on the Y-axis. 0 means linear, 1 means
                logarithmic. Defaults to 0.
            pos: wx.Point, optional
                Window position in pixels. Defaults to (0, 0).
            size: wx.Size, optional
                Window size in pixels. Defaults to (300, 200).
            style: int, optional
                Window style (see wx.Window documentation). Defaults to 0.

        """

        def __init__(
            self, parent, lowfreq=0, highfreq=22050, fscaling=0, mscaling=0, pos=(0, 0), size=(300, 200), style=0
        ):
            super(PyoGuiSpectrum, self).__init__(parent, 1, lowfreq, highfreq, fscaling, mscaling, pos, size, style)

        def update(self, points):
            """
            Display updating method.

            This method is automatically called by the audio analyzer
            object (Spectrum) with points to draw as arguments. The points
            are already formatted for the current drawing surface to save
            CPU cycles.

            The method setAnalyzer(obj) must be used to register the audio
            analyzer object.

            :Args:

                points: list of list of tuples
                    A list containing n-channels list of tuples. A tuple
                    is a point (X-Y coordinates) to draw.

            """
            wx.CallAfter(self.setImage, points)

        def setAnalyzer(self, object):
            """
            Register an audio analyzer object (Spectrum).

            :Args:

                object: Spectrum object
                    The audio object performing the frequency analysis.

            """
            self.obj = object
            self.obj.setFunction(self.update)
            self.obj.setLowFreq(self.lowfreq)
            self.obj.setHighFreq(self.highfreq)
            self.obj.setFscaling(self.fscaling)
            self.obj.setMscaling(self.mscaling)
            self.setChnls(len(self.obj))

        def setLowFreq(self, x):
            """
            Changes the lowest frequency of the display.

            This method propagates the value to the audio analyzer.

            :Args:

                x: int or float
                    New lowest frequency.

            """
            if self.obj is not None:
                self.obj.setLowFreq(x)
            super(PyoGuiSpectrum, self).setLowFreq(x)

        def setHighFreq(self, x):
            """
            Changes the highest frequency of the display.

            This method propagates the value to the audio analyzer.

            :Args:

                x: int or float
                    New highest frequency.

            """
            if self.obj is not None:
                self.obj.setHighFreq(x)
            super(PyoGuiSpectrum, self).setHighFreq(x)

        def setFscaling(self, x):
            """
            Changes the frequency scaling (X-axis) of the display.

            This method propagates the value to the audio analyzer.

            :Args:

                x: int
                    0 means linear scaling, 1 means logarithmic scaling.

            """
            if self.obj is not None:
                self.obj.setFscaling(x)
            super(PyoGuiSpectrum, self).setFscaling(x)

        def setMscaling(self, x):
            """
            Changes the magnitude scaling (Y-axis) of the display.

            This method propagates the value to the audio analyzer.

            :Args:

                x: int
                    0 means linear scaling, 1 means logarithmic scaling.

            """
            if self.obj is not None:
                self.obj.setMscaling(x)
            super(PyoGuiSpectrum, self).setMscaling(x)

    class PyoGuiScope(ScopePanel):
        """
        Oscilloscope display.

        This widget should be used with the Scope object, which computes
        the waveform of an input signal to display on a GUI.

        To create the bridge between the analyzer and the display, the
        Scope object must be registered in the PyoGuiScope object with
        the setAnalyzer(obj) method. The Scope object will automatically
        call the update(points) method to refresh the display.

        :Parent: wx.Panel

        :Args:

            parent: wx.Window
                The parent window.
            length: float, optional
                Length, in seconds, of the waveform segment displayed on
                the window. Defaults to 0.05.
            gain: float, optional
                Linear gain applied to the signal to be displayed.
                Defaults to 0.67.
            pos: wx.Point, optional
                Window position in pixels. Defaults to (0, 0).
            size: wx.Size, optional
                Window size in pixels. Defaults to (300, 200).
            style: int, optional
                Window style (see wx.Window documentation). Defaults to 0.

        """

        def __init__(self, parent, length=0.05, gain=0.67, pos=(0, 0), size=(300, 200), style=0):
            super(PyoGuiScope, self).__init__(parent, None, pos, size, style)
            super(PyoGuiScope, self).setLength(length)
            super(PyoGuiScope, self).setGain(gain)

        def update(self, points):
            """
            Display updating method.

            This method is automatically called by the audio analyzer
            object (Scope) with points to draw as arguments. The points
            are already formatted for the current drawing surface to save
            CPU cycles.

            The method setAnalyzer(obj) must be used to register the audio
            analyzer object.

            :Args:

                points: list of list of tuples
                    A list containing n-channels list of tuples. A tuple
                    is a point (X-Y coordinates) to draw.

            """
            wx.CallAfter(self.setImage, points)

        def setAnalyzer(self, object):
            """
            Register an audio analyzer object (Scope).

            :Args:

                object: Scope object
                    The audio object performing the waveform analysis.

            """
            self.obj = object
            self.obj.setFunction(self.update)
            self.obj.setLength(self.length)
            self.obj.setGain(self.gain)
            self.setChnls(len(self.obj))

        def setLength(self, x):
            """
            Changes the length, in seconds, of the displayed audio segment.

            This method propagates the value to the audio analyzer.

            :Args:

                x: float
                    New segment length in seconds.

            """
            if self.obj is not None:
                self.obj.setLength(x)
            super(PyoGuiScope, self).setLength(x)

        def setGain(self, x):
            """
            Changes the gain applied to the input signal.

            This method propagates the value to the audio analyzer.

            :Args:

                x: float
                    New linear gain.

            """
            if self.obj is not None:
                self.obj.setGain(x)
            super(PyoGuiScope, self).setGain(x)

    # left-click + drag ===> event position
    # right-click + drag ===> highlight a selection + event selection (start, end)
    # Shift + right-click + drag ===> move the selection + event selection (start, end)
    # Ctrl + right-click ===> delete the selection + event selection (0.0, 1.0)
    class PyoGuiSndView(wx.Panel):
        """
        Soundfile display.

        This widget should be used with the SndTable object, which keeps
        soundfile in memory and computes the waveform to display on the GUI.

        To create the bridge between the audio memory and the display, the
        SndTable object must be registered in the PyoGuiSndView object with
        the setTable(object) method.

        The SndTable object will automatically call the update() method to
        refresh the display when the table is modified.

        :Parent: wx.Panel

        :Events:

            EVT_PYO_GUI_SNDVIEW_MOUSE_POSITION
                Sent when the mouse is moving on the panel with the left
                button pressed. The `value` attribute of the event will
                hold the normalized position of the mouse into the sound.
                For X-axis value, 0.0 is the beginning of the sound and 1.0
                is the end of the sound. For the Y-axis, 0.0 is the bottom
                of the panel and 1.0 is the top. The object itself can be
                retrieve with the `object` attribute of the event and the
                object's id with the `id` attrbute.
            EVT_PYO_GUI_SNDVIEW_SELECTION
                Sent when a new region is selected on the panel. A new
                selection is created with a Right-click and drag on the panel.
                The current selection can be moved with Shift+Right-click and
                drag. Ctrl+Right-click (Cmd on OSX) remove the selected region.
                The `value` attribute of the event will hold the normalized
                selection as a tuple (min, max). 0.0 means the beginning of
                the sound and 1.0 means the end of the sound. The object itself
                can be retrieve with the `object` attribute of the event and the
                object's id with the `id` attrbute.

        :Args:

            parent: wx.Window
                The parent window.
            pos: wx.Point, optional
                Window position in pixels. Defaults to (0, 0).
            size: wx.Size, optional
                Window size in pixels. Defaults to (300, 200).
            style: int, optional
                Window style (see wx.Window documentation). Defaults to 0.

        """

        def __init__(self, parent, pos=(0, 0), size=(300, 200), style=0):
            wx.Panel.__init__(self, parent, pos=pos, size=size, style=style)
            box = wx.BoxSizer(wx.VERTICAL)
            self._curzoom = (0.0, 1.0)
            self.sndview = SndViewTablePanel(self, None, self._position_callback, self._select_callback)
            box.Add(self.sndview, 1, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, 5)
            self.zoom = HRangeSlider(
                self,
                minvalue=0,
                maxvalue=1,
                valtype="float",
                function=self._setZoom,
                backColour=parent.GetBackgroundColour(),
            )
            box.Add(self.zoom, 0, wx.EXPAND | wx.LEFT | wx.RIGHT, 5)
            self.SetSizer(box)

        def _setZoom(self, values=None):
            if values is None:
                values = self._curzoom
            dur = self.sndview.getDur()
            self.sndview.setBegin(dur * values[0])
            self.sndview.setEnd(dur * values[1])
            self._curzoom = values
            self.update()

        def _position_callback(self, pos):
            evt = PyoGuiSndViewMousePositionEvent(value=pos, id=self.GetId(), object=self)
            wx.QueueEvent(self, evt)

        def _select_callback(self, selection):
            selection = (max(0.0, min(selection)), min(max(selection), 1.0))
            evt = PyoGuiSndViewSelectionEvent(value=selection, id=self.GetId(), object=self)
            wx.QueueEvent(self, evt)

        def __del__(self):
            if self.sndview.obj is not None:
                self.sndview.obj._setViewFrame(None)

        def update(self):
            """
            Display updating method.

            This method is automatically called by the audio memory
            object (SndTable) when the table is modified.

            The method setTable(obj) must be used to register the audio
            memory object.

            """
            wx.CallAfter(self.sndview.setImage)

        def setTable(self, object):
            """
            Register an audio memory object (SndTable).

            :Args:

                object: SndTable object
                    The audio table keeping the sound in memory.

            """
            object._setViewFrame(self)
            self.sndview.obj = object
            self.sndview.setBegin(0.0)
            self.sndview.setEnd(object.getDur(False))
            self.sndview.chnls = len(object)
            self.update()

        def setSelection(self, start, stop):
            """
            Changes the selected region.

            This method will trigger a EVT_PYO_GUI_SNDVIEW_SELECTION event
            with a tuple (start, stop) as value.

            :Args:

                start: float
                    The starting point of the selected region. This value
                    must be normalized between 0 and 1 (0 is the beginning
                    of the sound, 1 is the end).
                stop: float
                    The ending point of the selected region. This value
                    must be normalized between 0 and 1 (0 is the beginning
                    of the sound, 1 is the end).

            """
            self.sndview.setSelection(start, stop)

        def resetSelection(self):
            """
            Removes the selected region.

            This method will trigger a EVT_PYO_GUI_SNDVIEW_SELECTION event
            with a tuple (0.0, 1.0) as value.

            """
            self.sndview.resetSelection()

    class PyoGuiKeyboard(Keyboard):
        """
        Virtual MIDI keyboard.

        :Parent: wx.Panel

        :Events:

            EVT_PYO_GUI_KEYBOARD
                Sent whenever a note change on the keyboard. The `value`
                attribute of the event will hold a (pitch, velocity) tuple.
                The object itself can be retrieve with the `object`
                attribute of the event and the object's id with the `id` attrbute.

        :Args:

            parent: wx.Window
                The parent window.
            poly: int, optional
                Maximum number of notes that can be held at the same time.
                Defaults to 64.
            pos: wx.Point, optional
                Window position in pixels. Defaults to (0, 0).
            size: wx.Size, optional
                Window size in pixels. Defaults to (300, 200).
            style: int, optional
                Window style (see wx.Window documentation). Defaults to 0.

        """

        def __init__(self, parent, poly=64, pos=(0, 0), size=(600, 100), style=0):
            super(PyoGuiKeyboard, self).__init__(parent, wx.ID_ANY, pos, size, poly, self._outFunction, style)

        def _outFunction(self, value):
            evt = PyoGuiKeyboardEvent(value=value, id=self.GetId(), object=self)
            wx.QueueEvent(self, evt)

        def getCurrentNotes(self):
            """
            Returns a list of the current notes.

            """
            return super(PyoGuiKeyboard, self).getCurrentNotes()

        def reset(self):
            """
            Resets the keyboard state.

            """
            return super(PyoGuiKeyboard, self).reset()

        def setPoly(self, poly):
            """
            Sets the maximum number of notes that can be held at the same time.

            :Args:

                poly: int
                    New maximum number of notes held at once.

            """
            super(PyoGuiKeyboard, self).setPoly(poly)
