Here's the procedure to compile the [pyo~] object:

First of all, update pyo's submodules to get the latest Pd's external objects builder (pd-lib-builder):

```
git submodule update --init --recursive
```

Then, from `embedded/puredata` directory, make the object with the following command:
```
make
```

You need to create a directory with your Pd externals, usually that's `~/Documents/Pd/externals`. Move the [pyo~] external there, in its own directory, with its resources (pyo~-meta.pd, pyo~-help.pd, and ounkmaster.aif). Don't forget to add the directory to puredata's search paths.

If when loading the object in Pd you get the following error:
```
pyo~.pd_linux:libpython3.13.so.1.0: cannot open shared object file: No such file or directory
```
run `sudo ldconfig` right after building the object.
