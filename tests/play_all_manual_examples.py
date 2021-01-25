import os
import sys
import time
import inspect
from pyo import *

# To automate audio output validation, every random has to be properly
# seeded for the examples to be determinitic.

def play_example(cls, dur=5, toprint=True, double=False):
    """
    Execute the documentation example of the object given as an argument.

    :Args:

        cls: PyoObject class or string
            Class reference of the desired object example. If this argument
            is the string of the full path of an example (as returned by the
            getPyoExamples() function), it will be executed.
        dur: float, optional
            Duration of the example.
        toprint: boolean, optional
            If True, the example script will be printed to the console.
            Defaults to True.
        double: boolean, optional
            If True, force the example to run in double precision (64-bit)
            Defaults to False.

    """
    root_dir = os.path.join(os.path.split(__file__)[0], "manual_example_references")
    if not os.path.isdir(root_dir):
        os.mkdir(root_dir)

    executable = sys.executable
    if not executable or executable is None:
        executable = "python3"

    doc = cls.__doc__.splitlines()
    filename = cls.__name__ + ".wav"
    filepath = os.path.join(root_dir, filename)
    lines = []
    store = False
    for line in doc:
        if not store:
            if ">>> s = Server" in line:
                line = line.replace("Server()", 'Server(audio="offline")')
                line = line + "\ns.recordOptions(filename=r'{}', dur={})".format(filepath, dur)
                store = True
        if store:
            if line.strip() == "":
                store = False
            elif 's.start()' in line:
                pass
            else:
                lines.append(line)

    if lines == []:
        print("There is no manual example for %s object." % cls.__name__)
        return

    ex_lines = [l.lstrip("    ") for l in lines if ">>>" in l or "..." in l]
    if hasattr(builtins, "pyo_use_double") or double:
        ex = "import time\nfrom pyo64 import *\n"
    else:
        ex = "import time\nfrom pyo import *\n"
    for line in ex_lines:
        if ">>>" in line:
            line = line.lstrip(">>> ")
        if "..." in line:
            line = "    " + line.lstrip("... ")
        ex += line + "\n"
    ex += "s.start()\ns.shutdown()\n"
    f = tempfile.NamedTemporaryFile(delete=False)
    if toprint:
        f.write(tobytes('print(r"""\n%s\n""")\n' % ex))
    f.write(tobytes(ex))
    f.close()
    call([executable, f.name])


tree = OBJECTS_TREE["PyoObjectBase"]
_list = []
for k in tree["PyoObject"].keys():
    _list.extend(tree["PyoObject"][k])
_list.extend(tree["PyoMatrixObject"])
_list.extend(tree["PyoTableObject"])
_list.extend(tree["PyoPVObject"])

if sys.platform == "win32":
    _list.remove("SharedTable")

print(_list)

t = time.time()
for i, obj in enumerate(_list):
    play_example(eval(obj), toprint=False, double=True)
print("Elapsed time: {}".format(time.time() - t))
