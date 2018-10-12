Compiling pyo from sources
==========================

Here is how you can compile pyo from sources on Linux and MacOS (if you are
interested in the adventure of compiling pyo from sources on Windows, you can 
take a look at my personal notes in `windows-7-build-routine.txt 
<https://github.com/belangeo/pyo/blob/master/scripts/win/windows-7-build-routine.txt>`_).

Dependencies
------------

To compile pyo with all its features, you will need the following dependencies: 

- `Python 2.7 or 3.5 or higher <https://www.python.org/downloads/>`_. On Windows, install the 32-bit version of Python.
- `WxPython 3.0.2.0 (classic) or 4.0.0 (phoenix, recommended) <http://www.wxpython.org/download.php/>`_
- `Portaudio <http://www.portaudio.com/>`_
- `Portmidi <http://portmedia.sourceforge.net/portmidi/>`_
- `libsndfile <http://www.mega-nerd.com/libsndfile/>`_
- `liblo <http://liblo.sourceforge.net/>`_
- `git <https://git-scm.com/>`_ (if you want the latest sources)

Please note that under MacOS you will need to install the 
**Apple's developer tools** to compile pyo.

Getting sources
---------------

You can download pyo's sources by checking out the source code 
`here <https://github.com/belangeo/pyo>`_: 

.. code-block:: bash

    git clone https://github.com/belangeo/pyo.git

Compilation
---------------

Once you have all the required dependencies, go in pyo's directory: 

.. code-block:: bash

    cd path/to/pyo

And build the library: 
    
.. code-block:: bash

    sudo python setup.py install

You can customize your compilation by giving some flags to the command line.

.. _compilation-flags-label:

Compilation flags
*****************

If you want to be able to use coreaudio (MacOS): 

.. code-block:: bash

    --use-coreaudio

If you want JACK support (Linux, MacOS): 

.. code-block:: bash

    --use-jack

If you want to be able to use a 64-bit pyo (All platforms, this is the sample
resolution, not the architecture), this will build both single and double 
precisions: 

.. code-block:: bash

    --use-double

If you want to disable most of messages printed to the console:

.. code-block:: bash
    
    --no-messages

If you want to compile external classes defined in pyo/externals folder:

.. code-block:: bash

    --compile-externals

By default, debug symbols are off. If you want to compile pyo with debug symbols:

.. code-block:: bash

    --debug

By default, optimizations are activated. If you want to compile pyo without 
optimizations:

.. code-block:: bash

    --fast-compile

If you want to compile pyo with minimal dependencies (mostly for integrated use
in a host environment):

.. code-block:: bash

    --minimal

This will compile pyo without portaudio, portmidi and liblo support.

Compilation scripts
*******************

In the ./scripts folder, there is some alternate scripts to simplify the 
compilation process a little bit. These scripts will compile pyo for the
version of python pointed to by the command `python`.

To compile both 32-bit and 64-bit resolutions on linux with jack support:

.. code-block:: bash

    sudo sh scripts/compile_linux_withJack.sh

To compile both 32-bit and 64-bit resolutions on macOS without Jack support:

.. code-block:: bash

    sudo sh scripts/compile_OSX.sh

To compile both 32-bit and 64-bit resolutions on macOS with Jack support (Jack headers must be present on the system):

.. code-block:: bash

    sudo sh scripts/compile_OSX_withJack.sh

Debian & Ubuntu (apt-get)
-------------------------

Under Debian & Ubuntu you can type the following commands to get pyo up 
and running.

For Python 2.7
************** 

.. code-block:: bash

    sudo apt-get install libjack-jackd2-dev libportmidi-dev portaudio19-dev liblo-dev libsndfile-dev
    sudo apt-get install python-dev python-tk python-imaging-tk python-wxgtk3.0
    git clone https://github.com/belangeo/pyo.git
    cd pyo
    sudo python setup.py install --use-jack --use-double

* On Ubuntu system prior to vivid, wxpython 3.0 must be compiled from sources.

For Python 3.5 and higher
*************************

.. code-block:: bash

    sudo apt-get install libjack-jackd2-dev libportmidi-dev portaudio19-dev liblo-dev libsndfile-dev
    sudo apt-get install python3-dev python3-tk python3-pil.imagetk python3-pip
    git clone https://github.com/belangeo/pyo.git
    cd pyo
    sudo python3 setup.py install --use-jack --use-double

If you want to be able to use all of pyo's gui widgets, you will need wxPython Phoenix. 

- To install wxPython with pip on linux, follow the instructions on the wxPython's `downloads <https://wxpython.org/pages/downloads/>`_ page. 

MacOS (Homebrew)
----------------

Under macOS, it is very simple to build pyo from sources with the Homebrew 
package manager.

The first step is to install the official `Python <https://www.python.org/downloads/>`_ from python.org. 

Second step, if you want to be able to use all of pyo's gui widgets, you will need wxPython Phoenix. Install with pip:

.. code-block:: bash

    sudo pip3 install -U wxpython

The third step is to install `Homebrew <http://brew.sh/>`_.

Finally, in a terminal window, install pyo's dependencies, clone and build pyo:

.. code-block:: bash

    brew install liblo libsndfile portaudio portmidi
    git clone https://github.com/belangeo/pyo.git
    cd pyo
    python setup.py install --use-coreaudio --use-double 
