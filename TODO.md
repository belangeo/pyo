To do ASAP!
===========

- [DOC] Compilation for anaconda (mail from Stanley Rosenbaum).

- [DOC] Live recording without xruns, use Server.recstart() to record to a
  file on a tmpfs, which is basically a filesystem in RAM.
  mail: "Server recording to wav generates many xruns (Aug 26)".

- Review every "PyErr_SetString(PyExc_TypeError)" and "PySys_WriteStdout()" calls.

- stdlib for Expr object.

- pyo with jack support for Windows and MacOS.

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


MIDI
----

- Jack on windows.

- sysex support in MidiListener.

GUI
---

- Implement all GUI components with Tkinter and make it the default GUI
  toolkit (instead of wxpython). WxPython classes could be removed from
  pyo sources and built as an optional extra package (pyo-wxgui). The idea
  is to remove an extra dependency, as tk is generally builtin with python.

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
