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
from ._core import *
from ._maps import *
from ._widgets import createGraphWindow, createDataGraphWindow, createSndViewTableWindow
from math import pi
import copy

######################################################################
### Tables
######################################################################
class HarmTable(PyoTableObject):
    """
    Harmonic waveform generator.

    Generates composite waveforms made up of weighted sums
    of simple sinusoids.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list: list, optional
            Relative strengths of the fixed harmonic partial numbers 1,2,3, etc.
            Defaults to [1].
        size: int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Square wave up to 9th harmonic
    >>> t = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    >>> a = Osc(table=t, freq=[199,200], mul=.2).out()

    """

    def __init__(self, list=[1.0, 0.0], size=8192):
        pyoArgsAssert(self, "lI", list, size)
        PyoTableObject.__init__(self, size)
        self._auto_normalize = False
        self._list = copy.deepcopy(list)
        self._base_objs = [HarmTable_base(self._list, size)]

    def autoNormalize(self, x):
        """
        Activate/deactivate automatic normalization when harmonics changed.

        :Args:

            x: boolean
                True for activating automatic normalization, False for
                deactivating it.

        """
        self._auto_normalize = x
        if self._auto_normalize:
            self.normalize()

    def replace(self, list):
        """
        Redraw the waveform according to a new set of harmonics
        relative strengths.

        :Args:

            list: list
                Relative strengths of the fixed harmonic partial
                numbers 1,2,3, etc.

        """
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        if self._auto_normalize:
            self.normalize()
        self.refreshView()

    def _get_current_data(self):
        # internal that returns the data to draw in a DataTableGrapher.
        return self._list

    def graph(self, yrange=(-1.0, 1.0), title=None, wxnoserver=False):
        """
        Opens a multislider window to control the data values.

        When editing the grapher with the mouse, the new values are
        sent to the object to replace the table content.

        :Args:

            yrange: tuple, optional
                Set the min and max values of the Y axis of the multislider.
                Defaults to (0.0, 1.0).
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        .. note::

            The number of bars in the graph is initialized to the length
            of the list of relative strentghs at the time the graph is
            created.

        """
        createDataGraphWindow(self, yrange, title, wxnoserver)

    @property
    def list(self):
        """list. Relative strengths of the fixed harmonic partial numbers."""
        return self._list

    @list.setter
    def list(self, x):
        self.replace(x)


class SawTable(PyoTableObject):
    """
    Sawtooth waveform generator.

    Generates sawtooth waveforms made up of fixed number of harmonics.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        order: int, optional
            Number of harmonics sawtooth is made of.
            Defaults to 10.
        size: int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = SawTable(order=12).normalize()
    >>> a = Osc(table=t, freq=[199,200], mul=.2).out()

    """

    def __init__(self, order=10, size=8192):
        pyoArgsAssert(self, "II", order, size)
        PyoTableObject.__init__(self, size)
        self._order = order
        list = [1.0 / i for i in range(1, (order + 1))]
        self._base_objs = [HarmTable_base(list, size)]

    def setOrder(self, x):
        """
        Change the `order` attribute and redraw the waveform.

        :Args:

            x: int
                New number of harmonics

        """
        pyoArgsAssert(self, "I", x)
        self._order = x
        list = [1.0 / i for i in range(1, (self._order + 1))]
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    @property
    def order(self):
        """int. Number of harmonics sawtooth is made of."""
        return self._order

    @order.setter
    def order(self, x):
        self.setOrder(x)


class SquareTable(PyoTableObject):
    """
    Square waveform generator.

    Generates square waveforms made up of fixed number of harmonics.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        order: int, optional
            Number of harmonics square waveform is made of. The waveform will
            contains `order` odd harmonics. Defaults to 10.
        size: int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = SquareTable(order=15).normalize()
    >>> a = Osc(table=t, freq=[199,200], mul=.2).out()

    """

    def __init__(self, order=10, size=8192):
        pyoArgsAssert(self, "II", order, size)
        PyoTableObject.__init__(self, size)
        self._order = order
        list = []
        for i in range(1, (order * 2)):
            if i % 2 == 1:
                list.append(1.0 / i)
            else:
                list.append(0.0)
        self._base_objs = [HarmTable_base(list, size)]

    def setOrder(self, x):
        """
        Change the `order` attribute and redraw the waveform.

        :Args:

            x: int
                New number of harmonics

        """
        pyoArgsAssert(self, "I", x)
        self._order = x
        list = []
        for i in range(1, (self._order * 2)):
            if i % 2 == 1:
                list.append(1.0 / i)
            else:
                list.append(0.0)
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    @property
    def order(self):
        """int. Number of harmonics square waveform is made of."""
        return self._order

    @order.setter
    def order(self, x):
        self.setOrder(x)


class TriangleTable(PyoTableObject):
    """
    Triangle waveform generator.

    Generates triangle waveforms made up of fixed number of harmonics.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        order: int, optional
            Number of harmonics triangle waveform is made of. The waveform will
            contains `order` odd harmonics. Defaults to 10.
        size: int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = TriangleTable(order=15).normalize()
    >>> a = Osc(table=t, freq=[199,200], mul=.2).out()

    """

    def __init__(self, order=10, size=8192):
        pyoArgsAssert(self, "II", order, size)
        PyoTableObject.__init__(self, size)
        self._order = order
        list = []
        ph = 1.0
        for i in range(1, (order * 2)):
            if i % 2 == 1:
                list.append(ph / pow(i, 2))
                ph *= -1
            else:
                list.append(0.0)
        self._base_objs = [HarmTable_base(list, size)]

    def setOrder(self, x):
        """
        Change the `order` attribute and redraw the waveform.

        :Args:

            x: int
                New number of harmonics

        """
        pyoArgsAssert(self, "I", x)
        self._order = x
        list = []
        ph = 1.0
        for i in range(1, (self._order * 2)):
            if i % 2 == 1:
                list.append(ph / pow(i, 2))
                ph *= -1
            else:
                list.append(0.0)
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    @property
    def order(self):
        """int. Number of harmonics triangle waveform is made of."""
        return self._order

    @order.setter
    def order(self, x):
        self.setOrder(x)


class ChebyTable(PyoTableObject):
    """
    Chebyshev polynomials of the first kind.

    Uses Chebyshev coefficients to generate stored polynomial functions
    which, under waveshaping, can be used to split a sinusoid into
    harmonic partials having a pre-definable spectrum.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list: list, optional
            Relative strengths of partials numbers 1,2,3, ..., 12 that will
            result when a sinusoid of amplitude 1 is waveshaped using this
            function table. Up to 12 partials can be specified. Defaults to [1].
        size: int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = ChebyTable([1,0,.33,0,.2,0,.143,0,.111])
    >>> lfo = Sine(freq=.25, mul=0.45, add=0.5)
    >>> a = Sine(freq=[200,201], mul=lfo)
    >>> b = Lookup(table=t, index=a, mul=1-lfo).out()

    """

    def __init__(self, list=[1.0, 0.0], size=8192):
        pyoArgsAssert(self, "lI", list, size)
        PyoTableObject.__init__(self, size)
        self._auto_normalize = False
        self._list = copy.deepcopy(list)
        self._base_objs = [ChebyTable_base(self._list, size)]

    def autoNormalize(self, x):
        """
        Activate/deactivate automatic normalization when harmonics changed.

        :Args:

            x: boolean
                True for activating automatic normalization, False for
                deactivating it.

        """
        self._auto_normalize = x
        if self._auto_normalize:
            self.normalize()

    def replace(self, list):
        """
        Redraw the waveform according to a new set of harmonics
        relative strengths that will result when a sinusoid of
        amplitude 1 is waveshaped using this function table.

        :Args:

            list: list
                Relative strengths of the fixed harmonic partial
                numbers 1,2,3, ..., 12. Up to 12 partials can be specified.

        """
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        if self._auto_normalize:
            self.normalize()
        self.refreshView()

    def getNormTable(self):
        """
        Return a DataTable filled with the normalization function
        corresponding to the current polynomial.

        """
        if sum(self._list[1::2]) == 0:
            data = self._base_objs[0].getNormTable(0)
        else:
            data = self._base_objs[0].getNormTable(1)
        return DataTable(size=len(data), init=data).normalize()

    def _get_current_data(self):
        # internal that returns the data to draw in a DataTableGrapher.
        return self._list

    def graph(self, yrange=(-1.0, 1.0), title=None, wxnoserver=False):
        """
        Opens a multislider window to control the data values.

        When editing the grapher with the mouse, the new values are
        sent to the object to replace the table content.

        :Args:

            yrange: tuple, optional
                Set the min and max values of the Y axis of the multislider.
                Defaults to (0.0, 1.0).
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        .. note::

            The number of bars in the graph is initialized to the length
            of the list of relative strentghs at the time the graph is
            created.

        """
        createDataGraphWindow(self, yrange, title, wxnoserver)

    @property
    def list(self):
        """list. Relative strengths of the fixed harmonic partial numbers."""
        return self._list

    @list.setter
    def list(self, x):
        self.replace(x)


class HannTable(PyoTableObject):
    """
    Generates Hanning window function.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        size: int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Hanning envelope
    >>> t = HannTable()
    >>> a = Osc(table=t, freq=2, mul=.2)
    >>> b = Sine(freq=[299,300], mul=a).out()

    """

    def __init__(self, size=8192):
        pyoArgsAssert(self, "I", size)
        PyoTableObject.__init__(self, size)
        self._base_objs = [HannTable_base(size)]


class SincTable(PyoTableObject):
    """
    Generates sinc window function.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        freq: float, optional
            Frequency, in radians, of the sinc function. Defaults to pi*2.
        windowed: boolean, optional
            If True, an hanning window is applied on the sinc function. Defaults to False.
        size: int, optional
            Table size in samples. Defaults to 8192.

    >>> import math
    >>> s = Server().boot()
    >>> s.start()
    >>> t = SincTable(freq=math.pi*6, windowed=True)
    >>> a = Osc(t, freq=[199,200], mul=.2).out()

    """

    def __init__(self, freq=pi * 2, windowed=False, size=8192):
        pyoArgsAssert(self, "NBI", freq, windowed, size)
        PyoTableObject.__init__(self, size)
        self._freq = freq
        self._windowed = windowed
        self._base_objs = [SincTable_base(freq, windowed, size)]

    def setFreq(self, x):
        """
        Change the frequency of the sinc function. This will redraw the envelope.

        :Args:

            x: float
                New frequency in radians.

        """
        pyoArgsAssert(self, "N", x)
        self._freq = x
        [obj.setFreq(x) for obj in self._base_objs]
        self.refreshView()

    def setWindowed(self, x):
        """
        Change the windowed flag. This will redraw the envelope.

        :Args:

            x: boolean
                New windowed flag.

        """
        pyoArgsAssert(self, "B", x)
        self._windowed = x
        [obj.setWindowed(x) for obj in self._base_objs]
        self.refreshView()

    @property
    def freq(self):
        """float. Frequency of the sinc function."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def windowed(self):
        """boolean. Windowed flag."""
        return self._windowed

    @windowed.setter
    def windowed(self, x):
        self.setWindowed(x)


class WinTable(PyoTableObject):
    """
    Generates different kind of windowing functions.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        type: int, optional
            Windowing function. Possible choices are:
                0. Rectangular (no window)
                1. Hamming
                2. Hanning (default)
                3. Bartlett (triangular)
                4. Blackman 3-term
                5. Blackman-Harris 4-term
                6. Blackman-Harris 7-term
                7. Tuckey (alpha = 0.66)
                8. Sine (half-sine window)
        size: int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Triangular envelope
    >>> t = WinTable(type=3)
    >>> a = Osc(table=t, freq=2, mul=.2)
    >>> b = SineLoop(freq=[199,200], feedback=0.05, mul=a).out()

    """

    def __init__(self, type=2, size=8192):
        pyoArgsAssert(self, "II", type, size)
        PyoTableObject.__init__(self, size)
        self._type = type
        self._base_objs = [WinTable_base(type, size)]

    def setType(self, type):
        """
        Sets the windowing function.

        :Args:

            type: int {0 -> 8}
                Windowing function.

        """
        pyoArgsAssert(self, "I", type)
        self._type = type
        [obj.setType(type) for obj in self._base_objs]
        self.refreshView()

    @property
    def type(self):
        """int. Windowing function."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class ParaTable(PyoTableObject):
    """
    Generates parabola window function.

    The parabola is a conic section, the intersection of a right circular conical
    surface and a plane parallel to a generating straight line of that surface.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        size: int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Parabola envelope
    >>> t = ParaTable()
    >>> a = Osc(table=t, freq=2, mul=.2)
    >>> b = SineLoop(freq=[299,300], feedback=0.05, mul=a).out()

    """

    def __init__(self, size=8192):
        pyoArgsAssert(self, "I", size)
        PyoTableObject.__init__(self, size)
        self._base_objs = [ParaTable_base(size)]


class LinTable(PyoTableObject):
    """
    Construct a table from segments of straight lines in breakpoint fashion.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list: list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8191, 1.)], creates a
            straight line from 0.0 at location 0 to 1.0 at the end of the
            table (size - 1). Location must be an integer.
        size: int, optional
            Table size in samples. Defaults to 8192.

    .. note::

        Locations in the list must be in increasing order. If the last value
        is less than size, the rest of the table will be filled with zeros.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Sharp attack envelope
    >>> t = LinTable([(0,0), (100,1), (1000,.25), (8191,0)])
    >>> a = Osc(table=t, freq=2, mul=.25)
    >>> b = SineLoop(freq=[299,300], feedback=0.05, mul=a).out()

    """

    def __init__(self, list=[(0, 0.0), (8191, 1.0)], size=8192):
        pyoArgsAssert(self, "lI", list, size)
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print("LinTable warning: size smaller than last point position.")
            print("                  Increased size to last point position + 1")
            size = list[-1][0] + 1
            self._size = size
        self._base_objs = [LinTable_base(copy.deepcopy(list), size)]

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list: list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
        """
        Import an automation recording file in the table.

        loadRecFile takes a recording file, usually from a ControlRec object,
        as `filename` parameter, applies a filtering pre-processing to eliminate
        redundancies and loads the result in the table as a list of points.
        Filtering process can be controled with the `tolerance` parameter.

        :Args:

            filename: string
                Full path of an automation recording file.
            tolerance: float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        pyoArgsAssert(self, "SN", filename, tolerance)
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files, i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0] * scl), v[1]) for v in values]
            f.close()
            values = reducePoints(values, tolerance=tolerance)
            self._list = values
            obj.replace(values)
        self.refreshView()

    def getPoints(self):
        """
        Returns list of points of the current table.

        """
        return self._base_objs[0].getPoints()

    def graph(self, yrange=(0.0, 1.0), title=None, wxnoserver=False):
        """
        Opens a grapher window to control the shape of the envelope.

        When editing the grapher with the mouse, the new set of points
        will be send to the object on mouse up.

        Ctrl+C with focus on the grapher will copy the list of points to the
        clipboard, giving an easy way to insert the new shape in a script.

        :Args:

            yrange: tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createGraphWindow(self, 0, self._size, yrange, title, wxnoserver)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table."""
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class LogTable(PyoTableObject):
    """
    Construct a table from logarithmic segments in breakpoint fashion.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list: list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8191, 1.)], creates a
            logarithmic line from 0.0 at location 0 to 1.0 at the end of
            the table (size - 1). Location must be an integer.
        size: int, optional
            Table size in samples. Defaults to 8192.

    .. note::

        Locations in the list must be in increasing order. If the last value
        is less than size, the rest of the table will be filled with zeros.

        Values must be greater than 0.0.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = LogTable([(0,0), (4095,1), (8192,0)])
    >>> a = Osc(table=t, freq=2, mul=.25)
    >>> b = SineLoop(freq=[599,600], feedback=0.05, mul=a).out()

    """

    def __init__(self, list=[(0, 0.0), (8191, 1.0)], size=8192):
        pyoArgsAssert(self, "lI", list, size)
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print("LogTable warning: size smaller than last point position.")
            print("                  Increased size to last point position + 1")
            size = list[-1][0] + 1
            self._size = size
        self._base_objs = [LogTable_base(copy.deepcopy(list), size)]

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list: list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
        """
        Import an automation recording file in the table.

        loadRecFile takes a recording file, usually from a ControlRec object,
        as `filename` parameter, applies a filtering pre-processing to eliminate
        redundancies and loads the result in the table as a list of points.
        Filtering process can be controled with the `tolerance` parameter.

        :Args:

            filename: string
                Full path of an automation recording file.
            tolerance: float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        pyoArgsAssert(self, "SN", filename, tolerance)
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files, i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0] * scl), v[1]) for v in values]
            f.close()
            values = reducePoints(values, tolerance=tolerance)
            self._list = values
            obj.replace(values)
        self.refreshView()

    def getPoints(self):
        """
        Returns list of points of the current table.

        """
        return self._base_objs[0].getPoints()

    def graph(self, yrange=(0.0, 1.0), title=None, wxnoserver=False):
        """
        Opens a grapher window to control the shape of the envelope.

        When editing the grapher with the mouse, the new set of points
        will be send to the object on mouse up.

        Ctrl+C with focus on the grapher will copy the list of points to the
        clipboard, giving an easy way to insert the new shape in a script.

        :Args:

            yrange: tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createGraphWindow(self, 4, self._size, yrange, title, wxnoserver)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table."""
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class CosLogTable(PyoTableObject):
    """
    Construct a table from logarithmic-cosine segments in breakpoint fashion.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list: list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8191, 1.)], creates a
            logarithmic line from 0.0 at location 0 to 1.0 at the end of
            the table (size - 1). Location must be an integer.
        size: int, optional
            Table size in samples. Defaults to 8192.

    .. note::

        Locations in the list must be in increasing order. If the last value
        is less than size, the rest of the table will be filled with zeros.

        Values must be greater than 0.0.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = CosLogTable([(0,0), (4095,1), (8192,0)])
    >>> a = Osc(table=t, freq=2, mul=.25)
    >>> b = SineLoop(freq=[599,600], feedback=0.05, mul=a).out()

    """

    def __init__(self, list=[(0, 0.0), (8191, 1.0)], size=8192):
        pyoArgsAssert(self, "lI", list, size)
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print("CosLogTable warning: size smaller than last point position.")
            print("                     Increased size to last point position + 1")
            size = list[-1][0] + 1
            self._size = size
        self._base_objs = [CosLogTable_base(copy.deepcopy(list), size)]

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list: list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
        """
        Import an automation recording file in the table.

        loadRecFile takes a recording file, usually from a ControlRec object,
        as `filename` parameter, applies a filtering pre-processing to eliminate
        redundancies and loads the result in the table as a list of points.
        Filtering process can be controled with the `tolerance` parameter.

        :Args:

            filename: string
                Full path of an automation recording file.
            tolerance: float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        pyoArgsAssert(self, "SN", filename, tolerance)
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files, i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0] * scl), v[1]) for v in values]
            f.close()
            values = reducePoints(values, tolerance=tolerance)
            self._list = values
            obj.replace(values)
        self.refreshView()

    def getPoints(self):
        """
        Returns list of points of the current table.

        """
        return self._base_objs[0].getPoints()

    def graph(self, yrange=(0.0, 1.0), title=None, wxnoserver=False):
        """
        Opens a grapher window to control the shape of the envelope.

        When editing the grapher with the mouse, the new set of points
        will be send to the object on mouse up.

        Ctrl+C with focus on the grapher will copy the list of points to the
        clipboard, giving an easy way to insert the new shape in a script.

        :Args:

            yrange: tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createGraphWindow(self, 5, self._size, yrange, title, wxnoserver)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table."""
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class CosTable(PyoTableObject):
    """
    Construct a table from cosine interpolated segments.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list: list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8191, 1.)], creates a
            cosine line from 0.0 at location 0 to 1.0 at the end of the
            table (size - 1). Location must be an integer.
        size: int, optional
            Table size in samples. Defaults to 8192.

    .. note::

        Locations in the list must be in increasing order. If the last value
        is less than size, the rest of the table will be filled with zeros.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Sharp attack envelope
    >>> t = CosTable([(0,0), (100,1), (1000,.25), (8191,0)])
    >>> a = Osc(table=t, freq=2, mul=.25)
    >>> b = SineLoop(freq=[299,300], feedback=0.05, mul=a).out()

    """

    def __init__(self, list=[(0, 0.0), (8191, 1.0)], size=8192):
        pyoArgsAssert(self, "lI", list, size)
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print("CosTable warning: size smaller than last point position.")
            print("                  Increased size to last point position + 1")
            size = list[-1][0] + 1
            self._size = size
        self._base_objs = [CosTable_base(copy.deepcopy(list), size)]

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list: list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
        """
        Import an automation recording file in the table.

        loadRecFile takes a recording file, usually from a ControlRec object,
        as `filename` parameter, applies a filtering pre-processing to eliminate
        redundancies and loads the result in the table as a list of points.
        Filtering process can be controled with the `tolerance` parameter.

        :Args:

            filename: string
                Full path of an automation recording file.
            tolerance: float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        pyoArgsAssert(self, "SN", filename, tolerance)
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files, i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0] * scl), v[1]) for v in values]
            f.close()
            values = reducePoints(values, tolerance=tolerance)
            self._list = values
            obj.replace(values)
        self.refreshView()

    def getPoints(self):
        """
        Returns list of points of the current table.

        """
        return self._base_objs[0].getPoints()

    def graph(self, yrange=(0.0, 1.0), title=None, wxnoserver=False):
        """
        Opens a grapher window to control the shape of the envelope.

        When editing the grapher with the mouse, the new set of points
        will be send to the object on mouse up.

        Ctrl+C with focus on the grapher will copy the list of points to the
        clipboard, giving an easy way to insert the new shape in a script.

        :Args:

            yrange: tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createGraphWindow(self, 1, self._size, yrange, title, wxnoserver)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table."""
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class CurveTable(PyoTableObject):
    """
    Construct a table from curve interpolated segments.

    CurveTable uses Hermite interpolation (sort of cubic interpolation)
    to calculate each points of the curve. This algorithm allows tension
    and biasing controls. Tension can be used to tighten up the curvature
    at the known points. The bias is used to twist the curve about the
    known points.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list: list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8191, 1.)], creates a
            curved line from 0.0 at location 0 to 1.0 at the end of the
            table (size - 1). Location must be an integer.
        tension: float, optional
            Curvature at the known points. 1 is high, 0 normal, -1 is low.
            Defaults to 0.
        bias: float, optional
            Curve attraction (for each segments) toward bundary points.
            0 is even, positive is towards first point, negative is towards
            the second point. Defaults to 0.
        size: int, optional
            Table size in samples. Defaults to 8192.

    .. note::

        Locations in the list must be in increasing order. If the last value
        is less than size, the rest of the table will be filled with zeros.

        High tension or bias values can create unstable or very loud table,
        use normalize method to keep the curve between -1 and 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = CurveTable([(0,0),(2048,.5),(4096,.2),(6144,.5),(8192,0)], 0, 20)
    >>> t.normalize()
    >>> a = Osc(table=t, freq=2, mul=.25)
    >>> b = SineLoop(freq=[299,300], feedback=0.05, mul=a).out()

    """

    def __init__(self, list=[(0, 0.0), (8191, 1.0)], tension=0, bias=0, size=8192):
        pyoArgsAssert(self, "lNNI", list, tension, bias, size)
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print("CurveTable warning: size smaller than last point position.")
            print("                    Increased size to last point position + 1")
            size = list[-1][0] + 1
            self._size = size
        self._tension = tension
        self._bias = bias
        self._base_objs = [CurveTable_base(copy.deepcopy(list), tension, bias, size)]

    def setTension(self, x):
        """
        Replace the `tension` attribute.

        1 is high, 0 normal, -1 is low.

        :Args:

            x: float
                New `tension` attribute.

        """
        pyoArgsAssert(self, "N", x)
        self._tension = x
        [obj.setTension(x) for obj in self._base_objs]
        self.refreshView()

    def setBias(self, x):
        """
        Replace the `bias` attribute.

        0 is even, positive is towards first point, negative is towards
        the second point.

        :Args:

            x: float
                New `bias` attribute.

        """
        pyoArgsAssert(self, "N", x)
        self._bias = x
        [obj.setBias(x) for obj in self._base_objs]
        self.refreshView()

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list: list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
        """
        Import an automation recording file in the table.

        loadRecFile takes a recording file, usually from a ControlRec object,
        as `filename` parameter, applies a filtering pre-processing to eliminate
        redundancies and loads the result in the table as a list of points.
        Filtering process can be controled with the `tolerance` parameter.

        :Args:

            filename: string
                Full path of an automation recording file.
            tolerance: float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        pyoArgsAssert(self, "SN", filename, tolerance)
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files, i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0] * scl), v[1]) for v in values]
            f.close()
            values = reducePoints(values, tolerance=tolerance)
            self._list = values
            obj.replace(values)
        self.refreshView()

    def getPoints(self):
        """
        Returns list of points of the current table.

        """
        return self._base_objs[0].getPoints()

    def graph(self, yrange=(0.0, 1.0), title=None, wxnoserver=False):
        """
        Opens a grapher window to control the shape of the envelope.

        When editing the grapher with the mouse, the new set of points
        will be send to the object on mouse up.

        Ctrl+C with focus on the grapher will copy the list of points to the
        clipboard, giving an easy way to insert the new shape in a script.

        :Args:

            yrange: tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createGraphWindow(self, 3, self._size, yrange, title, wxnoserver)

    @property
    def tension(self):
        """float. Curvature tension."""
        return self._tension

    @tension.setter
    def tension(self, x):
        self.setTension(x)

    @property
    def bias(self):
        """float. Curve Attraction."""
        return self._bias

    @bias.setter
    def bias(self, x):
        self.setBias(x)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table."""
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class ExpTable(PyoTableObject):
    """
    Construct a table from exponential interpolated segments.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list: list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8192, 1.)], creates a
            exponential line from 0.0 at location 0 to 1.0 at the end of
            the table. Location must be an integer.
        exp: float, optional
            Exponent factor. Used to control the slope of the curve.
            Defaults to 10.
        inverse: boolean, optional
            If True, downward slope will be inversed. Useful to create
            biexponential curves. Defaults to True.
        size: int, optional
            Table size in samples. Defaults to 8192.

    .. note::

        Locations in the list must be in increasing order. If the last value
        is less than size, the rest of the table will be filled with zeros.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = ExpTable([(0,0),(4096,1),(8192,0)], exp=5, inverse=True)
    >>> a = Osc(table=t, freq=2, mul=.3)
    >>> b = Sine(freq=[299,300], mul=a).out()

    """

    def __init__(self, list=[(0, 0.0), (8192, 1.0)], exp=10, inverse=True, size=8192):
        pyoArgsAssert(self, "lNBI", list, exp, inverse, size)
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print("ExpTable warning: size smaller than last point position.")
            print("                  Increased size to last point position + 1")
            size = list[-1][0] + 1
            self._size = size
        self._exp = exp
        self._inverse = inverse
        self._base_objs = [ExpTable_base(copy.deepcopy(list), exp, inverse, size)]

    def setExp(self, x):
        """
        Replace the `exp` attribute.

        :Args:

            x: float
                New `exp` attribute.

        """
        pyoArgsAssert(self, "N", x)
        self._exp = x
        [obj.setExp(x) for obj in self._base_objs]
        self.refreshView()

    def setInverse(self, x):
        """
        Replace the `inverse` attribute.

        :Args:

            x: boolean
                New `inverse` attribute.

        """
        pyoArgsAssert(self, "B", x)
        self._inverse = x
        [obj.setInverse(x) for obj in self._base_objs]
        self.refreshView()

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list: list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
        """
        Import an automation recording file in the table.

        loadRecFile takes a recording file, usually from a ControlRec object,
        as `filename` parameter, applies a filtering pre-processing to eliminate
        redundancies and loads the result in the table as a list of points.
        Filtering process can be controled with the `tolerance` parameter.

        :Args:

            filename: string
                Full path of an automation recording file.
            tolerance: float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        pyoArgsAssert(self, "SN", filename, tolerance)
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files, i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0] * scl), v[1]) for v in values]
            f.close()
            values = reducePoints(values, tolerance=tolerance)
            self._list = values
            obj.replace(values)
        self.refreshView()

    def getPoints(self):
        """
        Returns list of points of the current table.

        """
        return self._base_objs[0].getPoints()

    def graph(self, yrange=(0.0, 1.0), title=None, wxnoserver=False):
        """
        Opens a grapher window to control the shape of the envelope.

        When editing the grapher with the mouse, the new set of points
        will be send to the object on mouse up.

        Ctrl+C with focus on the grapher will copy the list of points to the
        clipboard, giving an easy way to insert the new shape in a script.

        :Args:

            yrange: tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createGraphWindow(self, 2, self._size, yrange, title, wxnoserver)

    @property
    def exp(self):
        """float. Exponent factor."""
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)

    @property
    def inverse(self):
        """boolean. Inverse factor."""
        return self._inverse

    @inverse.setter
    def inverse(self, x):
        self.setInverse(x)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table."""
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class SndTable(PyoTableObject):
    """
    Transfers data from a soundfile into a function table.

    If `chnl` is None, the table will contain as many table streams as
    necessary to read all channels of the loaded sound.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        path: string, optional
            Full path name of the sound. The defaults, None, creates an empty
            table.
        chnl: int, optional
            Channel number to read in. The count starts at 0 (first channel is
            is 0, second is 1 and so on). Available at initialization time only.
            The default (None) reads all channels.
        start: float, optional
            Begins reading at `start` seconds into the file. Available at
            initialization time only. Defaults to 0.
        stop: float, optional
            Stops reading at `stop` seconds into the file. Available at
            initialization time only. The default (None) means the end of
            the file.
        initchnls: int, optional
            Number of channels for an empty table (path=None). Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> snd_path = SNDS_PATH + '/transparent.aif'
    >>> t = SndTable(snd_path)
    >>> freq = t.getRate()
    >>> a = Osc(table=t, freq=[freq, freq*.995], mul=.3).out()

    """

    def __init__(self, path=None, chnl=None, start=0, stop=None, initchnls=1):
        PyoTableObject.__init__(self)
        self._path = path
        self._chnl = chnl
        self._start = start
        self._stop = stop
        self._size = []
        self._dur = []
        self._base_objs = []
        path, lmax = convertArgsToLists(path)
        if self._path is None:
            self._base_objs = [SndTable_base("", 0, 0) for i in range(initchnls)]
        else:
            for p in path:
                _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(p, raise_on_failure=True)
                if chnl is None:
                    if stop is None:
                        self._base_objs.extend([SndTable_base(stringencode(p), i, start) for i in range(_snd_chnls)])
                    else:
                        self._base_objs.extend(
                            [SndTable_base(stringencode(p), i, start, stop) for i in range(_snd_chnls)]
                        )
                else:
                    if stop is None:
                        self._base_objs.append(SndTable_base(stringencode(p), chnl, start))
                    else:
                        self._base_objs.append(SndTable_base(stringencode(p), chnl, start, stop))
                self._size.append(self._base_objs[-1].getSize())
                self._dur.append(self._size[-1] / float(_snd_sr))
            if lmax == 1:
                self._size = self._base_objs[-1].getSize()
                self._dur = self._size / float(_snd_sr)

    def setSound(self, path, start=0, stop=None):
        """
        Load a new sound in the table.

        Keeps the number of channels of the sound loaded at initialization.
        If the new sound has less channels, it will wrap around and load
        the same channels many times. If the new sound has more channels,
        the extra channels will be skipped.

        :Args:

            path: string
                Full path of the new sound.
            start: float, optional
                Begins reading at `start` seconds into the file. Defaults to 0.
            stop: float, optional
                Stops reading at `stop` seconds into the file. The default (None)
                means the end of the file.

        """
        self._path = path
        if type(path) == list:
            self._size = []
            self._dur = []
            path, lmax = convertArgsToLists(path)
            for i, obj in enumerate(self._base_objs):
                p = path[i % lmax]
                _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(p, raise_on_failure=True)
                self._size.append(_size)
                self._dur.append(_dur)
                if stop is None:
                    obj.setSound(stringencode(p), 0, start)
                else:
                    obj.setSound(stringencode(p), 0, start, stop)
        else:
            _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(path, raise_on_failure=True)
            self._size = _size
            self._dur = _dur
            if stop is None:
                [obj.setSound(stringencode(path), (i % _snd_chnls), start) for i, obj in enumerate(self._base_objs)]
            else:
                [
                    obj.setSound(stringencode(path), (i % _snd_chnls), start, stop)
                    for i, obj in enumerate(self._base_objs)
                ]
        self.refreshView()
        self._resetView()

    def append(self, path, crossfade=0, start=0, stop=None):
        """
        Append a sound to the one already in the table with crossfade.

        Keeps the number of channels of the sound loaded at initialization.
        If the new sound has less channels, it will wrap around and load
        the same channels many times. If the new sound has more channels,
        the extra channels will be skipped.

        :Args:

            path: string
                Full path of the new sound.
            crossfade: float, optional
                Crossfade time, in seconds, between the sound already in the table
                and the new one. Defaults to 0.
            start: float, optional
                Begins reading at `start` seconds into the file. Defaults to 0.
            stop: float, optional
                Stops reading at `stop` seconds into the file. The default, None,
                means the end of the file.

        """
        if type(path) == list:
            self._size = []
            self._dur = []
            path, lmax = convertArgsToLists(path)
            for i, obj in enumerate(self._base_objs):
                p = path[i % lmax]
                _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(p, raise_on_failure=True)
                self._size.append(_size)
                self._dur.append(_dur)
                if stop is None:
                    obj.append(stringencode(p), crossfade, 0, start)
                else:
                    obj.append(stringencode(p), crossfade, 0, start, stop)
        else:
            _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(path, raise_on_failure=True)
            self._size = _size
            self._dur = _dur
            if stop is None:
                [
                    obj.append(stringencode(path), crossfade, (i % _snd_chnls), start)
                    for i, obj in enumerate(self._base_objs)
                ]
            else:
                [
                    obj.append(stringencode(path), crossfade, (i % _snd_chnls), start, stop)
                    for i, obj in enumerate(self._base_objs)
                ]
        self.refreshView()

    def insert(self, path, pos=0, crossfade=0, start=0, stop=None):
        """
        Insert a sound into the one already in the table with crossfade.

        Insert a sound at position `pos`, specified in seconds,
        with crossfading at the beginning and the end of the insertion.

        Keeps the number of channels of the sound loaded at initialization.
        If the new sound has less channels, it will wrap around and load
        the same channels many times. If the new sound has more channels,
        the extra channels will be skipped.

        :Args:

            path: string
                Full path of the new sound.
            pos: float, optional
                Position in the table, in seconds, where to insert the new sound.
                Defaults to 0.
            crossfade: float, optional
                Crossfade time, in seconds, between the sound already in the table
                and the new one. Defaults to 0.
            start: float, optional
                Begins reading at `start` seconds into the file. Defaults to 0.
            stop: float, optional
                Stops reading at `stop` seconds into the file. The default, None,
                means the end of the file.

        """
        if type(path) == list:
            self._size = []
            self._dur = []
            path, lmax = convertArgsToLists(path)
            for i, obj in enumerate(self._base_objs):
                p = path[i % lmax]
                _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(p, raise_on_failure=True)
                self._size.append(_size)
                self._dur.append(_dur)
                if stop is None:
                    obj.insert(stringencode(p), pos, crossfade, 0, start)
                else:
                    obj.insert(stringencode(p), pos, crossfade, 0, start, stop)
        else:
            _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(path, raise_on_failure=True)
            self._size = _size
            self._dur = _dur
            if stop is None:
                [
                    obj.insert(stringencode(path), pos, crossfade, (i % _snd_chnls), start)
                    for i, obj in enumerate(self._base_objs)
                ]
            else:
                [
                    obj.insert(stringencode(path), pos, crossfade, (i % _snd_chnls), start, stop)
                    for i, obj in enumerate(self._base_objs)
                ]
        self.refreshView()

    def getViewTable(self, size, begin=0, end=0):
        """
        Return a list of points (in X, Y pixel values) for each channel in the table.
        These lists can be draw on a DC (WxPython) with a DrawLines method.

        :Args:

            size: tuple
                Size, (X, Y) pixel values, of the waveform container window.
            begin: float, optional
                First position in the the table, in seconds, where to get samples.
                Defaults to 0.
            end: float, optional
                Last position in the table, in seconds, where to get samples.

                if this value is set to 0, that means the end of the table. Defaults to 0.

        """
        w, h = size
        chnls = len(self._base_objs)
        img = []
        imgHeight = h // chnls
        for i in range(chnls):
            off = h // chnls * i
            img.append(self._base_objs[i].getViewTable((w, imgHeight), begin, end, off))
        return img

    def getEnvelope(self, points):
        """
        Return the amplitude envelope of the table.

        Return a list, of length `chnl`, of lists of length `points` filled
        with the amplitude envelope of the table.

        :Args:

            points: int
                Number of points of the amplitude analysis.

        """
        return [obj.getEnvelope(points) for obj in self._base_objs]

    def view(self, title="Sound waveform", wxnoserver=False, mouse_callback=None):
        """
        Opens a window showing the contents of the table.

        :Args:

            title: string, optional
                Window title. Defaults to "Table waveform".
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.
            mouse_callback: callable
                If provided, this function will be called with the mouse
                position, inside the frame, as argument. Defaults to None.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createSndViewTableWindow(self, title, wxnoserver, self.__class__.__name__, mouse_callback)

    def refreshView(self):
        if self.viewFrame is not None:
            self.viewFrame.update()

    def _resetView(self):
        if self.viewFrame is not None:
            if hasattr(self.viewFrame, "_setZoom"):
                self.viewFrame._setZoom()

    @property
    def sound(self):
        """string. Full path of the sound."""
        return self._path

    @sound.setter
    def sound(self, x):
        self.setSound(x)

    @property
    def path(self):
        """string. Full path of the sound."""
        return self._path

    @path.setter
    def path(self, x):
        self.setSound(x)

    @property
    def chnl(self):
        """int. Channel to read in."""
        return self._chnl

    @chnl.setter
    def chnl(self, x):
        print("'chnl' attribute is read-only.")

    @property
    def start(self):
        """float. Start point, in seconds, to read into the file."""
        return self._start

    @start.setter
    def start(self, x):
        print("'start' attribute is read-only.")

    @property
    def stop(self):
        """float. Stop point, in seconds, to read into the file."""
        return self._stop

    @stop.setter
    def stop(self, x):
        print("SndTable 'stop' attribute is read-only.")

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, x):
        print("SndTable 'size' attribute is read-only.")


class NewTable(PyoTableObject):
    """
    Create an empty table ready for recording.

    See :py:class:`TableRec` to write samples in the table.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        length: float
            Length of the table in seconds.
        chnls: int, optional
            Number of channels that will be handled by the table.
            Defaults to 1.
        init: list of floats, optional
            Initial table. List of list can match the number of channels,
            otherwise, the list will be loaded in all tablestreams.
            Defaults to None.
        feedback: float, optional
            Amount of old data to mix with a new recording. Defaults to 0.0.

    .. seealso::

        :py:class:`DataTable`, :py:class:`TableRec`

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> t = NewTable(length=2, chnls=1)
    >>> a = Input(0)
    >>> b = TableRec(a, t, .01)
    >>> amp = Iter(b["trig"], [.5])
    >>> freq = t.getRate()
    >>> c = Osc(table=t, freq=[freq, freq*.99], mul=amp).out()
    >>> # to record in the empty table, call:
    >>> # b.play()

    """

    def __init__(self, length, chnls=1, init=None, feedback=0.0):
        pyoArgsAssert(self, "NILN", length, chnls, init, feedback)
        PyoTableObject.__init__(self)
        self._length = length
        self._chnls = chnls
        self._init = init
        self._feedback = feedback
        if init is None:
            self._base_objs = [NewTable_base(length, None, feedback) for i in range(chnls)]
        else:
            if type(init[0]) != list:
                init = [init]
            self._base_objs = [NewTable_base(length, wrap(init, i), feedback) for i in range(chnls)]
        self._size = self._base_objs[0].getSize()

    def _check_data_size(self, lst):
        # Validate that the data passed to the object has the same size as self._size.
        if len(lst) < self._size:
            lst += [0] * (self._size - len(lst))
            print("Warning: NewTable data length < size... padded with 0s.")
        elif len(lst) > self._size:
            lst = lst[:self._size]
            print("Warning: NewTable data length > size... truncated to size.")
        return lst

    def replace(self, x):
        """
        Replaces the actual table.

        :Args:

            x: list of floats
                New table. Must be of the same size as the actual table.

                List of list can match the number of channels, otherwise,
                the list will be loaded in all tablestreams.

        """
        pyoArgsAssert(self, "l", x)
        if type(x[0]) != list:
            x = [x]
        [obj.setTable(self._check_data_size(wrap(x, i))) for i, obj in enumerate(self._base_objs)]
        self.refreshView()

    def setFeedback(self, x):
        """
        Replaces the`feedback` attribute.

        :Args:

            x: float
                New `feedback` value.

        """
        pyoArgsAssert(self, "N", x)
        self._feedback = x
        [obj.setFeedback(x) for i, obj in enumerate(self._base_objs)]

    def setLength(self, length):
        """
        Change the length of the table.

        If the new length is longer than the current one, new samples will
        be initialized to 0.

        :Args:

            length: float
                New table length in seconds.

        """
        pyoArgsAssert(self, "N", length)
        self._length = length
        [obj.setLength(length) for obj in self._base_objs]
        self.refreshView()

    def getFeedback(self):
        """
        Returns the current feedback value.

        """
        return self._feedback

    def getLength(self):
        """
        Returns the length of the table in seconds.

        """
        return self._base_objs[0].getLength()

    def getViewTable(self, size, begin=0, end=0):
        """
        Return a list of points (in X, Y pixel values) for each channel in the table.
        These lists can be draw on a DC (WxPython) with a DrawLines method.

        :Args:

            size: tuple
                Size, (X, Y) pixel values, of the waveform container window.
            begin: float, optional
                First position in the the table, in seconds, where to get samples.
                Defaults to 0.
            end: float, optional
                Last position in the table, in seconds, where to get samples.

                if this value is set to 0, that means the end of the table. Defaults to 0.

        """
        w, h = size
        chnls = len(self._base_objs)
        img = []
        imgHeight = h // chnls
        for i in range(chnls):
            off = h // chnls * i
            img.append(self._base_objs[i].getViewTable((w, imgHeight), begin, end, off))
        return img

    def view(self, title="Sound waveform", wxnoserver=False, mouse_callback=None):
        """
        Opens a window showing the contents of the table.

        :Args:

            title: string, optional
                Window title. Defaults to "Table waveform".
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.
            mouse_callback: callable
                If provided, this function will be called with the mouse
                position, inside the frame, as argument. Defaults to None.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createSndViewTableWindow(self, title, wxnoserver, self.__class__.__name__, mouse_callback)

    def refreshView(self):
        if self.viewFrame is not None:
            self.viewFrame.update()

    @property
    def length(self):
        """float. Length of the table in seconds."""
        return self._length

    @length.setter
    def length(self, x):
        print("'length' attribute is read-only.")

    @property
    def chnls(self):
        """int. Number of channels that will be handled by the table."""
        return self._chnls

    @chnls.setter
    def chnls(self, x):
        print("'chnls' attribute is read-only.")

    @property
    def init(self):
        """list of floats. Initial table."""
        return self._init

    @init.setter
    def init(self, x):
        print("'init' attribute is read-only.")

    @property
    def feedback(self):
        """float. Amount of old data to mix with a new recording."""
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, x):
        print("NewTable 'size' attribute is read-only.")


class DataTable(PyoTableObject):
    """
    Create an empty table ready for data recording.

    See :py:class:`TableRec` to write samples in the table.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        size: int
            Size of the table in samples.
        chnls: int, optional
            Number of channels that will be handled by the table.
            Defaults to 1.
        init: list of floats, optional
            Initial table. List of list can match the number of channels,
            otherwise, the list will be loaded in all tablestreams.

    .. seealso::

        :py:class:`NewTable`, :py:class:`TableRec`

    >>> s = Server().boot()
    >>> s.start()
    >>> import random
    >>> notes = [midiToHz(random.randint(48,72)) for i in range(10)]
    >>> tab = DataTable(size=10, init=notes)
    >>> ind = RandInt(10, 8)
    >>> pit = TableIndex(tab, ind)
    >>> a = SineLoop(freq=[pit,pit*0.99], feedback = 0.07, mul=.2).out()

    """

    def __init__(self, size, chnls=1, init=None):
        pyoArgsAssert(self, "IIL", size, chnls, init)
        PyoTableObject.__init__(self, size)
        self._chnls = chnls
        self._init = init
        if init is None:
            self._base_objs = [DataTable_base(size) for i in range(chnls)]
        else:
            if type(init[0]) != list:
                init = [init]
            self._base_objs = [DataTable_base(size, self._check_data_size(wrap(init, i))) for i in range(chnls)]

    def _check_data_size(self, lst):
        # Validate that the data passed to the object has the same size as self._size.
        if len(lst) < self._size:
            lst += [0] * (self._size - len(lst))
            print("Warning: DataTable data length < size... padded with 0s.")
        elif len(lst) > self._size:
            lst = lst[:self._size]
            print("Warning: DataTable data length > size... truncated to size.")
        return lst

    def replace(self, x):
        """
        Replaces the actual table.

        :Args:

            x: list of floats
                New table. Must be of the same size as the actual table.

                List of list can match the number of channels, otherwise,
                the list will be loaded in all tablestreams.

        """
        pyoArgsAssert(self, "l", x)
        if type(x[0]) != list:
            x = [x]
        [obj.setTable(self._check_data_size(wrap(x, i))) for i, obj in enumerate(self._base_objs)]
        self.refreshView()

    def _get_current_data(self):
        # internal that returns the data to draw in a DataTableGrapher.
        return self.getTable()

    def graph(self, yrange=(0.0, 1.0), title=None, wxnoserver=False):
        """
        Opens a multislider window to control the data values.

        When editing the grapher with the mouse, the new values are
        sent to the object to replace the table content.

        :Args:

            yrange: tuple, optional
                Set the min and max values of the Y axis of the multislider.
                Defaults to (0.0, 1.0).
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createDataGraphWindow(self, yrange, title, wxnoserver)

    @property
    def size(self):
        """int. Length of the table in samples."""
        return self._size

    @size.setter
    def size(self, x):
        print("DataTable 'size' attribute is read-only.")

    @property
    def chnls(self):
        """int. Number of channels that will be handled by the table."""
        return self._chnls

    @chnls.setter
    def chnls(self, x):
        print("'chnls' attribute is read-only.")

    @property
    def init(self):
        """list of floats. Initial table."""
        return self._init

    @init.setter
    def init(self, x):
        print("'init' attribute is read-only.")


class AtanTable(PyoTableObject):
    """
    Generates an arctangent transfert function.

    This table allow the creation of the classic arctangent transfert function,
    useful in distortion design. See Lookup object for a simple table lookup
    process.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        slope: float, optional
            Slope of the arctangent function, between 0 and 1. Defaults to 0.5.
        size: int, optional
            Table size in samples. Defaults to 8192.

    >>> import math
    >>> s = Server().boot()
    >>> s.start()
    >>> t = AtanTable(slope=0.8)
    >>> a = Sine(freq=[149,150])
    >>> l = Lookup(table=t, index=a, mul=0.3).out()

    """

    def __init__(self, slope=0.5, size=8192):
        pyoArgsAssert(self, "NI", slope, size)
        PyoTableObject.__init__(self, size)
        self._slope = slope
        self._base_objs = [AtanTable_base(slope, size)]

    def setSlope(self, x):
        """
        Change the slope of the arctangent function. This will redraw the table.

        :Args:

            x: float
                New slope between 0 and 1.

        """
        pyoArgsAssert(self, "N", x)
        self._slope = x
        [obj.setSlope(x) for obj in self._base_objs]
        self.refreshView()

    @property
    def slope(self):
        """float. slope of the arctangent function."""
        return self._slope

    @slope.setter
    def slope(self, x):
        self.setSlope(x)


class PartialTable(PyoTableObject):
    """
    Inharmonic waveform generator.

    Generates waveforms made of inharmonic components. Partials are
    given as a list of 2-values tuple, where the first one is the
    partial number (can be float) and the second one is the strength
    of the partial.

    The object uses the first two decimal values of each partial to
    compute a higher harmonic at a multiple of 100 (so each component
    is in reality truly harmonic). If the oscillator has a frequency
    divided by 100, the real desired partials will be restituted.

    The list:

    [(1, 1), (1.1, 0.7), (1.15, 0.5)] will draw a table with:

    harmonic 100: amplitude = 1
    harmonic 110: amplitude = 0.7
    harmonic 115: amplitude = 0.5

    To listen to a signal composed of 200, 220 and 230 Hz, one should
    declared an oscillator like this (frequency of 200Hz divided by 100):

    a = Osc(t, freq=2, mul=0.5).out()

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list: list of tuple, optional
            List of 2-values tuples. First value is the partial number (float up
            to two decimal values) and second value is its amplitude (relative to
            the other harmonics). Defaults to [(1,1), (1.33,0.5),(1.67,0.3)].
        size: int, optional
            Table size in samples. Because computed harmonics are very high in
            frequency, the table size must be bigger than a classic HarmTable.
            Defaults to 65536.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = PartialTable([(1,1), (2.37, 0.5), (4.55, 0.3)]).normalize()
    >>> # Play with fundamentals 199 and 200 Hz
    >>> a = Osc(table=t, freq=[1.99,2], mul=.2).out()

    """

    def __init__(self, list=[(1, 1), (1.33, 0.5), (1.67, 0.3)], size=65536):
        pyoArgsAssert(self, "lI", list, size)
        PyoTableObject.__init__(self, size)
        self._list = list
        self._par_table = HarmTable(self._create_list(), size)
        self._base_objs = self._par_table.getBaseObjects()
        self.normalize()

    def _create_list(self):
        # internal method used to compute the harmonics's weight
        hrms = [(int(x * 100.0), y) for x, y in self._list]
        l = []
        ind = 0
        for i in range(10000):
            if i == hrms[ind][0]:
                l.append(hrms[ind][1])
                ind += 1
                if ind == len(hrms):
                    break
            else:
                l.append(0)
        return l

    def replace(self, list):
        """
        Redraw the waveform according to a new set of harmonics
        relative strengths.

        :Args:

            list: list of tuples
                Each tuple contains the partial number, as a float,
                and its strength.

        """
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(self._create_list()) for obj in self._base_objs]
        self.normalize()
        self.refreshView()

    @property
    def list(self):
        """list. List of partial numbers and strength."""
        return self._list

    @list.setter
    def list(self, x):
        self.replace(x)


class PadSynthTable(PyoTableObject):
    """
    Generates wavetable with the PadSynth algorithm from Nasca Octavian Paul.

    This object generates a wavetable with the PadSynth algorithm describe here:

    http://zynaddsubfx.sourceforge.net/doc/PADsynth/PADsynth.htm

    This algorithm generates some large wavetables that can be played at
    different speeds to get the desired sound. This algorithm describes
    only how these wavetables are generated. The result is a perfectly
    looped wavetable.

    To get the desired pitch from the table, the playback speed must be
    `sr / table size`. This speed can be transposed to obtain different
    pitches from a single wavetable.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        basefreq: float, optional
            The base frequency of the algorithm in Hz. If the spreading factor
            is near 1.0, this frequency is the fundamental of the spectrum.
            Defaults to 440.
        spread: float, optional
            The spreading factor for the harmonics. Each harmonic real frequency
            is computed as `basefreq * pow(n, spread)` where `n` is the harmonic
            order. Defaults to 1.
        bw: float, optional
            The bandwidth of the first harmonic in cents. The bandwidth allows
            to control the harmonic profile using a gaussian distribution (bell
            shape). Defaults to 50.
        bwscl: float, optional
            The bandswidth scale specifies how much the bandwidth of the
            harmonic increase according to its frequency. Defaults to 1.
        nharms: int, optional
            The number of harmonics in the generated wavetable. Higher
            numbers of harmonics take more time to generate the wavetable.
            Defaults to 64.
        damp: float, optional
            The amplitude damping factor specifies how much the amplitude
            of the harmonic decrease according to its order. It uses a
            simple power serie, `amp = pow(damp, n)` where `n` is the
            harmonic order. Defaults to 0.7.
        size: int, optional
            Table size in samples. Must be a power-of-two, usually a big one!
            Defaults to 262144.

    .. note::

        Many thanks to Nasca Octavian Paul for making this beautiful algorithm
        and releasing it under Public Domain.

    >>> s = Server().boot()
    >>> s.start()
    >>> f = s.getSamplingRate() / 262144
    >>> t = PadSynthTable(basefreq=midiToHz(48), spread=1.205, bw=10, bwscl=1.5)
    >>> a = Osc(table=t, freq=f, phase=[0, 0.5], mul=0.5).out()

    """

    def __init__(self, basefreq=440, spread=1, bw=50, bwscl=1, nharms=64, damp=0.7, size=262144):
        pyoArgsAssert(self, "NNNNINI", basefreq, spread, bw, bwscl, nharms, damp, size)
        PyoTableObject.__init__(self, size)
        self._basefreq = basefreq
        self._spread = spread
        self._bw = bw
        self._bwscl = bwscl
        self._nharms = nharms
        self._damp = damp
        self._base_objs = [PadSynthTable_base(basefreq, spread, bw, bwscl, nharms, damp, size)]

    def setBaseFreq(self, x, generate=True):
        """
        Change the base frequency of the algorithm.

        :Args:

            x: float
                New base frequency in Hz.
            generate: boolean, optional
                If True, a new table will be computed with changed value.

        """
        pyoArgsAssert(self, "NB", x, generate)
        self._basefreq = x
        [obj.setBaseFreq(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setSpread(self, x, generate=True):
        """
        Change the frequency spreading factor of the algorithm.

        :Args:

            x: float
                New spread factor.
            generate: boolean, optional
                If True, a new table will be computed with changed value.

        """
        pyoArgsAssert(self, "NB", x, generate)
        self._spread = x
        [obj.setSpread(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setBw(self, x, generate=True):
        """
        Change the bandwidth of the first harmonic.

        :Args:

            x: float
                New bandwidth in cents.
            generate: boolean, optional
                If True, a new table will be computed with changed value.

        """
        pyoArgsAssert(self, "NB", x, generate)
        self._bw = x
        [obj.setBw(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setBwScl(self, x, generate=True):
        """
        Change the bandwidth scaling factor.

        :Args:

            x: float
                New bandwidth scaling factor.
            generate: boolean, optional
                If True, a new table will be computed with changed value.

        """
        pyoArgsAssert(self, "NB", x, generate)
        self._bwscl = x
        [obj.setBwScl(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setNharms(self, x, generate=True):
        """
        Change the number of harmonics.

        :Args:

            x: int
                New number of harmonics.
            generate: boolean, optional
                If True, a new table will be computed with changed value.

        """
        pyoArgsAssert(self, "IB", x, generate)
        self._nharms = x
        [obj.setNharms(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setDamp(self, x, generate=True):
        """
        Change the amplitude damping factor.

        :Args:

            x: float
                New amplitude damping factor.
            generate: boolean, optional
                If True, a new table will be computed with changed value.

        """
        pyoArgsAssert(self, "NB", x, generate)
        self._damp = x
        [obj.setDamp(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setSize(self, size, generate=True):
        """
        Change the size of the table.

        This will erase the previously drawn waveform.

        :Args:

            size: int
                New table size in samples. Must be a power-of-two.
            generate: boolean, optional
                If True, a new table will be computed with changed value.

        """
        pyoArgsAssert(self, "IB", size, generate)
        self._size = size
        [obj.setSize(size, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    @property
    def basefreq(self):
        """float. Base frequency in Hz."""
        return self._basefreq

    @basefreq.setter
    def basefreq(self, x):
        self.setBaseFreq(x)

    @property
    def spread(self):
        """float. Frequency spreading factor."""
        return self._spread

    @spread.setter
    def spread(self, x):
        self.setSpread(x)

    @property
    def bw(self):
        """float. Bandwitdh of the first harmonic in cents."""
        return self._bw

    @bw.setter
    def bw(self, x):
        self.setBw(x)

    @property
    def bwscl(self):
        """float. Bandwitdh scaling factor."""
        return self._bwscl

    @bwscl.setter
    def bwscl(self, x):
        self.setBwScl(x)

    @property
    def nharms(self):
        """int. Number of harmonics."""
        return self._nharms

    @nharms.setter
    def nharms(self, x):
        self.setNharms(x)

    @property
    def damp(self):
        """float. Amplitude damping factor."""
        return self._damp

    @damp.setter
    def damp(self, x):
        self.setDamp(x)


class SharedTable(PyoTableObject):
    """
    Create an inter-process shared memory table.

    This table uses the given name to open an internal shared memory
    object, used as the data memory of the table. Two or more tables
    from different processes, if they use the same name, can read and
    write to the same memory space.

    .. note::

        SharedTable is not implemented yet for Windows (unix only).

    :Parent: :py:class:`PyoTableObject`

    :Args:

        name: string
            Unique name in the system shared memory. Two or more tables created
            with the same name will shared the same memory space. The name
            must conform to the construction rules for a unix pathname (ie.
            it must begin with a slash). Available at initialization time only.
        create: boolean
            If True, an entry will be create in the system shared memory.
            If False, the object will use an already created shared memory.
            Can't be a list. Available at initialization time only.
        size: int
            Size of the table in samples. Can't be a list.
            Available at initialization time only.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Creating parent table.
    >>> table = SharedTable(["/sharedl", "/sharedr"], True, s.getBufferSize())
    >>> # Creating child table.
    >>> shared = SharedTable(["/sharedl", "/sharedr"], False, s.getBufferSize())
    >>> # Read and output the content of the parent table.
    >>> tabread = TableRead(table, table.getRate(), True).out()
    >>> # Record some signal signal in the child table.
    >>> lfo = Sine(freq=[0.2, 0.25]).range(98, 102)
    >>> wave = LFO(freq=lfo, type=4, sharp=0.7, mul=0.3)
    >>> pos = Phasor(shared.getRate())
    >>> record = TableWrite(wave, pos, shared)

    """

    def __init__(self, name, create, size):
        if sys.platform == "win32":
            raise Exception("SharedTable is not implemented yet for Windows.")
        pyoArgsAssert(self, "sBI", name, create, size)
        PyoTableObject.__init__(self, size)
        self._name = name
        self._create = create
        name, lmax = convertArgsToLists(name)
        self._base_objs = [SharedTable_base(wrap(name, i), create, size) for i in range(lmax)]

    @property
    def size(self):
        """int. Length of the table in samples."""
        return self._size

    @size.setter
    def size(self, x):
        print("SharedTable 'size' attribute is read-only.")
