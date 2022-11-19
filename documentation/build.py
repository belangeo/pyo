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

   First steps <01-intro/index>
   Parameter control <02-controls/index>
   Synthesis generators <03-generators/index>
   Playing with soundfiles <04-soundfiles/index>
   Amplitude envelopes <05-envelopes/index>
   Filtering <06-filters/index>
   Creating sound effects <07-effects/index>
   Dynamic range of audio signals <08-dynamics/index>
   Calling python functions from audio objects <09-callbacks/index>
   Using tables <10-tables/index>
   How to use MIDI with pyo <16-midi/index>
   How to use OSC with pyo <17-osc/index>
   Multirate audio processing <19-multirate/index>
   Multicore audio programming <20-multicore/index>
   Utilities <21-utilities/index>
   Events framework <22-events/index>
   Evaluating prefix expression <23-expression/index>

"""
    )
folders = sorted([d for d in os.listdir(src_example_dir) if d[0] in ["0", "1", "2"]])
for dir in folders:
    os.mkdir("source/examples/%s" % dir)
    index = open("source/examples/%s/index.rst" % dir, "w")
    index.write(dir + "\n")
    index.write("=" * 40)
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
    os.system("cd build_latex; pdflatex -interaction nonstopmode Pyo;  pdflatex -interaction nonstopmode Pyo")

if build_format == "html":
    os.system("cp -r build_html/* ../docs/")
    os.system("rm -r build_html")

os.system("rm -r source/examples")
