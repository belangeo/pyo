Here's the procedure to compile the [pyo~] object:

First of all, you'll need Pd's current external objects builder, which you can get here https://github.com/pure-data/pd-lib-builder.
You need to create a directory with your Pd externals, usually that's /home/Documents/Pd/externals. Move the pd-lib-builder directory in there, and the [pyo~] external sources there too (in its own directory).
Copy the m_pyo.h file into the pyo~ external directory.

Then make the object with the following:
```
make PY_LIBS="-lpython3.13"
```
The last two bits (m_pyo.h and the make commad) should use the Python version you have installed or built Pyo against.

If when loading the object in Pd you get the following error:
```
pyo~.pd_linux:libpython3.13.so.1.0: cannot open shared object file: No such file or directory
```
run `sudo ldconfig` right after building the object.
