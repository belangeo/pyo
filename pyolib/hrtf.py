from __future__ import division
from __future__ import print_function
from __future__ import absolute_import
"""
Set of objects to manage 3D spatialization with HRTF algorithm.

"""

"""
Copyright 2009-2018 Olivier Belanger

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
import os
from ._core import *
from ._maps import *

class HRTFData:
    def __init__(self, path=None, length=128):
        if path is None and length == 128:
            path = os.path.join(SNDS_PATH, "hrtf_compact")
        else:
            print("HRTF custom dataset is not supported yet.")

        root = []
        subfolders = os.listdir(path)
        for i in range(14):
            root.append([])
            subfolder = os.path.join(path, "elev%d" % ((i-4)*10))
            files = sorted([f for f in os.listdir(subfolder) if f.endswith(".wav")])
            for file in files:
                root[i].append(os.path.join(subfolder, file))
        self._data = HRTFData_base(root, length)

class HRTF(PyoObject):
    """
    Head-Related Transfert Function 3D spatialization.

    HRTF describes how a given sound wave input is filtered by the 
    diffraction and reflection properties of the head, pinna, and 
    torso, before the sound reaches the transduction machinery of 
    the eardrum and inner ear. 

    This object takes a source signal and spatialises it in the 3D 
    space around a listener by convolving the source with stored 
    Head Related Impulse Response (HRIR) based filters. This works
    under ideal listening context, ie. when listening with headphones. 

    HRTF generates two outpout streams per input stream.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        azimuth: float or PyoObject
            Position of the sound on the horizontal plane, between -180
            and 180 degrees. Defaults to 0.
        elevation: float or PyoObject
            Position of the sound on the vertical plane, between -40
            and 90 degrees. Defaults to 0.
        hrtfdata: HRTFData, optional
            Custom impulse responses dataset. Not used yet. Leave it
            to None.

    .. note::

        Currently, HRTF uses the HRIRs from Gardner and Martin at MIT lab. 

        http://alumni.media.mit.edu/~kdm/hrtfdoc/hrtfdoc.html

        These impulses where recorded at a sampling rate of 44.1 kHz. They
        will probably work just as well at 48 kHz, but surely not at higher 
        sampling rate.

        There is plan to add the possibility for the user to load its own 
        dataset in the future.

    .. seealso::

        :py:class:`SPan`, :py:class:`Pan`

    >>> s = Server(nchnls=2).boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
    >>> azi = Phasor(0.2, mul=360)
    >>> ele = Sine(0.1).range(0, 90)
    >>> mv = HRTF(sf, azi, ele, mul=0.5).out()

    """
    def __init__(self, input, azimuth=0.0, elevation=0.0, hrtfdata=None, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, azimuth, elevation, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        if hrtfdata is None:
            self._hrtfdata = HRTFData()
        else:
            self._hrtfdata = hrtfdata
        self._azimuth = azimuth
        self._elevation = elevation
        self._in_fader = InputFader(input)
        in_fader, azimuth, elevation, mul, add, lmax = convertArgsToLists(self._in_fader, azimuth, elevation, mul, add)
        self._base_players = [HRTFSpatter_base(wrap(in_fader,i), self._hrtfdata._data, wrap(azimuth,i), wrap(elevation,i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax):
            for j in range(2):
                self._base_objs.append(HRTF_base(wrap(self._base_players,i), j, wrap(mul,i), wrap(add,i)))
        self.play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setAzimuth(self, x):
        """
        Replace the `azimuth` attribute.

        :Args:

            x: float or PyoObject
                new `azimuth` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._azimuth = x
        x, lmax = convertArgsToLists(x)
        [obj.setAzimuth(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setElevation(self, x):
        """
        Replace the `elevation` attribute.

        :Args:

            x: float or PyoObject
                new `elevation` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._elevation = x
        x, lmax = convertArgsToLists(x)
        [obj.setElevation(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(-180, 180, "lin", "azimuth", self._azimuth),
                          SLMap(-40, 90, 'lin', 'elevation', self._elevation),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def azimuth(self):
        """float or PyoObject. Position of the sound on the horizontal plane."""
        return self._azimuth
    @azimuth.setter
    def azimuth(self, x): self.setAzimuth(x)

    @property
    def elevation(self):
        """float or PyoObject. Position of the sound on the vertical plane."""
        return self._elevation
    @elevation.setter
    def elevation(self, x): self.setElevation(x)
