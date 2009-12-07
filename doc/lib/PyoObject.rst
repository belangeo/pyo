:class:`PyoObject` --- Base class for all pyo audio objects
===========================================================

.. class:: PyoObject()

    Base class for all pyo objects that manipulate vectors of samples.
    
    User should never instantiates an object of this class.


.. method:: PyoObject.play()

    Start processing without sending samples to output. This method is called automatically
    at the object creation.
        
.. method:: PyoObject.stop()
    
    Stop processing.
    
.. method:: PyoObject.out(chnl=0)

    :param chnl: float or :class:`PyoObject`, optional 

    Start processing and send samples to audio output beginning at *chnl*.
    
.. method:: PyoObject.mix(voices=1)

    :param voices: float or :class:`PyoObject`, optional

    Mix object's audio streams into *voices* streams and return the Mix object.
    
.. method:: PyoObject.setMul(x)

    :param x: float or :class:`PyoObject`

    Replace the *mul* attribute.
    
.. method:: PyoObject.setAdd(x)

    :param x: float or :class:`PyoObject`

    Replace the *add* attribute.
    
.. method:: PyoObject.setDiv(x)

    :param x: float or :class:`PyoObject`

    Replace and inverse the *mul* attribute.
    
.. method:: PyoObject.setSub(x)

    :param x: float or :class:`PyoObject`

    Replace and inverse the *add* attribute.
    

.. note::

    **Other operations**
    
    len(obj) : Return the number of audio streams in an object.
    
    obj[x] : Return stream *x* of the object. *x* is a number from 0 to len(obj) - 1.
    
    del obj : Perform a clean delete of the object.
    
    **Mathematics**
    
    Multiplication, addition, division and substraction can be applied between pyo objects
    or between pyo object and number. Return a Dummy object with the result of the operation.
    
    *b = a * 0.5* creates a Dummy object *b* with *mul* attribute set to 0.5 and leave *a* untouched.
    
    Inplace multiplication, addition, division and substraction can be applied between pyo 
    objects or between pyo object and number. These operations will replace the *mul* or *add*
    factor of the object. 
    
    *a *= 0.5* replace *mul* attribute of *a*.
    

.. attribute:: PyoObject.mul

    float or :class:`PyoObject`. Multiplication factor.

.. attribute:: PyoObject.add

    float or :class:`PyoObject`. Addition factor.
