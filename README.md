CMRInet - Non-blocking protocol handling
========================================

This library is designed to permit an Arduino sketch to implement the CMRI
protocol in addition to whatever else it needs to do.  By performing all of
the CMRI protocol handling in non-blocking fashion, the loop() method in
the sketch can continue to do other things.

Why would one want to do this?  For example, an Arduino is perfectly
capable of flashing a light (as you might use for a flashing signal aspect,
or for grade crossing lights).  But doing this evenly requires either a lot
of interrupt handling, or a non-blocking CMRI implementation.

Using CMRI, you (the sketch developer) might be told that "output 7" is ON.
But the sketch might blink the light associated with #7 every 1/2 second. 
The CMRI server does not need to time a series of ON/OFF/ON/OFF messages --
it only knows that #7 is ON and you let the Arduino do the rest.

Or you might establish a single virtual output (a grade crossing) that is
either on or off, but you then use several outputs to handle all of the
lights, and then flash them such that the left & right lights of a pair
flash in an alternating fashion.  


This library only handles the CMRI protocol -- it knows nothing of the
various types of hardware that you might have on your Arduino node.

That's why you install input & output handler functions.  These functions
will be called by the CMRI library when an input status needs to be read,
or when an output is being changed.


For more information, please visit the library home page at https://github.com/davidzuhn/CMRI/wiki





Information
===========
If you have any questions about this code, please send me an email
at <zoo@blueknobby.com>, or ask on the Arduini yahoo group at
http://groups.yahoo.com/neo/groups/Arduini/info
