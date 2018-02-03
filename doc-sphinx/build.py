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

f = open(os.getcwd() + '/source/api/alphabetical.rst', 'w')

# Header
f.write("Alphabetical class reference\n")
f.write("=======================================\n")
f.write("\n\n.. module:: pyo\n\n")

def getDocFirstLine(obj):
    try:
        text = eval(obj).__doc__
        if text == None:
            text = ''
    except:
        text = ''
        
    if text != '':
        spl = text.split('\n')
        if len(spl) == 1:
            f = spl[0]
        else:    
            f = spl[1]
    else:
        f = text
    return f    

format = "- :py:class:`%s` : %s\n"

lines = []
for key in ['Server', 'Stream', 'TableStream', 'PyoObjectBase', 'Map', 
            'MidiListener', 'OscListener', 'PyoGui']:
    if type(OBJECTS_TREE[key]) == list:
        if key in ['MidiListener', 'OscListener']:
            lines.append(format % (key, getDocFirstLine(key)))
        else:
            for obj in OBJECTS_TREE[key]:
                lines.append(format % (obj, getDocFirstLine(obj)))
    else:
        if key == 'Map': key2list = ['SLMap']
        else: key2list = ['PyoMatrixObject', 'PyoTableObject', 'PyoObject', 'PyoPVObject']
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
os.mkdir("source/examples")
folders = sorted([d for d in os.listdir("../examples") if d[0] in ['0', '1']])
for dir in folders:
    os.mkdir("source/examples/%s" % dir)
    index = open("source/examples/%s/index.rst" % dir, "w")
    index.write(dir + "\n")
    index.write("="*40)
    index.write("\n\n.. toctree::\n   :maxdepth: 1\n\n")
    for name in sorted(os.listdir("../examples/%s" % dir)):
        index.write("   " + name[:-3] + "\n")
        with open("../examples/%s/%s" % (dir, name), "r") as f:
            text = f.read()
        with open("source/examples/%s/%s.rst" % (dir, name[:-3]), "w") as f:
            pos = text.find('"""')
            pos = text.find('"""', pos+3)
            code = text[pos+3:].strip()
            intro = text[:pos+3].replace('"""', '').strip()
            tpos = intro.find("\n")
            title = intro[:tpos]
            f.write(title + "\n")
            f.write("="*140)
            f.write("\n")
            f.write(intro[tpos:])
            f.write("\n\n")
            f.write(".. code-block:: python\n\n")
            for line in code.splitlines():
                f.write("    " + line + "\n")
            f.write("\n")
    index.close()

os.system("cp ../utils/E-PyoIcon.png source/")

#os.system("sphinx-build -a -E -j 4 -b %s ./source %s" % (build_format, build_folder))
os.system("sphinx-build -a -E -b %s ./source %s" % (build_format, build_folder))

if build_format == "latex":
    os.system("cd build_latex; pdflatex -interaction nonstopmode Pyo;  pdflatex -interaction nonstopmode Pyo")

rep = input("Do you want to upload to ajax server (y/n) ? ")
if rep == "y":
        os.system("scp -r build_html/* jeadum1@ajaxsoundstudio.com:/home/jeadum1/ajaxsoundstudio.com/pyodoc")

os.system("rm -r source/examples")

os.system("rm  source/E-PyoIcon.png")
