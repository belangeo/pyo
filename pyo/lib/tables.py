# Copyright 2009-2021 Olivier Belanger
# 
# This file is part of pyo, a python module to help digital signal
# processing script creation.
#
# pyo is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# pyo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with pyo.  If not, see <http://www.gnu.org/licenses/>.

from ._core import *
from math import pi
import copy

# For simplified SndTable
import sndhdr
import struct
import wave

######################################################################
### Tables
######################################################################
class HarmTable(PyoTableObject):

    def __init__(self, list=[1.0, 0.0], size=8192):
        pyoArgsAssert(self, "lI", list, size)
        PyoTableObject.__init__(self, size)
        self._auto_normalize = False
        self._list = copy.deepcopy(list)
        self._base_objs = [HarmTable_base(self._list, size)]

    def autoNormalize(self, x):
        self._auto_normalize = x
        if self._auto_normalize:
            self.normalize()

    def replace(self, list):
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        if self._auto_normalize:
            self.normalize()
        self.refreshView()

    @property
    def list(self):
        return self._list

    @list.setter
    def list(self, x):
        self.replace(x)


class SawTable(PyoTableObject):

    def __init__(self, order=10, size=8192):
        pyoArgsAssert(self, "II", order, size)
        PyoTableObject.__init__(self, size)
        self._order = order
        list = [1.0 / i for i in range(1, (order + 1))]
        self._base_objs = [HarmTable_base(list, size)]

    def setOrder(self, x):
        pyoArgsAssert(self, "I", x)
        self._order = x
        list = [1.0 / i for i in range(1, (self._order + 1))]
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    @property
    def order(self):
        return self._order

    @order.setter
    def order(self, x):
        self.setOrder(x)


class SquareTable(PyoTableObject):

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
        return self._order

    @order.setter
    def order(self, x):
        self.setOrder(x)


class TriangleTable(PyoTableObject):

    def __init__(self, order=10, size=8192):
        pyoArgsAssert(self, "II", order, size)
        PyoTableObject.__init__(self, size)
        self._order = order
        list = []
        for i in range(1, (order * 2)):
            if i % 2 == 1:
                list.append(1.0 / pow(i, 2))
            else:
                list.append(0.0)
        self._base_objs = [HarmTable_base(list, size)]

    def setOrder(self, x):
        pyoArgsAssert(self, "I", x)
        self._order = x
        list = []
        for i in range(1, (self._order * 2)):
            if i % 2 == 1:
                list.append(1.0 / pow(i, 2))
            else:
                list.append(0.0)
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    @property
    def order(self):
        return self._order

    @order.setter
    def order(self, x):
        self.setOrder(x)


class ChebyTable(PyoTableObject):

    def __init__(self, list=[1.0, 0.0], size=8192):
        pyoArgsAssert(self, "lI", list, size)
        PyoTableObject.__init__(self, size)
        self._auto_normalize = False
        self._list = copy.deepcopy(list)
        self._base_objs = [ChebyTable_base(self._list, size)]

    def autoNormalize(self, x):
        self._auto_normalize = x
        if self._auto_normalize:
            self.normalize()

    def replace(self, list):
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        if self._auto_normalize:
            self.normalize()
        self.refreshView()

    def getNormTable(self):
        if sum(self._list[1::2]) == 0:
            data = self._base_objs[0].getNormTable(0)
        else:
            data = self._base_objs[0].getNormTable(1)
        return DataTable(size=len(data), init=data).normalize()

    @property
    def list(self):
        return self._list

    @list.setter
    def list(self, x):
        self.replace(x)


class HannTable(PyoTableObject):

    def __init__(self, size=8192):
        pyoArgsAssert(self, "I", size)
        PyoTableObject.__init__(self, size)
        self._base_objs = [HannTable_base(size)]


class SincTable(PyoTableObject):

    def __init__(self, freq=pi * 2, windowed=False, size=8192):
        pyoArgsAssert(self, "NBI", freq, windowed, size)
        PyoTableObject.__init__(self, size)
        self._freq = freq
        self._windowed = windowed
        self._base_objs = [SincTable_base(freq, windowed, size)]

    def setFreq(self, x):
        pyoArgsAssert(self, "N", x)
        self._freq = x
        [obj.setFreq(x) for obj in self._base_objs]
        self.refreshView()

    def setWindowed(self, x):
        pyoArgsAssert(self, "B", x)
        self._windowed = x
        [obj.setWindowed(x) for obj in self._base_objs]
        self.refreshView()

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def windowed(self):
        return self._windowed

    @windowed.setter
    def windowed(self, x):
        self.setWindowed(x)


class WinTable(PyoTableObject):

    def __init__(self, type=2, size=8192):
        pyoArgsAssert(self, "II", type, size)
        PyoTableObject.__init__(self, size)
        self._type = type
        self._base_objs = [WinTable_base(type, size)]

    def setType(self, type):
        pyoArgsAssert(self, "I", type)
        self._type = type
        [obj.setType(type) for obj in self._base_objs]
        self.refreshView()

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class ParaTable(PyoTableObject):

    def __init__(self, size=8192):
        pyoArgsAssert(self, "I", size)
        PyoTableObject.__init__(self, size)
        self._base_objs = [ParaTable_base(size)]


class LinTable(PyoTableObject):

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
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
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
        return self._base_objs[0].getPoints()

    @property
    def list(self):
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class LogTable(PyoTableObject):

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
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
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
        return self._base_objs[0].getPoints()

    @property
    def list(self):
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class CosLogTable(PyoTableObject):

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
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
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
        return self._base_objs[0].getPoints()

    @property
    def list(self):
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class CosTable(PyoTableObject):

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
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
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
        return self._base_objs[0].getPoints()

    @property
    def list(self):
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class CurveTable(PyoTableObject):

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
        pyoArgsAssert(self, "N", x)
        self._tension = x
        [obj.setTension(x) for obj in self._base_objs]
        self.refreshView()

    def setBias(self, x):
        pyoArgsAssert(self, "N", x)
        self._bias = x
        [obj.setBias(x) for obj in self._base_objs]
        self.refreshView()

    def replace(self, list):
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
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
        return self._base_objs[0].getPoints()

    @property
    def tension(self):
        return self._tension

    @tension.setter
    def tension(self, x):
        self.setTension(x)

    @property
    def bias(self):
        return self._bias

    @bias.setter
    def bias(self, x):
        self.setBias(x)

    @property
    def list(self):
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class ExpTable(PyoTableObject):

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
        pyoArgsAssert(self, "N", x)
        self._exp = x
        [obj.setExp(x) for obj in self._base_objs]
        self.refreshView()

    def setInverse(self, x):
        pyoArgsAssert(self, "B", x)
        self._inverse = x
        [obj.setInverse(x) for obj in self._base_objs]
        self.refreshView()

    def replace(self, list):
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(list) for obj in self._base_objs]
        self.refreshView()

    def loadRecFile(self, filename, tolerance=0.02):
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
        return self._base_objs[0].getPoints()

    @property
    def exp(self):
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)

    @property
    def inverse(self):
        return self._inverse

    @inverse.setter
    def inverse(self, x):
        self.setInverse(x)

    @property
    def list(self):
        return self.getPoints()

    @list.setter
    def list(self, x):
        self.replace(x)


class NewTable(PyoTableObject):

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
        pyoArgsAssert(self, "l", x)
        if type(x[0]) != list:
            x = [x]
        [obj.setTable(self._check_data_size(wrap(x, i))) for i, obj in enumerate(self._base_objs)]
        self.refreshView()

    def setFeedback(self, x):
        pyoArgsAssert(self, "N", x)
        self._feedback = x
        [obj.setFeedback(x) for i, obj in enumerate(self._base_objs)]

    def getLength(self):
        return self._base_objs[0].getLength()

    def getDur(self, all=True):
        return self._base_objs[0].getLength()

    def getRate(self):
        return self._base_objs[0].getRate()

    @property
    def length(self):
        return self._length

    @length.setter
    def length(self, x):
        print("'length' attribute is read-only.")

    @property
    def chnls(self):
        return self._chnls

    @chnls.setter
    def chnls(self, x):
        print("'chnls' attribute is read-only.")

    @property
    def init(self):
        return self._init

    @init.setter
    def init(self, x):
        print("'init' attribute is read-only.")

    @property
    def feedback(self):
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
        pyoArgsAssert(self, "l", x)
        if type(x[0]) != list:
            x = [x]
        [obj.setTable(self._check_data_size(wrap(x, i))) for i, obj in enumerate(self._base_objs)]
        self.refreshView()

    def getRate(self):
        return self._base_objs[0].getRate()

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, x):
        print("DataTable 'size' attribute is read-only.")

    @property
    def chnls(self):
        return self._chnls

    @chnls.setter
    def chnls(self, x):
        print("'chnls' attribute is read-only.")

    @property
    def init(self):
        return self._init

    @init.setter
    def init(self, x):
        print("'init' attribute is read-only.")


class AtanTable(PyoTableObject):

    def __init__(self, slope=0.5, size=8192):
        pyoArgsAssert(self, "NI", slope, size)
        PyoTableObject.__init__(self, size)
        self._slope = slope
        self._base_objs = [AtanTable_base(slope, size)]

    def setSlope(self, x):
        pyoArgsAssert(self, "N", x)
        self._slope = x
        [obj.setSlope(x) for obj in self._base_objs]
        self.refreshView()

    @property
    def slope(self):
        return self._slope

    @slope.setter
    def slope(self, x):
        self.setSlope(x)


class PartialTable(PyoTableObject):

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
        pyoArgsAssert(self, "l", list)
        self._list = list
        [obj.replace(self._create_list()) for obj in self._base_objs]
        self.normalize()
        self.refreshView()

    @property
    def list(self):
        return self._list

    @list.setter
    def list(self, x):
        self.replace(x)


class PadSynthTable(PyoTableObject):

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
        pyoArgsAssert(self, "NB", x, generate)
        self._basefreq = x
        [obj.setBaseFreq(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setSpread(self, x, generate=True):
        pyoArgsAssert(self, "NB", x, generate)
        self._spread = x
        [obj.setSpread(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setBw(self, x, generate=True):
        pyoArgsAssert(self, "NB", x, generate)
        self._bw = x
        [obj.setBw(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setBwScl(self, x, generate=True):
        pyoArgsAssert(self, "NB", x, generate)
        self._bwscl = x
        [obj.setBwScl(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setNharms(self, x, generate=True):
        pyoArgsAssert(self, "IB", x, generate)
        self._nharms = x
        [obj.setNharms(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setDamp(self, x, generate=True):
        pyoArgsAssert(self, "NB", x, generate)
        self._damp = x
        [obj.setDamp(x, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    def setSize(self, size, generate=True):
        pyoArgsAssert(self, "IB", size, generate)
        self._size = size
        [obj.setSize(size, generate) for obj in self._base_objs]
        if generate:
            self.refreshView()

    @property
    def basefreq(self):
        return self._basefreq

    @basefreq.setter
    def basefreq(self, x):
        self.setBaseFreq(x)

    @property
    def spread(self):
        return self._spread

    @spread.setter
    def spread(self, x):
        self.setSpread(x)

    @property
    def bw(self):
        return self._bw

    @bw.setter
    def bw(self, x):
        self.setBw(x)

    @property
    def bwscl(self):
        return self._bwscl

    @bwscl.setter
    def bwscl(self, x):
        self.setBwScl(x)

    @property
    def nharms(self):
        return self._nharms

    @nharms.setter
    def nharms(self, x):
        self.setNharms(x)

    @property
    def damp(self):
        return self._damp

    @damp.setter
    def damp(self, x):
        self.setDamp(x)


class SharedTable(PyoTableObject):

    def __init__(self, name, create, size):
        if sys.platform == "win32":
            raise Exception("SharedTable is not implemented yet for Windows.")
        pyoArgsAssert(self, "sBI", name, create, size)
        PyoTableObject.__init__(self, size)
        self._name = name
        self._create = create
        name, lmax = convertArgsToLists(name)
        self._base_objs = [SharedTable_base(wrap(name, i), create, size) for i in range(lmax)]

    def getRate(self):
        return self._base_objs[0].getRate()

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, x):
        print("SharedTable 'size' attribute is read-only.")

# SndTable in "stripped" branch only accept WAV file, 8-bit int or 16-bit int.
class SndTable(PyoTableObject):

    def __init__(self, path=None, chnl=None, start=0, stop=None, initchnls=1):
        PyoTableObject.__init__(self)

        header = sndhdr.what(path)
        if header is None:
            path = None

        reader = None
        if path is not None:
            if header[0] == "wav":
                reader = wave.open(path, "rb")
            else:
                path = None

        data = [[]]
        if reader is not None:
            sr = reader.getframerate()
            nchnls = reader.getnchannels()
            nframes = reader.getnframes()
            maxamp = 2 ** (reader.getsampwidth() * 8 - 1)

            fstart = int(start * sr)
            if fstart > nframes:
                fstart = 0
            fstop = nframes
            if stop is not None:
                fstop = int(stop * sr)
                if fstop > nframes or fstop < fstart:
                    fstop = nframes

            nframes = fstop - fstart
            reader.setpos(fstart)
            wavedata = reader.readframes(nframes)
            data = struct.unpack("<{}h".format(nframes*nchnls), wavedata)
            data = [x / maxamp for x in data]
            if nchnls > 1:
                data = [[data[i + chnl] for i in range(0, len(data), nchnls)] for chnl in range(nchnls)]
            else:
                data = [data]

            reader.close()

        self._path = path
        self._chnl = chnl
        self._start = start
        self._stop = stop
        self._size = []
        self._dur = []
        self._base_objs = []
        path, lmax = convertArgsToLists(path)
        if self._path is None:
            self._base_objs = [DataTable_base(self.getSamplingRate()) for i in range(initchnls)]
        else:
            if chnl is None:
                self._base_objs = [DataTable(len(l), init=l) for l in data]
            elif chnl < len(data):
                self._base_objs = [DataTable(len(data[chnl]), init=data[chnl])]
            else:
                self._base_objs = [DataTable(len(data[0]), init=data[0])]

            self._size = self._base_objs[-1].getSize()
            self._dur = self._size / self.getSamplingRate()

    def setSound(self, path, start=0, stop=None):
        print("SndTable.setSound is a no-op in stripped branch.")

    def append(self, path, crossfade=0, start=0, stop=None):
        print("SndTable.append is a no-op in stripped branch.")

    def insert(self, path, pos=0, crossfade=0, start=0, stop=None):
        print("SndTable.insert is a no-op in stripped branch.")

    def getRate(self, all=True):
        if type(self._path) == list:
            _rate = [obj.getRate() for obj in self._base_objs]
        else:
            _rate = self._base_objs[0].getRate()

        if all:
            return _rate
        else:
            if type(_rate) == list:
                return _rate[0]
            else:
                return _rate

    def getDur(self, all=True):
        if type(self._path) == list:
            _dur = [1.0 / obj.getRate() for obj in self._base_objs]
        else:
            _dur = 1.0 / self._base_objs[0].getRate()

        if all:
            return _dur
        else:
            if type(_dur) == list:
                return _dur[0]
            else:
                return _dur

    def setSize(self, x):
        print("SndTable has no setSize method!")

    def getSize(self, all=True):
        if len(self._base_objs) > 1:
            _size = [obj.getSize() for obj in self._base_objs]
        else:
            _size = self._base_objs[0].getSize()

        if all:
            return _size
        else:
            if type(_size) == list:
                return _size[0]
            else:
                return _size

    @property
    def sound(self):
        return self._path

    @sound.setter
    def sound(self, x):
        self.setSound(x)

    @property
    def path(self):
        return self._path

    @path.setter
    def path(self, x):
        self.setSound(x)

    @property
    def chnl(self):
        return self._chnl

    @chnl.setter
    def chnl(self, x):
        print("'chnl' attribute is read-only.")

    @property
    def start(self):
        return self._start

    @start.setter
    def start(self, x):
        print("'start' attribute is read-only.")

    @property
    def stop(self):
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
