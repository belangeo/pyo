Compiling pyo from sources
==========================

Here is how you can compile pyo from sources on Linux and macOS (if you are
interested in the adventure of compiling pyo from sources on Windows, you can 
take a look at my personal notes in `windows-10-64bit-build-routine.txt
<https://github.com/belangeo/pyo/blob/master/scripts/win/windows-10-64bit-build-routine.txt>`_).

See below for complete build routines for Debian/Ubuntu and MacOS.

Dependencies
------------

To compile pyo with all its features, you will need the following dependencies: 

- `Python 3.9, 3.10, 3.11, 3.12, or 3.13 <https://www.python.org/downloads/>`_.
- `WxPython Phoenix 4.2.0 or higher <https://www.wxpython.org/pages/downloads/>`_
- `Portaudio <http://www.portaudio.com/>`_
- `Portmidi <http://portmedia.sourceforge.net/portmidi/>`_
- `libsndfile <http://www.mega-nerd.com/libsndfile/>`_ (1.0.30+)
- `liblo <http://liblo.sourceforge.net/>`_ (0.32+)
- `git <https://git-scm.com/>`_ (if you want the latest sources)

Please note that under macOS you will need to install the 
**Apple's Developer Tools** to compile pyo.

Getting sources
---------------

You can download pyo's sources by checking out the source code 
`here <https://github.com/belangeo/pyo>`_: 

.. code-block:: bash

    git clone https://github.com/belangeo/pyo.git

Compilation
---------------

First, you need to install the build frontend module: 

.. code-block:: bash

    python -m pip install build

Once you have all the required dependencies, go in pyo's directory: 

.. code-block:: bash

    cd path/to/pyo

And build the library: 
    
.. code-block:: bash

    python -m build

You can customize your compilation by giving some flags to the build frontend with ``--config-setting``.
You can add as many `--config-setting` as you need in your build command.

.. code-block:: bash

    --config-setting="--build-option=<FLAG>"
    # Example for building with ``--use-double`` on Windows
    py -3.13 -m build --config-setting="--build-option=--use-double"

.. _compilation-flags-label:

Compilation flags
*****************

If you want to be able to use coreaudio (MacOS): 

.. code-block:: bash

    --use-coreaudio

If you want JACK support (Linux, macOS): 

.. code-block:: bash

    --use-jack

If you want to be able to use a 64-bit pyo (All platforms, this is the sample
resolution, not the architecture), this will build both single and double 
precision: 

.. code-block:: bash

    --use-double

If you want to disable most of the messages printed to the console:

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

Installation
---------------

Once built, you can install the created wheel file in your Python distribution with pip:

.. code-block:: bash

    python -m pip install --user dist/<YOUR_WHEEL_FILE.whl>

With the ``--user`` flag, pyo will be installed in the user's site-packages; otherwise, pyo will be installed system-wide.

Debian & Ubuntu (apt-get)
-------------------------

Under Debian & Ubuntu, you can type the following commands to get pyo up and running.

For Python 3.9 and higher
*************************

.. code-block:: bash

    # For Python 3.11
    sudo apt-get install libjack-jackd2-dev libportmidi-dev portaudio19-dev liblo-dev libsndfile-dev
    sudo apt-get install python3-dev python3-tk python3-pil.imagetk python3-pip
    python3 -m pip install build
    git clone https://github.com/belangeo/pyo.git
    cd pyo
    python3 -m build --config-setting="--build-option=--use-jack" --config-setting="--build-option=--use-double"
    python3 -m pip install dist/pyo-1.0.6-cp311-cp311-linux_x86_64.whl

If you want to be able to use all of pyo's GUI widgets, you will need WxPython Phoenix 4.2.0 or higher. 

- To install WxPython with pip on Linux, follow the instructions on the WxPython's `downloads <https://wxpython.org/pages/downloads/>`_ page. 

MacOS (Homebrew)
----------------

Under macOS, it is very simple to build pyo from sources with the Homebrew 
package manager.

The first step is to install the official Python from `python.org <https://www.python.org/downloads/>`_.

Second step, if you want to be able to use all of pyo's GUI widgets, you will need WxPython Phoenix. Install with pip:

.. code-block:: bash

    python3 -m pip install --user wxPython

The third step is to install `Homebrew <http://brew.sh/>`_.

Finally, in a terminal window, install pyo's dependencies, clone, and build pyo:

.. code-block:: bash

    brew install liblo libsndfile portaudio portmidi
    git clone https://github.com/belangeo/pyo.git
    cd pyo
    python3 -m build --config-setting="--build-option=--use-coreaudio" --config-setting="--build-option=--use-double"
    python3 -m pip install --user dist/<YOUR_WHEEL_FILE.whl>
