# scanner

This is the scanner software for the University of Manitoba ECE Group 16 Capstone Project. This software requires NXP's libnfc_linux-nci software stack, NXP's OM5577 installed on a Raspberry Pi, libxml, and wiringPi. 

To compile, run "make".

To run in bus mode run:
"sudo ./scan bus"
or to run in kiosk mode (requiring a seperate kiosk client to connect to) run:
.sudo /scan kiosk

To exit, press Ctrl-C at any point.

# options
Macros in tcp.h and functions.h manage program options for testing and debugging purposes.

functions.h:
TRANSFERSOFF: 1 to disable transfer system, 0 to enable.
TRANSFERSEC: in seconds, determines how long a transfer is valid.
ISDST: 0 for daylight savings off, 1 for daylight savings on. needed for time comparisons.
LEDON: if not defined, will disable all wiringPi/LED code.
LEDDELAY: determines LED blinking speed in ms.

tcp.h:
MESSAGESON: 0 disables calls to sendMessageToServer (database communication), 1 enables.
CACHING: 0 disables calls to getFullCache (database caching), 1 enables.
REQTIME: 0 disable calls database time synchronization, 1 enables.
TIMEOUTSEC: determines length until timeout for database communications in seconds.

