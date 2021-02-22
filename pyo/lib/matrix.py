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

######################################################################
### Matrix
######################################################################
class NewMatrix(PyoMatrixObject):

    def __init__(self, width, height, init=None):
        pyoArgsAssert(self, "IIL", width, height, init)
        PyoMatrixObject.__init__(self)
        self._size = (width, height)
        if init is None:
            self._base_objs = [NewMatrix_base(width, height)]
        else:
            self._base_objs = [NewMatrix_base(width, height, init)]

    def replace(self, x):
        pyoArgsAssert(self, "l", x)
        [obj.setMatrix(x) for obj in self._base_objs]
        self.refreshView()

    def getRate(self):
        return self._base_objs[0].getRate()

    def genSineTerrain(self, freq=1, phase=0.0625):
        pyoArgsAssert(self, "NN", freq, phase)
        [obj.genSineTerrain(freq, phase) for obj in self._base_objs]
        self.refreshView()
