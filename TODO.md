To do ASAP!
===========

- make pip packages.

This is a list of features/fixes to implement for future releases
=================================================================

Internal
--------

- Replace malloc/realloc with a custom memory pool allocator.

- Use inheritance (instead of a bunch of macros) at the C level. 
  PyoObject_base or Stream) should handle `mul`, `add` and 
  stream-related functions for every PyoObject. It should make the 
  code much simple and easier to maintain/extend, and also reduce 
  compiling time and binary size.

Server
------

- A method to retrieve a graph of the internal state of the server 
  (active objects, connections, attribute values, ...).

Examples
--------

- A section about building fx and synth classes.

Objects
-------

- Add VBAP algorithm.

- Add Binaural object (VBAP+HRTF).

Jack
----

- Jack support for Windows and MacOS.

MIDI
----

- sysex support in MidiListener.

GUI
---

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

- Complete review of E-Pyo on Windows. Lot of features don't seem to work.
    - Does not seem to save file with utf-8 encoding.
    - Project tree refresh function doesn't work.

- We need A way to let the user interact with the script via input() 
  and raw_input() functions.

