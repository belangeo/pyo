Installing pyo with pip
=======================

In most use cases the best way to install pyo on your system is by using a pre-built
package for your operating system. These packages are available through pip.
Python supported version are 2.7, 3.5, 3.6 and 3.7. To install, run this command
(if you have both python2 and python3 installed and want to target python3, use `pip3`)::

    pip install -U pyo

If you are not sure to which version of python the `pip` command is bound to, you can
invoke it as a script from the `python` command.

.. code-block:: bash

    # for python 2
    python2 -m pip install -U pyo

    # for python 3.5
    python3.6 -m pip install -U pyo

    # for python 3.6 under Windows (`py` is the python launcher command)
    py -3.6 -m pip install -U pyo

    # if you want to install system-wide under MacOS or linux, use `sudo`
    sudo python3 -m pip install -U pyo

For instructions on building from source package, see `Compiling pyo from sources <compiling.html>`_.
This information is useful mainly for advanced users.

Pyo is a python module...
-------------------------

... which means that python must be present (version 2.7, 3.5, 3.6 or 3.7) 
on the system. If python is not installed, you can download it on 
`python.org <https://www.python.org/downloads/>`_.

Pyo also offers some GUI facilities to control or visualize the audio processing.
If you want to use all of pyo's GUI features, you must install WxPython 3.0 
(**classic** for python 2.7 and **phoenix** for python 3.5+), available on 
`wxpython.org <http://wxpython.org/download.php>`_.

Removing older version of pyo (prior to 1.0.0)
----------------------------------------------

Prior to version 1.0.0, pyo was installed with binary installers on MacOS and Windows. `pip` knows
nothing about these files and therefore cannot automatically removed them. To avoid conflicts between
versions, it's best to remove older installation of pyo.

**MacOS**

On MacOS, you can run this script for the version of python for which you want to use 1.0.0.

`https://github.com/belangeo/pyo/blob/master/scripts/cleanup_older_pyo_versions.py 
<https://github.com/belangeo/pyo/blob/master/scripts/cleanup_older_pyo_versions.py>`_

As an example, if you want to remove an old installation under python 3.5, run

.. code-block:: bash

    sudo python3.5 cleanup_older_pyo_versions.py

You should also delete the **E-pyo** app in the `Applications` folder.

**Windows**

Under Windows, all you have to do is to run the uninstaller that comes with pyo to 
remove all its components. It is located in the python root directory (ex.: C:\Python27) 
and is named `unins000.exe`.

**linux**

On linux, if you installed pyo from the system's package manager, just uninstall it.
If you compile pyo from sources, you have to delete the files manually (in site-packages 
or dist-packages folder of your python distribution). Files to delete are:
pyo.py, pyo64.py, _pyo.so, _pyo64.so, pyolib/ and pyo...egg

Running the EPyo text editor
----------------------------

To be "pip" compliant, I've had to make a choice and removed the pre-built E-Pyo application
previously installed alongside pyo. But the truth is that this application lives in a single
python script and can be easily used as long as WxPython is installed under the current python
distribution. The installation with `pip` puts this program under the `Scripts` folder (or `bin`
folder, depending on which system you are running) of the python distribution. If python's path
is in your PATH environment variable (what should really be the case), all you have to do to start
EPyo is to run this command:

.. code-block:: bash

    epyo
