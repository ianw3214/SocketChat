all: server client

server: server.cpp
	g++ -std=c++0x server.cpp -pthread -o bin/server -Wall

client: client.cpp
	g++ -std=c++0x client.cpp -pthread -o bin/client -Wall
