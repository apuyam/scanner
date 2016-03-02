# scanner<br/>

This is the scanner software for the University of Manitoba ECE Group 16 Capstone Project. This software requires NXP's libnfc_linux-nci software stack, NXP's OM5577 installed on a Raspberry Pi, libxml, and wiringPi. <br/><br/>

To compile, run "make".<br/><br/>

To run in bus mode run:<br/>
"sudo ./scan bus"<br/>
or to run in kiosk mode (requiring a seperate kiosk client to connect to) run:<br/>
.sudo /scan kiosk<br/><br/>

To exit, press Ctrl-C at any point.<br/><br/>

# options<br/>
Macros in tcp.h and functions.h manage program options for testing and debugging purposes.<br/><br/>

functions.h:<br/>
TRANSFERSOFF: 1 to disable transfer system, 0 to enable.<br/>
TRANSFERSEC: in seconds, determines how long a transfer is valid.<br/>
ISDST: 0 for daylight savings off, 1 for daylight savings on. needed for time comparisons.<br/>
LEDON: if not defined, will disable all wiringPi/LED code.<br/>
LEDDELAY: determines LED blinking speed in ms.<br/><br/>

tcp.h:<br/>
MESSAGESON: 0 disables calls to sendMessageToServer (database communication), 1 enables.<br/>
CACHING: 0 disables calls to getFullCache (database caching), 1 enables.<br/>
REQTIME: 0 disable calls database time synchronization, 1 enables.<br/>
TIMEOUTSEC: determines length until timeout for database communications in seconds.<br/>

