INCLUDE=/usr/local/include /usr/local/include/ndicapi ~/include ./include /usr/lo
#LIB = /usr/lib /usr/local/lib /usr/local/lib/isi_api
LIB = /usr/local/lib

CC = g++
CFLAGS = -no-pie -pthread -Wall
LDFLAGS = -lserial -lrt -lpthread -lndicapi

INC_PARAMS = $(addprefix -I,$(INCLUDE))
LIB_PARAMS = $(addprefix -L,$(LIB))

default : track_server

track_server : track_server.o mak_packet.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PARAMS) $(LDFLAGS)

track_server.o : ./src/track_server.cpp
	$(CC) $(CFLAGS) $(INC_PARAMS) -o $@ -c $<

mak_packet.o : ./src/mak_packet.c
	$(CC) $(CFLAGS) $(INC_PARAMS) -o $@ -c $<

clean :
	rm -r serial_test.o
