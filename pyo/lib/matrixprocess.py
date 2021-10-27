"""
PyoObjects to perform operations on PyoMatrixObjects.

PyoMatrixObjects are 2 dimensions table containers. They can be used
to store audio samples or algorithmic sequences. Writing and reading
are done by giving row and column positions.

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
from ._core import *
from ._maps import *


class MatrixRec(PyoObject):
    """
    MatrixRec records samples into a previously created NewMatrix.

    See :py:class:`NewMatrix` to create an empty matrix.

    The play method is not called at the object creation time. It starts
    the recording into the matrix, row after row, until the matrix is full.
    Calling the play method again restarts the recording and overwrites
    previously recorded samples. The stop method stops the recording.
    Otherwise, the default behaviour is to record through the end of the matrix.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Audio signal to write in the matrix.
        matrix: PyoMatrixObject
            The matrix where to write samples.
        fadetime: float, optional
            Fade time at the beginning and the end of the recording
            in seconds. Defaults to 0.
        delay: int, optional
            Delay time, in samples, before the recording begins.
            Available at initialization time only. Defaults to 0.

    .. note::

        The out() method is bypassed. MatrixRec returns no signal.

        MatrixRec has no `mul` and `add` attributes.

        MatrixRec will send a trigger signal at the end of the recording.
        User can retreive the trigger streams by calling obj['trig']. See
        `TableRec` documentation for an example.

    .. seealso::

        :py:class:`NewMatrix`

    >>> s = Server().boot()
    >>> s.start()
    >>> SIZE = 256
    >>> mm = NewMatrix(SIZE, SIZE)
    >>> fmind = Sine(.2, 0, 2, 2.5)
    >>> fmrat = Sine(.33, 0, .05, .5)
    >>> aa = FM(carrier=10, ratio=fmrat, index=fmind)
    >>> rec = MatrixRec(aa, mm, 0).play()
    >>> lfx = Sine(.1, 0, .24, .25)
    >>> lfy = Sine(.15, 0, .124, .25)
    >>> x = Sine([500,501], 0, lfx, .5)
    >>> y = Sine([10.5,10], 0, lfy, .5)
    >>> c = MatrixPointer(mm, x, y, .2).out()

    """

    def __init__(self, input, matrix, fadetime=0, delay=0):
        pyoArgsAssert(self, "omni", input, matrix, fadetime, delay)
        PyoObject.__init__(self)
        self._input = input
        self._matrix = matrix
        self._in_fader = InputFader(input).stop()
        in_fader, matrix, fadetime, delay, lmax = convertArgsToLists(self._in_fader, matrix, fadetime, delay)
        self._base_objs = [
            MatrixRec_base(wrap(in_fader, i), wrap(matrix, i), wrap(fadetime, i), wrap(delay, i))
            for i in range(len(matrix))
        ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMatrix(self, x):
        """
        Replace the `matrix` attribute.

        :Args:

            x: NewMatrix
                new `matrix` attribute.

        """
        pyoArgsAssert(self, "m", x)
        self._matrix = x
        x, lmax = convertArgsToLists(x)
        [obj.setMatrix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Audio signal to record in the matrix."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def matrix(self):
        """PyoMatrixObject. The matrix where to write samples."""
        return self._matrix

    @matrix.setter
    def matrix(self, x):
        self.setMatrix(x)


class MatrixRecLoop(PyoObject):
    """
    MatrixRecLoop records samples in loop into a previously created NewMatrix.

    See :py:class:`NewMatrix` to create an empty matrix.

    MatrixRecLoop records samples into the matrix, row after row, until
    the matrix is full and then loop back to the beginning.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Audio signal to write in the matrix.
        matrix: PyoMatrixObject
            The matrix where to write samples.

    .. note::

        The out() method is bypassed. MatrixRecLoop returns no signal.

        MatrixRecLoop has no `mul` and `add` attributes.

        MatrixRecLoop will send a trigger signal when reaching the end
        of the matrix. User can retreive the trigger streams by calling
        obj['trig']. See `TableRec` documentation for an example.

    .. seealso::

        :py:class:`NewMatrix`

    >>> s = Server().boot()
    >>> s.start()
    >>> env = CosTable([(0,0), (300,1), (1000,.4), (8191,0)])
    >>> matrix = NewMatrix(8192, 8)
    >>> src = SfPlayer(SNDS_PATH+'/transparent.aif', loop=True, mul=.3)
    >>> m_rec = MatrixRecLoop(src, matrix)
    >>> period = 8192 / s.getSamplingRate()
    >>> metro = Metro(time=period/2, poly=2).play()
    >>> x = TrigLinseg(metro, [(0,0), (period,1)])
    >>> y = TrigRandInt(metro, max=2, mul=0.125)
    >>> amp = TrigEnv(metro, table=env, dur=period)
    >>> out = MatrixPointer(matrix, x, y, amp).out()

    """

    def __init__(self, input, matrix):
        pyoArgsAssert(self, "om", input, matrix)
        PyoObject.__init__(self)
        self._input = input
        self._matrix = matrix
        self._in_fader = InputFader(input)
        in_fader, matrix, lmax = convertArgsToLists(self._in_fader, matrix)
        self._base_objs = [MatrixRecLoop_base(wrap(in_fader, i), wrap(matrix, i)) for i in range(len(matrix))]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMatrix(self, x):
        """
        Replace the `matrix` attribute.

        :Args:

            x: NewMatrix
                new `matrix` attribute.

        """
        pyoArgsAssert(self, "m", x)
        self._matrix = x
        x, lmax = convertArgsToLists(x)
        [obj.setMatrix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Audio signal to record in the matrix."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def matrix(self):
        """PyoMatrixObject. The matrix where to write samples."""
        return self._matrix

    @matrix.setter
    def matrix(self, x):
        self.setMatrix(x)


class MatrixPointer(PyoObject):
    """
    Matrix reader with control on the 2D pointer position.

    :Parent: :py:class:`PyoObject`

    :Args:

        matrix: PyoMatrixObject
            Matrix containing the waveform samples.
        x: PyoObject
            Normalized X position in the matrix between 0 and 1.
        y: PyoObject
            Normalized Y position in the matrix between 0 and 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> SIZE = 256
    >>> mm = NewMatrix(SIZE, SIZE)
    >>> fmind = Sine(.2, 0, 2, 2.5)
    >>> fmrat = Sine(.33, 0, .05, .5)
    >>> aa = FM(carrier=10, ratio=fmrat, index=fmind)
    >>> rec = MatrixRec(aa, mm, 0).play()
    >>> lfx = Sine(.1, 0, .24, .25)
    >>> lfy = Sine(.15, 0, .124, .25)
    >>> x = Sine([500,501], 0, lfx, .5)
    >>> y = Sine([10.5,10], 0, lfy, .5)
    >>> c = MatrixPointer(mm, x, y, .2).out()

    """

    def __init__(self, matrix, x, y, mul=1, add=0):
        pyoArgsAssert(self, "mooOO", matrix, x, y, mul, add)
        PyoObject.__init__(self, mul, add)
        self._matrix = matrix
        self._x = x
        self._y = y
        matrix, x, y, mul, add, lmax = convertArgsToLists(matrix, x, y, mul, add)
        self._base_objs = [
            MatrixPointer_base(wrap(matrix, i), wrap(x, i), wrap(y, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setMatrix(self, x):
        """
        Replace the `matrix` attribute.

        :Args:

            x: PyoTableObject
                new `matrix` attribute.

        """
        pyoArgsAssert(self, "m", x)
        self._matrix = x
        x, lmax = convertArgsToLists(x)
        [obj.setMatrix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setX(self, x):
        """
        Replace the `x` attribute.

        :Args:

            x: PyoObject
                new `x` attribute.

        """
        pyoArgsAssert(self, "o", x)
        self._x = x
        x, lmax = convertArgsToLists(x)
        [obj.setX(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setY(self, x):
        """
        Replace the `y` attribute.

        :Args:

            y: PyoObject
                new `y` attribute.

        """
        pyoArgsAssert(self, "o", x)
        self._y = x
        x, lmax = convertArgsToLists(x)
        [obj.setY(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def matrix(self):
        """PyoMatrixObject. Matrix containing the samples."""
        return self._matrix

    @matrix.setter
    def matrix(self, x):
        self.setMatrix(x)

    @property
    def x(self):
        """PyoObject. Normalized X position in the matrix."""
        return self._x

    @x.setter
    def x(self, x):
        self.setX(x)

    @property
    def y(self):
        """PyoObject. Normalized Y position in the matrix."""
        return self._y

    @y.setter
    def y(self, x):
        self.setY(x)


class MatrixMorph(PyoObject):
    """
    Morphs between multiple PyoMatrixObjects.

    Uses an index into a list of PyoMatrixObjects to morph between adjacent
    matrices in the list. The resulting morphed function is written into the
    `matrix` object at the beginning of each buffer size. The matrices in the
    list and the resulting matrix must be equal in size.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Morphing index between 0 and 1. 0 is the first matrix in the list
            and 1 is the last.
        matrix: NewMatrix
            The matrix where to write morphed function.
        sources: list of PyoMatrixObject
            List of matrices to interpolate from.

    .. note::

        The out() method is bypassed. MatrixMorph returns no signal.

        MatrixMorph has no `mul` and `add` attributes.

    >>> s = Server().boot()
    >>> s.start()
    >>> m1 = NewMatrix(256, 256)
    >>> m1.genSineTerrain(1, 4)
    >>> m2 = NewMatrix(256, 256)
    >>> m2.genSineTerrain(2, 8)
    >>> mm = NewMatrix(256, 256)
    >>> inter = Sine(.2, 0, .5, .5)
    >>> morph = MatrixMorph(inter, mm, [m1,m2])
    >>> x = Sine([49,50], 0, .45, .5)
    >>> y = Sine([49.49,50.5], 0, .45, .5)
    >>> c = MatrixPointer(mm, x, y, .2).out()

    """

    def __init__(self, input, matrix, sources):
        pyoArgsAssert(self, "oml", input, matrix, sources)
        PyoObject.__init__(self)
        self._input = input
        self._matrix = matrix
        self._sources = sources
        self._in_fader = InputFader(input)
        in_fader, matrix, lmax = convertArgsToLists(self._in_fader, matrix)
        self._base_sources = [source[0] for source in sources]
        self._base_objs = [
            MatrixMorph_base(wrap(in_fader, i), wrap(matrix, i), self._base_sources) for i in range(len(matrix))
        ]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMatrix(self, x):
        """
        Replace the `matrix` attribute.

        :Args:

            x: NewMatrix
                new `matrix` attribute.

        """
        pyoArgsAssert(self, "m", x)
        self._matrix = x
        x, lmax = convertArgsToLists(x)
        [obj.setMatrix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSources(self, x):
        """
         Replace the `sources` attribute.

        :Args:

            x: list of PyoMatrixObject
                new `sources` attribute.

        """
        pyoArgsAssert(self, "l", x)
        self._sources = x
        self._base_sources = [source[0] for source in x]
        [obj.setSources(self._base_sources) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Morphing index between 0 and 1."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def matrix(self):
        """NewMatrix. The matrix where to write samples."""
        return self._matrix

    @matrix.setter
    def matrix(self, x):
        self.setMatrix(x)

    @property
    def sources(self):
        """list of PyoMatrixObject. List of matrices to interpolate from."""
        return self._sources

    @sources.setter
    def sources(self, x):
        self.setSources(x)
