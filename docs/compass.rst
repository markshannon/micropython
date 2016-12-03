Compass
*******

.. py:module:: microbit.compass

This module lets you access the built-in electronic compass. Before using,
the compass should be calibrated, otherwise the readings may be wrong.

.. warning::

    Calibrating the compass will cause your program to pause until calibration
    is complete. Calibration consists of a little game to draw a circle on the
    LED display by rotating the device.


Functions
=========

.. py:function:: calibrate()

    Starts the calibration process. A brief message will be scrolled
    to the user after which they will need to rotate the device in order to
    keep the flashing pixel pointing in the same direction.

.. py:function:: is_calibrated()

    Returns ``True`` if the compass has been successfully calibrated, and
    returns ``False`` otherwise.


.. py:function:: clear_calibration()

    Undoes the calibration, making the compass uncalibrated again.


.. py:function:: get_x()

    Gets the reading of the magnetic force on the ``x`` axis, as a
    positive or negative integer, depending on the direction of the
    force.


.. py:function:: get_y()

    Gets the reading of the magnetic force on the ``x`` axis, as a
    positive or negative integer, depending on the direction of the
    force.


.. py:function:: get_z()

    Gets the reading of the magnetic force on the ``x`` axis, as a
    positive or negative integer, depending on the direction of the
    force.

.. py:function:: get_values()

    Gets the reading of the magnetic force on all axes at once, as a three-element
    tuple of integers ordered as X, Y, Z.

.. py:function:: heading()

    Gives the compass heading, calculated from the above readings, as an
    integer in the range from 0 to 360, representing the angle in degrees,
    clockwise, with north as 0.
    If the compass has not been calibrated, then this will call ``calibrate``.

.. py:function:: start_calibrating()

   Starts the compass calibrating in the background. The quality of the calibration
   will depend on how much the micro:bit is rotated during calibration. Spinning 360 degrees
   should be sufficient, but rotating in all three dimensions is better.

.. py:function:: stop_calibrating()

    Stop the background calibration and store the value into persistent storage.

.. py:function:: get_field_strength()

    Returns an integer indication of the magnitude of the magnetic field around
    the device.


Example
=======

.. include:: ../examples/compass.py
    :code: python
