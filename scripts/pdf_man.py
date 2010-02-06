import os, inspect 
from types import StringType, TupleType, ListType, DictType
from pyo import *
 
f = open(os.getcwd() + '/doc/pyo_man.tex', 'w')

# Header
f.write("%!TEX TS-program = /usr/texbin/pdflatex\n")
f.write("\documentclass[12pt,oneside]{article}\n")
f.write("\usepackage[utf8]{inputenc}\n")
f.write("\usepackage{dcolumn,amsmath}\n\n")
#f.write("\usepackage{listings}\n")
f.write("\oddsidemargin 0in\n")
f.write("\evensidemargin 0in\n")
f.write("\marginparwidth 40pt\n") 
f.write("\marginparsep 10pt\n")
f.write("\\topmargin 0pt\n") 
f.write("\headsep .3in\n")
f.write("\\textheight 8.6in \\textwidth 6.5in\n")
f.write("\small\n")

f.write("\\begin{document}\n\n")
#f.write("\lstset{language=Python}\n")
def getDoc(obj):
    try:
        args = '\n\n' + eval(obj).args().replace("_", "\_") + '\n\n\\vspace{0.5cm}\n'
    except:
        args = '\n\n' 
        
    try:
        text = eval(obj).__doc__
    except:
        text = ''
    try:        
        doc_str = getFormattedDoc(text, obj)
    except:
        doc_str = ''
    try:        
        methods = getMethodsDoc(text, obj)
    except:
        methods = ''    
    page_text = args + doc_str + methods + "\n\n"
    if len(methods):
        page_text += '\end{verbatim}\n\n\\normalsize\n\n'    
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
                    args = inspect.getargspec(getattr(eval(obj), meth))
                    args = inspect.formatargspec(*args)
                    if add_tab:
                        methods += '    ' + obj + '.' + meth + args + ':\n'
                        add_tab = False
                    else:    
                        methods += obj + '.' + meth + args + ':\n'
                    docstr = getattr(eval(obj), meth).__doc__.rstrip()
                    methods += docstr + '\n\n    '
                
        if 'Methods:' in line: 
            flag = True
            add_tab = True
            verbatim = True
            methods += '\n\\begin{large}{\\bf Methods details:}\n\\end{large}\n'
            methods += '\n\small\n\\begin{verbatim}\n'
        
        for key in DOC_KEYWORDS:
            if key != 'Methods':
                if key in line: 
                    flag = False
    return methods

def getFormattedDoc(text, obj):
    lines = text.splitlines(True)
    text = ''
    skip_empty_line = False
    no_indent = True
    first_flag = 0
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
                    if first_flag == 0:
                        first_flag = 1
                    flag = True
                    skip_empty_line = True
                    break
        if flag:
            if verbatim:
                text += '\\end{verbatim}\n\\normalsize\n\n'
                verbatim = False
            if first_flag == 1:   
                text += '\n\\vspace{0.5cm}\n\n'
                first_flag = 2 
            text += '\n\\begin{large}' + '{\\bf ' + line + '}\\end{large}'
            text += '\n\small\n\\begin{verbatim}\n'
            verbatim = True
        elif see_also:
            if verbatim:
                text += '\\end{verbatim}\n\\normalsize\n\n'
                verbatim = False
            text +=  '\n\\begin{large}' + '{\\bf See also : }\\end{large}'
            line_tmp = line.replace('See also:', '')
            words = line_tmp.split(',')
            for word in words:
                text += word
        elif 'Parent class' in line:
            no_indent = False
            text += '\n\\vspace{0.5cm}\n\n'
            text +=  '\n\\begin{large} Parent class : \\end{large}'
            text += line.split(':')[1].strip()
        else:
            if skip_empty_line:
                skip_empty_line = False
            else:
                if no_indent and not verbatim:
                    text += '\\noindent '  
                text += line
    if verbatim:            
        text += '\\end{verbatim}\n\\normalsize\n\n'
        verbatim = False
    return text

f.write('\\begin{LARGE}pyo documentation\\end{LARGE}\n\n') 
f.write('\\vspace{0.5cm}\n\n')
f.write('pyo is a Python module written in C to help digital signal processing script creation.\n\n')    
newpage = True   
for key in sorted(OBJECTS_TREE.keys()):
    if key == 'functions': newpage = False
    else: newpage = True 
    f.write('\\newpage\n\n')
    f.write('\section{%s}\n\n' % key.replace("_", "\_"))
    f.write(getDoc(key))
    if type(OBJECTS_TREE[key]) == ListType:
        for obj in OBJECTS_TREE[key]:
            obj = obj.replace("_", "\_")
            if newpage:
                f.write('\\newpage\n\n')
            f.write('\subsection{%s}\n\n' % obj)
            f.write(getDoc(obj))
    else:
        for key2 in sorted(OBJECTS_TREE[key]):
            if newpage:
                f.write('\\newpage\n\n')
            f.write('\subsection{%s}\n\n' % key2)
            f.write(getDoc(key2))
            for obj in OBJECTS_TREE[key][key2]:
                if newpage:
                    f.write('\\newpage\n\n')
                f.write('\subsubsection{%s}\n\n' % obj)
                f.write(getDoc(obj))

f.write('\tableofcontents\n')  
f.write('\end{document}\n')
f.close()

os.chdir('doc/')
os.system('/usr/texbin/pdflatex pyo_man.tex')
for f in os.listdir('.'):
    if 'pyo_man' in f and not f.endswith('.pdf'):
        os.remove(f)
