all: scan

CC = gcc
CFLAGS = -Wall -g -pthread
LFLAGS = -L/usr/local/lib
LIBS = -lnfc_nci_linux

scan: tools.o main.o 
	$(CC) $(CFLAGS) /usr/local/lib/libnfc_nci_linux.so /usr/local/lib/libnfc_nci_linux-1.so tools.o main.o -o scan
tools.o: tools.c
	$(CC) $(CFLAGS) -c tools.c
main.o: main.c
	$(CC) $(CFLAGS) -c main.c



clean:
	rm -rf scan *.o all