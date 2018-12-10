import os

path = '/home/olivier/git/pyo/src/engine/'

files = [f for f in os.listdir(path) if f.endswith(".c")]
files_to_exclude = ["servermodule.c"]

for file in files:
    fullpath = os.path.join(path, file)
    with open(fullpath, "r") as f:
        for i, line in enumerate(f):
            return_py_obj = "static PyObject" in line
            stop_func = "_stop(" in line
            #stop_macro = "STOP" in line
            if return_py_obj and stop_func:
                print("%s : line %d : %s" % (file, i, line), end="")

        f.seek(0)
        for i, line in enumerate(f):
            pycfunc = "(PyCFunction)" in line
            stop_func = '"stop"' in line
            if pycfunc and stop_func:
                print("%s : line %d : %s" % (file, i, line), end="")
    