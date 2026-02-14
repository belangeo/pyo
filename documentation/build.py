import os, sys
from pyo import *

build_format = "html"
build_folder = "./build_html"
if "--latex" in sys.argv:
    build_format = "latex"
    build_folder = "./build_latex"
elif "--man" in sys.argv:
    build_format = "man"
    build_folder = "./build_man"

f = open(os.getcwd() + "/source/api/alphabetical.rst", "w")

# Header
f.write("Alphabetical class reference\n")
f.write("=======================================\n")
f.write("\n\n.. module:: pyo\n\n")


def getDocFirstLine(obj):
    try:
        text = eval(obj).__doc__
        if text == None:
            text = ""
    except:
        text = ""

    if text != "":
        spl = text.split("\n")
        if len(spl) == 1:
            f = spl[0]
        else:
            f = spl[1]
    else:
        f = text
    return f


format = "- :py:class:`%s` : %s\n"

lines = []
for key in [
    "Server",
    "Stream",
    "TableStream",
    "PyoObjectBase",
    "Map",
    "MidiListener",
    "MidiDispatcher",
    "OscListener",
    "PyoGui",
]:
    if type(OBJECTS_TREE[key]) == list:
        if key in ["MidiListener", "MidiDispatcher", "OscListener"]:
            lines.append(format % (key, getDocFirstLine(key)))
        else:
            for obj in OBJECTS_TREE[key]:
                lines.append(format % (obj, getDocFirstLine(obj)))
    else:
        if key == "Map":
            key2list = ["SLMap"]
        else:
            key2list = ["PyoMatrixObject", "PyoTableObject", "PyoObject", "PyoPVObject"]
        for key2 in key2list:
            if type(OBJECTS_TREE[key][key2]) == list:
                for obj in OBJECTS_TREE[key][key2]:
                    lines.append(format % (obj, getDocFirstLine(obj)))
            else:
                for key3 in sorted(OBJECTS_TREE[key][key2]):
                    for obj in OBJECTS_TREE[key][key2][key3]:
                        lines.append(format % (obj, getDocFirstLine(obj)))

lines.sort()
for line in lines:
    f.write(line)

f.close()

# New examples listing
src_example_dir = "../pyo/examples"
dest_example_dir = "source/examples"
if os.path.isdir(dest_example_dir):
    os.system("rm -r source/examples")
os.mkdir("source/examples")
with open("source/examples/index.rst", "w") as f:
    f.write(
        """
Examples
================

.. toctree::
   :maxdepth: 1

   First steps <x01-intro/index>
   Parameter control <x02-controls/index>
   Synthesis generators <x03-generators/index>
   Playing with soundfiles <x04-soundfiles/index>
   Amplitude envelopes <x05-envelopes/index>
   Filtering <x06-filters/index>
   Creating sound effects <x07-effects/index>
   Dynamic range of audio signals <x08-dynamics/index>
   Calling python functions from audio objects <x09-callbacks/index>
   Using tables <x10-tables/index>
   Processing in the spectral domain <x14-spectral/index>
   Using 2D tables (matrix) <x15-matrix/index>
   How to use MIDI with pyo <x16-midi/index>
   How to use OSC with pyo <x17-osc/index>
   Multirate audio processing <x19-multirate/index>
   Multicore audio programming <x20-multicore/index>
   Utilities <x21-utilities/index>
   Events framework <x22-events/index>
   Evaluating prefix expression <x23-expression/index>

"""
    )
folders = sorted([d for d in os.listdir(src_example_dir) if d[0:2] in ["x0", "x1", "x2"]])
for dir in folders:
    os.mkdir("source/examples/%s" % dir)
    index = open("source/examples/%s/index.rst" % dir, "w")
    index.write(dir + "\n")
    index.write("=" * 40)

    # Section description from the __init__.py file
    initfile = os.path.join(src_example_dir, dir, "__init__.py")
    if os.path.isfile(initfile):
        with open(initfile, "r") as f:
            text = f.read()
            pos1 = text.find('"""')
            pos2 = text.find('"""', pos1 + 3)
            desc = text[pos1 + 3 : pos2].strip()
            index.write("\n\n" + desc + "\n\n")

    index.write("\n\n.. toctree::\n   :maxdepth: 1\n\n")
    for name in sorted(os.listdir(os.path.join(src_example_dir, dir))):
        if name == "__init__.py":
            continue
        if name.endswith(".expr"):
            continue
        index.write("   " + name[:-3] + "\n")
        with open(os.path.join(src_example_dir, dir, name), "r") as f:
            text = f.read()
        with open("source/examples/%s/%s.rst" % (dir, name[:-3]), "w") as f:
            pos = text.find('"""')
            pos = text.find('"""', pos + 3)
            code = text[pos + 3 :].strip()
            intro = text[: pos + 3].replace('"""', "").strip()
            tpos = intro.find("\n")
            title = intro[:tpos]
            f.write(title + "\n")
            f.write("=" * 140)
            f.write("\n")
            f.write(intro[tpos:])
            f.write("\n\n")
            f.write(".. code-block:: python\n\n")
            for line in code.splitlines():
                f.write("    " + line + "\n")
            f.write("\n")
    index.close()


os.system("sphinx-build -a -E -j 4 -b %s ./source %s" % (build_format, build_folder))

if build_format == "latex":
    os.system(
        "cd build_latex; pdflatex -interaction nonstopmode Pyo;  pdflatex -interaction nonstopmode Pyo"
    )

if build_format == "html":
    os.system("cp -r build_html/* ../docs/")
    os.system("rm -r build_html")

os.system("rm -r source/examples")
