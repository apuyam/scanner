# scanner

This is the scanner software for the University of Manitoba ECE Group 16 Capstone Project. This software requires NXP's libnfc_linux-nci software stack, NXP's OM5577 installed on a Raspberry Pi, libxml, and wiringPi. 

To compile, run "make".

To run in bus mode run:
"./scan bus"
or to run in kiosk mode (requiring a seperate kiosk client to connect to) run:
./scan kiosk

To exit, press Ctrl-C at any point.
