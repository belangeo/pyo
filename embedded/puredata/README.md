Here's the procedure to compile the [pyo~] object:

First of all, update pyo's submodules to get the latest Pd's external objects builder (pd-lib-builder):

```
git submodule update --init --recursive
```

Then, from `embedded/puredata` directory, make the object with the following command:
```
make
```

On Windows, the build searches `PATH` in order for a native `python.exe` that
can import `pyo` and has Python's embedding headers and library. It skips
installations without those files and Windows Store aliases, and prints the
selected interpreter. The installation directory and library name are queried
from that Python, so no user directory or Python version is fixed in the build.
To choose a specific interpreter, set `PYTHON_EXECUTABLE`:

```sh
make -B PYTHON_EXECUTABLE="C:/path/to/Python/python.exe"
```

You can also select an installation with `PYTHON_DIR="C:/path/to/Python"`.
The build copies the matching Python DLL beside `pyo~.dll`. Keep both DLLs together when
moving the external to Pd's externals directory. The external initializes
Python with the configured `PYTHON_DIR` as its home, so that installation and
its pyo package must remain available. Pd also needs MinGW's
`libwinpthread-1.dll`, either alongside the external or on its DLL search path.

To use the MSYS2 Python reported by `pkg-config` instead, run `make -B PYTHON_DIR=`.
On Linux and macOS, the build continues to use `pkg-config` by default.
Initialization failures are reported in Pd's console, including the underlying
Python traceback.

You need to create a directory with your Pd externals, usually that's `~/Documents/Pd/externals`. Move the [pyo~] external there, in its own directory, with its resources (pyo~-meta.pd, pyo~-help.pd, and ounkmaster.aif). Don't forget to add the directory to puredata's search paths.

If when loading the object in Pd you get the following error:
```
pyo~.pd_linux:libpython3.13.so.1.0: cannot open shared object file: No such file or directory
```
run `sudo ldconfig` right after building the object.
