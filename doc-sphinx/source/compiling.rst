Compiling pyo from sources
==========================

Here is how you can compile pyo from sources on Linux and MacOS (if you are
interested in the adventure of compiling pyo from sources on Windows, you can 
take a look at my personal notes in `windows-7-build-routine.txt 
<https://github.com/belangeo/pyo/blob/master/scripts/win/windows-7-build-routine.txt>`_).

Dependencies
------------

To compile pyo with all its features, you will need the following dependencies: 

- `Python 2.7.x or 3.5.x <https://www.python.org/downloads/>`_
- `WxPython 3.0 <http://www.wxpython.org/download.php/>`_
- `Portaudio <http://www.portaudio.com/>`_
- `Portmidi <http://portmedia.sourceforge.net/portmidi/>`_
- `libsndfile <http://www.mega-nerd.com/libsndfile/>`_
- `liblo <http://liblo.sourceforge.net/>`_
- `git <https://git-scm.com/>`_ (if you want the latest sources)

Getting sources
---------------

You can download pyo's sources by checking out the source code 
`here <https://github.com/belangeo/pyo>`_: 

.. code-block:: bash

    git clone https://github.com/belangeo/pyo.git

Compilation
---------------

Please note that under MacOS you will need to install the 
**Apple's developer tools** to compile pyo.

Once you have all the required dependencies, go in pyo's directory: 

.. code-block:: bash

    cd path/to/pyo

And build the library: 

MacOS:
    
.. code-block:: bash

    sudo python setup.py install

Debian & Ubuntu:
    
.. code-block:: bash

    sudo python setup.py install --install-layout=deb

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
compilation process a little bit.

To compile both 32-bit and 64-bit resolutions on linux (with jack support):

.. code-block:: bash

    sudo sh scripts/compile_linux_withJack.sh

To compile both 32-bit and 64-bit resolutions on macOS (without Jack):

.. code-block:: bash

    sudo sh scripts/compile_OSX.sh

To compile both 32-bit and 64-bit resolutions on macOS (with Jack):

.. code-block:: bash

    sudo sh scripts/compile_OSX_withJack.sh

Debian & Ubuntu (apt-get)
-------------------------

Under Debian & Ubuntu you can type the following commands to get pyo up 
and running: 

.. code-block:: bash

    sudo apt-get install libjack-jackd2-dev libportmidi-dev portaudio19-dev liblo-dev 
    sudo apt-get install libsndfile-dev python-dev python-tk 
    sudo apt-get install python-imaging-tk python-wxgtk3.0
    git clone https://github.com/belangeo/pyo.git
    cd pyo
    sudo python setup.py install --install-layout=deb --use-jack --use-double

* On Ubuntu system prior to vivid, wxpython 3.0 must be compiled from sources.
 
MacOS (Homebrew)
----------------

Under macOS, it is very simple to build pyo from sources with the Homebrew 
package manager.

First, install Python and WxPython from the binary installers.

Second, you need to install `Homebrew <http://brew.sh/>`_. Then, in a terminal 
window:

.. code-block:: bash

    brew install liblo libsndfile portaudio portmidi --universal
    git clone https://github.com/belangeo/pyo.git
    cd pyo
    python setup.py install --use-coreaudio --use-double 
