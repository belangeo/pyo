import os, inspect 
from types import StringType, TupleType, ListType, DictType
from pyo import *
                  
_OBJECTS_TREE = {'functions': sorted(['pa_count_devices', 'pa_get_default_input', 'pa_get_default_output', 'pa_list_devices', 
                    'pm_count_devices', 'pm_list_devices', 'sndinfo']),
        'PyoObject': {'Filters': sorted(['Biquad', 'Port', 'Hilbert', 'Tone', 'DCBlock']),
                      'Open Sound Control': sorted(['OscReceive', 'OscSend']),
                      'Effects': sorted(['BandSplit',  'Clip', 'Compress', 'Delay', 'Disto', 'Freeverb','Pan', 'SPan', 'Waveguide']),
                      'Triggers': sorted(['Metro', 'TrigEnv', 'TrigRand', 'Select', 'Counter', 'TrigChoice', 'TrigFunc']),
                      'Generators': sorted(['Noise', 'Osc', 'Phasor', 'Sig', 'SigTo', 'Sine', 'Input', 'Fader']),
                      'MIDI': sorted(['Midictl', 'Notein']),
                      'Sound players': sorted(['SfMarkerShuffler', 'SfPlayer']),
                      'Patterns': sorted(['Pattern']),
                      'Table process': sorted(['TableRec', 'Pointer', 'Granulator']),
                      'Analysis': sorted(['Follower']),
                      'Internal objects': sorted(['Dummy', 'InputFader', 'Mix'])},
        'Map': {'SLMap': sorted(['SLMapFreq'])},
        'PyoTableObject': sorted(['LinTable', 'NewTable', 'SndTable', 'HannTable', 'HarmTable']),
        'Server': [], 
        'Stream': [], 
        'TableStream': []}

_DOC_KEYWORDS = ['Attributes', 'Examples', 'Parameters', 'Methods', 'Notes', 'Methods details', 'See also']
 
f = open(os.getcwd() + '/doc/pyo_man.tex', 'w')

# Header
f.write("\documentclass[12pt,oneside]{article}\n")
f.write("\usepackage[utf8]{inputenc}\n")
f.write("\usepackage{dcolumn,amsmath}\n\n")
f.write("\usepackage{html,makeidx}\n\n")
f.write("\\begin{document}\n\n")

def getDoc(obj):
    try:
        args = '\n\n' + eval(obj).args() + '\n'
    except:
        args = '\n\n' 
        
    try:
        text = eval(obj).__doc__
        doc_str = getFormattedDoc(text, obj)
        methods = getMethodsDoc(text, obj)
        page_text = args + doc_str + methods + "\n\n"
        if len(methods):
            page_text += '\end{verbatim}\n\n'    
    except:
        page_text = args + "\nnot documented yet...\n\n"
        
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
            methods += '\n\\begin{Large}{\\bf Methods details:}\n\\end{Large}\n'
            methods += '\n\\begin{verbatim}\n\n'
        
        for key in _DOC_KEYWORDS:
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
        for key in _DOC_KEYWORDS:
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

f.write('\\begin{LARGE}pyo documentation\\end{LARGE}\n\n') 
f.write('pyo is a Python module written in C to help digital signal processing script creation.\n\n')       
for key in sorted(_OBJECTS_TREE.keys()):
    f.write('\section{%s}\n\n' % key)
    f.write(getDoc(key))
    if type(_OBJECTS_TREE[key]) == ListType:
        for obj in _OBJECTS_TREE[key]:
            f.write('\subsection{%s}\n\n' % obj)
            f.write(getDoc(obj))
    else:
        for key2 in sorted(_OBJECTS_TREE[key]):
            f.write('\subsection{%s}\n\n' % key2)
            f.write(getDoc(key2))
            for obj in _OBJECTS_TREE[key][key2]:
                f.write('\subsubsection{%s}\n\n' % obj)
                f.write(getDoc(obj))
  
f.write('\end{document}\n')
f.close()

os.chdir('doc/')
os.system('latex2html -html_version 4.0,unicode -noinfo -long_titles 1 -noaddress -local_icons -image_type gif pyo_man.tex')
os.system('rm pyo_man/WARNINGS')
os.chdir('../..')
