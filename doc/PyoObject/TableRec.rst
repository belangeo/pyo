:class:`TableRec` --- Write samples in a table object
=====================================================

.. class:: TableRec(input, table, fadetime=0)

    Parent class : :class:`PyoObject`

    TableRec is for writing samples in a previously created :class:`NewTable`. 
    See :class:`NewTable` to create an empty table.
    
    :param input: :class:`PyoObject`
    
    Audio signal to write in the table.
    
    :param table: :class:`NewTable`
    
    The table where to write samples.
    
    :param fadetime: float, optional
    
    Fade time at the beginning and the end of the recording in seconds. 
    Defaults to 0.


.. method:: TableRec.setInput(x, fadetime=0.05)

    Replace the `input` attribute.

    :param x: :class:`PyoObject`

    New input signal to process.

    :param fadetime: float, optional

    Crossfade time between old and new input. Defaults to 0.05.

.. method:: TableRec.setTable(x)

    Replace the `table` attribute.

    :param x: :class:`NewTable`

.. method:: TableRec.play()

    Start the recording at the beginning of the table.
     
.. method:: TableRec.stop()

    Stop the recording. Otherwise, record through the end of the table.

.. note::

    The out() method is bypassed. :class:`TableRec` returns no signal.
    
    :class:`TableRec` has no `mul` and `add` attributes.

.. attribute:: TableRec.input

    :class:`PyoObject`. Input signal to process.

.. attribute:: TableRec.table

    :class:`NewTable`. Table to record in.
