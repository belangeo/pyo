Conversions
=======================

Functions that perform the conversion of values from one range to another range.

.. currentmodule:: pyo

Functions in this category
------------------------------

- :py:func:`midiToHz` :     Converts a midi note value to frequency in Hertz.
- :py:func:`midiToTranspo` :     Converts a midi note value to transposition factor (central key = 60).
- :py:func:`sampsToSec` :     Returns the duration in seconds equivalent to the number of samples given as an argument.
- :py:func:`secToSamps` :     Returns the number of samples equivalent to the duration in seconds given as an argument.
- :py:func:`beatToDur` :     Converts a beat value (multiplier of a quarter note) to a duration in seconds.
- :py:func:`linToCosCurve` :     Creates a cosinus interpolated curve from a list of points.
- :py:func:`rescale` :     Converts values from an input range to an output range.
- :py:func:`floatmap` :     Converts values from a 0-1 range to an output range.
- :py:func:`distanceToSegment` :     Find the distance from a point to a line or line segment.
- :py:func:`reducePoints` :     Douglas-Peucker curve reduction algorithm.


*midiToHz*
---------------------------------

.. autofunction:: midiToHz(x)

*midiToTranspo*
---------------------------------

.. autofunction:: midiToTranspo(x)

*sampsToSec*
---------------------------------

.. autofunction:: sampsToSec(x)

*secToSamps*
---------------------------------

.. autofunction:: secToSamps(x)

*beatToDur*
---------------------------------

.. autofunction:: beatToDur(beat, bpm=120)

*linToCosCurve*
---------------------------------

.. autofunction:: linToCosCurve(data, yrange=[0, 1], totaldur=1, points=1024, log=False)

*rescale*
---------------------------------

.. autofunction:: rescale(data, xmin=0.0, xmax=1.0, ymin=0.0, ymax=1.0, xlog=False, ylog=False)

*floatmap*
---------------------------------

.. autofunction:: floatmap(x, min=0.0, max=1.0, exp=1.0)

*distanceToSegment*
---------------------------------

.. autofunction:: distanceToSegment(p, p1, p2, xmin=0.0, xmax=1.0, ymin=0.0, ymax=1.0, xlog=False, ylog=False)

*reducePoints*
---------------------------------

.. autofunction:: reducePoints(pointlist, tolerance=0.02)

