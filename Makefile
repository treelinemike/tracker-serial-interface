INCLUDE_SYS=/usr/local/include /usr/local/include/ndicapi /usr/lo
INCLUDE=~/include ./include
#LIB = /usr/lib /usr/local/lib /usr/local/lib/isi_api
LIB = /usr/local/lib

CC = g++
CFLAGS = -no-pie -pthread -Wall
LDFLAGS_SERVER = -lserial -lpthread -lndicapi
LDFLAGS_CLIENT = -lserial -lpthread

INC_SYS_PARAMS = $(addprefix -isystem,$(INCLUDE_SYS))
INC_PARAMS = $(addprefix -I,$(INCLUDE))
LIB_PARAMS = $(addprefix -L,$(LIB))

default : track_server track_client read_test

track_server : track_server.o mak_packet.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PARAMS) $(LDFLAGS_SERVER)

track_client : track_client.o mak_packet.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PARAMS) $(LDFLAGS_CLIENT)

read_test : read_test.o mak_packet.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PARAMS) $(LDFLAGS_CLIENT)

track_server.o : ./src/track_server.cpp
	$(CC) $(CFLAGS) $(INC_SYS_PARAMS) $(INC_PARAMS) -o $@ -c $<

track_client.o : ./src/track_client.cpp
	$(CC) $(CFLAGS) $(INC_SYS_PARAMS) $(INC_PARAMS) -o $@ -c $<

mak_packet.o : ./src/mak_packet.c
	$(CC) $(CFLAGS) $(INC_SYS_PARAMS) $(INC_PARAMS) -o $@ -c $<

read_test.o : ./src/read_test.cpp
	$(CC) $(CFLAGS) $(INC_SYS_PARAMS) $(INC_PARAMS) -o $@ -c $<

clean :
	rm -r *.o
