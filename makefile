CC=gcc
CFLAGS=-c -Wall  -I/usr/local/include  -I/usr/include/mysql -DBIG_JOINS=1  -fno-strict-aliasing  -g
LDFLAGS=-L/usr/local/lib  -lwiringPi  -lwiringPiDev   -L/usr/lib/arm-linux-gnueabihf -lmysqlclient -lpthread -lz -lm -lrt -ldl
SOURCES=main.c homey.c out.c shmem.c db.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=homey

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
	
	
clean:
	rm -rf *o homey