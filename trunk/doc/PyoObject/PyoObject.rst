:class:`PyoObject` --- Base class for all pyo audio objects
===========================================================

.. class:: PyoObject()

    Base class for all pyo objects that manipulate vectors of samples.
    
    The user should never instantiate an object of this class.


.. method:: PyoObject.play()

    Start processing without sending samples to output. This method is called automatically
    at the object creation.
        
.. method:: PyoObject.stop()
    
    Stop processing.
    
.. method:: PyoObject.out(chnl=0, inc=1)

    Start processing and send samples to audio output beginning at `chnl`.

    :param chnl: int, optional 
    
    Physical output assigned to the first audio stream of the object. Defaults to 0.

    If `chnl` is an integer equal or greater than 0: successive streams 
    increment the output number by `inc` and wrap around the global number of channels.
            
    If `chnl` is a negative integer: streams begin at 0, increment the output 
    number by `inc` and wrap around the global number of channels. Then, the list
    of streams is scrambled.
            
    If `chnl` is a list: successive values in the list will be assigned to successive streams.

    :param inc: int, optional 

    Output increment value.

.. method:: PyoObject.mix(voices=1)

    Mix the object's audio streams into `voices` streams and return the Mix object.

    :param voices: int, optional
    
    Number of audio streams of the Mix object created by this method. If more
    than 1, object's streams are alternated and added into Mix object's streams. 
    Defaults to 1.
    
.. method:: PyoObject.setMul(x)

    Replace the `mul` attribute.

    :param x: float or :class:`PyoObject`
    
.. method:: PyoObject.setAdd(x)

    Replace the `add` attribute.

    :param x: float or :class:`PyoObject`
    
.. method:: PyoObject.setDiv(x)

    Replace and inverse the `mul` attribute.

    :param x: float or :class:`PyoObject`
    
.. method:: PyoObject.setSub(x)

    Replace and inverse the `add` attribute.

    :param x: float or :class:`PyoObject`
    

.. note::

    **Other operations**
    
    len(obj) : Return the number of audio streams in an object.
    
    obj[x] : Return stream *x* of the object. *x* is a number from 0 to len(obj) - 1.
    
    del obj : Perform a clean delete of the object.
   
.. note::
    
    **Arithmetics**
    
    Multiplication, addition, division and substraction can be applied between pyo objects
    or between pyo objects and numbers. Doing so returns a Dummy object with the result of the operation.
    
    `b = a * 0.5` creates a Dummy object `b` with the `mul` attribute set to 0.5 and leaves `a` untouched.
    
    Inplace multiplication, addition, division and substraction can be applied between pyo 
    objects or between pyo objects and numbers. These operations will replace the `mul` or `add`
    factor of the object. 
    
    `a *= 0.5` replaces the `mul` attribute of `a`.
    

.. attribute:: PyoObject.mul

    float or :class:`PyoObject`. Multiplication factor.

.. attribute:: PyoObject.add

    float or :class:`PyoObject`. Addition factor.


**Child objects**

.. toctree::
    :glob:

    *
