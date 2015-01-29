Conversions
=======================

.. module:: pyo

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
