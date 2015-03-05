"""
Copyright 2009-2015 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
import os, inspect, shutil
from types import ListType
from pyo import *

#######################################################
######## DEPRECATED DOCUMENTATION GENERATOR ###########
#######################################################

# 'manuel-dev' for development, 'manual' for production  
man_file = 'manual-dev'
if man_file == 'manual-dev':
    man_version = 'latest sources'
else:
    man_version = 'version %s' % PYO_VERSION
print PYO_VERSION

try:
    os.mkdir(os.getcwd() + '/doc')
except OSError:
    pass

f = open(os.getcwd() + '/doc/%s.tex' % man_file, 'w')

# Header
f.write("\documentclass[12pt,oneside]{article}\n")
f.write("\usepackage[utf8]{inputenc}\n")
f.write("\usepackage{dcolumn,amsmath}\n\n")
f.write("\usepackage{html,makeidx}\n\n")
f.write("\\begin{document}\n\n")

def getDoc(obj):
    try:
        init = '\n\n' + class_args(eval(obj)).replace("_", "\_") + '\n\n'
        init = init.replace("self, ", "")
    except:
        init = '\n\n' 
         
    try:
        text = eval(obj).__doc__
        if text == None:
            text = ''
    except:
        text = ''
    try:        
        doc_str = getFormattedDoc(text, obj)
    except:
        doc_str = text
    try:        
        methods = getMethodsDoc(text, obj)
    except:
        methods = ''    
    page_text = init + doc_str + methods + "\n\n"
    if len(methods):
        page_text += '\end{verbatim}\n\n'    
    if doc_str == '':
        page_text += "\nnot documented yet...\n\n"
        
    return page_text    

def getMethodsDoc(text, obj):
    lines = text.splitlines(True)
    flag = False
    add_tab = False
    verbatim = False
    methods = ''
    for line in lines:
        if flag:
            if line.strip() == '': continue
            else:
                l = line.lstrip()
                ppos = l.find('(')
                if ppos != -1:
                    meth = l[0:ppos]
                    arg, varargs, varkw, defaults = inspect.getargspec(getattr(eval(obj), meth))
                    arg = inspect.formatargspec(arg, varargs, varkw, defaults, formatvalue=removeExtraDecimals)
                    arg = arg.replace("self, ", "")
                    if add_tab:
                        methods += '    ' + obj + '.' + meth + arg + ':\n'
                        add_tab = False
                    else:    
                        methods += obj + '.' + meth + arg + ':\n'
                    docstr = getattr(eval(obj), meth).__doc__.rstrip()
                    methods += docstr + '\n\n\n    '
                
        if 'Methods:' in line: 
            flag = True
            add_tab = True
            verbatim = True
            methods += '\n\\begin{Large}{\\bf Methods details:}\n\\end{Large}\n'
            methods += '\n\\begin{verbatim}\n\n'
        
        for key in DOC_KEYWORDS:
            if key != 'Methods':
                if key in line: 
                    flag = False
    return methods

def getFormattedDoc(text, obj):
    lines = text.splitlines(True)
    text = ''
    title = ''
    skip_empty_line = False
    verbatim = False
    for line in lines:
        flag = False
        see_also = False
        for key in DOC_KEYWORDS:
            if key in line:
                if key == 'See also':
                    see_also = True
                    break
                elif key == "Parentclass":
                    break
                else:
                    flag = True
                    skip_empty_line = True
                    last_title = title
                    title = key
                    break
        if flag:
            if verbatim:
                text += '\\end{verbatim}\n'
                verbatim = False
            text += '\n\\begin{Large}' + '{\\bf ' + line + '}\\end{Large}'
            text += '\n\\begin{verbatim}\n'
            if title == 'Examples':
                text += "from pyo import *\n"
            verbatim = True
        elif see_also:
            if verbatim:
                text += '\\end{verbatim}\n'
                verbatim = False
            text +=  '\n\\begin{Large}' + '{\\bf See also: }\\end{Large}'
            line_tmp = line.replace('See also:', '')
            words = line_tmp.split(',')
            for word in words:
                text += '\\htmladdnormallink{%s}{%s.html} ' % (word, word)
        elif 'Parentclass' in line:
            text +=  '\n\\begin{large} {\\bf Parentclass}: \\end{large}'
            text += '\\htmladdnormallink{%s}{%s.html}\n' % (line.split(':')[1].strip(), line.split(':')[1].strip())
        else:
            if skip_empty_line:
                skip_empty_line = False
            else:     
                if title == 'Examples':
                    if line.strip() == '':
                        if "s = Server()" in text:
                            text += 's.gui(locals())\n'
                        skip_empty_line = True
                    if ">>>" in line: 
                        line.lstrip("    ")
                        line = line.lstrip(">>> ")
                    if "..." in line: 
                        line.lstrip("    ")
                        line = "    " +  line.lstrip("... ")
                text += line
                    
    if verbatim:            
        text += '\\end{verbatim}\n'
        verbatim = False
    return text

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

# Generates the LaTeX file                    
f.write('\\begin{Huge}pyo documentation ( %s )\\end{Huge}\n\n' % man_version) 
f.write('pyo is a Python module written in C to help digital signal processing script creation.\n\n')     
f.write("""
pyo is a Python module containing classes for a wide variety of audio signal processing types. With pyo, user will be
able to include signal processing chains directly in Python scripts or projects, and to manipulate them in real time
through the interpreter. Tools in pyo module offer primitives, like mathematical operations on audio signal, basic
signal processing (filters, delays, synthesis generators, etc.), but also complex algorithms to create sound granulation
and others creative sound manipulations. pyo supports OSC protocol (Open Sound Control), to ease communications
between softwares, and MIDI protocol, for generating sound events and controlling process parameters. pyo allows
creation of sophisticated signal processing chains with all the benefits of a mature, and wild used, general programming language.
""")  
for key in ['Server', 'Stream', 'TableStream', 'PyoObjectBase', 'Map', 'functions']:
    f.write('\chapter[%s</A> : %s]{%s}\n\n' % (key, getDocFirstLine(key), key))
    f.write(getDoc(key))
    if type(OBJECTS_TREE[key]) == ListType:
        for obj in OBJECTS_TREE[key]:
            f.write('\section[%s</A> : %s]{%s}\n\n' % (obj, getDocFirstLine(obj), obj))
            f.write(getDoc(obj))
    else:
        if key == 'Map': key2list = ['SLMap']
        else: key2list = ['PyoMatrixObject', 'PyoTableObject', 'PyoObject']
        for key2 in key2list:
            f.write('\section[%s</A> : %s]{%s}\n\n' % (key2, getDocFirstLine(key2), key2))
            f.write(getDoc(key2))
            if type(OBJECTS_TREE[key][key2]) == ListType:
                for obj in OBJECTS_TREE[key][key2]:
                    f.write('\subsection[%s</A> : %s]{%s}\n\n' % (obj, getDocFirstLine(obj), obj))
                    f.write(getDoc(obj))
            else:
                for key3 in sorted(OBJECTS_TREE[key][key2]):
                    f.write('\subsection[%s</A> : %s]{%s}\n\n' % (key3, getDocFirstLine(key3), key3))
                    f.write(getDoc(key3))
                    for obj in OBJECTS_TREE[key][key2][key3]:
                        f.write('\subsubsection[%s</A> : %s]{%s}\n\n' % (obj, getDocFirstLine(obj), obj))
                        f.write(getDoc(obj))
                
  
f.write('\end{document}\n')
f.close()

# LaTeX -> html
os.chdir('doc/')
os.system('latex2html %s.tex' % man_file)

# Post-processing on html files
for file in os.listdir(man_file):
    with open("%s/%s" % (man_file, file), 'r') as f:
        text = f.read()
    text = text.replace("&lt;/A&gt;", "</A>")
    if text.find('href="prettify.css"') == -1:
            st = '<LINK REL="STYLESHEET" HREF="%s.css">' % man_file
            stnew = """
<LINK REL="STYLESHEET" HREF="%s.css">
<link href="prettify.css" type="text/css" rel="stylesheet" />
<script type="text/javascript" src="prettify.js"></script>

""" % man_file
            text = text.replace(st, stnew)

            st = '<BODY'
            stnew = '<BODY onload="prettyPrint()"'
            text = text.replace(st, stnew)

            ex_pos = text.find('<BIG CLASS="XLARGE"><B>Examples:')
            if ex_pos != -1:
                st = '<PRE>'
                stnew = '<PRE class="prettyprint">'
                text_first = text[:ex_pos]
                text_second = text[ex_pos:]
                text_second = text_second.replace(st, stnew, 1)
                text = text_first + text_second

    with open("%s/%s" % (man_file, file), 'w') as f:
        f.write(text)
        
# Clean-up        
os.remove('%s.tex' % man_file)
os.chdir('../')
shutil.copy('scripts/prettify.css', 'doc/%s' % man_file)
shutil.copy('scripts/prettify.js', 'doc/%s' % man_file)

# Upload on iACT server
print "Upload documentation (y/n)?"
ans = raw_input()
if (ans == 'y'):
    os.system('scp -r doc/%s sysop@132.204.178.49:/Library/WebServer/Documents/pyo/' % man_file)
