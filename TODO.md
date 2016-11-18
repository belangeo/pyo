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

- Remove, if possible, PyGILState_Ensure/PyGILState_Release from 
  the process_buffers function.


Examples
--------

- A section about building fx and synth classes.


GUI
---

- MixerGUI, an interface to control the routing of a Mixer object.


Tables
------

- Objects that can write to tables should accept any PyoTableObject,
  not just a NewTable or a DataTable.


E-Pyo
-----

- Window splitter to show more than one file at the time (multiple 
  views) ?

- We need A way to let the user interact with the script via input() 
  and raw_input() functions.
