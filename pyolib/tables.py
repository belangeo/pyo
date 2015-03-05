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
from _core import *
from _maps import *
from _widgets import createGraphWindow, createDataGraphWindow, createSndViewTableWindow
from types import ListType
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

        list : list, optional
            Relative strengths of the fixed harmonic partial numbers 1,2,3, etc.
            Defaults to [1].
        size : int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Square wave up to 9th harmonic
    >>> t = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    >>> a = Osc(table=t, freq=[199,200], mul=.2).out()

    """
    def __init__(self, list=[1., 0.], size=8192):
        PyoTableObject.__init__(self, size)
        self._list = copy.deepcopy(list)
        self._base_objs = [HarmTable_base(self._list, size)]

    def replace(self, list):
        """
        Redraw the waveform according to a new set of harmonics
        relative strengths.

        :Args:

            list : list
                Relative strengths of the fixed harmonic partial
                numbers 1,2,3, etc.

        """
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    @property
    def list(self):
        """list. Relative strengths of the fixed harmonic partial numbers."""
        return self._list
    @list.setter
    def list(self, x): self.replace(x)

class SawTable(PyoTableObject):
    """
    Sawtooth waveform generator.

    Generates sawtooth waveforms made up of fixed number of harmonics.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        order : int, optional
            Number of harmonics sawtooth is made of.
            Defaults to 10.
        size : int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = SawTable(order=12).normalize()
    >>> a = Osc(table=t, freq=[199,200], mul=.2).out()

    """
    def __init__(self, order=10, size=8192):
        PyoTableObject.__init__(self, size)
        self._order = order
        list = [1./i for i in range(1,(order+1))]
        self._base_objs = [HarmTable_base(list, size)]

    def setOrder(self, x):
        """
        Change the `order` attribute and redraw the waveform.

        :Args:

            x : int
                New number of harmonics

        """
        self._order = x
        list = [1./i for i in range(1,(self._order+1))]
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    @property
    def order(self):
        """int. Number of harmonics sawtooth is made of."""
        return self._order
    @order.setter
    def order(self, x): self.setOrder(x)

class SquareTable(PyoTableObject):
    """
    Square waveform generator.

    Generates square waveforms made up of fixed number of harmonics.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        order : int, optional
            Number of harmonics square waveform is made of. The waveform will
            contains `order` odd harmonics. Defaults to 10.
        size : int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = SquareTable(order=15).normalize()
    >>> a = Osc(table=t, freq=[199,200], mul=.2).out()

    """
    def __init__(self, order=10, size=8192):
        PyoTableObject.__init__(self, size)
        self._order = order
        list = []
        for i in range(1,(order*2)):
            if i%2 == 1:
                list.append(1./i)
            else:
                list.append(0.)
        self._base_objs = [HarmTable_base(list, size)]

    def setOrder(self, x):
        """
        Change the `order` attribute and redraw the waveform.

        :Args:

            x : int
                New number of harmonics

        """
        self._order = x
        list = []
        for i in range(1,(self._order*2)):
            if i%2 == 1:
                list.append(1./i)
            else:
                list.append(0.)
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    @property
    def order(self):
        """int. Number of harmonics square waveform is made of."""
        return self._order
    @order.setter
    def order(self, x): self.setOrder(x)

class ChebyTable(PyoTableObject):
    """
    Chebyshev polynomials of the first kind.

    Uses Chebyshev coefficients to generate stored polynomial functions
    which, under waveshaping, can be used to split a sinusoid into
    harmonic partials having a pre-definable spectrum.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list : list, optional
            Relative strengths of partials numbers 1,2,3, ..., 12 that will
            result when a sinusoid of amplitude 1 is waveshaped using this
            function table. Up to 12 partials can be specified. Defaults to [1].
        size : int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = ChebyTable([1,0,.33,0,.2,0,.143,0,.111])
    >>> lfo = Sine(freq=.25, mul=0.45, add=0.5)
    >>> a = Sine(freq=[200,201], mul=lfo)
    >>> b = Lookup(table=t, index=a, mul=1-lfo).out()

    """
    def __init__(self, list=[1., 0.], size=8192):
        PyoTableObject.__init__(self, size)
        self._list = copy.deepcopy(list)
        self._base_objs = [ChebyTable_base(self._list, size)]

    def replace(self, list):
        """
        Redraw the waveform according to a new set of harmonics
        relative strengths that will result when a sinusoid of
        amplitude 1 is waveshaped using this function table.

        :Args:

            list : list
                Relative strengths of the fixed harmonic partial
                numbers 1,2,3, ..., 12. Up to 12 partials can be specified.

        """
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
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

    @property
    def list(self):
        """list. Relative strengths of the fixed harmonic partial numbers."""
        return self._list
    @list.setter
    def list(self, x): self.replace(x)

class HannTable(PyoTableObject):
    """
    Generates Hanning window function.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        size : int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Hanning envelope
    >>> t = HannTable()
    >>> a = Osc(table=t, freq=2, mul=.2)
    >>> b = Sine(freq=[299,300], mul=a).out()

    """
    def __init__(self, size=8192):
        PyoTableObject.__init__(self, size)
        self._base_objs = [HannTable_base(size)]

class SincTable(PyoTableObject):
    """
    Generates sinc window function.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        freq : float, optional
            Frequency, in radians, of the sinc function. Defaults to pi*2.
        windowed : boolean, optional
            If True, an hanning window is applied on the sinc function. Defaults to False.
        size : int, optional
            Table size in samples. Defaults to 8192.

    >>> import math
    >>> s = Server().boot()
    >>> s.start()
    >>> t = SincTable(freq=math.pi*6, windowed=True)
    >>> a = Osc(t, freq=[199,200], mul=.2).out()

    """
    def __init__(self, freq=pi*2, windowed=False, size=8192):
        PyoTableObject.__init__(self, size)
        self._freq = freq
        self._windowed = windowed
        self._base_objs = [SincTable_base(freq, windowed, size)]

    def setFreq(self, x):
        """
        Change the frequency of the sinc function. This will redraw the envelope.

        :Args:

            x : float
                New frequency in radians.

        """
        self._freq = x
        [obj.setFreq(x) for obj in self._base_objs]
        self.refreshView()

    def setWindowed(self, x):
        """
        Change the windowed flag. This will redraw the envelope.

        :Args:

            x : boolean
                New windowed flag.

        """
        self._windowed = x
        [obj.setWindowed(x) for obj in self._base_objs]
        self.refreshView()

    @property
    def freq(self):
        """float. Frequency of the sinc function."""
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)
    @property
    def windowed(self):
        """boolean. Windowed flag."""
        return self._windowed
    @windowed.setter
    def windowed(self, x): self.setWindowed(x)

class WinTable(PyoTableObject):
    """
    Generates different kind of windowing functions.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        type : int, optional
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
        size : int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Triangular envelope
    >>> t = WinTable(type=3)
    >>> a = Osc(table=t, freq=2, mul=.2)
    >>> b = SineLoop(freq=[199,200], feedback=0.05, mul=a).out()

    """
    def __init__(self, type=2, size=8192):
        PyoTableObject.__init__(self, size)
        self._type = type
        self._base_objs = [WinTable_base(type, size)]

    def setType(self, type):
        """
        Sets the windowing function.

        :Args:

            type : int {0 -> 8}
                Windowing function.

        """
        self._type = type
        [obj.setType(type) for obj in self._base_objs]
        self.refreshView()

    @property
    def type(self):
        """int. Windowing function."""
        return self._type
    @type.setter
    def type(self, x): self.setType(x)

class ParaTable(PyoTableObject):
    """
    Generates parabola window function.

    The parabola is a conic section, the intersection of a right circular conical
    surface and a plane parallel to a generating straight line of that surface.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        size : int, optional
            Table size in samples. Defaults to 8192.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Parabola envelope
    >>> t = ParaTable()
    >>> a = Osc(table=t, freq=2, mul=.2)
    >>> b = SineLoop(freq=[299,300], feedback=0.05, mul=a).out()

    """
    def __init__(self, size=8192):
        PyoTableObject.__init__(self, size)
        self._base_objs = [ParaTable_base(size)]

class LinTable(PyoTableObject):
    """
    Construct a table from segments of straight lines in breakpoint fashion.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list : list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8191, 1.)], creates a
            straight line from 0.0 at location 0 to 1.0 at the end of the
            table (size - 1). Location must be an integer.
        size : int, optional
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
    def __init__(self, list=[(0, 0.), (8191, 1.)], size=8192):
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print "LinTable warning : size smaller than last point position."
            print "                   Increased size to last point position + 1"
            size = list[-1][0] + 1
            self._size = size
        self._base_objs = [LinTable_base(copy.deepcopy(list), size)]

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list : list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
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

            filename : string
                Full path of an automation recording file.
            tolerance : float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files,i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0]*scl), v[1]) for v in values]
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

            yrange : tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title : string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver : boolean, optional
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
    def list(self, x): self.replace(x)

class LogTable(PyoTableObject):
    """
    Construct a table from logarithmic segments in breakpoint fashion.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list : list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8191, 1.)], creates a
            logarithmic line from 0.0 at location 0 to 1.0 at the end of
            the table (size - 1). Location must be an integer.
        size : int, optional
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
    def __init__(self, list=[(0, 0.), (8191, 1.)], size=8192):
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print "LogTable warning : size smaller than last point position."
            print "                   Increased size to last point position + 1"
            size = list[-1][0] + 1
            self._size = size
        self._base_objs = [LogTable_base(copy.deepcopy(list), size)]

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list : list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
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

            filename : string
                Full path of an automation recording file.
            tolerance : float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files,i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0]*scl), v[1]) for v in values]
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

            yrange : tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title : string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver : boolean, optional
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
    def list(self, x): self.replace(x)

class CosLogTable(PyoTableObject):
    """
    Construct a table from logarithmic-cosine segments in breakpoint fashion.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list : list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8191, 1.)], creates a
            logarithmic line from 0.0 at location 0 to 1.0 at the end of
            the table (size - 1). Location must be an integer.
        size : int, optional
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
    def __init__(self, list=[(0, 0.), (8191, 1.)], size=8192):
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print "CosLogTable warning : size smaller than last point position."
            print "                   Increased size to last point position + 1"
            size = list[-1][0] + 1
            self._size = size
        self._base_objs = [CosLogTable_base(copy.deepcopy(list), size)]

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list : list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
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

            filename : string
                Full path of an automation recording file.
            tolerance : float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files,i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0]*scl), v[1]) for v in values]
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

            yrange : tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title : string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver : boolean, optional
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
    def list(self, x): self.replace(x)

class CosTable(PyoTableObject):
    """
    Construct a table from cosine interpolated segments.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list : list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8191, 1.)], creates a
            cosine line from 0.0 at location 0 to 1.0 at the end of the
            table (size - 1). Location must be an integer.
        size : int, optional
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
    def __init__(self, list=[(0, 0.), (8191, 1.)], size=8192):
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print "CosTable warning : size smaller than last point position."
            print "                   Increased size to last point position + 1"
            size = list[-1][0] + 1
            self._size = size
        self._base_objs = [CosTable_base(copy.deepcopy(list), size)]

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list : list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
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

            filename : string
                Full path of an automation recording file.
            tolerance : float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files,i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0]*scl), v[1]) for v in values]
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

            yrange : tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title : string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver : boolean, optional
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
    def list(self, x): self.replace(x)

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

        list : list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8191, 1.)], creates a
            curved line from 0.0 at location 0 to 1.0 at the end of the
            table (size - 1). Location must be an integer.
        tension : float, optional
            Curvature at the known points. 1 is high, 0 normal, -1 is low.
            Defaults to 0.
        bias : float, optional
            Curve attraction (for each segments) toward bundary points.
            0 is even, positive is towards first point, negative is towards
            the second point. Defaults to 0.
        size : int, optional
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
    def __init__(self, list=[(0, 0.), (8191, 1.)], tension=0, bias=0, size=8192):
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print "CurveTable warning : size smaller than last point position."
            print "                     Increased size to last point position + 1"
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

            x : float
                New `tension` attribute.

        """
        self._tension = x
        [obj.setTension(x) for obj in self._base_objs]
        self.refreshView()

    def setBias(self, x):
        """
        Replace the `bias` attribute.

        0 is even, positive is towards first point, negative is towards
        the second point.

        :Args:

            x : float
                New `bias` attribute.

        """
        self._bias = x
        [obj.setBias(x) for obj in self._base_objs]
        self.refreshView()

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list : list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
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

            filename : string
                Full path of an automation recording file.
            tolerance : float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files,i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0]*scl), v[1]) for v in values]
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

            yrange : tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title : string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver : boolean, optional
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
    def tension(self, x): self.setTension(x)

    @property
    def bias(self):
        """float. Curve Attraction."""
        return self._bias
    @bias.setter
    def bias(self, x): self.setBias(x)

    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table."""
        return self.getPoints()
    @list.setter
    def list(self, x): self.replace(x)

class ExpTable(PyoTableObject):
    """
    Construct a table from exponential interpolated segments.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list : list, optional
            List of tuples indicating location and value of each points
            in the table. The default, [(0,0.), (8192, 1.)], creates a
            exponential line from 0.0 at location 0 to 1.0 at the end of
            the table. Location must be an integer.
        exp : float, optional
            Exponent factor. Used to control the slope of the curve.
            Defaults to 10.
        inverse : boolean, optional
            If True, downward slope will be inversed. Useful to create
            biexponential curves. Defaults to True.
        size : int, optional
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
    def __init__(self, list=[(0, 0.), (8192, 1.)], exp=10, inverse=True, size=8192):
        PyoTableObject.__init__(self, size)
        if size < list[-1][0]:
            print "ExpTable warning : size smaller than last point position."
            print "                   Increased size to last point position + 1"
            size = list[-1][0] + 1
            self._size = size
        self._exp = exp
        self._inverse = inverse
        self._base_objs = [ExpTable_base(copy.deepcopy(list), exp, inverse, size)]

    def setExp(self, x):
        """
        Replace the `exp` attribute.

        :Args:

            x : float
                New `exp` attribute.

        """
        self._exp = x
        [obj.setExp(x) for obj in self._base_objs]
        self.refreshView()

    def setInverse(self, x):
        """
        Replace the `inverse` attribute.

        :Args:

            x : boolean
                New `inverse` attribute.

        """
        self._inverse = x
        [obj.setInverse(x) for obj in self._base_objs]
        self.refreshView()

    def replace(self, list):
        """
        Draw a new envelope according to the new `list` parameter.

        :Args:

            list : list
                List of tuples indicating location and value of each points
                in the table. Location must be integer.

        """
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

            filename : string
                Full path of an automation recording file.
            tolerance : float, optional
                Tolerance of the filter. A higher value will eliminate more points.
                Defaults to 0.02.

        """
        _path, _name = os.path.split(filename)
        # files = sorted([f for f in os.listdir(_path) if _name+"_" in f])
        # if _name not in files: files.append(_name)
        files = [filename]
        for i, obj in enumerate(self._base_objs):
            p = os.path.join(_path, wrap(files,i))
            f = open(p, "r")
            values = [(float(l.split()[0]), float(l.split()[1])) for l in f.readlines()]
            scl = self._size / values[-1][0]
            values = [(int(v[0]*scl), v[1]) for v in values]
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

            yrange : tuple, optional
                Set the min and max values of the Y axis of the graph.
                Defaults to (0.0, 1.0).
            title : string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver : boolean, optional
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
    def exp(self, x): self.setExp(x)
    @property
    def inverse(self):
        """boolean. Inverse factor."""
        return self._inverse
    @inverse.setter
    def inverse(self, x): self.setInverse(x)
    @property
    def list(self):
        """list. List of tuples indicating location and value of each points in the table."""
        return self.getPoints()
    @list.setter
    def list(self, x): self.replace(x)

class SndTable(PyoTableObject):
    """
    Transfers data from a soundfile into a function table.

    If `chnl` is None, the table will contain as many table streams as
    necessary to read all channels of the loaded sound.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        path : string, optional
            Full path name of the sound. The defaults, None, creates an empty
            table.
        chnl : int, optional
            Channel number to read in. Available at initialization time only.
            The default (None) reads all channels.
        start : float, optional
            Begins reading at `start` seconds into the file. Available at
            initialization time only. Defaults to 0.
        stop : float, optional
            Stops reading at `stop` seconds into the file. Available at
            initialization time only. The default (None) means the end of
            the file.

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
        if self._path == None:
            self._base_objs = [SndTable_base("", 0, 0) for i in range(initchnls)]
        else:
            for p in path:
                _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(p)
                if chnl == None:
                    if stop == None:
                        self._base_objs.extend([SndTable_base(p, i, start) for i in range(_snd_chnls)])
                    else:
                        self._base_objs.extend([SndTable_base(p, i, start, stop) for i in range(_snd_chnls)])
                else:
                    if stop == None:
                        self._base_objs.append(SndTable_base(p, chnl, start))
                    else:
                        self._base_objs.append(SndTable_base(p, chnl, start, stop))
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

            path : string
                Full path of the new sound.
            start : float, optional
                Begins reading at `start` seconds into the file. Defaults to 0.
            stop : float, optional
                Stops reading at `stop` seconds into the file. The default (None)
                means the end of the file.

        """
        self._path = path
        if type(path) == ListType:
            self._size = []
            self._dur = []
            path, lmax = convertArgsToLists(path)
            for i, obj in enumerate(self._base_objs):
                p = path[i%lmax]
                _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(p)
                self._size.append(_size)
                self._dur.append(_dur)
                if stop == None:
                    obj.setSound(p, 0, start)
                else:
                    obj.setSound(p, 0, start, stop)
        else:
            _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(path)
            self._size = _size
            self._dur = _dur
            if stop == None:
                [obj.setSound(path, (i%_snd_chnls), start) for i, obj in enumerate(self._base_objs)]
            else:
                [obj.setSound(path, (i%_snd_chnls), start, stop) for i, obj in enumerate(self._base_objs)]
        self.refreshView()

    def append(self, path, crossfade=0, start=0, stop=None):
        """
        Append a sound to the one already in the table with crossfade.

        Keeps the number of channels of the sound loaded at initialization.
        If the new sound has less channels, it will wrap around and load
        the same channels many times. If the new sound has more channels,
        the extra channels will be skipped.

        :Args:

            path : string
                Full path of the new sound.
            crossfade : float, optional
                Crossfade time, in seconds, between the sound already in the table
                and the new one. Defaults to 0.
            start : float, optional
                Begins reading at `start` seconds into the file. Defaults to 0.
            stop : float, optional
                Stops reading at `stop` seconds into the file. The default, None,
                means the end of the file.

        """
        if type(path) == ListType:
            self._size = []
            self._dur = []
            path, lmax = convertArgsToLists(path)
            for i, obj in enumerate(self._base_objs):
                p = path[i%lmax]
                _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(p)
                self._size.append(_size)
                self._dur.append(_dur)
                if stop == None:
                    obj.append(p, crossfade, 0, start)
                else:
                    obj.append(p, crossfade, 0, start, stop)
        else:
            _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(path)
            self._size = _size
            self._dur = _dur
            if stop == None:
                [obj.append(path, crossfade, (i%_snd_chnls), start) for i, obj in enumerate(self._base_objs)]
            else:
                [obj.append(path, crossfade, (i%_snd_chnls), start, stop) for i, obj in enumerate(self._base_objs)]
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

            path : string
                Full path of the new sound.
            pos : float, optional
                Position in the table, in seconds, where to insert the new sound.
                Defaults to 0.
            crossfade : float, optional
                Crossfade time, in seconds, between the sound already in the table
                and the new one. Defaults to 0.
            start : float, optional
                Begins reading at `start` seconds into the file. Defaults to 0.
            stop : float, optional
                Stops reading at `stop` seconds into the file. The default, None,
                means the end of the file.

        """
        if type(path) == ListType:
            self._size = []
            self._dur = []
            path, lmax = convertArgsToLists(path)
            for i, obj in enumerate(self._base_objs):
                p = path[i%lmax]
                _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(p)
                self._size.append(_size)
                self._dur.append(_dur)
                if stop == None:
                    obj.insert(p, pos, crossfade, 0, start)
                else:
                    obj.insert(p, pos, crossfade, 0, start, stop)
        else:
            _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(path)
            self._size = _size
            self._dur = _dur
            if stop == None:
                [obj.insert(path, pos, crossfade, (i%_snd_chnls), start) for i, obj in enumerate(self._base_objs)]
            else:
                [obj.insert(path, pos, crossfade, (i%_snd_chnls), start, stop) for i, obj in enumerate(self._base_objs)]
        self.refreshView()

    def getRate(self):
        """
        Return the frequency in cps at which the sound will be read at its
        original pitch.

        """
        if type(self._path) == ListType:
            return [obj.getRate() for obj in self._base_objs]
        else:
            return self._base_objs[0].getRate()

    def getDur(self, all=True):
        """
        Return the duration of the sound in seconds.

        :Args:

            all : boolean
                If the table contains more than one sound and `all` is True,
                returns a list of all durations. Otherwise, returns only the
                first duration as a float.

        """
        if type(self._path) == ListType:
            _dur = [1./obj.getRate() for obj in self._base_objs]
        else:
            _dur = 1./self._base_objs[0].getRate()

        if all:
            return _dur
        else:
            if type(_dur) == ListType:
                return _dur[0]
            else:
                return _dur

    def setSize(self, x):
        print "SndTable has no setSize method!"

    def getSize(self, all=True):
        """
        Return the size of the table in samples.

        :Args:

            all : boolean
                If the table contains more than one sound and `all` is True,
                returns a list of all sizes. Otherwise, returns only the
                first size as an int.

        """
        if len(self._base_objs) > 1:
            _size = [obj.getSize() for obj in self._base_objs]
        else:
            _size = self._base_objs[0].getSize()

        if all:
            return _size
        else:
            if type(_size) == ListType:
                return _size[0]
            else:
                return _size

    def getViewTable(self, size, begin=0, end=0):
        """
        Return a list of points (in X, Y pixel values) for each channel in the table.
        These lists can be draw on a DC (WxPython) with a DrawLines method.

        :Args:

            size : tuple
                Size, (X, Y) pixel values, of the waveform container window.
            begin : float, optional
                First position in the the table, in seconds, where to get samples.
                Defaults to 0.
            end : float, optional
                Last position in the table, in seconds, where to get samples.

                if this value is set to 0, that means the end of the table. Defaults to 0.

        """
        w, h = size
        chnls = len(self._base_objs)
        img = []
        imgHeight = h/chnls
        for i in range(chnls):
            off = h/chnls*i
            img.append(self._base_objs[i].getViewTable((w, imgHeight), begin, end, off))
        return img

    def getEnvelope(self, points):
        """
        Return the amplitude envelope of the table.

        Return a list, of length `chnl`, of lists of length `points` filled
        with the amplitude envelope of the table.

        :Args:

            points : int
                Number of points of the amplitude analysis.

        """
        return [obj.getEnvelope(points) for obj in self._base_objs]

    def view(self, title="Sound waveform", wxnoserver=False, mouse_callback=None):
        """
        Opens a window showing the contents of the table.

        :Args:

            title : string, optional
                Window title. Defaults to "Table waveform".
            wxnoserver : boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.
            mouse_callback : callable
                If provided, this function will be called with the mouse
                position, inside the frame, as argument. Defaults to None.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createSndViewTableWindow(self, title, wxnoserver, self.__class__.__name__, mouse_callback)

    def refreshView(self):
        if self.viewFrame != None:
            self.viewFrame.update()

    @property
    def sound(self):
        """string. Full path of the sound."""
        return self._path
    @sound.setter
    def sound(self, x): self.setSound(x)

    @property
    def path(self):
        """string. Full path of the sound."""
        return self._path
    @path.setter
    def path(self, x): self.setSound(x)

    @property
    def chnl(self):
        """int. Channel to read in."""
        return self._chnl
    @chnl.setter
    def chnl(self, x): print "'chnl' attribute is read-only."

    @property
    def start(self):
        """float. Start point, in seconds, to read into the file."""
        return self._start
    @start.setter
    def start(self, x): print "'start' attribute is read-only."

    @property
    def stop(self):
        """float. Stop point, in seconds, to read into the file."""
        return self._stop
    @stop.setter
    def stop(self, x): print "SndTable 'stop' attribute is read-only."

    @property
    def size(self):
        return self._size
    @size.setter
    def size(self, x): print "SndTable 'size' attribute is read-only."

class NewTable(PyoTableObject):
    """
    Create an empty table ready for recording.

    See :py:class:`TableRec` to write samples in the table.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        length : float
            Length of the table in seconds.
        chnls : int, optional
            Number of channels that will be handled by the table.
            Defaults to 1.
        init : list of floats, optional
            Initial table. List of list can match the number of channels,
            otherwise, the list will be loaded in all tablestreams.
            Defaults to None.
        feedback : float, optional
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
        PyoTableObject.__init__(self)
        self._length = length
        self._chnls = chnls
        self._init = init
        self._feedback = feedback
        if init == None:
            self._base_objs = [NewTable_base(length, None, feedback) for i in range(chnls)]
        else:
            if type(init[0]) != ListType:
                init = [init]
            self._base_objs = [NewTable_base(length, wrap(init,i), feedback) for i in range(chnls)]
        self._size = self._base_objs[0].getSize()

    def replace(self, x):
        """
        Replaces the actual table.

        :Args:

            x : list of floats
                New table. Must be of the same size as the actual table.

                List of list can match the number of channels, otherwise,
                the list will be loaded in all tablestreams.

        """
        if type(x[0]) != ListType:
            x = [x]
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        self.refreshView()

    def setFeedback(self, x):
        """
        Replaces the`feedback` attribute.

        :Args:

            x : float
                New `feedback` value.

        """
        self._feedback = x
        [obj.setFeedback(x) for i, obj in enumerate(self._base_objs)]

    def getLength(self):
        """
        Returns the length of the table in seconds.

        """
        return self._base_objs[0].getLength()

    def getDur(self, all=True):
        """
        Returns the length of the table in seconds.

        The `all` argument is there for compatibility with SndTable but
        is not used for now.

        """
        return self._base_objs[0].getLength()

    def getRate(self):
        """
        Returns the frequency (cycle per second) to give to an
        oscillator to read the sound at its original pitch.

        """
        return self._base_objs[0].getRate()

    def getViewTable(self, size, begin=0, end=0):
        """
        Return a list of points (in X, Y pixel values) for each channel in the table.
        These lists can be draw on a DC (WxPython) with a DrawLines method.

        :Args:

            size : tuple
                Size, (X, Y) pixel values, of the waveform container window.
            begin : float, optional
                First position in the the table, in seconds, where to get samples.
                Defaults to 0.
            end : float, optional
                Last position in the table, in seconds, where to get samples.

                if this value is set to 0, that means the end of the table. Defaults to 0.

        """
        w, h = size
        chnls = len(self._base_objs)
        img = []
        imgHeight = h/chnls
        for i in range(chnls):
            off = h/chnls*i
            img.append(self._base_objs[i].getViewTable((w, imgHeight), begin, end, off))
        return img

    def view(self, title="Sound waveform", wxnoserver=False, mouse_callback=None):
        """
        Opens a window showing the contents of the table.

        :Args:

            title : string, optional
                Window title. Defaults to "Table waveform".
            wxnoserver : boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.
            mouse_callback : callable
                If provided, this function will be called with the mouse
                position, inside the frame, as argument. Defaults to None.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        createSndViewTableWindow(self, title, wxnoserver, self.__class__.__name__, mouse_callback)

    def refreshView(self):
        if self.viewFrame != None:
            self.viewFrame.update()

    @property
    def length(self):
        """float. Length of the table in seconds."""
        return self._length
    @length.setter
    def length(self, x): print "'length' attribute is read-only."

    @property
    def chnls(self):
        """int. Number of channels that will be handled by the table."""
        return self._chnls
    @chnls.setter
    def chnls(self, x): print "'chnls' attribute is read-only."

    @property
    def init(self):
        """list of floats. Initial table."""
        return self._init
    @init.setter
    def init(self, x): print "'init' attribute is read-only."

    @property
    def feedback(self):
        """float. Amount of old data to mix with a new recording."""
        return self._feedback
    @feedback.setter
    def feedback(self, x): self.setFeedback(x)

    @property
    def size(self):
        return self._size
    @size.setter
    def size(self, x): print "NewTable 'size' attribute is read-only."

class DataTable(PyoTableObject):
    """
    Create an empty table ready for data recording.

    See :py:class:`TableRec` to write samples in the table.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        size : int
            Size of the table in samples.
        chnls : int, optional
            Number of channels that will be handled by the table.
            Defaults to 1.
        init : list of floats, optional
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
        PyoTableObject.__init__(self, size)
        self._chnls = chnls
        self._init = init
        if init == None:
            self._base_objs = [DataTable_base(size) for i in range(chnls)]
        else:
            if type(init[0]) != ListType:
                init = [init]
            self._base_objs = [DataTable_base(size, wrap(init,i)) for i in range(chnls)]

    def replace(self, x):
        """
        Replaces the actual table.

        :Args:

            x : list of floats
                New table. Must be of the same size as the actual table.

                List of list can match the number of channels, otherwise,
                the list will be loaded in all tablestreams.

        """
        if type(x[0]) != ListType:
            x = [x]
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        self.refreshView()

    def getRate(self):
        """
        Returns the frequency (cycle per second) to give to an
        oscillator to read the sound at its original pitch.

        """
        return self._base_objs[0].getRate()

    def graph(self, yrange=(0.0, 1.0), title=None, wxnoserver=False):
        """
        Opens a multislider window to control the data values.

        When editing the grapher with the mouse, the new values are
        sent to the object to replace the table content.

        :Args:

            yrange : tuple, optional
                Set the min and max values of the Y axis of the multislider.
                Defaults to (0.0, 1.0).
            title : string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver : boolean, optional
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
    def size(self, x): print "DataTable 'size' attribute is read-only."

    @property
    def chnls(self):
        """int. Number of channels that will be handled by the table."""
        return self._chnls
    @chnls.setter
    def chnls(self, x): print "'chnls' attribute is read-only."

    @property
    def init(self):
        """list of floats. Initial table."""
        return self._init
    @init.setter
    def init(self, x): print "'init' attribute is read-only."

class AtanTable(PyoTableObject):
    """
    Generates an arctangent transfert function.

    This table allow the creation the classic arctangent transfert functions,
    useful in distortion design. See Lookup object for a simple table lookup
    process.

    :Parent: :py:class:`PyoTableObject`

    :Args:

        slope : float, optional
            Slope of the arctangent function, between 0 and 1. Defaults to 0.5.
        size : int, optional
            Table size in samples. Defaults to 8192.

    >>> import math
    >>> s = Server().boot()
    >>> s.start()
    >>> t = AtanTable(slope=0.8)
    >>> a = Sine(freq=[149,150])
    >>> l = Lookup(table=t, index=a, mul=0.3).out()

    """
    def __init__(self, slope=0.5, size=8192):
        PyoTableObject.__init__(self, size)
        self._slope = slope
        self._base_objs = [AtanTable_base(slope, size)]

    def setSlope(self, x):
        """
        Change the slope of the arctangent function. This will redraw the table.

        :Args:

            x : float
                New slope between 0 and 1.

        """
        self._slope = x
        [obj.setSlope(x) for obj in self._base_objs]
        self.refreshView()

    @property
    def slope(self):
        """float. slope of the arctangent function."""
        return self._slope
    @slope.setter
    def slope(self, x): self.setSlope(x)

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

    harmonic 100 : amplitude = 1
    harmonic 110 : amplitude = 0.7
    harmonic 115 : amplitude = 0.5

    To listen to a signal composed of 200, 220 and 230 Hz, one should
    declared an oscillator like this (frequency of 200Hz divided by 100):

    a = Osc(t, freq=2, mul=0.5).out()

    :Parent: :py:class:`PyoTableObject`

    :Args:

        list : list of tuple, optional
            List of 2-values tuples. First value is the partial number (float up
            to two decimal values) and second value is its amplitude (relative to
            the other harmonics). Defaults to [(1,1), (1.33,0.5),(1.67,0.3)].
        size : int, optional
            Table size in samples. Because computed harmonics are very high in
            frequency, the table size must be bigger than a classic HarmTable.
            Defaults to 65536.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = PartialTable([(1,1), (2.37, 0.5), (4.55, 0.3)]).normalize()
    >>> # Play with fundamentals 199 and 200 Hz
    >>> a = Osc(table=t, freq=[1.99,2], mul=.2).out()

    """
    def __init__(self, list=[(1,1), (1.33,0.5),(1.67,0.3)], size=65536):
        PyoTableObject.__init__(self, size)
        self._list = list
        self._par_table = HarmTable(self._create_list(), size)
        self._base_objs = self._par_table.getBaseObjects()
        self.normalize()

    def _create_list(self):
        # internal method used to compute the harmonics's weight
        hrms = [(int(x*100.), y) for x, y in self._list]
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

            list : list of tuples
                Each tuple contains the partial number, as a float,
                and its strength.

        """
        self._list = list
        [obj.replace(self._create_list()) for obj in self._base_objs]
        self.normalize()
        self.refreshView()

    @property
    def list(self):
        """list. List of partial numbers and strength."""
        return self._list
    @list.setter
    def list(self, x): self.replace(x)