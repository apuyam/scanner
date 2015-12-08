all: scan

CC = gcc
CFLAGS = -Wall -g -pthread
LFLAGS = -L/usr/local/lib
LIBS = -lnfc_nci_linux

scan: main.o
	$(CC) $(CFLAGS) main.o /usr/local/lib/libnfc_nci_linux.so -o scan
scan.o: main.c
	$(CC) $(CFLAGS) -c main.c


clean:
	rm -rf scan *.o all