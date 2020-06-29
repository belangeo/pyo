This is a list of features/fixes to implement for future releases
=================================================================

Roadmap 1.0.2
-------------

- Finish MML implementation

- Add Midifile object

- Fixed Windows crash related to MIDI devices

- Add an optional callback on the "stop" event of the PyoObject. 

- Clean up reducePoints function.

- Fix compatibility issues with WxPython 4.1.0.
    - Search ctrl in the documentation frame does not work.
    - Review Style editor layout.

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

Jack
----

- Jack support for Windows and MacOS.

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

Tables
------

- Objects that can write to tables should accept any PyoTableObject,
  not just a NewTable or a DataTable.

Matrices
--------

- Implement the buffer protocol as in the PyoTableObject.

E-Pyo
-----

- We need a way to let the user interact with the script with input() function.

Link
----

- Added Link integration to the Event Framework.
