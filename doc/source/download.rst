Downloading the installer
==========================

Installers are available for Windows (XP/Vista/7/8/10) and for MacOS 
(from 10.6 to 10.12).

To download the latest pre-compiled version of pyo, go to the pyo's 
`web page <http://ajaxsoundstudio.com/software/pyo/>`_.

Under Debian and Fedora distros, you can get pyo from the package manager. 
The library's name is **python-pyo** or **python2-pyo** for Python 2.7
and **python3-pyo** for Python 3.5+. 

If you are running Arch linux, the package is called **python2-pyo**. There
is no package yet for Python 3.5+.


Content of the installer
----------------------------

The installer installs two distinct softwares on the system. First, it will 
install the pyo module (compiled for both single and double precision) and its 
dependencies under the current python distribution. Secondly, it will install 
E-Pyo, a simple text editor especially tuned to edit and run audio python script. 

Pyo is a python module...
-----------------------------

... which means that python must be present (version 2.7, 3.5 or 3.6 (recommended)) 
on the system. If python is not installed, you can download it on 
`python.org <https://www.python.org/downloads/>`_.

Pyo also offers some GUI facilities to control or visualize the audio processing.
If you want to use all of pyo's GUI features, you must install WxPython 3.0 
(**classic** for python 2.7 and **phoenix** for python 3.5+), available on 
`wxpython.org <http://wxpython.org/download.php>`_.