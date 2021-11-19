This is a list of features/fixes to implement for future releases
=================================================================

Roadmap 1.0.4
-------------

- Add Midifile object

- Add an optional callback on the "stop" event of the PyoObject. 

- Install m_pyo.h configured for the target version in pythonlib/pyo/include/.

- Update pyo-linux-wheels to use manylinux2010 image instead of manylinux1.

- unit testing with pytest

- NewTable and DataTable mutable size attribute.
  Overall PyoTableObject.setSize method does not behave consistently for every table.
  Most of them re-generate the table after a resize, but the doc says that the table is zero'd.

- PyoTableObject.getSize(all=False), but SndTable.getSize(all=True)

Roadmap 1.0.5
-------------

- Remove handling of python2 vs python3 and keep only python3 code.

- PyoMatrixObject.get(x, y) inverts arguments. Add PyoMatrixObject.getValue(x, y) with x and y in the good order (also putValue(value, x, y))
- PyoMatrixObject.getMatrix()
- A new implementation, Pyo2DTableObject, doint the right things.

Internal
--------

- Use inheritance (instead of a bunch of macros) at the C level. 
  PyoObject_base or Stream) should handle `mul`, `add` and 
  stream-related functions for every PyoObject. It should make the 
  code much simple and easier to maintain/extend, and also reduce 
  compiling time and binary size.

Server
------

- Updating coreaudio support (lot of deprecated functions).

- A method to retrieve a graph of the internal state of the server 
  (active objects, connections, attribute values, ...).

Examples
--------

- finish the examples.

- A section about building fx and synth classes.

Objects
-------

- PVInterp

- PVGain with controlable gain value over time.

- Integrator and Differentiator

- TrigAdsr, to generate envelope from trigger with constant fadein time.

- Add VBAP and LBAP spat algorithms.

- Add a plate reverb.

MIDI
----

- MidiFile & MidiFileReader

- sysex support in MidiListener.

GUI
---

- PyoGuiRangeSlider (instead of importing pyo.lib._widgets.HRangeSlider

- MixerGUI, an interface to control the routing of a Mixer object.

- A graphical representation of a parametric EQ.

- Analysis display (cartesian plane, sonnagram, filter frequency response)


Matrices
--------

- Implement the buffer protocol as in the PyoTableObject.

Link
----

- Added Link integration to the Event Framework.
