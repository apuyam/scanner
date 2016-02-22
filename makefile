all: scan

CC = gcc
CFLAGS = -Wall -g -pthread
IFLAGS = -I/usr/include/libxml2
LFLAGS = -L/usr/local/lib -L/usr/lib
LIBS = -lnfc_nci_linux -lxml2 -lwiringPi

scan: tools.o tcp.o list.o functions.o xmlfunctions.o main.o 
	$(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) $(LIBS) /usr/local/lib/libnfc_nci_linux.so /usr/local/lib/libnfc_nci_linux-1.so tools.o tcp.o list.o functions.o xmlfunctions.o main.o -o scan
tcp.o: tcp.c
	$(CC) $(CFLAGS) -c tcp.c	
list.o: list.c
	$(CC) $(CFLAGS) -c list.c
tools.o: tools.c
	$(CC) $(CFLAGS) -c tools.c
functions.o: functions.c
	$(CC) $(CFLAGS) -c functions.c
xmlfunctions.o: xmlfunctions.c
	$(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) $(LIBS) -c xmlfunctions.c
main.o: main.c
	$(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) $(LIBS) -c main.c



clean:
	rm -rf scan *.o all