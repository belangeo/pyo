Compiling
=====================

Here is how you can compile pyo from sources.

Dependencies
--------------

To compile pyo, you will need the following dependencies: 

- `Python 2.6 or 2.7 <http://www.python.org/download/releases/>`_
- `WxPython 3.0 <http://www.wxpython.org/download.php/>`_
- `Portaudio <http://www.portaudio.com/>`_
- `Portmidi <http://portmedia.sourceforge.net/portmidi/>`_
- `libsndfile <http://www.mega-nerd.com/libsndfile/>`_
- `liblo <http://liblo.sourceforge.net/>`_

Under Mac OS X, you can use Homebrew to retrieve necessary dependency librairies and headers (except for wxpython 3.0) to compile pyo.

First, install Homebrew with this command: 

.. code-block:: bash

    ruby -e "$(curl -fsSL https://raw.github.com/Homebrew/homebrew/go/install)"

Then, install packages: 

.. code-block:: bash

    brew install python liblo libsndfile portaudio portmidi --universal

Getting sources
-------------------

You can download pyo's source checking out the source code here: 

.. code-block:: bash

    svn checkout http://pyo.googlecode.com/svn/trunk/ pyo-read-only

Compilation
---------------

Please note that under Mac OS X you will need to install the **Apple's developer tools** to compile pyo.

Once you have all the required dependencies, go in pyo's directory: 

.. code-block:: bash

    cd pyo-read-only

You then need to build the extension: 

.. code-block:: bash

    sudo python setup.py install

You can customize you compilation by giving some flags to the command line.

.. _compilation-flags-label:

Compilation flags
*********************

If you want to be able to use coreaudio (Mac OS X): 

.. code-block:: bash

    --use-coreaudio

If you want JACK support (Linux, Mac OS X): 

.. code-block:: bash

    --use-jack

If you want to be able to use a 64-bit pyo (All platforms), this will build both single and double precision: 

.. code-block:: bash

    --use-double

If you want to disable most of messages printed to the console:

.. code-block:: bash
    
    --no-messages

If you want to compile external classes defined in externals folder:

.. code-block:: bash

    --compile-externals

Compilation scripts
**********************

To compile both 32-bit and 64-bit resolutions on linux (with jack support):

.. code-block:: bash

    sudo sh scripts/compile_linux_withJack.sh

To compile both 32-bit and 64-bit resolutions on OS X (without Jack):

.. code-block:: bash

    sudo sh scripts/compile_OSX.sh

To compile both 32-bit and 64-bit resolutions on OS X (with Jack):

.. code-block:: bash

    sudo sh scripts/compile_OSX_withJack.sh

Ubuntu (Debian)
-------------------

Under Ubuntu you can type the following commands to get pyo up and running: 

.. code-block:: bash

    sudo apt-get install libjack-jack2-dev libportmidi-dev portaudio19-dev liblo-dev 
    sudo apt-get install libsndfile-dev python-dev python-tk subversion 
    sudo apt-get install python-imaging-tk python-wxgtk2.8
    svn checkout http://pyo.googlecode.com/svn/trunk/ pyo-read-only
    cd pyo-read-only
    sudo python setup.py install --install-layout=deb --use-jack --use-double

OSX (Homebrew)
--------------------

Under OS X, it is very simple to build pyo from sources with the Homebrew package mananger:

.. code-block:: bash

    brew install python liblo libsndfile portaudio portmidi --universal
    svn checkout http://pyo.googlecode.com/svn/trunk/ pyo
    cd pyo
    python setup.py install --use-coreaudio --use-double 


