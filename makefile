CC=g++
%.o: %.c %.h
	$(CC) -c -o $@ $<

all: server

server: sbhukar_server.o server.o common.o
	$(CC) sbhukar_server.o server.o common.o -o server -lrt

sbhukar_server.o: sbhukar_server.cpp
	$(CC) -g -c  sbhukar_server.cpp
	
server.o: server.cpp
	$(CC) -g -c  server.cpp
	
common.o: common.cpp
	$(CC) -g -c  common.cpp

clean:
	rm -rf *o server
