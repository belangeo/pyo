import os, inspect 
from types import StringType, TupleType, ListType, DictType
from pyo import *
 
f = open(os.getcwd() + '/doc/manual.tex', 'w')

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
                    arg = inspect.getargspec(getattr(eval(obj), meth))
                    arg = inspect.formatargspec(*arg, formatvalue=removeExtraDecimals)
                    arg = arg.replace("self, ", "")
                    if add_tab:
                        methods += '    ' + obj + '.' + meth + arg + ':\n'
                        add_tab = False
                    else:    
                        methods += obj + '.' + meth + arg + ':\n'
                    docstr = getattr(eval(obj), meth).__doc__.rstrip()
                    methods += docstr + '\n\n    '
                
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
                else:
                    flag = True
                    skip_empty_line = True
                    break
        if flag:
            if verbatim:
                text += '\\end{verbatim}\n'
                verbatim = False
            text += '\n\\begin{Large}' + '{\\bf ' + line + '}\\end{Large}'
            text += '\n\\begin{verbatim}\n'
            verbatim = True
        elif see_also:
            if verbatim:
                text += '\\end{verbatim}\n'
                verbatim = False
            text +=  '\n\\begin{Large}' + '{\\bf See also : }\\end{Large}'
            line_tmp = line.replace('See also:', '')
            words = line_tmp.split(',')
            for word in words:
                text += '\\htmladdnormallink{%s}{%s.html} ' % (word, word)
        elif 'Parent class' in line:
            text +=  '\n\\begin{large} Parent class : \\end{large}'
            text += '\\htmladdnormallink{%s}{%s.html}\n' % (line.split(':')[1].strip(), line.split(':')[1].strip())
        else:
            if skip_empty_line:
                skip_empty_line = False
            else:     
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
                    
f.write('\\begin{LARGE}pyo documentation\\end{LARGE}\n\n') 
f.write('pyo is a Python module written in C to help digital signal processing script creation.\n\n')       
for key in sorted(OBJECTS_TREE.keys()):
    f.write('\section[%s</A> : %s]{%s}\n\n' % (key, getDocFirstLine(key), key))
    f.write(getDoc(key))
    if type(OBJECTS_TREE[key]) == ListType:
        for obj in OBJECTS_TREE[key]:
            f.write('\subsection[%s</A> : %s]{%s}\n\n' % (obj, getDocFirstLine(obj), obj))
            f.write(getDoc(obj))
    else:
        for key2 in sorted(OBJECTS_TREE[key]):
            f.write('\subsection[%s</A> : %s]{%s}\n\n' % (key2, getDocFirstLine(key2), key2))
            f.write(getDoc(key2))
            for obj in OBJECTS_TREE[key][key2]:
                f.write('\subsubsection[%s</A> : %s]{%s}\n\n' % (obj, getDocFirstLine(obj), obj))
                f.write(getDoc(obj))
  
f.write('\end{document}\n')
f.close()

os.chdir('doc/')
os.system('latex2html -html_version 4.0,unicode -noinfo -long_titles 1 -noaddress -local_icons -image_type gif manual.tex')
for file in os.listdir("manual"):
    with open("manual/%s" % file, 'r') as f:
        text = f.read()
    text = text.replace("&lt;/A&gt;", "</A>")
    with open("manual/%s" % file, 'w') as f:
        f.write(text)
os.remove('manual.tex')
os.chdir('../')
os.system('scp -r doc/manual sysop@132.204.178.49:/Library/WebServer/Documents/pyo/')
