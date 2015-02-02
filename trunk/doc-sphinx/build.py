import os, sys
from types import ListType
from pyo import *

build_format = "html"
build_folder = "./build_html"
if "--latex" in sys.argv:
    build_format = "latex"
    build_folder = "./build_latex"

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
for key in ['Server', 'Stream', 'TableStream', 'PyoObjectBase', 'Map']:
    if type(OBJECTS_TREE[key]) == ListType:
        for obj in OBJECTS_TREE[key]:
            lines.append(format % (obj, getDocFirstLine(obj)))
    else:
        if key == 'Map': key2list = ['SLMap']
        else: key2list = ['PyoMatrixObject', 'PyoTableObject', 'PyoObject', 'PyoPVObject']
        for key2 in key2list:
            if type(OBJECTS_TREE[key][key2]) == ListType:
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

os.system("sphinx-build -a -b %s ./source %s" % (build_format, build_folder))

if build_format == "latex":
    os.system("cd build_latex; pdflatex -interaction nonstopmode Pyo;  pdflatex -interaction nonstopmode Pyo")

rep = raw_input("Do you want to upload to ajax server (y/n) ? ")
if rep == "y":
        os.system("scp -r build_html/* jeadum1@ajaxsoundstudio.com:/home/jeadum1/ajaxsoundstudio.com/pyodoc")

