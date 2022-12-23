INCLUDE=/usr/local/include ~/include
#LIB = /usr/lib /usr/local/lib /usr/local/lib/isi_api
LIB = /usr/local/lib

CC = g++
CFLAGS = -no-pie -pthread -Wall
LDFLAGS = -lserial -lrt -lpthread

INC_PARAMS = $(addprefix -I,$(INCLUDE))
LIB_PARAMS = $(addprefix -L,$(LIB))

default : serial_test

serial_test : serial_test.o mak_packet.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PARAMS) $(LDFLAGS)

serial_test.o : ./src/serial_test.cpp
	$(CC) $(CFLAGS) $(INC_PARAMS) -o $@ -c $<

mak_packet.o : ./src/mak_packet.c
	$(CC) $(CFLAGS) $(INC_PARAMS) -o $@ -c $<

clean :
	rm -r serial_test.o
